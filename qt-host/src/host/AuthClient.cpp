#include "host/AuthClient.h"

#include "common/Protocol.h"
#include "host/UiMainWindow.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

namespace host {

AuthClient::AuthClient(QObject *parent) : QObject(parent), m_network(std::make_unique<QNetworkAccessManager>()) {}

AuthClient::~AuthClient() = default;

void AuthClient::start() {
    QNetworkRequest request(QUrl(QStringLiteral("%1%2").arg(protocol::kApiBase, protocol::paths::kDeviceStart)));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    auto *reply = m_network->post(request, QByteArrayLiteral("{}"));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        handleStartReply(reply);
        reply->deleteLater();
    });
}

void AuthClient::poll(const QString &deviceCode) {
    QJsonObject body;
    body.insert(protocol::json::kDeviceCode, deviceCode);

    QNetworkRequest request(QUrl(QStringLiteral("%1%2").arg(protocol::kApiBase, protocol::paths::kDevicePoll)));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    auto *reply = m_network->post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        handlePollReply(reply);
        reply->deleteLater();
    });
}

void AuthClient::handleStartReply(QNetworkReply *reply) {
    if (reply->error() != QNetworkReply::NoError) {
        emitNetworkError(reply, tr("device/start"));
        return;
    }

    const auto json = QJsonDocument::fromJson(reply->readAll()).object();
    DeviceCodeInfo info;
    info.deviceCode = json.value(protocol::json::kDeviceCode).toString();
    info.userCode = json.value(protocol::json::kUserCode).toString();
    info.verificationUri = json.value(protocol::json::kVerificationUri).toString();
    info.intervalSeconds = json.value(protocol::json::kInterval).toInt();
    info.expiresInSeconds = json.value(protocol::json::kExpiresIn).toInt();

    emit deviceCodeReceived(info);
}

void AuthClient::handlePollReply(QNetworkReply *reply) {
    if (reply->error() != QNetworkReply::NoError) {
        emitNetworkError(reply, tr("device/poll"));
        return;
    }

    const auto json = QJsonDocument::fromJson(reply->readAll()).object();
    const QString status = json.value(protocol::json::kStatus).toString();

    if (status == protocol::json::kApproved) {
        m_appToken = json.value(protocol::json::kAppToken).toString();
        const auto userObj = json.value(protocol::json::kUser).toObject();
        m_userId = userObj.value(protocol::json::kUserId).toString();
        emit approved(m_appToken, m_userId);
    } else {
        emit pending();
    }
}

void AuthClient::emitNetworkError(QNetworkReply *reply, const QString &context) {
    const QString message = tr("%1: %2").arg(context, reply->errorString());
    emit errorOccurred(message);
}

}  // namespace host

