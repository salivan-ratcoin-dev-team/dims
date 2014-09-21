// Copyright (c) 2014 Dims dev-team
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PAY_LOCAL_APPLICATION_ACTION_H
#define PAY_LOCAL_APPLICATION_ACTION_H

#include <boost/optional.hpp>

#include <boost/statechart/state_machine.hpp>

#include "common/action.h"
#include "common/request.h"
#include "configureNodeActionHadler.h"
#include "core.h"
#include "common/transactionStatus.h"

namespace common
{

template < class _RequestResponses >
class CSetResponseVisitor;

}

namespace client
{

struct CPrepareAndSendTransaction;

class CPayLocalApplicationAction : public common::CAction< NodeResponses >, public  boost::statechart::state_machine< CPayLocalApplicationAction, CPrepareAndSendTransaction >
{
public:
	CPayLocalApplicationAction( const CTransaction & _Transaction );

	void accept( common::CSetResponseVisitor< NodeResponses > & _visitor );

	common::CRequest< NodeResponses > * execute();

	void setRequest( common::CRequest< NodeResponses > * _request );

	CTransaction const & getTransaction() const;

	void setProcessingTrackerPtr( 	uintptr_t _ptr );

	uintptr_t getProcessingTrackerPtr() const;

	void setValidatedTransactionHash( uint256 _hash );

	uint256 getValidatedTransactionHash() const;
private:
	CTransaction m_transaction;

	common::ActionStatus::Enum m_actionStatus;

	common::CRequest< NodeResponses > * m_request;

	uintptr_t m_processingTrackerPtr;

	uint256 m_validatedTransactionHash;
};

}

#endif // PAY_LOCAL_APPLICATION_ACTION_H