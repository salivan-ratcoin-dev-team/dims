// Copyright (c) 2014 Dims dev-team
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CONNECT_TO_TRACKER_REQUEST_H
#define CONNECT_TO_TRACKER_REQUEST_H

#include "common/request.h"
#include "configureTrackerActionHandler.h"

namespace tracker
{

class CConnectToTrackerRequest : public common::CRequest< TrackerResponses >
{
public:
	CConnectToTrackerRequest( std::string const & _trackerAddress, CAddress const & _serviceAddress );

	virtual void accept( common::CMedium< TrackerResponses > * _medium ) const;

	virtual common::CMediumFilter< TrackerResponses > * getMediumFilter() const;

	std::string getAddress() const;

	CAddress getServiceAddress() const;

	~CConnectToTrackerRequest();
private:
	std::string const m_trackerAddress;

	CAddress const m_serviceAddress;
};


}

#endif // CONNECT_TO_TRACKER_REQUEST_H
