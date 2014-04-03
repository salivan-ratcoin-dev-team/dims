// Copyright (c) 2014 Ratcoin dev-team
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "networkAction.h"
#include "tracker/nodeMessages.h"
#include "setResponseVisitor.h"

#include "serialize.h"

namespace node
{


void
CSendTransaction::setTransactionStatus( self::TransactionsStatus::Enum const _transactionStatus )
{

}

void 
CSendTransaction::setTransactionToken( uint256 const & _token )
{

}

void
CSendTransaction::accept( CSetResponseVisitor & _visitor )
{
	_visitor.visit( *this );
}


CRequest *
CSendTransaction::execute()
{
	if ( m_actionStatus == ActionStatus::Unprepared )
	{
		m_actionStatus = ActionStatus::InProgress;
		return new CTransactionSendRequest( m_transaction );
	}
	else
	{
		if ( m_status == self::TransactionsStatus::DoubleSpend );
			// double spend
		else if ( m_status == self::TransactionsStatus::Confirmed );
			//confirmed 
		else
			return new CTransactionStatusRequest( m_token );
	}

	//create  transaction  message 
	// send transaction  message 

//  ping  for transaction 

// update  status  accordingly 

// maybe   visitor  needed  to  load  message  data 
}

CTransactionStatusRequest::CTransactionStatusRequest( uint256 const & _token )
	: m_token( _token )
{
}

RequestKind::Enum 
CTransactionStatusRequest::getKind() const
{
	return RequestKind::TransactionStatus;
}

void
CTransactionStatusRequest::serialize( CBufferAsStream & _bufferStream )
{
	int infoReq =  self::CClientMessageType::TrackerInfoReq;
	_bufferStream << infoReq;
	_bufferStream << m_token;
}


CTransactionSendRequest::CTransactionSendRequest( CTransaction const & _transaction )
	: m_transaction( _transaction )
{
}

RequestKind::Enum 
CTransactionSendRequest::getKind() const
{
	return RequestKind::Transaction;
}

void 
CTransactionSendRequest::serialize( CBufferAsStream & _bufferStream )
{
	int transactionKind = self::CClientMessageType::Transaction;
	_bufferStream << transactionKind;
	_bufferStream << m_transaction;
}

}