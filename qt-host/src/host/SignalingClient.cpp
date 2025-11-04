#include "host/SignalingClient.h"

#include "common/Protocol.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QUrl>
#include <QWebSocket>
#include <QAbstractSocket>

namespace host {

SignalingClient::SignalingClient(QObject *parent) : QObject(parent), m_socket(std::make_unique<QWebSocket>()) {
    connect(m_socket.get(), &QWebSocket::connected, this, &SignalingClient::handleConnected);
    connect(m_socket.get(), &QWebSocket::disconnected, this, &SignalingClient::handleDisconnected);
    connect(m_socket.get(), &QWebSocket::textMessageReceived, this, &SignalingClient::handleTextMessage);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    connect(m_socket.get(), &QWebSocket::errorOccurred, this, &SignalingClient::handleError);
#else
    connect(m_socket.get(), SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(handleError()));
#endif
}

SignalingClient::~SignalingClient() {
    disconnectFromServer();
}

void SignalingClient::connectTo(const QString &endpoint, const QString &apiKey, const QString &topic, const QString &appToken) {
    Q_UNUSED(apiKey);
    m_topic = topic;
    m_appToken = appToken;
    m_joined = false;

    if (m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->close();
    }

    if (!m_heartbeatTimer) {
        m_heartbeatTimer = new QTimer(this);
        m_heartbeatTimer->setInterval(30000);
        connect(m_heartbeatTimer, &QTimer::timeout, this, &SignalingClient::sendHeartbeat);
    }

    m_socket->open(QUrl(endpoint));
    emit logMessage(tr("Connecting to %1").arg(endpoint));
}

void SignalingClient::disconnectFromServer() {
    if (m_socket && m_socket->isValid()) {
        m_socket->close();
    }
    if (m_heartbeatTimer) {
        m_heartbeatTimer->stop();
    }
    m_joined = false;
}

void SignalingClient::sendSignal(const QJsonObject &payload) {
    if (!m_joined) {
        return;
    }

    QJsonObject broadcastPayload;
    broadcastPayload.insert(protocol::json::kType, protocol::json::kBroadcast);
    broadcastPayload.insert(protocol::json::kEvent, protocol::json::kSignal);
    broadcastPayload.insert(protocol::json::kPayload, payload);

    QJsonObject envelope;
    envelope.insert("topic", m_topic);
    envelope.insert("event", "broadcast");
    envelope.insert("payload", broadcastPayload);
    envelope.insert("ref", QString::number(m_refCounter++));

    sendEnvelope(envelope);
}

void SignalingClient::handleConnected() {
    emit connected();
    sendJoin();
    if (m_heartbeatTimer) {
        m_heartbeatTimer->start();
    }
}

void SignalingClient::handleDisconnected() {
    if (m_heartbeatTimer) {
        m_heartbeatTimer->stop();
    }
    m_joined = false;
    emit disconnected();
}

void SignalingClient::handleTextMessage(const QString &message) {
    const auto jsonDoc = QJsonDocument::fromJson(message.toUtf8());
    if (!jsonDoc.isObject()) {
        emit errorOccurred(tr("Invalid JSON from signaling"));
        return;
    }
    const auto obj = jsonDoc.object();
    const QString event = obj.value("event").toString();
    const QString topic = obj.value("topic").toString();
    const auto payloadValue = obj.value("payload");

    if (event == QStringLiteral("phx_reply") && topic == m_topic) {
        const auto payloadObj = payloadValue.toObject();
        if (payloadObj.value("status").toString() == QStringLiteral("ok")) {
            m_joined = true;
            emit logMessage(tr("Joined topic %1").arg(m_topic));
            emit joined();
        }
        return;
    }

    if (event == QStringLiteral("broadcast")) {
        const auto payloadObj = payloadValue.toObject();
        const QString type = payloadObj.value(protocol::json::kType).toString();
        const QString innerEvent = payloadObj.value(protocol::json::kEvent).toString();
        const auto messagePayload = payloadObj.value(protocol::json::kPayload).toObject();
        if (type == protocol::json::kBroadcast && innerEvent == protocol::json::kSignal) {
            emit messageReceived(messagePayload);
        }
    }
}

void SignalingClient::handleError() {
    emit errorOccurred(m_socket->errorString());
}

void SignalingClient::sendJoin() {
    QJsonObject payload;
    if (!m_appToken.isEmpty()) {
        payload.insert("authorization", QStringLiteral("Bearer %1").arg(m_appToken));
    }
    QJsonObject envelope;
    envelope.insert("topic", m_topic);
    envelope.insert("event", "phx_join");
    envelope.insert("payload", payload);
    envelope.insert("ref", QString::number(m_refCounter++));

    sendEnvelope(envelope);
}

void SignalingClient::sendHeartbeat() {
    QJsonObject envelope;
    envelope.insert("topic", QStringLiteral("phoenix"));
    envelope.insert("event", QStringLiteral("heartbeat"));
    envelope.insert("payload", QJsonObject());
    envelope.insert("ref", QString::number(m_refCounter++));
    sendEnvelope(envelope);
}

void SignalingClient::sendEnvelope(const QJsonObject &payload) {
    if (!m_socket->isValid()) {
        return;
    }
    const QByteArray data = QJsonDocument(payload).toJson(QJsonDocument::Compact);
    m_socket->sendTextMessage(QString::fromUtf8(data));
}

}  // namespace host

