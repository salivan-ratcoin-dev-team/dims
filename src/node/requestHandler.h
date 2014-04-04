// Copyright (c) 2014 Ratcoin dev-team
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef REQUEST_HANDLER_H
#define REQUEST_HANDLER_H

#include "tracker/validationManager.h"

#include "uint256.h"

#include "connectionProvider.h"

namespace node
{

class CMedium;

struct CTransactionStatus
{
	self::TransactionsStatus::Enum m_status;
	uint256 m_token;
};

struct CAccountBalance
{

};

typedef boost::variant< CTransactionStatus, CAccountBalance > RequestRespond;

struct CRequest
{
	virtual void serialize( CBufferAsStream & _bufferStream ) const = 0;

	virtual RequestKind::Enum getKind() const = 0;
};

class CRequestHandler
{
public:
	CRequestHandler( CMedium * _medium );

	RequestRespond getRespond( CRequest* _request ) const;

	bool isProcessed( CRequest* _request ) const;

	bool setRequest( CRequest* _request ) const;

	void runRequests();

	void readLoop();
private:
	std::vector<CRequest*> m_newRequest;
	std::map<CRequest*,uint256> m_pendingRequest;
	std::map<CRequest*,RequestRespond> m_processedRequests;

	CMedium * m_usedMedium;
};


/*
m_processedRequests.find( reqAction.first )
CClientMessageType::Enum
Transaction
, TrackerInfoReq
, MonitorInfoReq
, TransactionInfoReq

*/

}

#endif