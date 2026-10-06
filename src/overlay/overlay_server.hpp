/*
VYRA Bible - OBS Studio plugin
Copyright (C) 2026 VYRA Concept
SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

/*
 * The local web server that feeds the overlay page shown in an OBS Browser Source.
 *
 *   GET /          the overlay page (data/overlay/index.html), also /overlay.css and /overlay.js
 *   GET /state     the current frame, as JSON
 *   GET /events    Server-Sent Events: the current frame at once, then every published frame
 *
 * Listens on 127.0.0.1 only: nothing outside this computer can reach it. Read-only: it accepts GET and
 * nothing else, and serves only the three overlay files, never any other path.
 * HTTP parsing lives in http_routes (tested without sockets); this class only moves bytes.
 */

#include <QObject>
#include <QString>

#include <memory>
#include <vector>

#include "src/overlay/overlay_frame.hpp"

class QTcpServer;
class QTcpSocket;
class QTimer;

namespace vyra::overlay {

class OverlayServer : public QObject {
	Q_OBJECT

public:
	/** @param overlayDir  folder holding index.html, overlay.css and overlay.js (read at each request). */
	explicit OverlayServer(const QString &overlayDir, QObject *parent = nullptr);
	~OverlayServer() override;

	/** Listens on the first free port of [firstPort, lastPort]. False (with @p error filled) if none. */
	bool start(quint16 firstPort, quint16 lastPort, QString *error = nullptr);
	/** The port in use, 0 if not listening. */
	quint16 port() const;

	/** Sends @p frame to every connected overlay page, and remembers it for the ones that connect later. */
	void publish(const Frame &frame);

	int clientCount() const { return static_cast<int>(streams_.size()); }

	static constexpr int kMaxConnections = 32;

private:

	void onNewConnection();
	void onReadyRead(QTcpSocket *socket);
	void respond(QTcpSocket *socket, const QByteArray &head, const QByteArray &body);
	void serveFile(QTcpSocket *socket, const char *name, const char *contentType);
	void startStream(QTcpSocket *socket);
	void dropStream(QTcpSocket *socket);
	void sendKeepAlive();

	QString overlayDir_;
	QTcpServer *server_ = nullptr;
	QTimer *keepAlive_ = nullptr;
	QByteArray latestJson_;
	std::vector<QTcpSocket *> streams_; // sockets of /events, owned by Qt's parent mechanism
};

} // namespace vyra::overlay
