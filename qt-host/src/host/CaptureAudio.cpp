#include "host/CaptureAudio.h"

namespace host {

CaptureAudio::CaptureAudio(QObject *parent) : QObject(parent) {}

CaptureAudio::~CaptureAudio() = default;

bool CaptureAudio::start() {
#ifdef Q_OS_WIN
    emit errorOccurred(tr("WASAPI loopback capture not yet implemented."));
#else
    emit errorOccurred(tr("Audio capture not supported on this platform."));
#endif
    return false;
}

void CaptureAudio::stop() {}

}  // namespace host

