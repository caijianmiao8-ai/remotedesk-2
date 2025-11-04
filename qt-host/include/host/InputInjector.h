#pragma once

#include <QObject>
#include <QString>

class QJsonObject;

namespace host {

class InputInjector : public QObject {
    Q_OBJECT
public:
    explicit InputInjector(QObject *parent = nullptr);
    ~InputInjector() override;

    void setEnabled(bool enabled);
    bool enabled() const { return m_enabled; }

    void handleInputEvent(const QJsonObject &event);

signals:
    void errorOccurred(const QString &message);

private:
    bool m_enabled = false;
};

}  // namespace host

