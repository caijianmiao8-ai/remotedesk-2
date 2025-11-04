#include "host/WebRtcPeer.h"

#include "common/Protocol.h"
#include "host/CaptureAudio.h"
#include "host/CaptureVideo.h"
#include "host/InputInjector.h"
#include "host/SignalingClient.h"

#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLatin1String>
#include <cstdint>
#include <optional>
#include <string>
#include <variant>

#ifdef HOST_ENABLE_RTC
#include <rtc/rtc.hpp>
#endif

namespace host {

#ifdef HOST_ENABLE_RTC
namespace {
QString descriptionTypeToString(rtc::Description::Type type) {
    if (type == rtc::Description::Type::Offer) {
        return QString::fromUtf8(protocol::json::kOffer);
    }
    if (type == rtc::Description::Type::Answer) {
        return QString::fromUtf8(protocol::json::kAnswer);
    }
    return QStringLiteral("unknown");
}
}  // namespace
#endif

WebRtcPeer::WebRtcPeer(SignalingClient *signaling, QObject *parent)
    : QObject(parent), m_signaling(signaling), m_videoCapture(std::make_unique<CaptureVideo>(this)),
      m_audioCapture(std::make_unique<CaptureAudio>(this)), m_inputInjector(std::make_unique<InputInjector>(this)) {
    connect(m_videoCapture.get(), &CaptureVideo::errorOccurred, this, [this](const QString &message) {
        emit logLine(tr("Video capture error: %1").arg(message));
    });
    connect(m_audioCapture.get(), &CaptureAudio::errorOccurred, this, [this](const QString &message) {
        emit logLine(tr("Audio capture error: %1").arg(message));
    });
}

WebRtcPeer::~WebRtcPeer() { stop(); }

void WebRtcPeer::setIceConfig(const IceConfig &config) { m_iceConfig = config; }

void WebRtcPeer::setOptions(const Options &options) { m_options = options; }

void WebRtcPeer::setAllowControl(bool enabled) {
    m_allowControl = enabled;
    if (m_inputInjector) {
        m_inputInjector->setEnabled(enabled);
    }
}

void WebRtcPeer::start() {
#ifdef HOST_ENABLE_RTC
    createPeer();
    if (m_videoCapture) {
        m_videoCapture->setScreenIndex(m_options.screenIndex);
        m_videoCapture->setFrameRate(m_options.fps);
        if (!m_videoCapture->start()) {
            emit logLine(tr("Video capture did not start."));
        }
    }
    if (m_audioCapture) {
        if (!m_audioCapture->start()) {
            emit logLine(tr("Audio capture did not start."));
        }
    }
    if (m_inputInjector) {
        m_inputInjector->setEnabled(m_options.allowControl);
    }
#else
    Q_UNUSED(m_options);
    emit logLine(tr("libdatachannel not available, running in signalling-only mode."));
    emit stateChanged(tr("WebRTC disabled"));
#endif
}

void WebRtcPeer::stop() {
#ifdef HOST_ENABLE_RTC
    if (m_videoCapture) {
        m_videoCapture->stop();
    }
    if (m_audioCapture) {
        m_audioCapture->stop();
    }
    destroyPeer();
#endif
}

void WebRtcPeer::handleSignal(const QJsonObject &payload) {
#ifdef HOST_ENABLE_RTC
    if (!m_peer) {
        emit logLine(tr("Peer not ready"));
        return;
    }
    const QString type = payload.value(QLatin1String(protocol::json::kType)).toString();
    if (type == protocol::json::kOffer) {
        const auto sdp = payload.value(QLatin1String(protocol::json::kSdp)).toObject();
        rtc::Description description(sdp.value(QStringLiteral("sdp")).toString().toStdString(), type.toStdString());
        m_peer->setRemoteDescription(description);
        auto answer = m_peer->createAnswer();
        rtc::LocalDescriptionInit init;
        init.sdp = std::string(answer);
        m_peer->setLocalDescription(answer.type(), init);
    } else if (type == protocol::json::kIce) {
        const auto candidate = payload.value(QLatin1String(protocol::json::kCandidate)).toObject();
        const std::string cand = candidate.value(QStringLiteral("candidate")).toString().toStdString();
        const std::string mid = candidate.value(QStringLiteral("sdpMid")).toString().toStdString();
        const int mline = candidate.value(QStringLiteral("sdpMLineIndex")).toInt(-1);
        if (!cand.empty()) {
            std::optional<std::uint16_t> index;
            if (mline >= 0) {
                index = static_cast<std::uint16_t>(mline);
            }
            m_peer->addRemoteCandidate(rtc::Candidate(cand, mid, index));
        }
    }
#else
    Q_UNUSED(payload);
#endif
}

void WebRtcPeer::createPeer() {
#ifdef HOST_ENABLE_RTC
    rtc::Configuration config;
    for (const auto &server : m_iceConfig.servers) {
        rtc::IceServer ice(server.urls.toStdString());
        if (!server.username.isEmpty()) {
            ice.username = server.username.toStdString();
        }
        if (!server.credential.isEmpty()) {
            ice.credential = server.credential.toStdString();
        }
        config.iceServers.push_back(ice);
    }

    m_peer = std::make_unique<rtc::PeerConnection>(config);

    m_peer->onStateChange([this](rtc::PeerConnection::State state) {
        QString text;
        switch (state) {
        case rtc::PeerConnection::State::New:
            text = tr("Peer new");
            break;
        case rtc::PeerConnection::State::Connecting:
            text = tr("Peer connecting");
            break;
        case rtc::PeerConnection::State::Connected:
            text = tr("Peer connected");
            break;
        case rtc::PeerConnection::State::Disconnected:
            text = tr("Peer disconnected");
            break;
        case rtc::PeerConnection::State::Failed:
            text = tr("Peer failed");
            break;
        case rtc::PeerConnection::State::Closed:
            text = tr("Peer closed");
            break;
        }
        emit stateChanged(text);
    });

    m_peer->onGatheringStateChange([this](rtc::PeerConnection::GatheringState state) {
        if (state == rtc::PeerConnection::GatheringState::Complete) {
            emit logLine(tr("ICE gathering complete"));
        }
    });

    m_peer->onLocalDescription([this](rtc::Description desc) {
        sendLocalDescription(descriptionTypeToString(desc.type()), QString::fromStdString(std::string(desc)));
    });

    m_peer->onLocalCandidate([this](rtc::Candidate candidate) {
        QJsonObject candidateObj;
        candidateObj.insert(QStringLiteral("candidate"), QString::fromStdString(candidate.candidate()));
        candidateObj.insert(QStringLiteral("sdpMid"), QString::fromStdString(candidate.mid()));
        const auto index = candidate.mLineIndex();
        if (index.has_value()) {
            candidateObj.insert(QStringLiteral("sdpMLineIndex"), static_cast<int>(index.value()));
        }
        sendIceCandidate(candidateObj);
    });

    m_peer->onDataChannel([this](std::shared_ptr<rtc::DataChannel> channel) {
        if (!channel) {
            return;
        }
        const QString label = QString::fromStdString(channel->label());
        if (label != QString::fromUtf8(protocol::json::kDataChannelName)) {
            return;
        }
        channel->onMessage([this](rtc::message_variant message) {
            if (std::holds_alternative<std::string>(message)) {
                const auto &text = std::get<std::string>(message);
                const auto json = QJsonDocument::fromJson(QByteArray::fromStdString(text)).object();
                if (m_inputInjector) {
                    m_inputInjector->handleInputEvent(json);
                }
            }
        });
    });

    if (m_inputInjector) {
        m_inputInjector->setEnabled(m_options.allowControl);
    }
#else
    emit logLine(tr("libdatachannel not available"));
#endif
}

void WebRtcPeer::destroyPeer() {
#ifdef HOST_ENABLE_RTC
    if (!m_peer) {
        return;
    }
    m_peer.reset();
#endif
}

void WebRtcPeer::sendLocalDescription(const QString &type, const QString &sdp) {
#ifdef HOST_ENABLE_RTC
    QJsonObject payload;
    payload.insert(QLatin1String(protocol::json::kType), type);
    QJsonObject sdpObj;
    sdpObj.insert(QStringLiteral("type"), type);
    sdpObj.insert(QStringLiteral("sdp"), sdp);
    payload.insert(QLatin1String(protocol::json::kSdp), sdpObj);
    if (m_signaling) {
        m_signaling->sendSignal(payload);
    }
#else
    Q_UNUSED(type);
    Q_UNUSED(sdp);
#endif
}

void WebRtcPeer::sendIceCandidate(const QJsonObject &candidate) {
#ifdef HOST_ENABLE_RTC
    QJsonObject payload;
    payload.insert(QLatin1String(protocol::json::kType), QLatin1String(protocol::json::kIce));
    payload.insert(QLatin1String(protocol::json::kCandidate), candidate);
    if (m_signaling) {
        m_signaling->sendSignal(payload);
    }
#else
    Q_UNUSED(candidate);
#endif
}

}  // namespace host

