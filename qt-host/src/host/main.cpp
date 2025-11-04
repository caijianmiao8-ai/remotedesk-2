#include "host/App.h"

#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    host::App hostApp(app);
    return hostApp.run();
}

