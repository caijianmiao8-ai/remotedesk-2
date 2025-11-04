#include "host/SignalingClient.h"

#include "common/Protocol.h"

#include <QAbstractSocket>
#include <QJsonDocument>
#include <QLatin1String>
#include <QNetworkRequest>

namespace host {

namespace {
QLatin1String toKey(const char *key) {
    return QLatin1String(key);
}
}  // namespace

SignalingClient::SignalingClient(QObject *parent) : QObject(parent) {
    connect(&m_socket, &QWebSocket::connected, this, &SignalingClient::handleConnected);
    connect(&m_socket, &QWebSocket::textMessageReceived, this, &SignalingClient::handleTextMessage);
    connect(&m_socket, &QWebSocket::disconnected, this, &SignalingClient::handleClosed);
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    connect(&m_socket, &QWebSocket::errorOccurred, this, &SignalingClient::handleError);
#else
    connect(&m_socket,
            QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
            this,
            &SignalingClient::handleError);
#endif

    m_heartbeat.setInterval(30000);
    connect(&m_heartbeat, &QTimer::timeout, this, &SignalingClient::sendHeartbeat);
}

void SignalingClient::connectTo(const QString &url,
                                const QString &apiKey,
                                const QString &topic,
                                const QString &appToken) {
    if (m_socket.state() != QAbstractSocket::UnconnectedState) {
        m_socket.close();
    }

    m_url = QUrl(url);
    m_apiKey = apiKey;
    m_topic = topic;
    m_appToken = appToken;
    m_joined = false;

    if (!m_url.isValid()) {
        emit errorOccurred(tr("Invalid realtime URL: %1").arg(url));
        return;
    }

    emit logMessage(tr("Connecting to realtime %1").arg(m_url.toString()));

    QNetworkRequest request(m_url);
    if (!m_apiKey.isEmpty()) {
        request.setRawHeader("apikey", m_apiKey.toUtf8());
    }
    if (!m_appToken.isEmpty()) {
        request.setRawHeader("Authorization", QByteArray("Bearer ").append(m_appToken.toUtf8()));
    }

    m_socket.open(request);
}

void SignalingClient::disconnect() {
    m_heartbeat.stop();
    m_joined = false;
    if (m_socket.state() != QAbstractSocket::UnconnectedState) {
        m_socket.close();
    }
}

void SignalingClient::sendSignal(const QJsonObject &payload) {
    if (m_topic.isEmpty()) {
        emit logMessage(tr("Cannot send signal without a topic."));
        return;
    }
    sendBroadcast(QLatin1String(protocol::json::kSignal), payload);
}

void SignalingClient::handleConnected() {
    emit connected();
    emit logMessage(tr("Realtime socket connected"));
    m_joined = false;
    sendJoin();
    m_heartbeat.start();
}

void SignalingClient::handleTextMessage(const QString &message) {
    const auto document = QJsonDocument::fromJson(message.toUtf8());
    if (!document.isObject()) {
        emit logMessage(tr("Ignoring non-object realtime payload"));
        return;
    }

    const auto object = document.object();
    const QString event = object.value(toKey(protocol::json::kEvent)).toString();
    const QString topic = object.value(toKey(protocol::json::kTopic)).toString();

    if (event == QLatin1String("phx_reply")) {
        const auto payload = object.value(toKey(protocol::json::kPayload)).toObject();
        const QString status = payload.value(QLatin1String("status")).toString();
        if (!m_joined && status == QLatin1String("ok") && topic == m_topic) {
            m_joined = true;
            emit logMessage(tr("Realtime channel joined"));
            emit joined();
        }
        return;
    }

    if (event == QLatin1String(protocol::json::kBroadcast)) {
        const auto payload = object.value(toKey(protocol::json::kPayload)).toObject();
        const QString type = payload.value(toKey(protocol::json::kType)).toString();
        if (type != QLatin1String(protocol::json::kBroadcast)) {
            return;
        }
        if (payload.value(QLatin1String("event")).toString() != QLatin1String(protocol::json::kSignal)) {
            return;
        }
        const auto inner = payload.value(toKey(protocol::json::kPayload)).toObject();
        emit messageReceived(inner);
        return;
    }
}

void SignalingClient::handleClosed() {
    m_heartbeat.stop();
    m_joined = false;
    emit logMessage(tr("Realtime socket closed"));
    emit closed();
}

void SignalingClient::handleError(QAbstractSocket::SocketError) {
    const QString errorText = m_socket.errorString();
    emit logMessage(tr("Realtime socket error: %1").arg(errorText));
    emit errorOccurred(errorText);
}

void SignalingClient::sendHeartbeat() {
    QJsonObject payload;
    payload.insert(toKey(protocol::json::kTopic), QStringLiteral("phoenix"));
    payload.insert(toKey(protocol::json::kEvent), QLatin1String(protocol::json::kHeartbeat));
    payload.insert(toKey(protocol::json::kPayload), QJsonObject{});
    payload.insert(toKey(protocol::json::kRef), QString::number(m_refCounter++));
    sendRaw(payload);
}

void SignalingClient::sendJoin() {
    QJsonObject payload;
    if (!m_appToken.isEmpty()) {
        QJsonObject headers;
        headers.insert(QStringLiteral("Authorization"), QStringLiteral("Bearer %1").arg(m_appToken));
        QJsonObject config;
        config.insert(QStringLiteral("headers"), headers);
        payload.insert(QStringLiteral("config"), config);
    }

    QJsonObject message;
    message.insert(toKey(protocol::json::kTopic), m_topic);
    message.insert(toKey(protocol::json::kEvent), QLatin1String(protocol::json::kPhxJoin));
    message.insert(toKey(protocol::json::kPayload), payload);
    message.insert(toKey(protocol::json::kRef), QString::number(m_refCounter++));

    sendRaw(message);
}

void SignalingClient::sendBroadcast(const QString &event, const QJsonObject &payload) {
    QJsonObject outerPayload;
    outerPayload.insert(toKey(protocol::json::kType), QLatin1String(protocol::json::kBroadcast));
    outerPayload.insert(QStringLiteral("event"), event);
    outerPayload.insert(toKey(protocol::json::kPayload), payload);

    QJsonObject message;
    message.insert(toKey(protocol::json::kTopic), m_topic);
    message.insert(toKey(protocol::json::kEvent), QLatin1String(protocol::json::kBroadcast));
    message.insert(toKey(protocol::json::kPayload), outerPayload);
    message.insert(toKey(protocol::json::kRef), QString::number(m_refCounter++));

    sendRaw(message);
}

void SignalingClient::sendRaw(const QJsonObject &object) {
    const QByteArray data = QJsonDocument(object).toJson(QJsonDocument::Compact);
    m_socket.sendTextMessage(QString::fromUtf8(data));
}

}  // namespace host

