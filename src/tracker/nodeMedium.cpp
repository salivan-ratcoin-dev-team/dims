// Copyright (c) 2014 Ratcoin dev-team
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "common/medium.h"
#include "identifyRequest.h"
#include "nodeMedium.h"
#include "continueRequest.h"

namespace tracker
{

class CHandleNodeResponsesVisitor : public boost::static_visitor< void >
{
public:
	CHandleNodeResponsesVisitor(CNodeMedium * const _nodeMedium):m_nodeMedium( _nodeMedium ){};

	void operator()( CIdentifyMessage const & _identifyMessage ) const
	{

	}
private:
	CNodeMedium * const m_nodeMedium;
};

uint256 CNodeMedium::m_counter = 0;

bool
CNodeMedium::serviced() const
{
	return true;
}


bool
CNodeMedium::flush()
{
	BOOST_FOREACH( CMessage const & message ,m_messages )
	{
		m_usedNode->setMessageToSend( message );
	}
	return true;

}
// it piss me off that I have to  keep this  bitch   here
std::vector< uint256 > deleteList;

bool
CNodeMedium::getResponse( std::vector< TrackerResponses > & _requestResponse ) const
{
	boost::lock_guard<boost::mutex> lock( m_mutex );

	BOOST_FOREACH( uint256 const & id, m_indexes )
	{
		std::map< uint256, TrackerResponses >::const_iterator iterator = m_responses.find( id );
		if ( iterator != m_responses.end() )
		{
			_requestResponse.push_back( iterator->second );
			deleteList.push_back( id );
		}
		else
		{
			_requestResponse.push_back( CContinueResult( id ) );
		}
	}

	return true;
}

void
CNodeMedium::clearResponses()
{
	BOOST_FOREACH( uint256 const & id, deleteList )
	{
		m_responses.erase( id );
	}
	deleteList.clear();
}

void
CNodeMedium::setResponseMessage( ProtocolMessage const & _protocolMessage )
{
	boost::apply_visitor( CHandleNodeResponsesVisitor( this ), _protocolMessage );
}

void
CNodeMedium::setResponse( uint256 const & _id, TrackerResponses const & _responses )
{
	m_responses.insert( std::make_pair( _id, _responses ) );
}

void
CNodeMedium::add( common::CRequest< TrackerResponses > const * _request )
{
}

void
CNodeMedium::add( CIdentifyRequest const * _request )
{
	CIdentifyMessage identifyMessage;
	identifyMessage.m_payload = _request->getPayload();

	CMessage message( identifyMessage );

	m_messages.push_back( message );

	uint256 hash = Hash( &identifyMessage.m_payload.front(), &identifyMessage.m_payload.back() );
	m_indexes.push_back( hash );
}

void
CNodeMedium::add( CIdentifyResponse const * _request )
{
	CIdentifyMessage identifyMessage;

	identifyMessage.m_signed = _request->getSigned();

	identifyMessage.m_key = _request->getKeyID();

	CMessage message( identifyMessage );

	m_messages.push_back( message );

	m_indexes.push_back( _request->getPayloadHash() );

}


void
CNodeMedium::add( CContinueReqest const * _request )
{
	m_indexes.push_back( _request->getRequestId() );

}

}
