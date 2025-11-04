#pragma once

#include <QJsonObject>
#include <QMainWindow>
#include <QString>
#include <QTimer>
#include <functional>
#include <memory>

#include "host/IceConfig.h"

QT_BEGIN_NAMESPACE
class QLabel;
class QLineEdit;
class QPushButton;
class QComboBox;
class QCheckBox;
class QTextEdit;
class QSystemTrayIcon;
class QNetworkAccessManager;
QT_END_NAMESPACE

namespace host {

class AuthClient;
class SignalingClient;
class WebRtcPeer;

struct DeviceCodeInfo {
    QString deviceCode;
    QString userCode;
    QString verificationUri;
    int intervalSeconds = 5;
    int expiresInSeconds = 300;
};

class UiMainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit UiMainWindow(QWidget *parent = nullptr);
    ~UiMainWindow() override;

    void setInitialCode(const QString &code);
    void setInitialScreenIndex(int index);
    void setAllowControlDefault(bool enabled);

signals:
    void appTokenAvailable(const QString &token);
    void sessionJoined(const QString &sessionId);
    void sessionClosed();

private slots:
    void startDeviceCodeFlow();
    void pollDeviceCode();
    void onJoinClicked();
    void onDisconnectClicked();
    void onAllowControlChanged(bool enabled);
    void handleAuthError(const QString &message);
    void handleRealtimeMessage(const QJsonObject &message);
    void handlePeerStateChanged(const QString &state);
    void handleLog(const QString &line);

private:
    void setupUi();
    void setupTray();
    void updateDeviceCodeUi(const DeviceCodeInfo &info);
    void updateStatus(const QString &text);
    void enableUi(bool enabled);
    void createPeerIfNeeded();
    void destroyPeer();
    void joinRealtimeChannel(const QString &sessionId);

    std::unique_ptr<AuthClient> m_authClient;
    std::unique_ptr<SignalingClient> m_signalingClient;
    std::unique_ptr<WebRtcPeer> m_peer;
    std::unique_ptr<QNetworkAccessManager> m_network;

    QLabel *m_deviceCodeLabel = nullptr;
    QLabel *m_verificationUriLabel = nullptr;
    QLabel *m_statusLabel = nullptr;
    QLineEdit *m_codeEdit = nullptr;
    QPushButton *m_joinButton = nullptr;
    QPushButton *m_disconnectButton = nullptr;
    QComboBox *m_screenCombo = nullptr;
    QComboBox *m_fpsCombo = nullptr;
    QCheckBox *m_allowControlCheck = nullptr;
    QTextEdit *m_logView = nullptr;
    QSystemTrayIcon *m_trayIcon = nullptr;

    QTimer m_pollTimer;
    DeviceCodeInfo m_deviceCodeInfo;
    QString m_appToken;
    QString m_sessionId;
    QString m_initialCode;
    int m_initialScreenIndex = 0;
    bool m_initialAllowControl = false;
    bool m_allowControl = false;
    QString m_realtimeEndpoint;
    QString m_realtimeApiKey;
    QString m_realtimeTopic;
    QString m_realtimeSignedToken;
    QString m_realtimeExpiresAt;
    IceConfig m_iceConfig;
};

}  // namespace host

