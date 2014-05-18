// Copyright (c) 2014 Ratcoin dev-team
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "connectToTrackerRequest.h"
#include "common/medium.h"

namespace tracker
{

CConnectToTrackerRequest::CConnectToTrackerRequest( std::string const & _trackerAddress )
	:m_trackerAddress( _trackerAddress )
{
}

void
CConnectToTrackerRequest::accept( common::CMedium< TrackerResponses > * _medium ) const
{
	_medium->add( this );
}

int
CConnectToTrackerRequest::getKind() const
{
	return 0;
}

std::string
CConnectToTrackerRequest::getAddress() const
{
	return m_trackerAddress;
}


}