#include "host/UiMainWindow.h"

#include "common/Protocol.h"
#include "host/AuthClient.h"
#include "host/SignalingClient.h"
#include "host/WebRtcPeer.h"

#include <QCheckBox>
#include <QComboBox>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPushButton>
#include <QIcon>
#include <QSystemTrayIcon>
#include <QTextEdit>
#include <QTimer>
#include <QUrlQuery>
#include <QVBoxLayout>
#include <memory>

namespace {
constexpr int kDefaultPollIntervalSeconds = 5;
}

namespace host {

UiMainWindow::UiMainWindow(QWidget *parent)
    : QMainWindow(parent), m_network(std::make_unique<QNetworkAccessManager>()) {
    m_authClient = std::make_unique<AuthClient>();
    m_signalingClient = std::make_unique<SignalingClient>();
    setupUi();
    setupTray();

    connect(m_authClient.get(), &AuthClient::deviceCodeReceived, this, &UiMainWindow::updateDeviceCodeUi);
    connect(m_authClient.get(), &AuthClient::approved, this, [this](const QString &token, const QString &userId) {
        Q_UNUSED(userId);
        m_appToken = token;
        updateStatus(tr("Device approved. Ready to join a session."));
        m_pollTimer.stop();
        enableUi(true);
    });
    connect(m_authClient.get(), &AuthClient::pending, this, [this]() {
        updateStatus(tr("Waiting for approval..."));
    });
    connect(m_authClient.get(), &AuthClient::errorOccurred, this, &UiMainWindow::handleAuthError);

    connect(&m_pollTimer, &QTimer::timeout, this, &UiMainWindow::pollDeviceCode);

    connect(m_signalingClient.get(), &SignalingClient::messageReceived, this, &UiMainWindow::handleRealtimeMessage);
    connect(m_signalingClient.get(), &SignalingClient::errorOccurred, this, &UiMainWindow::handleAuthError);
    connect(m_signalingClient.get(), &SignalingClient::logMessage, this, &UiMainWindow::handleLog);
    connect(m_signalingClient.get(), &SignalingClient::joined, this, [this]() {
        updateStatus(tr("Realtime channel joined."));
        createPeerIfNeeded();
    });

    startDeviceCodeFlow();
}

UiMainWindow::~UiMainWindow() {
    destroyPeer();
}

void UiMainWindow::setInitialCode(const QString &code) {
    m_initialCode = code;
    if (m_codeEdit) {
        m_codeEdit->setText(code);
    }
}

void UiMainWindow::setInitialScreenIndex(int index) {
    m_initialScreenIndex = index;
    if (m_screenCombo && index < m_screenCombo->count()) {
        m_screenCombo->setCurrentIndex(index);
    }
}

void UiMainWindow::setAllowControlDefault(bool enabled) {
    m_initialAllowControl = enabled;
    if (m_allowControlCheck) {
        m_allowControlCheck->setChecked(enabled);
    }
}

void UiMainWindow::setupUi() {
    auto *central = new QWidget(this);
    auto *layout = new QVBoxLayout(central);

    m_deviceCodeLabel = new QLabel(tr("Requesting device code..."), central);
    m_verificationUriLabel = new QLabel(tr("Visit https://ruoshui.fun/device"), central);
    m_statusLabel = new QLabel(tr("Idle"), central);

    m_codeEdit = new QLineEdit(central);
    m_codeEdit->setPlaceholderText(tr("Enter 6 digit session code"));
    m_codeEdit->setMaxLength(6);

    m_joinButton = new QPushButton(tr("Join"), central);
    m_disconnectButton = new QPushButton(tr("Disconnect"), central);
    m_disconnectButton->setEnabled(false);

    m_screenCombo = new QComboBox(central);
    m_screenCombo->addItem(tr("Primary"));
    m_screenCombo->addItem(tr("Secondary"));

    m_fpsCombo = new QComboBox(central);
    m_fpsCombo->addItem("30");
    m_fpsCombo->addItem("60");

    m_allowControlCheck = new QCheckBox(tr("Allow control"), central);

    m_logView = new QTextEdit(central);
    m_logView->setReadOnly(true);

    layout->addWidget(m_deviceCodeLabel);
    layout->addWidget(m_verificationUriLabel);
    layout->addWidget(m_statusLabel);
    layout->addWidget(m_codeEdit);
    layout->addWidget(m_joinButton);
    layout->addWidget(m_disconnectButton);
    layout->addWidget(new QLabel(tr("Screen"), central));
    layout->addWidget(m_screenCombo);
    layout->addWidget(new QLabel(tr("FPS"), central));
    layout->addWidget(m_fpsCombo);
    layout->addWidget(m_allowControlCheck);
    layout->addWidget(m_logView, 1);

    setCentralWidget(central);

    connect(m_joinButton, &QPushButton::clicked, this, &UiMainWindow::onJoinClicked);
    connect(m_disconnectButton, &QPushButton::clicked, this, &UiMainWindow::onDisconnectClicked);
    connect(m_allowControlCheck, &QCheckBox::toggled, this, &UiMainWindow::onAllowControlChanged);
}

void UiMainWindow::setupTray() {
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        return;
    }
    m_trayIcon = new QSystemTrayIcon(QIcon(), this);
    m_trayIcon->setToolTip(tr("RemoteDesk Host"));
    m_trayIcon->show();
}

void UiMainWindow::startDeviceCodeFlow() {
    updateStatus(tr("Requesting device code..."));
    m_authClient->start();
}

void UiMainWindow::pollDeviceCode() {
    if (m_deviceCodeInfo.deviceCode.isEmpty()) {
        return;
    }
    m_authClient->poll(m_deviceCodeInfo.deviceCode);
}

void UiMainWindow::updateDeviceCodeUi(const DeviceCodeInfo &info) {
    m_deviceCodeInfo = info;
    const QString codeText = info.userCode.isEmpty() ? tr("--") : info.userCode;
    m_deviceCodeLabel->setText(tr("Device code: <b>%1</b>").arg(codeText));
    m_verificationUriLabel->setText(tr("Visit <a href=\"%1\">%1</a> to approve." ).arg(info.verificationUri));
    m_verificationUriLabel->setTextFormat(Qt::RichText);
    m_verificationUriLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    m_verificationUriLabel->setOpenExternalLinks(true);

    const int interval = info.intervalSeconds > 0 ? info.intervalSeconds : kDefaultPollIntervalSeconds;
    m_pollTimer.start(interval * 1000);
    updateStatus(tr("Waiting for approval..."));
}

void UiMainWindow::updateStatus(const QString &text) {
    if (m_statusLabel) {
        m_statusLabel->setText(text);
    }
    handleLog(text);
}

void UiMainWindow::enableUi(bool enabled) {
    m_codeEdit->setEnabled(enabled);
    m_joinButton->setEnabled(enabled);
    m_screenCombo->setEnabled(enabled);
    m_fpsCombo->setEnabled(enabled);
    m_allowControlCheck->setEnabled(enabled);
}

void UiMainWindow::handleAuthError(const QString &message) {
    updateStatus(tr("Error: %1").arg(message));
}

void UiMainWindow::onJoinClicked() {
    const QString code = m_codeEdit->text().trimmed();
    if (code.length() != 6) {
        updateStatus(tr("Invalid code."));
        return;
    }
    if (m_appToken.isEmpty()) {
        updateStatus(tr("Device not approved yet."));
        return;
    }

    QJsonObject body;
    body.insert(protocol::json::kCode6, code);
    body.insert(protocol::json::kRole, protocol::json::kHostRole);

    QNetworkRequest request(QUrl(QStringLiteral("%1%2").arg(protocol::kApiBase, protocol::paths::kSessionJoin)));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setRawHeader("Authorization", QByteArray("Bearer ") + m_appToken.toUtf8());
    auto *reply = m_network->post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            handleAuthError(reply->errorString());
            return;
        }
        const auto json = QJsonDocument::fromJson(reply->readAll()).object();
        m_sessionId = json.value(protocol::json::kSessionId).toString();
        if (m_sessionId.isEmpty()) {
            handleAuthError(tr("Missing sessionId"));
            return;
        }
        updateStatus(tr("Joined session %1").arg(m_sessionId));
        m_disconnectButton->setEnabled(true);
        enableUi(false);
        joinRealtimeChannel(m_sessionId);
    });
}

void UiMainWindow::onDisconnectClicked() {
    destroyPeer();
    if (m_sessionId.isEmpty()) {
        return;
    }

    QJsonObject body;
    body.insert(protocol::json::kSessionId, m_sessionId);

    QNetworkRequest request(QUrl(QStringLiteral("%1%2").arg(protocol::kApiBase, protocol::paths::kSessionClose)));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setRawHeader("Authorization", QByteArray("Bearer ") + m_appToken.toUtf8());
    auto *reply = m_network->post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);

    m_sessionId.clear();
    enableUi(true);
    m_disconnectButton->setEnabled(false);
    updateStatus(tr("Disconnected."));
}

void UiMainWindow::onAllowControlChanged(bool enabled) {
    m_allowControl = enabled;
    if (m_peer) {
        m_peer->setAllowControl(enabled);
    }
}

void UiMainWindow::handleRealtimeMessage(const QJsonObject &message) {
    if (!m_peer) {
        return;
    }
    m_peer->handleSignal(message);
}

void UiMainWindow::handlePeerStateChanged(const QString &state) {
    updateStatus(state);
}

void UiMainWindow::handleLog(const QString &line) {
    if (!m_logView) {
        return;
    }
    m_logView->append(line);
}

void UiMainWindow::createPeerIfNeeded() {
    if (m_peer) {
        return;
    }
    m_peer = std::make_unique<WebRtcPeer>(m_signalingClient.get(), this);
    connect(m_peer.get(), &WebRtcPeer::stateChanged, this, &UiMainWindow::handlePeerStateChanged);
    connect(m_peer.get(), &WebRtcPeer::logLine, this, &UiMainWindow::handleLog);
    WebRtcPeer::Options options;
    options.allowControl = m_allowControl;
    options.screenIndex = m_screenCombo->currentIndex();
    options.fps = m_fpsCombo->currentText().toInt();
    m_peer->setOptions(options);
    m_peer->setIceConfig(m_iceConfig);
    m_peer->start();
}

void UiMainWindow::destroyPeer() {
    if (!m_peer) {
        return;
    }
    m_peer->stop();
    m_peer.reset();
}

void UiMainWindow::joinRealtimeChannel(const QString &sessionId) {
    // Fetch signed topic
    QUrl url(QStringLiteral("%1%2").arg(protocol::kApiBase, protocol::paths::kRealtimeSignedTopic));
    QUrlQuery query;
    query.addQueryItem(protocol::json::kSessionId, sessionId);
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setRawHeader("Authorization", QByteArray("Bearer ") + m_appToken.toUtf8());

    auto *reply = m_network->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            handleAuthError(reply->errorString());
            return;
        }
        const auto json = QJsonDocument::fromJson(reply->readAll()).object();
        m_realtimeEndpoint = json.value(protocol::json::kEndpoint).toString();
        m_realtimeApiKey = json.value(protocol::json::kApiKey).toString();
        m_realtimeTopic = json.value(protocol::json::kTopic).toString();
        m_realtimeSignedToken = json.value(protocol::json::kSignedToken).toString();
        m_realtimeExpiresAt = json.value(protocol::json::kExpiresAt).toString();

        if (m_realtimeEndpoint.isEmpty() || m_realtimeApiKey.isEmpty() || m_realtimeTopic.isEmpty()) {
            handleAuthError(tr("Incomplete realtime metadata"));
            return;
        }

        // Fetch ICE configuration
        QNetworkRequest iceRequest(QUrl(QStringLiteral("%1%2").arg(protocol::kApiBase, protocol::paths::kIce)));
        auto *iceReply = m_network->get(iceRequest);
        connect(iceReply, &QNetworkReply::finished, this, [this, iceReply]() {
            iceReply->deleteLater();
            if (iceReply->error() != QNetworkReply::NoError) {
                handleAuthError(iceReply->errorString());
                return;
            }
            const auto json = QJsonDocument::fromJson(iceReply->readAll()).object();
            const auto servers = json.value(protocol::json::kIceServers).toArray();
            m_iceConfig.servers.clear();
            for (const auto &serverValue : servers) {
                const auto serverObj = serverValue.toObject();
                IceServerConfig server;
                server.urls = serverObj.value("urls").toString();
                server.username = serverObj.value("username").toString();
                server.credential = serverObj.value("credential").toString();
                m_iceConfig.servers.append(server);
            }

            const QString wsUrl = QStringLiteral("wss://%1/realtime/v1/websocket?apikey=%2&vsn=1.0.0")
                                      .arg(m_realtimeEndpoint, m_realtimeApiKey);
            m_signalingClient->connectTo(wsUrl, m_realtimeApiKey, m_realtimeTopic, m_appToken);
        });
    });
}

}  // namespace host

