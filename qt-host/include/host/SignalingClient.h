#pragma once
#include <QObject>
#include <QUrl>
#include <QWebSocket>
#include <QTimer>
#include <QJsonObject>

struct RealtimeCredentials {
    QUrl     endpoint;     // e.g. https://xxxx.supabase.co
    QString  apiKey;       // anon key
    QString  topic;        // "remote:<sessionId>"
    QString  signedToken;  // optional
};

struct SignalEnvelope {
    QString     type;      // "offer" | "answer" | "ice"
    QJsonObject data;      // sdp/candidate JSON
};

class SignalingClient : public QObject {
    Q_OBJECT
public:
    explicit SignalingClient(QObject* parent = nullptr);
    void setCredentials(const RealtimeCredentials& cred);
    void setAppToken(const QString& appToken);
    void connectToRealtime();
    void disconnectFromRealtime();
    void sendSignal(const SignalEnvelope& env);

signals:
    void connected();
    void joined();
    void signalReceived(const SignalEnvelope& env);
    void errorOccurred(const QString& message);
    void closed();

private slots:
    void onSocketConnected();
    void onSocketTextMessage(const QString& msg);
    void onSocketClosed();
    void onSocketError(QAbstractSocket::SocketError e);
    void onHeartbeat();

private:
    void sendJoin();
    void sendBroadcast(const QString& event, const QJsonObject& payload);
    void sendRaw(const QJsonObject& obj);
    QUrl buildRealtimeWsUrl(const QUrl& endpoint, const QString& apikey, const QString& token) const;

private:
    QWebSocket   m_socket;
    QTimer       m_heartbeat;
    RealtimeCredentials m_cred;
    QString      m_appToken;
    quint64      m_refCounter = 1;
    bool         m_joined = false;
};
