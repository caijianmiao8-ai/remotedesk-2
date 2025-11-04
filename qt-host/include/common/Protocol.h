#pragma once
#include <QString>

namespace host::protocol {

// 统一到你的绑定域名（可再做 QSettings 覆盖）
inline constexpr auto kApiBase = "https://www.ruoshui.fun";

namespace paths {
inline constexpr auto kDeviceStart           = "/api/device/start";
inline constexpr auto kDevicePoll            = "/api/device/poll";
inline constexpr auto kSessionJoin           = "/api/sessions/join";
inline constexpr auto kSessionClose          = "/api/sessions/close";
inline constexpr auto kRealtimeSignedTopic   = "/api/realtime/signed-topic";
inline constexpr auto kIce                   = "/api/ice";
} // namespace paths

namespace json {
// Phoenix
inline constexpr auto kTopic       = "topic";
inline constexpr auto kEvent       = "event";
inline constexpr auto kPayload     = "payload";
inline constexpr auto kRef         = "ref";
inline constexpr auto kPhxJoin     = "phx_join";
inline constexpr auto kHeartbeat   = "heartbeat";
inline constexpr auto kBroadcast   = "broadcast";

// 我们的广播内层
inline constexpr auto kType        = "type";
inline constexpr auto kSignal      = "signal";

// WebRTC 信令类型
inline constexpr auto kOffer       = "offer";
inline constexpr auto kAnswer      = "answer";
inline constexpr auto kIce         = "ice";
inline constexpr auto kSdp         = "sdp";
inline constexpr auto kCandidate   = "candidate";

// 其它
inline constexpr auto kAllowControl= "allowControl";
inline constexpr auto kMessage     = "message";
inline constexpr auto kError       = "error";
inline constexpr auto kDataChannel = "input";
} // namespace json

} // namespace host::protocol
