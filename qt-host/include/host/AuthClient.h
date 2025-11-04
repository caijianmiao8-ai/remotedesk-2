#pragma once

#include <QJsonObject>
#include <QObject>
#include <QUrl>
#include <memory>

class QNetworkAccessManager;
class QNetworkReply;

namespace host {

struct DeviceCodeInfo;

class AuthClient : public QObject {
    Q_OBJECT
public:
    explicit AuthClient(QObject *parent = nullptr);
    ~AuthClient() override;

    void start();
    void poll(const QString &deviceCode);

    QString appToken() const { return m_appToken; }
    QString userId() const { return m_userId; }

signals:
    void deviceCodeReceived(const DeviceCodeInfo &info);
    void approved(const QString &appToken, const QString &userId);
    void pending();
    void errorOccurred(const QString &message);

private:
    void handleStartReply(QNetworkReply *reply);
    void handlePollReply(QNetworkReply *reply);
    void emitNetworkError(QNetworkReply *reply, const QString &context);

    std::unique_ptr<QNetworkAccessManager> m_network;
    QString m_appToken;
    QString m_userId;
};

}  // namespace host

