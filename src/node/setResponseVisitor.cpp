// Copyright (c) 2014 Ratcoin dev-team
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "setResponseVisitor.h"
#include "sendTransactionAction.h"
#include "requestRespond.h"
#include "connectAction.h"
#include "sendBalanceInfoAction.h"

#include <boost/any.hpp>
#include <boost/optional.hpp>

using namespace  common;

namespace node
{

template < class T >
class CResponseVisitorBase : public boost::static_visitor< boost::optional< T > >
{
public:
	virtual boost::optional< T > operator()(CTransactionStatus & _transactionStatus ) const
	{
		return boost::optional< T >();
	}

	virtual boost::optional< T > operator()(CTrackerStats & _trackerStats ) const
	{
		return boost::optional< T >();
	}

	virtual boost::optional< T > operator()(CMonitorInfo & _monitorInfo ) const
	{
		return boost::optional< T >();
	}

	virtual boost::optional< T > operator()(CPending & _peding ) const
	{
		return boost::optional< T >();
	}


	virtual boost::optional< T > operator()(CAvailableCoins & _availableCoins ) const
	{
		return boost::optional< T >();
	}
	virtual boost::optional< T > operator()(CSystemError & _systemError ) const
	{
		return boost::optional< T >();
	}

};

class CGetTransactionStatus : public CResponseVisitorBase< common::TransactionsStatus::Enum >
{
public:
	boost::optional< common::TransactionsStatus::Enum > operator()(CTransactionStatus & _transactionStatus ) const
	{
		return _transactionStatus.m_status;
	}

};

class CGetToken : public CResponseVisitorBase< uint256 >
{
public:
	boost::optional< uint256 > operator()(CTransactionStatus & _transactionStatus ) const
	{
		return _transactionStatus.m_token;
	}
};

class CGetTrackerInfo : public CResponseVisitorBase< CTrackerStats >
{
public:
	boost::optional< CTrackerStats > operator()(CTrackerStats & _transactionStatus ) const
	{
		return _transactionStatus;
	}
};

class CGetMediumError : public CResponseVisitorBase< ErrorType::Enum >
{
public:
    boost::optional< ErrorType::Enum > operator()(CSystemError & _systemError ) const
    {
        return _systemError.m_errorType;
    }
};

class CGetBalance : public CResponseVisitorBase< std::vector< CCoins > >
{
public:
	boost::optional< std::vector< CCoins > > operator()(CAvailableCoins & _availableCoins ) const
	{

		return _availableCoins.m_availableCoins;
	}
};

void 
CSetResponseVisitor::visit( CSendTransactionAction & _action )
{
	_action.setTransactionStatus(boost::apply_visitor( (CResponseVisitorBase< common::TransactionsStatus::Enum > const &)CGetTransactionStatus(), m_requestRespond ));
	_action.setTransactionToken(boost::apply_visitor( (CResponseVisitorBase< uint256 > const &)CGetToken(), m_requestRespond ));
}


void
CSetResponseVisitor::visit( CConnectAction & _action )
{
	_action.setInProgressToken(boost::apply_visitor( (CResponseVisitorBase< uint256 > const &)CGetToken(), m_requestRespond ));
	_action.setTrackerInfo(boost::apply_visitor( (CResponseVisitorBase< CTrackerStats > const &)CGetTrackerInfo(), m_requestRespond ));
    _action.setMediumError(boost::apply_visitor( (CResponseVisitorBase< ErrorType::Enum > const &)CGetMediumError(), m_requestRespond ));
}

void
CSetResponseVisitor::visit( CSendBalanceInfoAction & _action )
{
	_action.setBalance(boost::apply_visitor( (CResponseVisitorBase< std::vector< CCoins > > const &)CGetBalance(), m_requestRespond ));
}

void
CSetResponseVisitor::visit( CAction & _action )
{

}


}
