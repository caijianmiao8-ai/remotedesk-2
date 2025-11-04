#include "host/App.h"

#include "host/UiMainWindow.h"

#include <QCommandLineOption>
#include <QCommandLineParser>

namespace host {

App::App(QApplication &qtApp) : QObject(&qtApp), m_qtApp(qtApp) {
    m_mainWindow = std::make_unique<UiMainWindow>();
}

App::~App() = default;

int App::run() {
    QCommandLineParser parser;
    parser.setApplicationDescription("RemoteDesk Host");
    parser.addHelpOption();
    QCommandLineOption codeOption({"c", "code"}, "Six digit session code", "code");
    QCommandLineOption screenOption({"s", "screen"}, "Screen index", "index", "0");
    QCommandLineOption fpsOption({"f", "fps"}, "Frame rate", "fps", "30");
    QCommandLineOption allowControlOption("allow-control", "Enable control by default", "0");
    parser.addOption(codeOption);
    parser.addOption(screenOption);
    parser.addOption(fpsOption);
    parser.addOption(allowControlOption);
    parser.process(m_qtApp);

    if (parser.isSet(codeOption)) {
        m_initialCode = parser.value(codeOption);
    }
    m_initialScreenIndex = parser.value(screenOption).toInt();
    m_initialAllowControl = parser.value(allowControlOption).toInt() != 0;

    if (!m_initialCode.isEmpty()) {
        m_mainWindow->setInitialCode(m_initialCode);
    }
    m_mainWindow->setInitialScreenIndex(m_initialScreenIndex);
    m_mainWindow->setAllowControlDefault(m_initialAllowControl);

    m_mainWindow->show();
    return m_qtApp.exec();
}

void App::setInitialCode(const QString &code) {
    m_initialCode = code;
    if (m_mainWindow) {
        m_mainWindow->setInitialCode(code);
    }
}

void App::setInitialScreenIndex(int index) {
    m_initialScreenIndex = index;
    if (m_mainWindow) {
        m_mainWindow->setInitialScreenIndex(index);
    }
}

void App::setAllowControlDefault(bool enabled) {
    m_initialAllowControl = enabled;
    if (m_mainWindow) {
        m_mainWindow->setAllowControlDefault(enabled);
    }
}

}  // namespace host

