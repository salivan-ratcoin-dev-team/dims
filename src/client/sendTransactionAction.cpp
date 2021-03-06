// Copyright (c) 2014-2015 DiMS dev-team
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/statechart/transition.hpp>
#include <boost/statechart/state.hpp>
#include <boost/statechart/custom_reaction.hpp>

#include "common/nodeMessages.h"
#include "common/setResponseVisitor.h"
#include "common/medium.h"
#include "common/requests.h"

#include "client/sendTransactionAction.h"
#include "client/filters.h"
#include "client/requests.h"
#include "client/events.h"
#include "client/control.h"

#include "serialize.h"
#include "wallet.h"

using namespace common;

namespace client
{
namespace
{
std::vector< std::pair< CKeyID, int64_t > > outputs;
std::vector< CSpendCoins > sendCoins;
}


struct CTransactionStatus;

struct CPrepareAndSendTransaction : boost::statechart::state< CPrepareAndSendTransaction, CSendTransactionAction >
{
	CPrepareAndSendTransaction( my_context ctx ) : my_base( ctx )
	{
		context< CSendTransactionAction >().forgetRequests();

		context< CSendTransactionAction >().addRequest(
				new client::CCreateTransactionRequest( outputs, sendCoins, new CMediumClassFilter( ClientMediums::TrackersBalanced, 1 ) ) );
	}

	boost::statechart::result react( common::CClientMessageResponse const & _message )
	{
		common::CTransactionAck transactionAckData;
		convertClientPayload( _message.m_clientMessage, transactionAckData );

		if ( transactionAckData.m_status == (int)common::TransactionsStatus::Validated )
		{
			CClientControl::getInstance()->addTransactionToModel( transactionAckData.m_transaction );
			context< CSendTransactionAction >().setTransaction( transactionAckData.m_transaction );
			return transit< CTransactionStatus >();
		}
		else
		{
			// indicate problem ??
			context< CSendTransactionAction >().forgetRequests();
			context< CSendTransactionAction >().setExit();
		}

		return discard_event();
	}

	typedef boost::mpl::list<
		boost::statechart::custom_reaction< common::CClientMessageResponse >
	> reactions;
};

struct CTransactionStatus : boost::statechart::state< CTransactionStatus, CSendTransactionAction >
{
	CTransactionStatus( my_context ctx ) : my_base( ctx )
	{
		common::CMediumFilter * filter =
				CTrackerLocalRanking::getInstance()->determinedTrackersCount() > 1 ?
											(common::CMediumFilter *)new CMediumClassWithExceptionFilter( context< CSendTransactionAction >().getProcessingTrackerPtr(), ClientMediums::TrackersBalanced, 1 )
										  : (common::CMediumFilter *)new CMediumClassFilter( ClientMediums::TrackersBalanced, 1 );

		context< CSendTransactionAction >().forgetRequests();

		context< CSendTransactionAction >().addRequest(
				new common::CSendClientMessageRequest(
					common::CMainRequestType::TransactionStatusReq
					, common::CClientTransactionStatusAsk(context< CSendTransactionAction >().getTransaction().GetHash())
					, filter ) );
	}

	boost::statechart::result react( common::CClientMessageResponse const & _message )
	{
		common::CTransactionStatus transactionStatus;
		convertClientPayload( _message.m_clientMessage, transactionStatus );
		if ( transactionStatus.m_status == (int)common::TransactionsStatus::Confirmed )
		{
			CClientControl::getInstance()->transactionAddmited(
						context< CSendTransactionAction >().getTransaction().GetHash()
						, context< CSendTransactionAction >().getTransaction() );

			context< CSendTransactionAction >().setExit();
		}
		else if ( transactionStatus.m_status == common::TransactionsStatus::Unconfirmed )
		{
			context< CSendTransactionAction >().forgetRequests();

			context< CSendTransactionAction >().addRequest(
					new common::CSendClientMessageRequest(
						common::CMainRequestType::TransactionStatusReq
						, common::CClientTransactionStatusAsk(context< CSendTransactionAction >().getTransaction().GetHash())
						, new CMediumClassWithExceptionFilter( _message.m_nodePtr, ClientMediums::TrackersRep, 1 ) ) );
		}
		return discard_event();
	}

	typedef boost::mpl::list<
	boost::statechart::custom_reaction< common::CClientMessageResponse >
	> reactions;
};

CSendTransactionAction::CSendTransactionAction( std::vector< std::pair< CKeyID, int64_t > > const & _outputs, std::vector< CSpendCoins > const & _sendCoins )
	: CAction()
{
	outputs = _outputs;
	sendCoins = _sendCoins;

	initiate();
}

void
CSendTransactionAction::accept( common::CSetResponseVisitor & _visitor )
{
	_visitor.visit( *this );
}

CTransaction const &
CSendTransactionAction::getTransaction() const
{
	return m_transaction;
}

void
CSendTransactionAction::setTransaction( CTransaction const & _tx )
{
	m_transaction = _tx;
}

void
CSendTransactionAction::setProcessingTrackerPtr( uintptr_t _ptr )
{
	m_processingTrackerPtr = _ptr;
}

uintptr_t
CSendTransactionAction::getProcessingTrackerPtr() const
{
	return m_processingTrackerPtr;
}

CSendTransactionAction::~CSendTransactionAction()
{
	CClientControl::getInstance()->updateTotalBalance( CWallet::getInstance()->GetBalance() );
}

}
