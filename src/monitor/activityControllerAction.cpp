// Copyright (c) 2014-2015 DiMS dev-team
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "core.h"

#include <boost/statechart/state.hpp>
#include <boost/statechart/transition.hpp>
#include <boost/statechart/custom_reaction.hpp>

#include "common/setResponseVisitor.h"
#include "common/requests.h"
#include "common/events.h"

#include "monitor/controller.h"
#include "monitor/filters.h"
#include "monitor/reputationTracer.h"
#include "monitor/activityControllerAction.h"
#include "monitor/connectNodeAction.h"

namespace monitor
{

//milisec
unsigned int const WaitTime = 20000;

struct CInitiateActivationEvent : boost::statechart::event< CInitiateActivationEvent >{};

struct CRecognizeNodeStateEvent : boost::statechart::event< CRecognizeNodeStateEvent >{};

struct CInitiateActivation;
struct CRecognizeNodeState;

namespace
{
CPubKey NodeKey;
CActivitySatatus::Enum Status;
CAddress Address;
}

struct CActivityInitial : boost::statechart::simple_state< CActivityInitial, CActivityControllerAction >
{
	typedef boost::mpl::list<
	boost::statechart::transition< CInitiateActivationEvent, CInitiateActivation >,
	boost::statechart::transition< CRecognizeNodeStateEvent, CRecognizeNodeState >
	> reactions;
};

struct CInitiateActivation : boost::statechart::state< CInitiateActivation, CActivityControllerAction >
{
	CInitiateActivation( my_context ctx )
		: my_base( ctx )
	{
		LogPrintf("activity controller action: %p initiate activation \n", &context< CActivityControllerAction >() );

		context< CActivityControllerAction >().addRequest(
					new common::CScheduleActionRequest(
						new CConnectNodeAction( Address )
						, new CMediumClassFilter( common::CMediumKinds::Schedule) ) );
	}

	boost::statechart::result react( common::CAckEvent const & _ackEvent )
	{
		context< CActivityControllerAction >().setExit();// simplified
		return discard_event();
	}

	boost::statechart::result react( common::CTimeEvent const & _timeEvent )
	{
		assert(!"may consider as  problem");
		context< CActivityControllerAction >().setExit();
		return discard_event();
	}

	boost::statechart::result react( common::CNoMedium const & _noMedium )
	{
		context< CActivityControllerAction >().setExit();
		return discard_event();
	}

	boost::statechart::result react( common::CFailureEvent const & _failureEvent )
	{
		if ( CReputationTracker::getInstance()->isPresentNode( NodeKey.GetID() ) )
		{
			if ( Status == CActivitySatatus::Inactive )
			{
				CReputationTracker::getInstance()->erasePresentNode( NodeKey.GetID() );
			}

			context< CActivityControllerAction >().addRequest(
					new common::CSendMessageRequest(
						common::CPayloadKind::ActivationStatus
						, common::CActivationStatus( NodeKey.GetID(),(int)CActivitySatatus::Inactive )
						, context< CActivityControllerAction >().getActionKey()
						, new CNodeExceptionFilter( common::CMediumKinds::DimsNodes, NodeKey.GetID() ) ) );

			context< CActivityControllerAction >().addRequest(
						new common::CTimeEventRequest(
							WaitTime
							, new CMediumClassFilter( common::CMediumKinds::Time ) ) );
		}
		return discard_event();
	}

	boost::statechart::result react( common::CNetworkInfoResult const & _networkInfoEvent )
	{
		if ( CReputationTracker::getInstance()->isPresentNode( NodeKey.GetID() ) )
			return discard_event();

		if ( Status == CActivitySatatus::Active )
		{
			CReputationTracker::getInstance()->setPresentNode( NodeKey.GetID() );

			context< CActivityControllerAction >().addRequest(
					new common::CSendMessageRequest(
						common::CPayloadKind::ActivationStatus
						, common::CActivationStatus( NodeKey.GetID(),(int)CActivitySatatus::Active )
						, context< CActivityControllerAction >().getActionKey()
						, new CNodeExceptionFilter( common::CMediumKinds::DimsNodes, NodeKey.GetID() ) ) );

			context< CActivityControllerAction >().addRequest(
						new common::CTimeEventRequest(
							WaitTime
							, new CMediumClassFilter( common::CMediumKinds::Time ) ) );
		}
		else
		{
			context< CActivityControllerAction >().setExit();
			return discard_event();
		}

		return discard_event();
	}

	typedef boost::mpl::list<
	boost::statechart::custom_reaction< common::CNetworkInfoResult >,
	boost::statechart::custom_reaction< common::CFailureEvent >,
	boost::statechart::custom_reaction< common::CNoMedium >,
	boost::statechart::custom_reaction< common::CAckEvent >,
	boost::statechart::custom_reaction< common::CTimeEvent >
	> reactions;
};

struct CRecognizeNodeState : boost::statechart::state< CRecognizeNodeState, CActivityControllerAction >
{
	CRecognizeNodeState( my_context ctx )
		: my_base( ctx )
	{
		m_alreadyInformed = false;
	}

	boost::statechart::result react( common::CMessageResult const & _messageResult )
	{
		std::set< CPubKey > alreadyInformed;

		m_lastKey = _messageResult.m_pubKey;

		common::CMessage orginalMessage;
		if ( !common::CommunicationProtocol::unwindMessageAndParticipants( _messageResult.m_message, orginalMessage, GetTime(), _messageResult.m_pubKey, alreadyInformed ) )
			assert( !"service it somehow" );

		BOOST_FOREACH( CPubKey const & pubKey, alreadyInformed )
		{
			m_informingNodes.insert( pubKey.GetID() );
		}

		if ( orginalMessage.m_header.m_payloadKind == common::CPayloadKind::ActivationStatus && !m_alreadyInformed )
		{
			m_alreadyInformed = true;

			common::convertPayload( orginalMessage, m_activationStatus );

			context< CActivityControllerAction >().addRequest(
					new common::CSendMessageRequest(
						common::CPayloadKind::Ack
						, common::CAck()
						, context< CActivityControllerAction >().getActionKey()
						, _messageResult.m_message.m_header.m_id
						, new CByKeyMediumFilter( _messageResult.m_pubKey ) ) );

			common::CTrackerData trackerData;
			CPubKey controllingMonitor;
			if ( CReputationTracker::getInstance()->checkForTracker( m_activationStatus.m_keyId, trackerData, controllingMonitor ) )
			{

				if ( m_activationStatus.m_status == CActivitySatatus::Inactive )
				{

					if ( m_informingNodes.find( controllingMonitor.GetID() ) != m_informingNodes.end() )
					{
						CReputationTracker::getInstance()->erasePresentNode( m_activationStatus.m_keyId );

						context< CActivityControllerAction >().addRequest(
									new common::CSendMessageRequest(
										m_message
										, m_lastKey
										, context< CActivityControllerAction >().getActionKey()
										, new CNodeExceptionFilter( common::CMediumKinds::DimsNodes, m_informingNodes ) ) );

						return discard_event();
					}

				}
			}
			CAddress address;
			if ( CReputationTracker::getInstance()->getAddresFromKey( NodeKey.GetID(), address ) )
			{
				context< CActivityControllerAction >().addRequest(
							new common::CScheduleActionRequest(
								new CConnectNodeAction( address )
								, new CMediumClassFilter( common::CMediumKinds::Schedule) ) );
			}
		}

		return discard_event();
	}

	boost::statechart::result react( common::CAckEvent const & _ackEvent )
	{
		context< CActivityControllerAction >().setExit();// simplified
		return discard_event();
	}

	boost::statechart::result react( common::CTimeEvent const & _timeEvent )
	{
		assert(!"may consider as  problem");
		context< CActivityControllerAction >().setExit();
		return discard_event();
	}

	boost::statechart::result react( common::CNoMedium const & _noMedium )
	{
		context< CActivityControllerAction >().setExit();
		return discard_event();
	}

	boost::statechart::result react( common::CFailureEvent const & _failureEvent )
	{
		if ( m_activationStatus.m_status == CActivitySatatus::Inactive )
		{
			CReputationTracker::getInstance()->erasePresentNode( m_activationStatus.m_keyId );
		}
		else
		{
			return discard_event();
		}

		context< CActivityControllerAction >().addRequest(
					new common::CSendMessageRequest(
						m_message
						, m_lastKey
						, context< CActivityControllerAction >().getActionKey()
						, new CNodeExceptionFilter( common::CMediumKinds::DimsNodes, m_informingNodes ) ) );

		return discard_event();
	}

	boost::statechart::result react( common::CNetworkInfoResult const & _networkInfoEvent )
	{
		if ( m_activationStatus.m_status == CActivitySatatus::Active )
		{
			CReputationTracker::getInstance()->setPresentNode( m_activationStatus.m_keyId );
		}
		else
		{
			return discard_event();
		}

		context< CActivityControllerAction >().addRequest(
					new common::CSendMessageRequest(
						m_message
						, m_lastKey
						, context< CActivityControllerAction >().getActionKey()
						, new CNodeExceptionFilter( common::CMediumKinds::DimsNodes, m_informingNodes ) ) );

		return discard_event();
	}

	typedef boost::mpl::list<
	boost::statechart::custom_reaction< common::CFailureEvent >,
	boost::statechart::custom_reaction< common::CNoMedium >,
	boost::statechart::custom_reaction< common::CAckEvent >,
	boost::statechart::custom_reaction< common::CTimeEvent >,
	boost::statechart::custom_reaction< common::CMessageResult >,
	boost::statechart::custom_reaction< common::CNetworkInfoResult >
	> reactions;

	std::set< uint160 > m_informingNodes;
	common::CMessage m_message;
	CPubKey m_lastKey;
	common::CActivationStatus m_activationStatus;
	bool m_alreadyInformed;
};

CActivityControllerAction::CActivityControllerAction( CPubKey const & _nodeKey, CAddress const & _address, CActivitySatatus::Enum _status )
{
	NodeKey = _nodeKey;
	Status = _status;
	Address = _address;
	initiate();
	process_event( CInitiateActivationEvent() );
}

CActivityControllerAction::CActivityControllerAction( uint256 const & _actionKey )
	: common::CAction( _actionKey )
{
	initiate();
	process_event( CRecognizeNodeStateEvent() );
}

void
CActivityControllerAction::accept( common::CSetResponseVisitor & _visitor )
{
	_visitor.visit( *this );
}

}