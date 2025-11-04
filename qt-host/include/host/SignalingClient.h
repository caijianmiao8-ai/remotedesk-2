#pragma once

#include <QJsonObject>
#include <QObject>
#include <QTimer>
#include <QUrl>
#include <QWebSocket>

namespace host {

class SignalingClient : public QObject {
    Q_OBJECT
public:
    explicit SignalingClient(QObject *parent = nullptr);

    void connectTo(const QString &url, const QString &apiKey, const QString &topic, const QString &appToken);
    void disconnect();

    void sendSignal(const QJsonObject &payload);

signals:
    void connected();
    void joined();
    void messageReceived(const QJsonObject &message);
    void errorOccurred(const QString &message);
    void closed();
    void logMessage(const QString &line);

private slots:
    void handleConnected();
    void handleTextMessage(const QString &message);
    void handleClosed();
    void handleError(QAbstractSocket::SocketError error);
    void sendHeartbeat();

private:
    void sendJoin();
    void sendBroadcast(const QString &event, const QJsonObject &payload);
    void sendRaw(const QJsonObject &object);

    QWebSocket m_socket;
    QTimer m_heartbeat;
    QUrl m_url;
    QString m_apiKey;
    QString m_topic;
    QString m_appToken;
    quint64 m_refCounter = 1;
    bool m_joined = false;
};

}  // namespace host

