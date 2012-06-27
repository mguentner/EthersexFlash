/*
 * Copyright (c) 2012 by Maximilian Güntner <maximilian.guentner@gmail.com>
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
 *
 * This file is part of an implementation in Qt of RFC 1350 ("TFTP Revision 2")
 *
 */
#ifndef QTFTPCONNECTION_H
#define QTFTPCONNECTION_H

#include <QObject>
#include <QUdpSocket>
#include <QtNetwork>
#include <stdint.h>

class QTftpConnection : public QObject
{
    Q_OBJECT




    enum State {
        Idle,
        Unconnected,
        HostLookup,
        Connecting,
        Connected,
        Closing
    };


    enum TransferType {
        NetAscii,
        Octet,
        Mail
    };
    enum Command {
        Read,
        Write
    };
public:
    explicit QTftpConnection(QObject *parent = 0);
    void initSocket();
    void readPendingDatagrams();


public slots:
private slots:
    void socketStateChanged(int state);
private:
    QUdpSocket *m_udpSocket;
    State m_State;

};

#endif // QTFTPCONNECTION_H
