// Copyright (c) 2014-2015 DiMS dev-team
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef GET_BALANCE_ACTION_H
#define GET_BALANCE_ACTION_H

#include <boost/statechart/state_machine.hpp>

#include "common/scheduleAbleAction.h"

namespace tracker
{

struct CUninitiatedBalance;

class CGetBalanceAction : public common::CScheduleAbleAction, public boost::statechart::state_machine< CGetBalanceAction, CUninitiatedBalance >
{
public:
	CGetBalanceAction();

	CGetBalanceAction( uint160 const & _keyId, uint256 const & _hash );

	virtual void accept( common::CSetResponseVisitor & _visitor );

	virtual void reset(){}

	uint256 getHash() const { return m_hash; }

	uint160 getKeyId() const { return m_keyId; }
private:
	uint160 const m_keyId;

	uint256 const m_hash;
};

}

#endif // GET_BALANCE_ACTION_H
