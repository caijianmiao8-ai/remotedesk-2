#pragma once

#include <QString>

namespace host::protocol {

inline constexpr auto kApiBase = "https://ruoshui.fun";

namespace paths {
inline constexpr auto kDeviceStart = "/api/device/start";
inline constexpr auto kDevicePoll = "/api/device/poll";
inline constexpr auto kSessionJoin = "/api/sessions/join";
inline constexpr auto kSessionClose = "/api/sessions/close";
inline constexpr auto kRealtimeSignedTopic = "/api/realtime/signed-topic";
inline constexpr auto kIce = "/api/ice";
}  // namespace paths

namespace json {
inline constexpr auto kStatus = "status";
inline constexpr auto kApproved = "approved";
inline constexpr auto kPending = "pending";
inline constexpr auto kAppToken = "app_token";
inline constexpr auto kUser = "user";
inline constexpr auto kUserId = "id";
inline constexpr auto kDeviceCode = "device_code";
inline constexpr auto kUserCode = "user_code";
inline constexpr auto kVerificationUri = "verification_uri";
inline constexpr auto kInterval = "interval";
inline constexpr auto kExpiresIn = "expires_in";
inline constexpr auto kCode6 = "code6";
inline constexpr auto kRole = "role";
inline constexpr auto kHostRole = "host";
inline constexpr auto kSessionId = "sessionId";
inline constexpr auto kEndpoint = "endpoint";
inline constexpr auto kApiKey = "apikey";
inline constexpr auto kTopic = "topic";
inline constexpr auto kSignedToken = "signedToken";
inline constexpr auto kExpiresAt = "expires_at";
inline constexpr auto kIceServers = "iceServers";
inline constexpr auto kType = "type";
inline constexpr auto kEvent = "event";
inline constexpr auto kPayload = "payload";
inline constexpr auto kBroadcast = "broadcast";
inline constexpr auto kSignal = "signal";
inline constexpr auto kOffer = "offer";
inline constexpr auto kAnswer = "answer";
inline constexpr auto kIce = "ice";
inline constexpr auto kSdp = "sdp";
inline constexpr auto kCandidate = "candidate";
inline constexpr auto kAllowControl = "allowControl";
inline constexpr auto kMessage = "message";
inline constexpr auto kError = "error";
inline constexpr auto kDataChannelName = "input";
}  // namespace json

}  // namespace host::protocol

