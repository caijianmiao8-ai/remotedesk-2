#include "host/SignalingClient.h"
#include <QJsonDocument>
#include <QUrlQuery>
#include <QNetworkRequest>

SignalingClient::SignalingClient(QObject* parent) : QObject(parent) {
    connect(&m_socket, &QWebSocket::connected, this, &SignalingClient::onSocketConnected);
    connect(&m_socket, &QWebSocket::textMessageReceived, this, &SignalingClient::onSocketTextMessage);
    connect(&m_socket, &QWebSocket::disconnected, this, &SignalingClient::onSocketClosed);
#if QT_VERSION >= QT_VERSION_CHECK(6,5,0)
    connect(&m_socket, &QWebSocket::errorOccurred, this, &SignalingClient::onSocketError);
#else
    connect(&m_socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onSocketError(QAbstractSocket::SocketError)));
#endif
    m_heartbeat.setInterval(30000);
    connect(&m_heartbeat, &QTimer::timeout, this, &SignalingClient::onHeartbeat);
}

void SignalingClient::setCredentials(const RealtimeCredentials& cred) { m_cred = cred; }
void SignalingClient::setAppToken(const QString& token) { m_appToken = token; }

QUrl SignalingClient::buildRealtimeWsUrl(const QUrl& endpoint, const QString& apikey, const QString& token) const {
    QUrl ws = endpoint;
    if (ws.scheme() == "https") ws.setScheme("wss");
    else if (ws.scheme() == "http") ws.setScheme("ws");
    ws.setPath("/realtime/v1/websocket");
    QUrlQuery q;
    q.addQueryItem("apikey", apikey);
    q.addQueryItem("vsn", "1.0.0");
    if (!token.isEmpty()) q.addQueryItem("token", token);
    ws.setQuery(q);
    return ws;
}

void SignalingClient::connectToRealtime() {
    if (!m_cred.endpoint.isValid() || m_cred.apiKey.isEmpty() || m_cred.topic.isEmpty()) {
        emit errorOccurred("Invalid realtime credentials");
        return;
    }
    const QUrl url = buildRealtimeWsUrl(m_cred.endpoint, m_cred.apiKey, m_cred.signedToken);
    QNetworkRequest req(url);
    if (!m_appToken.isEmpty())
        req.setRawHeader("Authorization", QByteArray("Bearer ").append(m_appToken.toUtf8()));
    m_socket.open(req);
}

void SignalingClient::disconnectFromRealtime() {
    m_heartbeat.stop();
    m_socket.close();
}

void SignalingClient::onSocketConnected() {
    emit connected();
    m_joined = false;
    sendJoin();
    m_heartbeat.start();
}

void SignalingClient::sendJoin() {
    const QJsonObject obj{
        {"topic",   m_cred.topic},
        {"event",   "phx_join"},
        {"payload", QJsonObject{}},
        {"ref",     QString::number(m_refCounter++)}
    };
    sendRaw(obj);
}

void SignalingClient::onHeartbeat() {
    const QJsonObject beat{
        {"topic",   "phoenix"},
        {"event",   "heartbeat"},
        {"payload", QJsonObject{}},
        {"ref",     QString::number(m_refCounter++)}
    };
    sendRaw(beat);
}

void SignalingClient::sendBroadcast(const QString& event, const QJsonObject& payload) {
    const QJsonObject obj{
        {"topic",   m_cred.topic},
        {"event",   "broadcast"},
        {"payload", QJsonObject{
            {"type",   "broadcast"},
            {"event",  event},       // "signal"
            {"payload", payload}
        }},
        {"ref",     QString::number(m_refCounter++)}
    };
    sendRaw(obj);
}

void SignalingClient::sendSignal(const SignalEnvelope& env) {
    QJsonObject inner = env.data;
    inner.insert("type", env.type);  // "offer"/"answer"/"ice"
    sendBroadcast("signal", inner);
}

void SignalingClient::sendRaw(const QJsonObject& obj) {
    m_socket.sendTextMessage(QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact)));
}

void SignalingClient::onSocketTextMessage(const QString& msg) {
    const auto obj = QJsonDocument::fromJson(msg.toUtf8()).object();
    const auto event = obj.value("event").toString();

    if (event == "phx_reply") {
        const auto payload = obj.value("payload").toObject();
        if (!m_joined && payload.value("status").toString() == "ok" &&
            obj.value("topic").toString() == m_cred.topic) {
            m_joined = true;
            emit joined();
        }
        return;
    }

    if (event == "broadcast") {
        const auto p = obj.value("payload").toObject();
        if (p.value("type").toString() == "broadcast" &&
            p.value("event").toString() == "signal") {
            const auto inner = p.value("payload").toObject();
            const auto t = inner.value("type").toString();
            QJsonObject data = inner; data.remove("type");
            emit signalReceived(SignalEnvelope{t, data});
        }
        return;
    }
}

void SignalingClient::onSocketClosed() { m_heartbeat.stop(); emit closed(); }
void SignalingClient::onSocketError(QAbstractSocket::SocketError) { emit errorOccurred(m_socket.errorString()); }
