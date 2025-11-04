#pragma once

#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QUrl>
#include <QTimer>
#include <memory>

QT_BEGIN_NAMESPACE
class QWebSocket;
QT_END_NAMESPACE

namespace host {

class SignalingClient : public QObject {
    Q_OBJECT
public:
    explicit SignalingClient(QObject *parent = nullptr);
    ~SignalingClient() override;

    void connectTo(const QString &endpoint, const QString &apiKey, const QString &topic, const QString &appToken);
    void disconnectFromServer();

    void sendSignal(const QJsonObject &payload);

signals:
    void connected();
    void disconnected();
    void joined();
    void messageReceived(const QJsonObject &payload);
    void errorOccurred(const QString &message);
    void logMessage(const QString &line);

private slots:
    void handleConnected();
    void handleDisconnected();
    void handleTextMessage(const QString &message);
    void handleError();

private:
    void sendJoin();
    void sendHeartbeat();
    void sendEnvelope(const QJsonObject &payload);

    std::unique_ptr<QWebSocket> m_socket;
    QString m_topic;
    QString m_appToken;
    int m_refCounter = 1;
    QTimer *m_heartbeatTimer = nullptr;
    bool m_joined = false;
};

}  // namespace host

