#pragma once

#include <QList>
#include <QString>

namespace host {

struct IceServerConfig {
    QString urls;
    QString username;
    QString credential;
};

struct IceConfig {
    QList<IceServerConfig> servers;
};

}  // namespace host

