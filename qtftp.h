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
 * This file is part of an implementation of RFC 1350 ("TFTP Revision 2") in Qt4
 *
 * Some design ideas and the class declaration (the header) have been partly copied
 * from Qt's QFTP class.
 * Source: http://qt.gitorious.org/qt/qt/blobs/4.8/src/network/access/qftp.h
 *
 * Note that this implementation currently only can handle one connection and one
 * command at a time. NetAscii and Mail are also unsupported.
 *
 */

#ifndef QTFTP_H
#define QTFTP_H
#include <QObject>
#include <QIODevice>
#include <QString>
#include <QUdpSocket>
#include <QHostAddress>
#include <QHostInfo>
#include <QTimer>
#include <stdint.h>

#define NETASCII "NetAscii"
#define OCTET "Octet"
#define MAIL "Mail"

class QTftp : public QObject
{
    Q_OBJECT
public:
    explicit QTftp(QObject *parent = 0);
    virtual ~QTftp();

    struct Tftp_packet_t {
        uint16_t type;
        union {
            // This will be used for RRQ and WRQ packets due to their diffrent length - OP codes 1 and 2
            char raw[0];
            // Data packet - OP code 3
            struct {
                uint16_t block;
                char data[0];
            } data;
            // ACK packet - OP code 4
            struct {
                uint16_t block;
            } ack;
            // Error packet - OP code 5
            struct {
                uint16_t code;
                char message[1];
            } error;
        } u;
    };

    enum TFtpErrorCode {
        NotDefined,
        FileNotFound,
        AccessViolation,
        AllocExceeded,
        IllegalOP,
        UnknownTransferID,
        FileExists,
        NoSuchUser
    };

    enum OpCode {
        ReadRequest = 1,
        WriteRequest = 2,
        Data = 3,
        Acknowledgment = 4,
        Error = 5
    };

    enum State {
        Idle,
        Unconnected,
        HostLookup,
        Connected,
        Transfering,
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
    enum ErrorCode {
        NoError,
        AbortedByUser,
        HostNotFound,
        ConnectionRefused,
        TransmissionTimedOut,
        NotConnected,
        ProtocolError,
        UnknownError
    };

    int connectToHost(const QString &host, qint16 port=69);
    void disconnectFromHost();
    int close();
    int get(const QString &file, QIODevice *dev=0, TransferType type = Octet);
    int put(QIODevice *dev, const QString &file, TransferType type = Octet);
    int put(const QByteArray &data, const QString &file, TransferType type = Octet);
    QTftp::ErrorCode getLastErrorCode() {
        return m_LastError;
    }
    QString getLastErrorMessage() {
        return m_LastErrorMessage;
    }

signals:
    void stateChanged(QTftp::State state);
    void dataTransferProgress(qint64 done, qint64 total);
    void done(bool error);
    void readyRead();
    void error(QTftp::ErrorCode, const QString&);

public slots:
    void abort();

private slots:
    void readPendingDatagrams();
    void lookedUp(const QHostInfo &host);
    void retransmitPacket();
    void stop(bool error);
    void setError(QTftp::ErrorCode errorCode, const QString &errorMessage);

private:
    void initSocket();
    void deleteCurrentPacket();
    void changeState(State state);
    void processTftpPacket(QByteArray packet, QHostAddress sender, quint16 senderPort);
    void writeDatagram(char *payload, quint16 size, QHostAddress sender, quint16 senderPort);
    void sendNextDataPacket(QHostAddress sender, quint16 senderPort);
    void sendAcknowledgment(QHostAddress host, quint16 port);
    void handleData(QByteArray packet, QHostAddress sender, quint16 senderPort);
    void handleAcknowledgment(QByteArray packet, QHostAddress sender, quint16 senderPort);
    void handleError(QByteArray packet, QHostAddress sender, quint16 senderPort);

private:
    /*
     * We sometimes need to resent packages, but always just the most recent one
     * so we simply store that
     */
    char *m_currentPacket;
    int  m_resentCount;
    QTimer *m_resentTimer;
    QHostAddress m_currentTarget;
    quint16 m_currentPort;
    quint16 m_currentSize;

    QUdpSocket *m_udpSocket;
    State m_State;

    Command m_CurrentCommand;
    QIODevice *m_currentIODevice;

    QHostAddress m_host;
    quint16 m_port;

    quint16 m_BlockCount;
    QTftp::ErrorCode m_LastError;
    QString m_LastErrorMessage;
};

#endif // QTFTP_H
