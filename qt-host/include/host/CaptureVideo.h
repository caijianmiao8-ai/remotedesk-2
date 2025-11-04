#pragma once

#include <QObject>
#include <QString>
#include <memory>

namespace host {

class CaptureVideo : public QObject {
    Q_OBJECT
public:
    explicit CaptureVideo(QObject *parent = nullptr);
    ~CaptureVideo() override;

    void setScreenIndex(int index);
    void setFrameRate(int fps);

    bool start();
    void stop();

signals:
    void frameCaptured(const QByteArray &i420Data, int width, int height, qint64 timestampUs);
    void errorOccurred(const QString &message);
};

}  // namespace host

