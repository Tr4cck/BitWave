#include "dbusscreensaver.h"

#include <QApplication>
#include <QDBusInterface>
#include <QDBusReply>
#include <utility>

DBusScreensaver::DBusScreensaver(QString service, QString path,
                                 QString interface)
        : service_(std::move(service)), path_(std::move(path)), interface_(std::move(interface)) {}

void DBusScreensaver::Inhibit() {
    QDBusInterface screensaver(this->service_, this->path_, this->interface_);
    QDBusReply<quint32> reply =
            screensaver.call("Inhibit", QApplication::applicationName(),
                             QObject::tr("Video Playing"));
    if (reply.isValid()) {
        cookie_ = reply.value();
    }
}

void DBusScreensaver::UnInhibit() {
    QDBusInterface screensaver(this->service_, this->path_, this->interface_);
    screensaver.call("UnInhibit", cookie_);
}