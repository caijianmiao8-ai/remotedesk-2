# Host.exe (Qt + libdatachannel)

This repository contains the Qt-based native host application for RemoteDesk. The goal of this initial milestone is to provide a working Windows-first implementation that authenticates via device codes, joins a remote control session using a six digit pairing code, exchanges WebRTC signalling messages over Supabase Realtime (Phoenix WebSocket) and shares the desktop while receiving remote input over a data channel.

> **Note:** Large parts of the implementation are platform specific to Windows (DXGI Desktop Duplication and SendInput). macOS/Linux currently provide stubs that compile but do not perform capture or input injection.

## Project layout

```
qt-host/
  CMakeLists.txt
  README.md
  include/
    common/Protocol.h
    host/
      App.h
      UiMainWindow.h
      AuthClient.h
      SignalingClient.h
      WebRtcPeer.h
      CaptureVideo.h
      CaptureAudio.h
      InputInjector.h
  src/host/
      *.cpp
```

## Building

### Prerequisites

* CMake 3.21+
* Ninja (recommended)
* Qt 6 (Core, Gui, Widgets, Network, WebSockets)
* vcpkg with the following packages installed for the `x64-windows` triplet:
  ```powershell
  vcpkg install libdatachannel[openssl,libjuice,usrsctp] libyuv opus openh264 libsrtp openssl --triplet x64-windows
  ```

### Configure and build (Windows)

```powershell
cmake -S . -B build -G "Ninja" \`
  -DCMAKE_TOOLCHAIN_FILE=C:/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake \`
  -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

This produces `build/Host.exe`.

## Running

1. Launch `Host.exe`.
2. The UI displays the device code (`user_code`) and instructions for binding at [https://ruoshui.fun/device](https://ruoshui.fun/device).
3. After the device is approved, input the six digit session code supplied by the controller and click **Join**.
4. Once the WebRTC connection is established, the application starts streaming the desktop and accepts input if **Allow control** is checked.
5. Click **Disconnect** to end the session; the host will notify the backend via `/api/sessions/close`.

Command line switches are available for automation:

```
Host.exe --code 123456 --screen 0 --fps 60 --allow-control 1
```

## HTTP self-test snippets

Use the following commands to verify backend connectivity:

```bash
# Request a device code
curl -X POST https://ruoshui.fun/api/device/start \
     -H "content-type: application/json" \
     -d "{}"

# After completing the browser approval flow, poll for an app token
curl -X POST https://ruoshui.fun/api/device/poll \
     -H "content-type: application/json" \
     -d '{"device_code":"dc_xxx"}'

# Query signalling metadata with the acquired app token
curl "https://ruoshui.fun/api/realtime/signed-topic?sessionId=<id>" \
     -H "authorization: Bearer <app_token>"

# Fetch ICE server configuration
curl "https://ruoshui.fun/api/ice"
```

## Status

* Device code login implemented using `AuthClient`.
* Session join/close implemented in `UiMainWindow` via `AuthClient` and `SignalingClient`.
* Supabase Realtime signalling (Phoenix WebSocket) handled in `SignalingClient`.
* WebRTC (libdatachannel) integration with desktop capture and input injection orchestrated by `WebRtcPeer`, `CaptureVideo`, `CaptureAudio` and `InputInjector`.
* macOS/Linux provide stub implementations for capture and input injection.

## Troubleshooting

* Ensure the Windows desktop compositor (DWM) is running; DXGI Desktop Duplication relies on it.
* If `Host.exe` fails to create a WebRTC peer because `libdatachannel` is missing, confirm vcpkg integration is configured for the build.
* Run the application with `--verbose` (environment variable `HOST_VERBOSE=1`) to enable detailed logging.

