// Copyright (c) 2014-2015 DiMS dev-team
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "internalMediumProvider.h"
#include "internalOperationsMedium.h"

#include "common/timeMedium.h"
#include "common/scheduledActionManager.h"

namespace monitor
{

CInternalMediumProvider * CInternalMediumProvider::ms_instance = NULL;

CInternalMediumProvider*
CInternalMediumProvider::getInstance( )
{
	if ( !ms_instance )
	{
		ms_instance = new CInternalMediumProvider();
	};
	return ms_instance;
}

CInternalMediumProvider::CInternalMediumProvider()
{
}

std::list< common::CMedium *>
CInternalMediumProvider::provideConnection( common::CMediumFilter const & _mediumFilter )
{
	return _mediumFilter.getMediums( this );
}

void
CInternalMediumProvider::registerRemoveCallback( CNodeSignals& nodeSignals )
{
	nodeSignals.NotifyAboutRemoval.connect( bind( &CInternalMediumProvider::removeNodeCallback, this, _1) );
}

void
CInternalMediumProvider::removeMedium( common::CMedium * _removeMedium )
{
	boost::lock_guard<boost::mutex> lock( m_mutex );

	CNode *remove;

	BOOST_FOREACH( PAIRTYPE( CNode *, common::CBitcoinNodeMedium * ) const medium, m_nodeToMedium )
	{
		if ( (common::CMedium * )medium.second == _removeMedium )
				remove = medium.first;
	}
	m_nodeToMedium.erase( remove );
	delete _removeMedium;
}

void
CInternalMediumProvider::removeNodeCallback( CNode * node )
{
	boost::lock_guard<boost::mutex> lock( m_mutex );

	if( m_nodeToMedium.find( node ) != m_nodeToMedium.end() )
		delete m_nodeToMedium.find( node )->second;
	m_nodeToMedium.erase( node );

}


std::list< common::CMedium *>
CInternalMediumProvider::getMediumByClass( common::CMediumKinds::Enum _mediumKind, unsigned int _mediumNumber )
{
	boost::lock_guard<boost::mutex> lock( m_mutex );
	std::list< common::CMedium *> mediums;

	if ( common::CMediumKinds::Internal == _mediumKind )
	{
		mediums.push_back( CInternalOperationsMedium::getInstance() );
	}
	else if ( common::CMediumKinds::Time == _mediumKind )
	{
		mediums.push_back( common::CTimeMedium::getInstance() );
	}
	else if ( common::CMediumKinds::Schedule == _mediumKind )
	{
		mediums.push_back( common::CScheduledActionManager::getInstance() );
	}
	else if ( common::CMediumKinds::BitcoinsNodes == _mediumKind )
	{
		LOCK(cs_vNodes);

		if ( m_nodeToMedium.size() < _mediumNumber )
		{

			unsigned int newMediums = ( vNodes.size() > _mediumNumber ? _mediumNumber : vNodes.size() ) - m_nodeToMedium.size();
			int i = 0;
			while( newMediums )
			{

				if ( m_nodeToMedium.find( vNodes[i] ) == m_nodeToMedium.end() )
				{
					m_nodeToMedium.insert( std::make_pair( vNodes[i], new common::CBitcoinNodeMedium( vNodes[i] ) ) );
					newMediums--;
				}
				i++;
			}

		}

		std::map< CNode *, common::CBitcoinNodeMedium * >::const_iterator iterator =  m_nodeToMedium.begin();
		//simplified  approach
		for ( unsigned int i = 0; i < ( vNodes.size() > _mediumNumber ? _mediumNumber : vNodes.size() ); i++ )
		{
			mediums.push_back( static_cast< common::CMedium * >( iterator->second ) );
			iterator++;
		}

	}
	return mediums;
}

unsigned int
CInternalMediumProvider::getBitcoinNodesAmount()const
{
	LOCK(cs_vNodes);
	return vNodes.size();
}

void
CInternalMediumProvider::setTransaction( CTransaction const & _response, CNode * _node )
{
	boost::lock_guard<boost::mutex> lock( m_mutex );

	std::map< CNode *, common::CBitcoinNodeMedium * >::iterator iterator = m_nodeToMedium.find( _node );

	if( iterator == m_nodeToMedium.end() ) return;// not  asked

	iterator->second->setResponse( _response );
}

void
CInternalMediumProvider::setMerkleBlock( CMerkleBlock const & _merkle, CNode * _node )
{
	boost::lock_guard<boost::mutex> lock( m_mutex );

	std::map< CNode *, common::CBitcoinNodeMedium * >::iterator iterator = m_nodeToMedium.find( _node );

	if( iterator == m_nodeToMedium.end() ) return;// not  asked

	iterator->second->setResponse( _merkle );
}

}
