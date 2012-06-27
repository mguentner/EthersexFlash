#include "qtftpconnection.h"

QTftpConnection::QTftpConnection(QObject *parent) :
	QObject(parent),

{
}

void QTftpConnection::initSocket()
{

}

void QTftpConnection::readPendingDatagrams()
{
	while (m_udpSocket->hasPendingDatagrams()) {
		QByteArray datagram;
		datagram.resize(m_udpSocket->pendingDatagramSize());
		QHostAddress sender;
		quint16 senderPort;

		m_udpSocket->readDatagram(datagram.data(), datagram.size(),
		                          &sender, &senderPort);

		processTheDatagram(datagram);
	}
}

void QTftpConnection::changeState(QTftpConnection::State state)
{
	m_State = state;
	emit stateChanged(m_State);
}

int QTftpConnection::connectToHost(const QString &host, uint16_t port=69)
{

}
