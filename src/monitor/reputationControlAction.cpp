// Copyright (c) 2014-2015 DiMS dev-team
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/statechart/simple_state.hpp>
#include <boost/statechart/state.hpp>
#include <boost/statechart/transition.hpp>
#include <boost/statechart/custom_reaction.hpp>

#include "common/requests.h"
#include "common/events.h"
#include "common/setResponseVisitor.h"

#include "monitor/reputationControlAction.h"
#include "monitor/reputationTracer.h"
#include "monitor/filters.h"

namespace monitor
{
CReputationControlAction * CReputationControlAction::ms_instance = NULL;

namespace
{

int64_t calculateNextTime()
{
	int64_t recalculationTime = CReputationTracker::getInstance()->getMeasureReputationTime();
	int64_t period = CReputationTracker::getInstance()->getRecalculateTime();
	int64_t nextTime = recalculationTime + period;
	nextTime -= GetTime();
	//assert( nextTime >= -10 );// this  failing means, that reputation  controlling  is  wrongly  initiated
	if ( nextTime <= 0 )
		nextTime = 1;

	return nextTime * 1000;
}
}

struct CSelfOperateEvent : boost::statechart::event< CSelfOperateEvent >{};
struct CCatchUpEvent : boost::statechart::event< CCatchUpEvent >{};

struct CCatchUp;
struct COperating;

struct CReputationControlInitial : boost::statechart::simple_state< CReputationControlInitial, CReputationControlAction >
{
	typedef boost::mpl::list<
	boost::statechart::transition< CSelfOperateEvent, COperating >,
	boost::statechart::transition< CCatchUpEvent, CCatchUp >
	> reactions;
};

struct CCatchUp : boost::statechart::state< CCatchUp, CReputationControlAction >
{
	CCatchUp( my_context ctx ) : my_base( ctx )
	{
		context< CReputationControlAction >().forgetRequests();
		context< CReputationControlAction >().addRequest(
					new common::CTimeEventRequest(
						calculateNextTime()
						, new CMediumClassFilter( common::CMediumKinds::Time ) ) );

		m_allyMonitorData = CReputationTracker::getInstance()->getAllyMonitors();
	}

	boost::statechart::result react( common::CTimeEvent const & _timeEvent )
	{
		CReputationTracker::getInstance()->clearTransactions();
		m_allyMonitorData = CReputationTracker::getInstance()->getAllyMonitors();

		CReputationTracker::getInstance()->setMeasureReputationTime(
					CReputationTracker::getInstance()->getMeasureReputationTime()
					+ CReputationTracker::getInstance()->getRecalculateTime() );

		context< CReputationControlAction >().addRequest(
					new common::CTimeEventRequest(
						calculateNextTime()
						, new CMediumClassFilter( common::CMediumKinds::Time ) ) );

		return discard_event();
	}

	boost::statechart::result react( common::CMessageResult const & _messageResult )
	{
		common::CMessage orginalMessage;
		if ( !common::CommunicationProtocol::unwindMessage( _messageResult.m_message, orginalMessage, GetTime(), _messageResult.m_pubKey ) )
			assert( !"service it somehow" );

		if ( _messageResult.m_message.m_header.m_payloadKind == common::CPayloadKind::RankingInfo )
		{
			common::CRankingInfo rankingInfo;
			common::convertPayload( orginalMessage, rankingInfo );

			context< CReputationControlAction >().addRequest(
						new common::CAckRequest(
							context< CReputationControlAction >().getActionKey()
							, orginalMessage.m_header.m_id
							, new CByKeyMediumFilter( _messageResult.m_pubKey ) ) );

			// do  some  sanity??
			CAddress address;
			if ( !CReputationTracker::getInstance()->getAddresFromKey( _messageResult.m_pubKey.GetID(), address ) )
				assert( !"problem" );

			m_allyMonitorData.erase( common::CAllyMonitorData(_messageResult.m_pubKey, address ) );
			BOOST_FOREACH( common::CTrackerData const & trackerData, rankingInfo.m_trackers )
			{
				CReputationTracker::getInstance()->addAllyTracker( common::CAllyTrackerData( trackerData, _messageResult.m_pubKey ) );
			}
			if ( m_allyMonitorData.empty() )
				return transit< COperating >();//ready to start normal operations
		}
		return discard_event();
	}

	boost::statechart::result react( common::CAckEvent const & _ackEvent )
	{
		return discard_event();
	}

	typedef boost::mpl::list<
	boost::statechart::custom_reaction< common::CTimeEvent >,
	boost::statechart::custom_reaction< common::CMessageResult >,
	boost::statechart::custom_reaction< common::CAckEvent >
	> reactions;

	std::set< common::CAllyMonitorData > m_allyMonitorData;

};

struct COperating : boost::statechart::state< COperating, CReputationControlAction >
{
	COperating( my_context ctx ) : my_base( ctx )
	{
		context< CReputationControlAction >().forgetRequests();
		context< CReputationControlAction >().addRequest(
					new common::CTimeEventRequest(
						calculateNextTime()
						, new CMediumClassFilter( common::CMediumKinds::Time ) ) );
	}

	boost::statechart::result react( common::CTimeEvent const & _timeEvent )
	{
		CReputationTracker::getInstance()->recalculateReputation();

		context< CReputationControlAction >().addRequest(
					new common::CTimeEventRequest(
						calculateNextTime()
						, new CMediumClassFilter( common::CMediumKinds::Time ) ) );

		common::CRankingInfo rankingInfo(
			CReputationTracker::getInstance()->getTrackers()
			, CReputationTracker::getInstance()->getMeasureReputationTime() );

		context< CReputationControlAction >().addRequest(
				new common::CSendMessageRequest(
					common::CPayloadKind::RankingInfo
					, rankingInfo
					, context< CReputationControlAction >().getActionKey()
					, new CMediumClassFilter( common::CMediumKinds::Monitors ) ) );

		return discard_event();
	}


	boost::statechart::result react( common::CMessageResult const & _messageResult )
	{
		common::CMessage orginalMessage;
		if ( !common::CommunicationProtocol::unwindMessage( _messageResult.m_message, orginalMessage, GetTime(), _messageResult.m_pubKey ) )
			assert( !"service it somehow" );

		if ( _messageResult.m_message.m_header.m_payloadKind == common::CPayloadKind::RankingInfo )
		{
		common::CRankingInfo rankingInfo;
		common::convertPayload( orginalMessage, rankingInfo );

		context< CReputationControlAction >().addRequest(
					new common::CAckRequest(
						  context< CReputationControlAction >().getActionKey()
						, orginalMessage.m_header.m_id
						, new CByKeyMediumFilter( _messageResult.m_pubKey ) ) );
		}
		return discard_event();
	}

	boost::statechart::result react( common::CAckEvent const & _ackEvent )
	{
		return discard_event();
	}

	typedef boost::mpl::list<
	boost::statechart::custom_reaction< common::CTimeEvent >,
	boost::statechart::custom_reaction< common::CMessageResult >,
	boost::statechart::custom_reaction< common::CAckEvent >
	> reactions;
};
/*
send  request


*/


CReputationControlAction *
CReputationControlAction::getInstance()
{
	return ms_instance;
}

CReputationControlAction *
CReputationControlAction::createInstance( uint256 const & _actionKey )
{
	if ( ms_instance )
		ms_instance->setExit();

	ms_instance = new CReputationControlAction( _actionKey );

	return ms_instance;
}

CReputationControlAction *
CReputationControlAction::createInstance()
{
	if ( ms_instance )
		ms_instance->setExit();

	ms_instance = new CReputationControlAction();

	return ms_instance;
}

CReputationControlAction::CReputationControlAction()
{
	LogPrintf("reputation controller action: %p self \n", this );

	initiate();
	process_event( CSelfOperateEvent() );
}

CReputationControlAction::CReputationControlAction( uint256 const & _actionKey )
	: common::CAction( _actionKey )
{
	LogPrintf("reputation controller action: %p catch up \n", this );

	initiate();
	process_event( CCatchUpEvent() );
}

void
CReputationControlAction::accept( common::CSetResponseVisitor & _visitor )
{
	_visitor.visit( *this );
}

}
