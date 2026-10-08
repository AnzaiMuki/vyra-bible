/*
VYRA Bible - OBS Studio plugin
Copyright (C) 2026 VYRA Concept
SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "src/overlay/overlay_server.hpp"

#include <QDir>
#include <QFile>
#include <QHostAddress>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>

#include <algorithm>

#include "src/overlay/http_routes.hpp"

namespace vyra::overlay {

namespace {

constexpr int kRequestTimeoutMs = 5000; // a client that does not finish its request is dropped
constexpr int kKeepAliveMs = 15000;
constexpr qint64 kMaxBacklogBytes = 1 << 20; // a stream client this far behind is dropped, never waited for

QByteArray bytes(const std::string &s)
{
	return QByteArray(s.data(), static_cast<qsizetype>(s.size()));
}

} // namespace

OverlayServer::OverlayServer(const QString &overlayDir, QObject *parent) : QObject(parent), overlayDir_(overlayDir)
{
	server_ = new QTcpServer(this);
	connect(server_, &QTcpServer::newConnection, this, &OverlayServer::onNewConnection);

	keepAlive_ = new QTimer(this);
	keepAlive_->setInterval(kKeepAliveMs);
	connect(keepAlive_, &QTimer::timeout, this, &OverlayServer::sendKeepAlive);

	latestJson_ = bytes(toJson(Frame{})); // before anything is published: nothing on the air
}

OverlayServer::~OverlayServer() = default;

bool OverlayServer::start(quint16 firstPort, quint16 lastPort, QString *error)
{
	for (quint16 p = firstPort; p <= lastPort; ++p) {
		if (server_->listen(QHostAddress::LocalHost, p)) {
			keepAlive_->start();
			return true;
		}
		if (p == 65535)
			break;
	}
	if (error)
		*error = server_->errorString();
	return false;
}

quint16 OverlayServer::port() const
{
	return server_->isListening() ? server_->serverPort() : 0;
}

void OverlayServer::publish(const Frame &frame)
{
	latestJson_ = bytes(toJson(frame));
	const QByteArray message = bytes(sseMessage(latestJson_.toStdString()));
	// Iterate over a copy: dropStream edits streams_.
	const std::vector<QTcpSocket *> targets = streams_;
	for (QTcpSocket *socket : targets) {
		if (socket->bytesToWrite() > kMaxBacklogBytes) {
			dropStream(socket);
			continue;
		}
		socket->write(message);
	}
}

void OverlayServer::onNewConnection()
{
	while (QTcpSocket *socket = server_->nextPendingConnection()) {
		socket->setParent(this);
		if (findChildren<QTcpSocket *>(Qt::FindDirectChildrenOnly).size() > kMaxConnections) {
			socket->abort();
			socket->deleteLater();
			continue;
		}
		connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
		connect(socket, &QTcpSocket::readyRead, this, [this, socket] { onReadyRead(socket); });
		QTimer::singleShot(kRequestTimeoutMs, socket, [this, socket] {
			// Still not answered (not a stream, not closing): the client was too slow.
			if (socket->state() == QAbstractSocket::ConnectedState &&
			    std::find(streams_.begin(), streams_.end(), socket) == streams_.end() &&
			    !socket->property("answered").toBool())
				socket->abort();
		});
	}
}

void OverlayServer::onReadyRead(QTcpSocket *socket)
{
	if (socket->property("answered").toBool()) {
		socket->readAll(); // anything after the request is ignored
		return;
	}
	QByteArray received = socket->property("received").toByteArray();
	received += socket->readAll();

	bool complete = false;
	const Route route =
		routeRequest(std::string_view(received.constData(), static_cast<size_t>(received.size())), complete);
	if (!complete) {
		socket->setProperty("received", received);
		return;
	}
	socket->setProperty("received", QByteArray());
	socket->setProperty("answered", true);

	switch (route) {
	case Route::Page:
		serveFile(socket, "index.html", "text/html; charset=utf-8");
		break;
	case Route::Style:
		serveFile(socket, "overlay.css", "text/css; charset=utf-8");
		break;
	case Route::Script:
		serveFile(socket, "overlay.js", "text/javascript; charset=utf-8");
		break;
	case Route::State:
		respond(socket,
			bytes(okHead("application/json; charset=utf-8", static_cast<size_t>(latestJson_.size()))),
			latestJson_);
		break;
	case Route::Events:
		startStream(socket);
		break;
	default:
		respond(socket, bytes(errorResponse(route)), QByteArray());
		break;
	}
}

void OverlayServer::respond(QTcpSocket *socket, const QByteArray &head, const QByteArray &body)
{
	socket->write(head);
	if (!body.isEmpty())
		socket->write(body);
	socket->disconnectFromHost(); // flushes what is pending, then closes
}

void OverlayServer::serveFile(QTcpSocket *socket, const char *name, const char *contentType)
{
	// Only the three fixed names above ever reach this point: no path comes from the client.
	QFile file(QDir(overlayDir_).filePath(QString::fromLatin1(name)));
	if (!file.open(QIODevice::ReadOnly)) {
		respond(socket, bytes(errorResponse(Route::NotFound)), QByteArray());
		return;
	}
	const QByteArray body = file.readAll();
	respond(socket, bytes(okHead(contentType, static_cast<size_t>(body.size()))), body);
}

void OverlayServer::startStream(QTcpSocket *socket)
{
	socket->write(bytes(eventStreamHead()));
	socket->write(bytes(sseMessage(latestJson_.toStdString()))); // the page is up to date from its first instant
	streams_.push_back(socket);
	connect(socket, &QTcpSocket::disconnected, this, [this, socket] { dropStream(socket); });
}

void OverlayServer::dropStream(QTcpSocket *socket)
{
	streams_.erase(std::remove(streams_.begin(), streams_.end(), socket), streams_.end());
	if (socket->state() != QAbstractSocket::UnconnectedState)
		socket->abort();
}

void OverlayServer::sendKeepAlive()
{
	const QByteArray ping = bytes(sseKeepAlive());
	const std::vector<QTcpSocket *> targets = streams_;
	for (QTcpSocket *socket : targets) {
		if (socket->bytesToWrite() > kMaxBacklogBytes)
			dropStream(socket);
		else
			socket->write(ping);
	}
}

} // namespace vyra::overlay
