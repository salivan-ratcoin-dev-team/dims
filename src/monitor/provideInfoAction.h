// Copyright (c) 2014-2015 DiMS dev-team
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PROVIDE_INFO_ACTION_H
#define PROVIDE_INFO_ACTION_H

#include "common/scheduleAbleAction.h"
#include "common/filters.h"
#include "common/mediumKinds.h"

#include <boost/statechart/state_machine.hpp>

#include "protocol.h"

/*
current communication protocol is ineffective
consider using ack  request after  every successful message  reception
*/

namespace monitor
{

struct CInit;
// rework  this  sooner  or later

class CProvideInfoAction : public common::CScheduleAbleAction, public  boost::statechart::state_machine< CProvideInfoAction, CInit >
{
public:
	CProvideInfoAction( uint256 const & _actionKey, uintptr_t _nodeIndicator );

	CProvideInfoAction( common::CInfoKind::Enum _infoKind, common::CMediumKinds::Enum _mediumKind );

	CProvideInfoAction( common::CInfoKind::Enum _infoKind, uintptr_t _nodePtr );

	CProvideInfoAction( common::CInfoKind::Enum _infoKind, CPubKey _pubKey );

	virtual void accept( common::CSetResponseVisitor & _visitor );

	uintptr_t getNodeIndicator() const;

	common::CInfoKind::Enum getInfo() const{ return m_infoKind; }

	~CProvideInfoAction(){};
private:

	uintptr_t m_nodeIndicator;

	common::CInfoKind::Enum m_infoKind;
};

}

#endif // PROVIDE_INFO_ACTION_H
