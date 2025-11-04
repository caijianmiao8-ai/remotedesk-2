#pragma once

#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QList>
#include <functional>
#include <memory>
#include <optional>

class QJsonObject;

namespace rtc {
class PeerConnection;
class Configuration;
class RtcpeerConnection;
class Description;
class IceCandidate;
}  // namespace rtc

namespace host {

class SignalingClient;
class CaptureVideo;
class CaptureAudio;
class InputInjector;

struct IceServerConfig {
    QString urls;
    QString username;
    QString credential;
};

struct IceConfig {
    QList<IceServerConfig> servers;
};

class WebRtcPeer : public QObject {
    Q_OBJECT
public:
    struct Options {
        bool allowControl = false;
        int screenIndex = 0;
        int fps = 30;
    };

    explicit WebRtcPeer(SignalingClient *signaling, QObject *parent = nullptr);
    ~WebRtcPeer() override;

    void setIceConfig(const IceConfig &config);
    void setOptions(const Options &options);
    void setAllowControl(bool enabled);

    void start();
    void stop();

signals:
    void stateChanged(const QString &state);
    void logLine(const QString &line);

public slots:
    void handleSignal(const QJsonObject &payload);

private:
    void createPeer();
    void destroyPeer();
    void sendLocalDescription(const QString &type, const QString &sdp);
    void sendIceCandidate(const QJsonObject &candidate);

#ifdef HOST_ENABLE_RTC
    std::unique_ptr<rtc::PeerConnection> m_peer;
#endif
    SignalingClient *m_signaling = nullptr;
    std::unique_ptr<CaptureVideo> m_videoCapture;
    std::unique_ptr<CaptureAudio> m_audioCapture;
    std::unique_ptr<InputInjector> m_inputInjector;
    IceConfig m_iceConfig;
    Options m_options;
    bool m_allowControl = false;
};

}  // namespace host

