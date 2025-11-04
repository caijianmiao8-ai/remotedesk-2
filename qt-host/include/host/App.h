#pragma once

#include <QApplication>
#include <QObject>
#include <QString>
#include <memory>

namespace host {

class UiMainWindow;

class App : public QObject {
    Q_OBJECT
public:
    explicit App(QApplication &qtApp);
    ~App() override;

    int run();

    void setInitialCode(const QString &code);
    void setInitialScreenIndex(int index);
    void setAllowControlDefault(bool enabled);

private:
    QApplication &m_qtApp;
    std::unique_ptr<UiMainWindow> m_mainWindow;
    QString m_initialCode;
    int m_initialScreenIndex = 0;
    bool m_initialAllowControl = false;
};

}  // namespace host

