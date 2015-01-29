// Copyright (c) 2014-2015 Dims dev-team
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "networkClient.h"
#include "serialize.h"
#include "version.h"
#include "common/requestHandler.h"
#include "sendBalanceInfoAction.h"
#include "sendTransactionAction.h"
#include "sendInfoRequestAction.h"
#include "clientRequests.h"

#include "common/support.h"


#include <QHostAddress>

namespace client
{

unsigned const
CNetworkClient::m_timeout = 5000;

CNetworkClient::CNetworkClient(QString const  &_ipAddr, ushort const _port )
	: m_ip( _ipAddr )
    , m_port( _port )
    , m_connectionInfo( NoActivity )
{
	m_socket = new QTcpSocket(this);

	m_pushStream = new CBufferAsStream( (char*)m_pushBuffer.m_buffer, MaxBufferSize, SER_NETWORK, CLIENT_VERSION);
	m_pushStream->SetPos(0);
    startThread();
}
 
int CNetworkClient::waitForInput()
{
	int bytesAvail = -1;

	while ( m_socket->state() == QAbstractSocket::ConnectedState && m_connectionInfo == Processing && bytesAvail < 0 )
	{
		if (m_socket->waitForReadyRead( m_timeout ))
		{
			bytesAvail = m_socket->bytesAvailable();
		}
		else
		{
			QThread::msleep( 50 );
		}
	}

	return bytesAvail;
}
 
unsigned int
CNetworkClient::read()
{
	int bytesAvail = waitForInput();
	m_pullBuffer.m_usedSize = 0;

	if (bytesAvail > 0)
    {
		bool endOfLine = false;
		bool endOfStream = false;

		if ( MaxBufferSize <= bytesAvail )
		{
			//throw error
			return 0; // this is  error so handle it as such  at some point
		}

		while ( m_pullBuffer.m_usedSize < bytesAvail && (!endOfLine) && (!endOfStream) )
		{
			m_pullBuffer.m_usedSize += m_socket->read( m_pullBuffer.m_buffer + m_pullBuffer.m_usedSize, MaxBufferSize - m_pullBuffer.m_usedSize );

		}
	}
	return m_pullBuffer.m_usedSize;
}
 
 
void CNetworkClient::write()
{
	if (m_pushBuffer.m_usedSize > 0)
	{
		m_socket->write( m_pushBuffer.m_buffer, m_pushBuffer.m_usedSize );

		if (! m_socket->waitForBytesWritten())
		{
			// error handle it somehow
		}
	}
}
 
void CNetworkClient::startThread()
{
	start();
}


void CNetworkClient::run()
{
    while(1)
    {

		if ( m_connectionInfo == Processing )
        {
			QMutexLocker lock( &m_mutex );
			QHostAddress hostAddr( m_ip );
            m_socket->connectToHost( hostAddr, m_port );
            if ( m_socket->waitForConnected( m_timeout ) )
            {
                write();
                read();
                m_connectionInfo = Processed;
            }
            else
            {
                m_connectionInfo = ServiceDenial;
            }
            m_socket->disconnectFromHost();
        }
    }
}

bool
CNetworkClient::serviced() const throw(common::CMediumException)
{
    if (m_connectionInfo == Processing )
        return false;
    else if ( m_connectionInfo == Processed )
        return true;
    else if ( m_connectionInfo == ServiceDenial )
		throw common::CMediumException(common::ErrorType::ServiceDenial );

    return false;
}


void 
CNetworkClient::add( common::CRequest< NodeResponses > const * _request )
{
}

void
CNetworkClient::add( CBalanceRequest const * _request )
{
	QMutexLocker lock( &m_mutex );
	serializeEnum(*m_pushStream, common::CMainRequestType::BalanceInfoReq );
	*m_pushStream << _request->m_address;
}

void
CNetworkClient::add( CTransactionSendRequest const * _request )
{
	QMutexLocker lock( &m_mutex );
	serializeEnum(*m_pushStream, common::CMainRequestType::Transaction );
	*m_pushStream  << _request->m_transaction;
}

void
CNetworkClient::add( CTransactionStatusRequest const * _request )
{
	common::serializeEnum( *m_pushStream, common::CMainRequestType::TransactionStatusReq );

	*m_pushStream << _request->m_transactionHash;
}

void
CNetworkClient::add( CInfoRequestContinueComplex const * _request )
{
	common::serializeEnum( *m_pushStream, common::CMainRequestType::ContinueReq );

	assert( _request->m_nodeToToken.find( common::convertToInt(this) ) != _request->m_nodeToToken.end() );

	*m_pushStream << _request->m_nodeToToken.find( common::convertToInt(this) )->second;
}

void
CNetworkClient::add( CInfoRequestContinue const * _request )
{
	common::serializeEnum( *m_pushStream, common::CMainRequestType::ContinueReq );

	*m_pushStream << _request->m_token;
}

void
CNetworkClient::add( CRecognizeNetworkRequest const * _request )
{
	common::serializeEnum( *m_pushStream, common::CMainRequestType::NetworkInfoReq );
}

void
CNetworkClient::add( CTrackersInfoRequest const * _request )
{
	common::serializeEnum( *m_pushStream, common::CMainRequestType::TrackerInfoReq );
}

void
CNetworkClient::add( CMonitorInfoRequest const * _request )
{
	common::serializeEnum( *m_pushStream, common::CMainRequestType::MonitorInfoReq );
}

bool
CNetworkClient::flush()
{
	QMutexLocker lock( &m_mutex );
	m_pushBuffer.m_usedSize = m_pushStream->GetPos();

	if ( !m_pushBuffer.m_usedSize )
		return true;
	m_pushStream->SetPos( 0 );

    if ( m_connectionInfo != NoActivity && m_connectionInfo != Processed )
        return false;

     m_connectionInfo = Processing;
}

void
CNetworkClient::clearResponses()
{
	m_pullBuffer.m_usedSize = 0;
}

bool
CNetworkClient::getResponse( std::vector< NodeResponses > & _requestResponse ) const
{
	QMutexLocker lock( &m_mutex );
	CBufferAsStream stream(
		  (char*)m_pullBuffer.m_buffer
		, m_pullBuffer.m_usedSize
		, SER_NETWORK
		, CLIENT_VERSION);

	while( !stream.eof() )
	{
		int messageType;

		stream >> messageType;

		if ( messageType == common::CMainRequestType::ContinueReq )
		{
			uint256 token;
			stream >> token;
			_requestResponse.push_back( common::CPending( token, common::convertToInt(this) ) );

		}
		else if ( messageType == common::CMainRequestType::TransactionStatusReq )
		{
			common::CTransactionStatusResponse transactionStatus;
			stream >> transactionStatus;

			_requestResponse.push_back( common::CTransactionStatus( ( common::TransactionsStatus::Enum )transactionStatus.m_status, transactionStatus.m_transactionHash, transactionStatus.m_signedHash ) );
		}
		else if ( messageType == common::CMainRequestType::Transaction )
		{
			common::CTransactionAck transactionAck;
			stream >> transactionAck;
			_requestResponse.push_back( transactionAck );
		}
		else if ( messageType == common::CMainRequestType::MonitorInfoReq )
		{
			common::CMonitorData monitorData;

			stream >> monitorData;

			common::CNodeSpecific< common::CMonitorData > stats( monitorData, m_ip.toStdString(), common::convertToInt(this));
			_requestResponse.push_back( stats );
		}
		else if ( messageType == common::CMainRequestType::TrackerInfoReq )
		{
			common::CTrackerSpecificStats trackerSpecificStats;

			stream >> trackerSpecificStats;

			common::CNodeSpecific< common::CTrackerSpecificStats > stats( trackerSpecificStats, m_ip.toStdString(), common::convertToInt(this));
			_requestResponse.push_back( stats );
		}
		else if ( messageType == common::CMainRequestType::RequestSatatusReq )
		{
		}
		else if ( messageType == common::CMainRequestType::BalanceInfoReq )
		{
			common::CAvailableCoins availableCoins;
			stream >> availableCoins;
			_requestResponse.push_back( availableCoins );
		}
		else if ( messageType == common::CMainRequestType::NetworkInfoReq )
		{
			common::CClientNetworkInfoResult networkResult;
			stream >> networkResult;

			common::CNodeSpecific< common::CClientNetworkInfoResult > stats( networkResult, m_ip.toStdString(), common::convertToInt(this));
			_requestResponse.push_back( stats );
		}
		else
		{
			throw;
		}
	}


}

CNetworkClient::~CNetworkClient()
{
	//  is this right??, maybe invent some other  way  to  terminate?
    terminate();
    delete m_pushStream;
}

}