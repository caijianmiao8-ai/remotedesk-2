#include "host/CaptureVideo.h"

#include <QByteArray>

namespace host {

CaptureVideo::CaptureVideo(QObject *parent) : QObject(parent) {}

CaptureVideo::~CaptureVideo() = default;

void CaptureVideo::setScreenIndex(int index) { Q_UNUSED(index); }

void CaptureVideo::setFrameRate(int fps) { Q_UNUSED(fps); }

bool CaptureVideo::start() {
#ifdef Q_OS_WIN
    emit errorOccurred(tr("DXGI Desktop Duplication capture not yet implemented."));
#else
    emit errorOccurred(tr("Video capture not supported on this platform."));
#endif
    return false;
}

void CaptureVideo::stop() {}

}  // namespace host

