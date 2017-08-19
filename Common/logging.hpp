/*
 * Copyright 2017 Tycho Softworks.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __LOGGING_HPP__
#define __LOGGING_HPP__

#include <QObject>
#include <QString>
#include <QEvent>
#include <QDebug>

class Logging final : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(Logging)

public:
    typedef enum {
        LOGGING_IGNORE = QEvent::User + 1,
        LOGGING_INFO,
        LOGGING_NOTICE,
        LOGGING_WARN,
        LOGGING_ERROR,
        LOGGING_FATAL
    } Level;

    static QDebug info(const char *prefix = "%%");
    static QDebug warn(const char *prefix = "@@");
    static QDebug err(const char *prefix = "**");
    static QDebug crit(int reason);

    // we overload warn, especially for older qt versions...

    inline static QDebug debug() {
        return warn("  ");
    }

    inline static QDebug notice() {
        return warn("!!");
    }

    static void init() {
        new Logging();
    }

    static Logging *instance() {
        Q_ASSERT(Instance != nullptr);
        return Instance;
    }

private:
    static Logging *Instance;

    Logging();
    ~Logging();

    bool event(QEvent *evt) final;

    void append(const char *category, const char *text);
    void close();

    static void logHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg);

signals:
    void notify(Level level, time_t timestamp, const QByteArray& msg);

private slots:
    void onStartup();
    void onShutdown();
    void onExiting();
    void onTimeout();
};

/*! Support for service logging.
 * \file logging.hpp
 */

/*!
 * \class Logging
 * \brief A generic server logging class.
 * This delegates the qDebug handler, and serializes log messages thru a private
 * thread event queue so that event logs can be issued from multiple threads
 * without overlapping i/o.  Standard logging methods are also offered.  On
 * Posix systems the syslog facility will also be used.  When logging to a log
 * file, the log is closed while inactive.  This allows for clean log rotation.
 * A signal is emitted so that other components can listen to log events.
 * \author David Sugar <tychosoft@gmail.com>
 */

#endif