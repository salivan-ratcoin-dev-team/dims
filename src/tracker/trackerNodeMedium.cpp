// Copyright (c) 2014-2015 Dims dev-team
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "common/medium.h"
#include "common/actionHandler.h"

#include "tracker/trackerNodeMedium.h"
#include "tracker/trackerNodesManager.h"
#include "tracker/connectNodeAction.h"
#include "tracker/trackerRequests.h"

namespace tracker
{

CTrackerMessage::CTrackerMessage( CSynchronizationBlock const & _synchronizationInfo, uint256 const & _actionKey, uint256 const & _id )
{
	m_header = common::CHeader( (int)common::CPayloadKind::SynchronizationBlock, std::vector<unsigned char>(), GetTime(), CPubKey(), _actionKey, _id );

	common::createPayload( _synchronizationInfo, m_payload );

	common::CommunicationProtocol::signPayload( m_payload, m_header.m_signedHash );
}

CTrackerMessage::CTrackerMessage( CSynchronizationSegmentHeader const & _synchronizationSegmentHeader, uint256 const & _actionKey, uint256 const & _id )
{
	m_header = common::CHeader( (int)common::CPayloadKind::SynchronizationHeader, std::vector<unsigned char>(), GetTime(), CPubKey(), _actionKey, _id );

	common::createPayload( _synchronizationSegmentHeader, m_payload );

	common::CommunicationProtocol::signPayload( m_payload, m_header.m_signedHash );
}

void
CTrackerNodeMedium::add( CGetSynchronizationInfoRequest const * _request )
{
	common::CSynchronizationInfo synchronizationInfo;

	synchronizationInfo.m_timeStamp = _request->getTimeStamp();

	common::CMessage message( synchronizationInfo, _request->getActionKey(), _request->getId() );

	m_messages.push_back( message );

	setLastRequest( _request->getId(), (common::CRequest< common::CTrackerTypes >*)_request );
}

void
CTrackerNodeMedium::add( CGetNextBlockRequest const * _request )
{
	common::CGet get;

	get.m_type = _request->getBlockKind();

	common::CMessage message( get, _request->getActionKey(), _request->getId() );

	m_messages.push_back( message );

	setLastRequest( _request->getId(), (common::CRequest< common::CTrackerTypes >*)_request );
}

void
CTrackerNodeMedium::add( CTransactionsPropagationRequest const * _request )
{
	common::CTransactionBundle transactionBundle;

	transactionBundle.m_transactions = _request->getTransactions();

	common::CMessage message( transactionBundle, _request->getActionKey(), _request->getId() );

	m_messages.push_back( message );

	setLastRequest( _request->getId(), (common::CRequest< common::CTrackerTypes >*)_request );
}

void
CTrackerNodeMedium::add( CSetNextBlockRequest< CSegmentHeader > const * _request )
{
	CSynchronizationSegmentHeader synchronizationSegmentHeader( _request->getBlock(), _request->getBlockIndex() );

	CTrackerMessage message( synchronizationSegmentHeader, _request->getActionKey(), _request->getId() );

	m_messages.push_back( message );

	setLastRequest( _request->getId(), (common::CRequest< common::CTrackerTypes >*)_request );
}

void
CTrackerNodeMedium::add( CSetNextBlockRequest< CDiskBlock > const * _request )
{
	CSynchronizationBlock synchronizationBlock( _request->getBlock(), _request->getBlockIndex() );

	CTrackerMessage message( synchronizationBlock, _request->getActionKey(), _request->getId() );

	m_messages.push_back( message );

	setLastRequest( _request->getId(), (common::CRequest< common::CTrackerTypes >*)_request );
}

void
CTrackerNodeMedium::add( CTransactionsStatusRequest const * _request )
{
	common::CMessage message( common::CTransactionsBundleStatus( _request->getBundleStatus() ), _request->getActionKey(), _request->getId() );

	m_messages.push_back( message );

	setLastRequest( _request->getId(), (common::CRequest< common::CTrackerTypes >*)_request );
}

void
CTrackerNodeMedium::add( CPassMessageRequest const * _request )
{
	common::CMessage message( _request->getMessage(), _request->getPreviousKey(), _request->getActionKey(), _request->getId() );

	m_messages.push_back( message );

	setLastRequest( _request->getId(), (common::CRequest< common::CTrackerTypes >*)_request );
}

void
CTrackerNodeMedium::add( CDeliverInfoRequest const * _request )
{
	common::CMessage message( common::CInfoResponseData(), _request->getActionKey(), _request->getId() );

	m_messages.push_back( message );

	setLastRequest( _request->getId(), (common::CRequest< common::CTrackerTypes >*)_request );
}

void
CTrackerNodeMedium::add( CAskForRegistrationRequest const * _request )
{
	common::CMessage message( common::CAdmitAsk(), _request->getActionKey(), _request->getId() );

	m_messages.push_back( message );

	setLastRequest( _request->getId(), (common::CRequest< common::CTrackerTypes >*)_request );
}

void
CTrackerNodeMedium::add( CRegisterProofRequest const * _request )
{
	common::CMessage message( common::CAdmitProof(), _request->getActionKey(), _request->getId() );

	m_messages.push_back( message );

	setLastRequest( _request->getId(), (common::CRequest< common::CTrackerTypes >*)_request );
}

}
