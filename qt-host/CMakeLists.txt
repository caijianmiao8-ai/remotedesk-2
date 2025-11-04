cmake_minimum_required(VERSION 3.21)
project(Host LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network WebSockets)

# Optional WebRTC/libdatachannel
find_package(rtc QUIET)
if (rtc_FOUND)
    message(STATUS "Found libdatachannel")
else()
    message(WARNING "libdatachannel not found. Building with stub WebRTC implementation.")
endif()

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
    include/host/SignalingClient.h
    include/host/WebRtcPeer.h
    include/host/CaptureVideo.h
    include/host/CaptureAudio.h
    include/host/InputInjector.h
)

add_executable(Host
    ${SOURCES}
    ${HEADERS}
)

target_include_directories(Host PRIVATE include)

target_link_libraries(Host PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Network Qt6::WebSockets)

if (rtc_FOUND)
    target_link_libraries(Host PRIVATE rtc::rtc)
    target_compile_definitions(Host PRIVATE HOST_ENABLE_RTC)
endif()

if (WIN32)
    target_link_libraries(Host PRIVATE dwmapi)
endif()

