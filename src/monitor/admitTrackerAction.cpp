// Copyright (c) 2014-2015 Dims dev-team
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "core.h"

#include <boost/statechart/state.hpp>
#include <boost/statechart/transition.hpp>
#include <boost/statechart/custom_reaction.hpp>

#include "common/authenticationProvider.h"
#include "common/setResponseVisitor.h"
#include "common/analyseTransaction.h"
#include "common/commonRequests.h"

#include "monitor/admitTrackerAction.h"
#include "monitor/monitorController.h"
#include "monitor/admitTransactionsBundle.h"
#include "monitor/filters.h"
#include "monitor/reputationTracer.h"
#include "monitor/monitorRequests.h"
#include "monitor/rankingDatabase.h"
#include "monitor/chargeRegister.h"

namespace monitor
{
struct CPaidRegistration;
struct CFreeRegistration;
struct CPaidRegistrationEmptyNetwork;
//milisec
unsigned int const WaitTime = 20000;

bool analyseTransaction( CTransaction & _transaction, uint256 const & _hash, CKeyID const & _trackerId )
{
	if ( !common::findKeyInInputs( _transaction, _trackerId ) )
		return false;

	std::vector < CTxOut > txOuts;
	std::vector< unsigned int > ids;

	common::findOutputInTransaction(
				_transaction
				, common::CAuthenticationProvider::getInstance()->getMyKey().GetID()
				, txOuts
				, ids );

	unsigned int value = 0;
	BOOST_FOREACH( CTxOut const & txOut, txOuts )
	{
		value += txOut.nValue;
	}
	return value >= CMonitorController::getInstance()->getPrice();
}

struct CWaitForInfo : boost::statechart::state< CWaitForInfo, CAdmitTrackerAction >
{
	CWaitForInfo( my_context ctx )
		: my_base( ctx )
	{
		LogPrintf("admit tracker action: %p wait for info \n", &context< CAdmitTrackerAction >() );
		context< CAdmitTrackerAction >().dropRequests();
		context< CAdmitTrackerAction >().addRequest( new common::CTimeEventRequest< common::CMonitorTypes >( WaitTime, new CMediumClassFilter( common::CMediumKinds::Time ) ) );

	}

	boost::statechart::result react( common::CMessageResult const & _messageResult )
	{
		common::CMessage orginalMessage;
		if ( !common::CommunicationProtocol::unwindMessage( _messageResult.m_message, orginalMessage, GetTime(), _messageResult.m_pubKey ) )
			assert( !"service it somehow" );

		// todo create alredy registered logic _messageResult.m_pubKey

		context< CAdmitTrackerAction >().dropRequests();

		context< CAdmitTrackerAction >().addRequest(
					new common::CAckRequest< common::CMonitorTypes >(
						context< CAdmitTrackerAction >().getActionKey()
						, _messageResult.m_message.m_header.m_id
						, new CSpecificMediumFilter( context< CAdmitTrackerAction >().getNodePtr() ) ) );

		context< CAdmitTrackerAction >().addRequest( new CRegistrationTerms(
														  CMonitorController::getInstance()->getPrice()
														 , CMonitorController::getInstance()->getPeriod()
														 , context< CAdmitTrackerAction >().getActionKey()
														 , _messageResult.m_message.m_header.m_id
														 , new CSpecificMediumFilter( context< CAdmitTrackerAction >().getNodePtr() ) ) );

		return discard_event();
	}

	boost::statechart::result react( common::CAckEvent const & _ackEvent )
	{
		if ( CMonitorController::getInstance()->getPrice() )
		{
			if ( CReputationTracker::getInstance()->getTrackers().empty() )
				return transit< CPaidRegistrationEmptyNetwork >();
		}
		else
			return transit< CFreeRegistration >();

		return discard_event();
	}

	boost::statechart::result react( common::CTimeEvent const & _timeEvent )
	{
		context< CAdmitTrackerAction >().dropRequests();
		return discard_event();
	}

	typedef boost::mpl::list<
	boost::statechart::custom_reaction< common::CAckEvent >,
	boost::statechart::custom_reaction< common::CTimeEvent >,
	boost::statechart::custom_reaction< common::CMessageResult >
	> reactions;
};

struct CFreeRegistration : boost::statechart::state< CFreeRegistration, CAdmitTrackerAction >
{
	CFreeRegistration( my_context ctx )
		: my_base( ctx )
	{

		LogPrintf("admit tracker action: %p free registration \n", &context< CAdmitTrackerAction >() );

		context< CAdmitTrackerAction >().dropRequests();
		context< CAdmitTrackerAction >().addRequest( new common::CTimeEventRequest< common::CMonitorTypes >( WaitTime, new CMediumClassFilter( common::CMediumKinds::Time ) ) );
	}

	boost::statechart::result react( common::CMessageResult const & _messageResult )
	{
		common::CMessage orginalMessage;
		if ( !common::CommunicationProtocol::unwindMessage( _messageResult.m_message, orginalMessage, GetTime(), _messageResult.m_pubKey ) )
			assert( !"service it somehow" );

		common::CAdmitProof admitMessage;

		common::convertPayload( orginalMessage, admitMessage );

		CReputationTracker::getInstance()->addTracker( common::CTrackerData( _messageResult.m_pubKey, 0, CMonitorController::getInstance()->getPeriod(), GetTime() ) );

		context< CAdmitTrackerAction >().addRequest(
					new common::CAckRequest< common::CMonitorTypes >(
						context< CAdmitTrackerAction >().getActionKey()
						, _messageResult.m_message.m_header.m_id
						, new CSpecificMediumFilter( context< CAdmitTrackerAction >().getNodePtr() ) ) );

		context< CAdmitTrackerAction >().addRequest(
					new common::CResultRequest< common::CMonitorTypes >(
						  context< CAdmitTrackerAction >().getActionKey()
						, _messageResult.m_message.m_header.m_id
						, 1
						, new CSpecificMediumFilter( context< CAdmitTrackerAction >().getNodePtr() ) ) );

		return discard_event();
	}

	boost::statechart::result react( common::CTimeEvent const & _timeEvent )
	{
		context< CAdmitTrackerAction >().dropRequests();
		return discard_event();
	}

	boost::statechart::result react( common::CAckEvent const & _ackEvent )
	{
		context< CAdmitTrackerAction >().dropRequests();
		return discard_event();
	}

	typedef boost::mpl::list<
	boost::statechart::custom_reaction< common::CAckEvent >,
	boost::statechart::custom_reaction< common::CTimeEvent >,
	boost::statechart::custom_reaction< common::CMessageResult >
	> reactions;

};

struct CPaidRegistrationEmptyNetwork : boost::statechart::state< CPaidRegistrationEmptyNetwork, CAdmitTrackerAction >
{
	CPaidRegistrationEmptyNetwork( my_context ctx )
		: my_base( ctx )
	{
		context< CAdmitTrackerAction >().dropRequests();
		context< CAdmitTrackerAction >().addRequest(
					new common::CTimeEventRequest< common::CMonitorTypes >(
						WaitTime
						, new CMediumClassFilter( common::CMediumKinds::Time ) ) );
	}

	boost::statechart::result react( common::CMessageResult const & _messageResult )
	{
		common::CMessage orginalMessage;
		if ( !common::CommunicationProtocol::unwindMessage( _messageResult.m_message, orginalMessage, GetTime(), _messageResult.m_pubKey ) )
			assert( !"service it somehow" );

		if ( _messageResult.m_message.m_header.m_payloadKind== common::CPayloadKind::AdmitAsk )
		{
			context< CAdmitTrackerAction >().addRequest(
						new common::CAckRequest< common::CMonitorTypes >(
							context< CAdmitTrackerAction >().getActionKey()
							, _messageResult.m_message.m_header.m_id
							, new CSpecificMediumFilter( context< CAdmitTrackerAction >().getNodePtr() ) ) );

			context< CAdmitTrackerAction >().addRequest(
						new common::CResultRequest< common::CMonitorTypes >(
							context< CAdmitTrackerAction >().getActionKey()
							, _messageResult.m_message.m_header.m_id
							, 1
							, new CSpecificMediumFilter( context< CAdmitTrackerAction >().getNodePtr() ) ) );

			CReputationTracker::getInstance()->addTracker( common::CTrackerData( _messageResult.m_pubKey, 0, CMonitorController::getInstance()->getPeriod(), GetTime() ) );

		}
		return discard_event();
	}

	boost::statechart::result react( common::CTimeEvent const & _timeEvent )
	{
		context< CAdmitTrackerAction >().setExit();
		return discard_event();
	}

	boost::statechart::result react( common::CAckEvent const & _ackEvent )
	{
		return transit< CPaidRegistration >();
		return discard_event();
	}

	typedef boost::mpl::list<
	boost::statechart::custom_reaction< common::CAckEvent >,
	boost::statechart::custom_reaction< common::CTimeEvent >,
	boost::statechart::custom_reaction< common::CMessageResult >
	> reactions;
};

struct CPaidRegistration : boost::statechart::state< CPaidRegistration, CAdmitTrackerAction >
{
	CPaidRegistration( my_context ctx )
		: my_base( ctx )
		, m_checkPeriod( 30000 )
	{
		LogPrintf("admit tracker action: %p paid registration \n", &context< CAdmitTrackerAction >() );
		context< CAdmitTrackerAction >().dropRequests();

		CChargeRegister::getInstance()->setStoreTransactions( true );
	}

	boost::statechart::result react( common::CMessageResult const & _messageResult )
	{
		common::CMessage orginalMessage;
		if ( !common::CommunicationProtocol::unwindMessage( _messageResult.m_message, orginalMessage, GetTime(), _messageResult.m_pubKey ) )
			assert( !"service it somehow" );

		common::CAdmitProof admitMessage;

		common::convertPayload( orginalMessage, admitMessage );

		CChargeRegister::getInstance()->addTransactionToSearch( admitMessage.m_proofTransactionHash, _messageResult.m_pubKey.GetID() );

		m_proofHash = admitMessage.m_proofTransactionHash;

		common::CSendMessageRequest< common::CMonitorTypes > * request =
				new common::CSendMessageRequest< common::CMonitorTypes >(
					common::CPayloadKind::Ack
					, context< CAdmitTrackerAction >().getActionKey()
					, _messageResult.m_message.m_header.m_id
					, new CSpecificMediumFilter( context< CAdmitTrackerAction >().getNodePtr() ) );

		request->addPayload( common::CAck() );

		context< CAdmitTrackerAction >().addRequest( request );

		context< CAdmitTrackerAction >().dropRequests();
		context< CAdmitTrackerAction >().addRequest(
					new common::CTimeEventRequest< common::CMonitorTypes >(
						m_checkPeriod
						, new CMediumClassFilter( common::CMediumKinds::Time ) ) );

		m_messageId = _messageResult.m_message.m_header.m_id;

		m_pubKey = _messageResult.m_pubKey;

		return discard_event();
	}

	boost::statechart::result react( common::CTimeEvent const & _timeEvent )
	{
		CTransaction transaction;

		if ( CChargeRegister::getInstance()->isTransactionPresent( m_proofHash ) )
		{
			CReputationTracker::getInstance()->addTracker( common::CTrackerData( m_pubKey, 0, CMonitorController::getInstance()->getPeriod(), GetTime() ) );

			context< CAdmitTrackerAction >().dropRequests();
			context< CAdmitTrackerAction >().addRequest(
						new common::CResultRequest< common::CMonitorTypes >(
							  context< CAdmitTrackerAction >().getActionKey()
							, m_messageId
							, 1
							, new CSpecificMediumFilter( context< CAdmitTrackerAction >().getNodePtr() ) ) );

			CPubKey pubKey;

			if ( CReputationTracker::getInstance()->getNodeToKey( context< CAdmitTrackerAction >().getNodePtr(), pubKey ) )
			{
				common::CTrackerData trackerData( pubKey, 0, GetTime(), CMonitorController::getInstance()->getPeriod() );

				CRankingDatabase::getInstance()->writeTrackerData( trackerData );

				CReputationTracker::getInstance()->addTracker( trackerData );
			}

			context< CAdmitTrackerAction >().setExit();
		}
		else
		{
			context< CAdmitTrackerAction >().dropRequests();

			context< CAdmitTrackerAction >().addRequest(
						new common::CTimeEventRequest< common::CMonitorTypes >(
							m_checkPeriod
							, new CMediumClassFilter( common::CMediumKinds::Time ) ) );
/*
			context< CAdmitTrackerAction >().addRequest(
						new common::CResultRequest< common::CMonitorTypes >(
							  context< CAdmitTrackerAction >().getActionKey()
							, m_messageId
							, 0
							, new CSpecificMediumFilter( context< CAdmitTrackerAction >().getNodePtr() ) ) );
*/
		}
		return discard_event();
	}

	~CPaidRegistration()
	{
		CChargeRegister::getInstance()->setStoreTransactions( false );
	}

	typedef boost::mpl::list<
	boost::statechart::custom_reaction< common::CTimeEvent >,
	boost::statechart::custom_reaction< common::CMessageResult >
	> reactions;

	uint256 m_proofHash;

	uint256 m_messageId;

	int64_t const m_checkPeriod;

	CPubKey m_pubKey;
};

CAdmitTrackerAction::CAdmitTrackerAction( uint256 const & _actionKey, uintptr_t _nodePtr )
	: common::CAction< common::CMonitorTypes >( _actionKey )
	, m_nodePtr( _nodePtr )
{
	initiate();
}

void
CAdmitTrackerAction::accept( common::CSetResponseVisitor< common::CMonitorTypes > & _visitor )
{
	_visitor.visit( *this );
}

}
