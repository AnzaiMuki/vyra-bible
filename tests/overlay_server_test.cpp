/*
VYRA Bible - OBS Studio plugin
Copyright (C) 2026 VYRA Concept
SPDX-License-Identifier: GPL-2.0-or-later
*/

// The overlay server with real sockets on 127.0.0.1. VYRA_OVERLAY_DIR = data/overlay.

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QHostAddress>
#include <QNetworkInterface>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>

#include <cstdio>

#include <random>
#include "src/overlay/http_routes.hpp"
#include "src/overlay/overlay_server.hpp"
#include "tests/check.hpp"

using namespace vyra::overlay;

namespace {

/** Runs the event loop until @p done() or until @p ms have passed. */
template <class F> bool waitFor(F done, int ms = 3000)
{
	QElapsedTimer t;
	t.start();
	while (!done() && t.elapsed() < ms)
		QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
	return done();
}

/** Sends @p raw, returns everything the server sends until it closes the connection. */
QByteArray exchange(quint16 port, const QByteArray &raw, bool *closed = nullptr)
{
	QTcpSocket s;
	s.connectToHost(QHostAddress::LocalHost, port);
	if (!waitFor([&] { return s.state() == QAbstractSocket::ConnectedState; }))
		return "CONNECT-FAILED";
	s.write(raw);
	QByteArray all;
	const bool ok = waitFor([&] {
		all += s.readAll();
		return s.state() == QAbstractSocket::UnconnectedState;
	});
	all += s.readAll();
	if (closed)
		*closed = ok;
	return all;
}

QByteArray get(quint16 port, const char *path)
{
	return exchange(port, QByteArray("GET ") + path + " HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n");
}

QByteArray bodyOf(const QByteArray &response)
{
	const int i = response.indexOf("\r\n\r\n");
	return i < 0 ? QByteArray() : response.mid(i + 4);
}

QByteArray fileBytes(const char *name)
{
	QFile f(QString::fromUtf8(VYRA_OVERLAY_DIR) + "/" + QString::fromLatin1(name));
	return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray("<missing>");
}

Frame onAir(unsigned long long rev, const char *ref, const char *text)
{
	Frame f;
	f.revision = rev;
	f.visible = true;
	f.reference = ref;
	f.verses.push_back({3, 16, text, false});
	return f;
}

/** A client of /events that keeps what it receives. */
struct Stream {
	QTcpSocket socket;
	QByteArray received;
	bool open(quint16 port)
	{
		socket.connectToHost(QHostAddress::LocalHost, port);
		if (!waitFor([&] { return socket.state() == QAbstractSocket::ConnectedState; }))
			return false;
		socket.write("GET /events HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n");
		QObject::connect(&socket, &QTcpSocket::readyRead, &socket, [this] { received += socket.readAll(); });
		return true;
	}
	int messages() const { return received.count("data: "); }
};

void testListening()
{
	OverlayServer server(VYRA_OVERLAY_DIR);
	CHECK(server.port() == 0);
	QString error;
	CHECK(server.start(18420, 18429, &error));
	CHECK(server.port() >= 18420 && server.port() <= 18429);

	// Only the loopback interface: the server must not accept connections on another address of this machine.
	for (const QHostAddress &a : QNetworkInterface::allAddresses()) {
		if (a.protocol() != QAbstractSocket::IPv4Protocol || a.isLoopback())
			continue;
		QTcpSocket s;
		s.connectToHost(a, server.port());
		s.waitForConnected(500);
		CHECK(s.state() != QAbstractSocket::ConnectedState);
	}
}

void testPortFallbackAndFailure()
{
	OverlayServer first(VYRA_OVERLAY_DIR);
	CHECK(first.start(18430, 18439));
	OverlayServer second(VYRA_OVERLAY_DIR);
	CHECK(second.start(18430, 18439));
	CHECK(second.port() == first.port() + 1);       // the next free port

	OverlayServer third(VYRA_OVERLAY_DIR);
	QString error;
	CHECK(!third.start(first.port(), first.port(), &error)); // the only port is taken
	CHECK(!error.isEmpty());
	CHECK(third.port() == 0);
}

void testFiles()
{
	OverlayServer server(VYRA_OVERLAY_DIR);
	CHECK(server.start(18440, 18449));
	const quint16 p = server.port();

	const QByteArray page = get(p, "/");
	CHECK(page.startsWith("HTTP/1.1 200 OK"));
	CHECK(page.contains("Content-Type: text/html; charset=utf-8"));
	CHECK(bodyOf(page) == fileBytes("index.html"));
	CHECK(bodyOf(get(p, "/overlay.css")) == fileBytes("overlay.css"));
	CHECK(bodyOf(get(p, "/overlay.js")) == fileBytes("overlay.js"));
	CHECK(get(p, "/overlay.js").contains("text/javascript"));

	// The JavaScript must reach the browser byte for byte, accents included (UTF-8, no re-encoding).
	CHECK(bodyOf(get(p, "/overlay.js")).contains("\xE2\x80\xA6"));
}

void testRefusals()
{
	OverlayServer server(VYRA_OVERLAY_DIR);
	CHECK(server.start(18450, 18459));
	const quint16 p = server.port();

	CHECK(get(p, "/nope").startsWith("HTTP/1.1 404"));
	CHECK(get(p, "/../../../etc/passwd").startsWith("HTTP/1.1 404"));
	CHECK(get(p, "/data/bibles/lsg1910.tsv").startsWith("HTTP/1.1 404"));
	CHECK(exchange(p, "POST /state HTTP/1.1\r\nContent-Length: 0\r\n\r\n").startsWith("HTTP/1.1 405"));
	CHECK(exchange(p, "hello\r\n\r\n").startsWith("HTTP/1.1 400"));
	CHECK(exchange(p, QByteArray("GET /") + QByteArray(20000, 'a')).startsWith("HTTP/1.1 431"));

	// A client that sends a partial request and then stays silent is dropped (after the 5 s timeout).
	QTcpSocket slow;
	slow.connectToHost(QHostAddress::LocalHost, p);
	waitFor([&] { return slow.state() == QAbstractSocket::ConnectedState; });
	slow.write("GET /sta");
	CHECK(waitFor([&] { return slow.state() == QAbstractSocket::UnconnectedState; }, 7000));

	// A request split in pieces still works.
	QTcpSocket pieces;
	pieces.connectToHost(QHostAddress::LocalHost, p);
	waitFor([&] { return pieces.state() == QAbstractSocket::ConnectedState; });
	pieces.write("GET /sta");
	pieces.flush();
	waitFor([] { return false; }, 100);
	pieces.write("te HTTP/1.1\r\n\r\n");
	QByteArray all;
	waitFor([&] {
		all += pieces.readAll();
		return pieces.state() == QAbstractSocket::UnconnectedState;
	});
	all += pieces.readAll();
	CHECK(all.startsWith("HTTP/1.1 200 OK") && bodyOf(all).contains("\"visible\":false"));

	// The server survives all of that.
	CHECK(get(p, "/state").startsWith("HTTP/1.1 200 OK"));
}

void testStateAndStream()
{
	OverlayServer server(VYRA_OVERLAY_DIR);
	CHECK(server.start(18460, 18469));
	const quint16 p = server.port();

	CHECK(bodyOf(get(p, "/state")) == "{\"rev\":0,\"visible\":false,\"theme\":\"lower\",\"page\":1,\"pages\":1,\"reference\":\"\",\"verses\":[]}");

	server.publish(onAir(1, "Jean 3:16", "Car Dieu"));
	CHECK(bodyOf(get(p, "/state")).contains("\"reference\":\"Jean 3:16\""));

	// A page that connects now is up to date at once, with no wait for the next publication.
	Stream a;
	CHECK(a.open(p));
	CHECK(waitFor([&] { return a.messages() == 1; }));
	CHECK(a.received.startsWith("HTTP/1.1 200 OK"));
	CHECK(a.received.contains("text/event-stream"));
	CHECK(a.received.contains("data: {\"rev\":1,\"visible\":true,\"theme\":\"lower\",\"page\":1,\"pages\":1,\"reference\":\"Jean 3:16\""));
	CHECK(waitFor([&] { return server.clientCount() == 1; }));

	// Every published frame reaches every client, in order.
	Stream b;
	CHECK(b.open(p));
	CHECK(waitFor([&] { return server.clientCount() == 2; }));
	server.publish(onAir(2, "Jean 3:17", "Dieu"));
	Frame hidden;
	hidden.revision = 3;
	server.publish(hidden);
	CHECK(waitFor([&] { return a.messages() == 3 && b.messages() == 3; }));
	const int i2 = a.received.indexOf("\"rev\":2"), i3 = a.received.indexOf("\"rev\":3");
	CHECK(i2 > 0 && i3 > i2);
	CHECK(b.received.contains("\"rev\":3,\"visible\":false"));

	// A client that goes away is forgotten; the others keep receiving.
	a.socket.abort();
	CHECK(waitFor([&] { return server.clientCount() == 1; }));
	server.publish(onAir(4, "Jean 3:18", "Celui qui croit"));
	CHECK(waitFor([&] { return b.messages() == 4; }));
	CHECK(b.received.contains("\"rev\":4"));
}

void testManyConnections()
{
	OverlayServer server(VYRA_OVERLAY_DIR);
	CHECK(server.start(18470, 18479));
	const quint16 p = server.port();

	// More stream clients than the limit: the extra ones are refused, the server stays healthy.
	std::vector<std::unique_ptr<Stream>> streams;
	for (int i = 0; i < OverlayServer::kMaxConnections + 8; ++i) {
		streams.push_back(std::make_unique<Stream>());
		streams.back()->open(p);
	}
	waitFor([&] { return false; }, 500);
	CHECK(server.clientCount() <= OverlayServer::kMaxConnections);
	CHECK(server.clientCount() >= OverlayServer::kMaxConnections - 1);

	streams.clear(); // all clients leave
	CHECK(waitFor([&] { return server.clientCount() == 0; }));
	CHECK(get(p, "/state").startsWith("HTTP/1.1 200 OK")); // and the server is still serving
}

void testPublishLargeFrameManyTimes()
{
	OverlayServer server(VYRA_OVERLAY_DIR);
	CHECK(server.start(18480, 18489));
	Stream s;
	CHECK(s.open(server.port()));
	CHECK(waitFor([&] { return server.clientCount() == 1; }));
	for (int i = 1; i <= 500; ++i)
		server.publish(onAir(static_cast<unsigned long long>(i), "Psaumes 119", "x"));
	CHECK(waitFor([&] { return s.messages() == 501; }));
	// every revision arrived, in order
	int last = -1;
	bool ordered = true;
	for (int i = 1; i <= 500; ++i) {
		const int at = s.received.indexOf(QByteArray("\"rev\":") + QByteArray::number(i) + ",");
		if (at < last)
			ordered = false;
		last = at;
	}
	CHECK(ordered);
}

void testHostileClients()
{
	OverlayServer server(VYRA_OVERLAY_DIR);
	CHECK(server.start(18490, 18499));
	const quint16 p = server.port();
	server.publish(onAir(1, "Jean 3:16", "Car Dieu"));

	// Random bytes and hostile requests: whatever happens, the server must answer the next honest client.
	std::mt19937 rng(7);
	for (int i = 0; i < 150; ++i) {
		QByteArray raw;
		switch (i % 6) {
		case 0: // pure garbage
			for (int k = 0; k < 200; ++k)
				raw.append(static_cast<char>(rng() & 0xff));
			raw += "\r\n\r\n";
			break;
		case 1: // enormous header
			raw = "GET /state HTTP/1.1\r\nX-Big: " + QByteArray(200000, 'a') + "\r\n\r\n";
			break;
		case 2: // many headers
			raw = "GET /state HTTP/1.1\r\n";
			for (int k = 0; k < 5000; ++k)
				raw += "X-" + QByteArray::number(k) + ": v\r\n";
			raw += "\r\n";
			break;
		case 3: // bad request lines
			raw = QByteArray("GET\r\n\r\n").repeated(1) + "\r\n";
			break;
		case 4: // traversal and odd paths
			raw = "GET /%2e%2e/%2e%2e/etc/passwd HTTP/1.1\r\n\r\n";
			break;
		default: // a NUL in the path, bad version
			raw = QByteArray("GET /st\0ate HTTP/9.9\r\n\r\n", 24);
			break;
		}
		const QByteArray answer = exchange(p, raw);
		// Either refused cleanly or served: never a crash, never a file outside the overlay folder.
		CHECK(!answer.contains("root:"));
		CHECK(answer != "CONNECT-FAILED");
	}
	// A client that sends garbage and never finishes is dropped by the timeout, while others are served.
	{
		QTcpSocket silent;
		silent.connectToHost(QHostAddress::LocalHost, p);
		CHECK(waitFor([&] { return silent.state() == QAbstractSocket::ConnectedState; }));
		silent.write("\x01\x02 garbage without end");
		CHECK(get(p, "/state").startsWith("HTTP/1.1 200 OK")); // served meanwhile
		CHECK(waitFor([&] { silent.readAll(); return silent.state() == QAbstractSocket::UnconnectedState; }, 8000));
	}
	const QByteArray state = get(p, "/state");
	CHECK(state.startsWith("HTTP/1.1 200 OK") && bodyOf(state).contains("\"reference\":\"Jean 3:16\""));

	// Streams that come and go while frames are published: no client is left behind, nothing crashes.
	for (int round = 0; round < 40; ++round) {
		std::vector<std::unique_ptr<Stream>> streams;
		for (int k = 0; k < 5; ++k) {
			streams.push_back(std::make_unique<Stream>());
			streams.back()->open(p);
		}
		server.publish(onAir(static_cast<unsigned long long>(100 + round), "Psaumes 23", "L'Éternel est mon berger"));
		if (round % 2)
			waitFor([&] { return false; }, 10);
		// the streams are destroyed here, some before their first message arrived
	}
	CHECK(waitFor([&] { return server.clientCount() == 0; }, 5000));
	CHECK(get(p, "/state").startsWith("HTTP/1.1 200 OK"));
}

} // namespace

int main(int argc, char **argv)
{
	QCoreApplication app(argc, argv);
	testListening();
	testPortFallbackAndFailure();
	testFiles();
	testRefusals();
	testStateAndStream();
	testManyConnections();
	testPublishLargeFrameManyTimes();
	testHostileClients();
	return vyra::testing::finish();
}
