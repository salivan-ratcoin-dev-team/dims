// Copyright (c) 2014-2015 Dims dev-team
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/foreach.hpp>
#include <exception> 

#include "settingsMedium.h"
#include "sendInfoRequestAction.h"

#include "common/nodeMessages.h"
#include "common/commonEvents.h"
#include "common/dimsParams.h"
#include "common/commonResponses.h"
#include "common/commonRequests.h"

#include "clientResponses.h"

using namespace common;

namespace client
{

CDefaultMedium * CDefaultMedium::ms_instance = NULL;

CDefaultMedium*
CDefaultMedium::getInstance( )
{
	if ( !ms_instance )
	{
		ms_instance = new CDefaultMedium();
	};
	return ms_instance;
}

CDefaultMedium::CDefaultMedium()
{
}

bool
CDefaultMedium::serviced() const
{
	return !m_requestResponse.empty();
}

void
CDefaultMedium::add( common::CRequest< common::CClientTypes > const * _request )
{
}

void
CDefaultMedium::add( CDnsInfoRequest const * _request )
{
	vector< CAddress > addresses;
	getSeedIps( addresses );

	CDnsInfo dnsInfo( addresses );

	m_requestResponse.insert( std::make_pair( ( common::CRequest< common::CClientTypes > * )_request, dnsInfo ) );
}

void
CDefaultMedium::getSeedIps( vector<CAddress> & _vAdd )
{
	const vector<CDNSSeedData> &vSeeds = dimsParams().DNSSeeds();

	BOOST_FOREACH(const CDNSSeedData &seed, vSeeds) {
		if (HaveNameProxy()) {
			AddOneShot(seed.host);
		} else {
			vector<CNetAddr> vIPs;

			if (LookupHost(seed.host.c_str(), vIPs))
			{
				BOOST_FOREACH(CNetAddr& ip, vIPs)
				{
					int nOneDay = 24*3600;
					CAddress addr = CAddress(CService(ip, dimsParams().GetDefaultPort()));
					addr.nTime = GetTime() - 3*nOneDay - GetRand(4*nOneDay); // use a random age between 3 and 7 days old
					_vAdd.push_back(addr);
				}
			}

		}
	}
}

bool
CDefaultMedium::flush()
{
	return true;
}

bool
CDefaultMedium::getResponseAndClear( std::multimap<  CRequest< common::CClientTypes >const*, ClientResponses > & _requestResponse )
{
	_requestResponse = m_requestResponse;

	clearResponses();

	return true;
}

void
CDefaultMedium::clearResponses()
{
	m_requestResponse.clear();
}

}
