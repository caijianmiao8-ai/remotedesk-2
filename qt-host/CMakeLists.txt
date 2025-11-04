cmake_minimum_required(VERSION 3.21)
project(Host LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Qt 组件：如果你的 CaptureAudio/CaptureVideo 用到了 Qt Multimedia，建议加上
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network WebSockets Multimedia)

# ===== 可选：WebRTC / libdatachannel =====
# 兼容两种常见包名：rtc（多数 vcpkg 端），datachannel（某些包）
set(LIBDATACHANNEL_TARGET "")
find_package(rtc QUIET)
if (rtc_FOUND)
  if (TARGET rtc::rtc)
    set(LIBDATACHANNEL_TARGET rtc::rtc)
  elseif (TARGET rtc)
    set(LIBDATACHANNEL_TARGET rtc)
  endif()
else()
  find_package(datachannel QUIET CONFIG)
  if (datachannel_FOUND AND TARGET datachannel::rtc)
    set(LIBDATACHANNEL_TARGET datachannel::rtc)
  endif()
endif()

# libdatachannel 几乎总是依赖 OpenSSL（通过 libjuice / DTLS），显式找一下更稳
# 如果你确实是 no-ssl 方案，可把这两行去掉
find_package(OpenSSL REQUIRED)

set(SOURCES
  src/host/App.cpp
  src/host/UiMainWindow.cpp
  src/host/AuthClient.cpp
  src/host/SignalingClient.cpp
  src/host/WebRtcPeer.cpp
  src/host/CaptureVideo.cpp
  src/host/CaptureAudio.cpp
  src/host/InputInjector.cpp
  src/host/main.cpp
)

set(HEADERS
  include/common/Protocol.h
  include/host/App.h
  include/host/UiMainWindow.h
  include/host/AuthClient.h
  include/host/IceConfig.h
  include/host/SignalingClient.h
  include/host/WebRtcPeer.h
  include/host/CaptureVideo.h
  include/host/CaptureAudio.h
  include/host/InputInjector.h
)

add_executable(Host ${SOURCES} ${HEADERS})
target_include_directories(Host PRIVATE include)

# 基础 Qt 链接
target_link_libraries(Host PRIVATE
  Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Network Qt6::WebSockets Qt6::Multimedia
)

# 如找到 libdatachannel，则开启 RTC 支持并显式链 OpenSSL
if (LIBDATACHANNEL_TARGET)
  target_link_libraries(Host PRIVATE ${LIBDATACHANNEL_TARGET} OpenSSL::SSL OpenSSL::Crypto)
  target_compile_definitions(Host PRIVATE HOST_ENABLE_RTC)
  message(STATUS "Using libdatachannel target: ${LIBDATACHANNEL_TARGET}")
else()
  message(WARNING "libdatachannel not found. Building with stub WebRTC implementation.")
endif()

# 平台特定库
if (WIN32)
  # WinSock & DWM（鼠标/窗口信息、投屏等可能需要）
  target_link_libraries(Host PRIVATE ws2_32 dwmapi)
endif()

# 友好的输出目录
set_target_properties(Host PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
