#ifndef SEED_REQUESTS_LIST_H
#define SEED_REQUESTS_LIST_H

#include "configureSeedActionHandler.h"

namespace common
{
template < class RequestType >
class CIdentifyRequest;

template < class RequestType >
class CContinueReqest;

template < class RequestType >
class CIdentifyResponse;

template < class RequestType >
class CConnectToNodeRequest;

template < class RequestType >
class CNetworkRoleRequest;

template < class RequestType >
class CAckRequest;

}

namespace seed
{

typedef boost::mpl::list< common::CIdentifyRequest< SeedResponses >, common::CContinueReqest< SeedResponses >, common::CIdentifyResponse< SeedResponses >, common::CConnectToNodeRequest< SeedResponses >, common::CNetworkRoleRequest< SeedResponses >, common::CAckRequest< SeedResponses > > SeedRequestsList;

}

#endif // SEED_REQUESTS_LIST_H
