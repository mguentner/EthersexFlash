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

#include "qtftp.h"
#include "qendian.h"
#include <QDebug>
#include <QTimer>

QTftp::QTftp(QObject *parent) :
    QObject(parent),
    m_currentPacket(NULL),
    m_resentTimer(new QTimer(this)),
    m_udpSocket(NULL),
    m_State(Idle)
{
    connect(m_resentTimer, SIGNAL(timeout()), this, SLOT(retransmitPacket()));
    connect(this, SIGNAL(done(bool)), this, SLOT(stop(bool)));
    connect(this, SIGNAL(error(QTftp::ErrorCode,QString)), this, SLOT(setError(QTftp::ErrorCode,QString)));
}

QTftp::~QTftp()
{
    close();
}

int QTftp::connectToHost(const QString &host, qint16 port)
{
    this->initSocket();
    QHostInfo::lookupHost(host, this, SLOT(lookedUp(QHostInfo)));
    m_port = port;
    this->changeState(HostLookup);
    return 0;
}

void QTftp::lookedUp(const QHostInfo &host)
{
    if (host.error() != QHostInfo::NoError) {
        emit error(HostNotFound,tr("Lookup failed."));
        changeState(Unconnected);
    }
    if (!host.addresses().isEmpty()) {
        m_host = host.addresses().first();
        this->changeState(Connected);
    }
}

void QTftp::disconnectFromHost()
{
    if (m_State > Unconnected)
        changeState(Unconnected);
}
int QTftp::close()
{
    changeState(Closing);
    deleteCurrentPacket();
    if (m_udpSocket != NULL)
        delete m_udpSocket;
    m_resentTimer->stop();
    changeState(Idle);
    return 0;
}

int QTftp::get(const QString &file, QIODevice *dev, QTftp::TransferType type)
{
    if (m_State < Connected) {
        emit error(NotConnected, tr("Not connected"));
        return -1;
    }
    //this should never happen
    if (dev == NULL)
        return -1;
    if (dev->isWritable() == false || dev->isOpen() == false)
        return -1;
    m_CurrentCommand = Read;
    m_currentIODevice = dev;
    m_BlockCount = 1;
    deleteCurrentPacket();
    char *rawPacket;
    QByteArray typeString;
    switch (type) {
    case (NetAscii):
        typeString = NETASCII;
        qDebug() << "NetAscii is not supported";
        emit error(UnknownError,"Transfer mode not supported");
        return -1;
        break;
    case (Octet):
        typeString = OCTET;
        break;
    case (Mail):
        typeString = MAIL;
        qDebug() << "Mail is not supported";
        emit error(UnknownError,"Transfer mode not supported");
        return -1;
        break;
    default:
        break;
    }
    //type + fileString + 1 (\0) + typeString + 1 (\0)
    int size = 2+file.toAscii().size()+typeString.size()+2;
    rawPacket = new char[size];
    Tftp_packet_t *Tftp_packet = (Tftp_packet_t*) rawPacket;
    char *request = &(Tftp_packet->u.raw[0]);
    memcpy(request, file.toAscii().data(), file.toAscii().size());
    request[file.toAscii().size()] = '\0';
    memcpy(request+file.toAscii().size()+1, typeString.data(), typeString.size());
    request[file.toAscii().size()+typeString.size()+1] = '\0';
    Tftp_packet->type = _htons(ReadRequest);
    this->writeDatagram(rawPacket, size , m_host, m_port);
    return 0;
}

int QTftp::put(QIODevice *dev, const QString &file, QTftp::TransferType type)
{
    if (m_State < Connected) {
        emit error(NotConnected,tr("Not connected"));
        return -1;
    }
    if (dev == NULL)
        return -1;
    if (dev->isReadable() == false || dev->isOpen() == false)
        return -1;
    m_BlockCount = 0;

    m_CurrentCommand = Write;
    m_currentIODevice = dev;
    deleteCurrentPacket();
    char *rawPacket;
    QByteArray typeString;
    switch (type) {
    case (NetAscii):
        typeString = NETASCII;
        qDebug() << "NetAscii is not supported";
        emit error(UnknownError,"Transfer mode not supported");
        return -1;
        break;
    case (Octet):
        typeString = OCTET;
        break;
    case (Mail):
        typeString = MAIL;
        qDebug() << "Mail is not supported";
        emit error(UnknownError,"Transfer mode not supported");
        return -1;
        break;
    default:
        break;
    }
    int size = 2+file.toAscii().size()+typeString.size()+2;
    rawPacket = new char[size];
    Tftp_packet_t *Tftp_packet = (Tftp_packet_t*) rawPacket;
    char *request = &(Tftp_packet->u.raw[0]);
    memcpy(request, file.toAscii().data(), file.toAscii().size());
    request[file.toAscii().size()] = '\0';
    memcpy(request+file.toAscii().size()+1, typeString.data(), typeString.size());
    request[file.toAscii().size()+typeString.size()+1] = '\0';

    Tftp_packet->type = _htons(WriteRequest);
    this->writeDatagram(rawPacket, size , m_host, m_port);
    return 0;
}

int QTftp::put(const QByteArray &data, const QString &file, QTftp::TransferType type)
{
    /* TODO */
    Q_UNUSED(data);
    Q_UNUSED(file);
    Q_UNUSED(type);
    qDebug() << "Sorry but this function hasn't been implemented yet.\n";
    return 0;
}
void QTftp::stop(bool error)
{
    if (!error)
        m_LastError = NoError;
    m_resentTimer->stop();
    deleteCurrentPacket();
    changeState(Connected);
}

void QTftp::setError(QTftp::ErrorCode errorCode, const QString &errorMessage)
{
    m_LastErrorMessage = errorMessage;
    m_LastError = errorCode;
}
void QTftp::abort()
{
    stop(true);
    emit error(AbortedByUser,tr("Operation aborted"));
    emit done(true);
}

void QTftp::readPendingDatagrams()
{
    while (m_udpSocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(m_udpSocket->pendingDatagramSize());
        QHostAddress sender;
        quint16 senderPort;
        m_udpSocket->readDatagram(datagram.data(), datagram.size(),
                                  &sender, &senderPort);
        processTftpPacket(datagram, sender, senderPort );
    }
}



void QTftp::writeDatagram(char *payload, quint16 size, QHostAddress sender, quint16 senderPort)
{
    m_resentCount = 0;
    m_currentPacket = payload;
    m_currentSize = size;
    m_currentTarget = sender;
    m_currentPort = senderPort;
    m_udpSocket->writeDatagram(payload, size , sender, senderPort);
    m_resentTimer->start(2500);
}

void QTftp::handleError(QByteArray packet, QHostAddress sender, quint16 senderPort)
{
    Q_UNUSED(sender);
    Q_UNUSED(senderPort);
    Tftp_packet_t *tftp_packet = (Tftp_packet_t*) packet.data();
    QString msg = tr("Protocol Error. Code ") + QString::number(_ntohs(tftp_packet->u.error.code));
    msg += tr("\nMessage: ") + QString(tftp_packet->u.error.message);
    emit error(ProtocolError, msg);
}
void QTftp::handleData(QByteArray packet, QHostAddress sender, quint16 senderPort)
{
    Tftp_packet_t *tftp_packet = (Tftp_packet_t*) packet.data();
    if (_ntohs(tftp_packet->u.data.block) == m_BlockCount) {
        int size = packet.size()-sizeof(tftp_packet->type)-sizeof(tftp_packet->u.data.block);
        if (m_BlockCount == 1) {
            changeState(Transfering);
            m_currentIODevice->seek(0);
        }
        if (m_State != Transfering)
            return;
        m_currentIODevice->write((char *)tftp_packet->u.data.data, size);
        sendAcknowledgment(sender, senderPort);
        m_BlockCount++;
        if (size < 512) {
            changeState(Connected);
            emit done(false);
        }

    }
}
void QTftp::deleteCurrentPacket()
{
    if (m_currentPacket != NULL) {
        delete[] m_currentPacket;
        m_currentPacket = NULL;
    }
}
void QTftp::sendNextDataPacket(QHostAddress sender, quint16 senderPort)
{
    //Allocate type (2B), block (2B) and payload (512B)
    deleteCurrentPacket();
    char *rawPacket = new char[512+4];
    int readBytes = 0;
    Tftp_packet_t *new_tftp_packet = (Tftp_packet_t*) rawPacket;
    if (m_BlockCount == 1) {
        m_currentIODevice->seek(0);
    }
    new_tftp_packet->type = _htons(Data);
    new_tftp_packet->u.data.block= _htons(m_BlockCount);
    readBytes = m_currentIODevice->read(new_tftp_packet->u.data.data, 512);
    this->writeDatagram(rawPacket, readBytes+4, sender, senderPort);
    emit dataTransferProgress(m_currentIODevice->pos(), m_currentIODevice->size());
    if (readBytes < 512) {
        changeState(Connected);
        emit done(false);
    }
}

void QTftp::retransmitPacket()
{
    if (m_resentCount > 3) {
        m_resentTimer->stop();
        emit error(TransmissionTimedOut,tr("Transmission timed out"));
        emit done(true);
    }
    m_udpSocket->writeDatagram(m_currentPacket, m_currentSize , m_currentTarget, m_currentPort);
    m_resentCount++;
}
void QTftp::handleAcknowledgment(QByteArray packet, QHostAddress sender, quint16 senderPort)
{
    Tftp_packet_t *tftp_packet = (Tftp_packet_t*) packet.data();
    if (_ntohs(tftp_packet->u.data.block) == m_BlockCount) {
        if (m_BlockCount == 0)
            changeState(Transfering);
        m_BlockCount++;
        m_resentTimer->stop();
        if (m_State == Transfering)
            sendNextDataPacket(sender, senderPort);
    }
}
void QTftp::sendAcknowledgment(QHostAddress host, quint16 port)
{
    deleteCurrentPacket();
    char *rawPacket = new char[2+2];
    Tftp_packet_t *Tftp_packet = (Tftp_packet_t*) rawPacket;
    Tftp_packet->type = _htons(Acknowledgment);
    Tftp_packet->u.ack.block = _htons(m_BlockCount);
    this->writeDatagram(rawPacket, 2+2 , host, port);
}
void QTftp::initSocket()
{
    if (m_State == Idle) {
        if (m_udpSocket != NULL)
            delete m_udpSocket;
        m_udpSocket = new QUdpSocket(this);
        m_udpSocket->bind(7755);
        connect(m_udpSocket, SIGNAL(readyRead()), this, SLOT(readPendingDatagrams()));
        this->changeState(Unconnected);
    }
}
void QTftp::changeState(QTftp::State state)
{
    m_State = state;
    emit stateChanged(m_State);
}

void QTftp::processTftpPacket(QByteArray packet, QHostAddress sender, quint16 senderPort)
{
    Tftp_packet_t *tftp_packet = (Tftp_packet_t*) packet.data();
    switch (_ntohs(tftp_packet->type)) {
    case Acknowledgment:
        qDebug() << "ACK Received";
        handleAcknowledgment(packet, sender, senderPort);
        break;
    case Read:
        qDebug() << "RRQ Received";
        break;
    case Write:
        qDebug() << "WRQ Received";
        break;
    case Error:
        qDebug() << "Error Received";
        handleError(packet, sender, senderPort);
        break;
    case Data:
        handleData(packet, sender, senderPort);
        qDebug() << "Data Received";
        break;
    default:
        qDebug() << "Error: Malformed packet received with an unknown type";
    }
}

