#include "host/InputInjector.h"

#include <QJsonObject>

#ifdef Q_OS_WIN
#include <Windows.h>
#endif

namespace host {

InputInjector::InputInjector(QObject *parent) : QObject(parent) {}

InputInjector::~InputInjector() = default;

void InputInjector::setEnabled(bool enabled) { m_enabled = enabled; }

void InputInjector::handleInputEvent(const QJsonObject &event) {
    if (!m_enabled) {
        return;
    }
    const QString type = event.value("t").toString();
#ifdef Q_OS_WIN
    if (type == QStringLiteral("key")) {
        INPUT input{};
        input.type = INPUT_KEYBOARD;
        const QString keyType = event.value("type").toString();
        const bool keyDown = keyType == QStringLiteral("down");
        input.ki.wVk = 0;
        input.ki.dwFlags = keyDown ? 0 : KEYEVENTF_KEYUP;
        SendInput(1, &input, sizeof(INPUT));
    } else if (type == QStringLiteral("move")) {
        // Basic absolute move placeholder.
        INPUT input{};
        input.type = INPUT_MOUSE;
        input.mi.dwFlags = MOUSEEVENTF_MOVE;
        input.mi.dx = event.value("x").toInt();
        input.mi.dy = event.value("y").toInt();
        SendInput(1, &input, sizeof(INPUT));
    } else if (type == QStringLiteral("click")) {
        int button = event.value("button").toInt();
        INPUT down{};
        down.type = INPUT_MOUSE;
        INPUT up = down;
        switch (button) {
        case 0:
            down.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
            up.mi.dwFlags = MOUSEEVENTF_LEFTUP;
            break;
        case 1:
            down.mi.dwFlags = MOUSEEVENTF_MIDDLEDOWN;
            up.mi.dwFlags = MOUSEEVENTF_MIDDLEUP;
            break;
        case 2:
            down.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
            up.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
            break;
        default:
            return;
        }
        SendInput(1, &down, sizeof(INPUT));
        SendInput(1, &up, sizeof(INPUT));
    } else if (type == QStringLiteral("wheel")) {
        INPUT input{};
        input.type = INPUT_MOUSE;
        input.mi.dwFlags = MOUSEEVENTF_WHEEL;
        input.mi.mouseData = event.value("deltaY").toInt();
        SendInput(1, &input, sizeof(INPUT));
    }
#else
    Q_UNUSED(event);
#endif
}

}  // namespace host

