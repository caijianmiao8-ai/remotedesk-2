#pragma once

#include <QObject>
#include <QString>

namespace host {

class CaptureAudio : public QObject {
    Q_OBJECT
public:
    explicit CaptureAudio(QObject *parent = nullptr);
    ~CaptureAudio() override;

    bool start();
    void stop();

signals:
    void audioFrameCaptured(const QByteArray &pcmData, int sampleRate, int channels, qint64 timestampUs);
    void errorOccurred(const QString &message);
};

}  // namespace host

