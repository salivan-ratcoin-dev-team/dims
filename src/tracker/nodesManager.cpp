// Copyright (c) 2014 Ratcoin dev-team
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "nodesManager.h"

namespace tracker
{

CNodesManager * CNodesManager::ms_instance = NULL;

CNodesManager*
CNodesManager::getInstance( )
{
	if ( !ms_instance )
	{
		ms_instance = new CNodesManager();
	};
	return ms_instance;
}


CNodesManager::CNodesManager()
{

}

void
CNodesManager::addNode( CNode * _node )
{
	boost::lock_guard<boost::mutex> lock( m_nodesLock );
	m_unidentified.push_back( _node );
}

void
CNodesManager::PropagateBundle( std::vector< CTransaction > _bundle )
{

}

void
CNodesManager::handleMessages()
{

}

bool
CNodesManager::processMessagesFormNode( CNode * _node, std::vector< CMessage > const & _messages )
{
	return true;
}

void
CNodesManager::connectNodes()
{
}

void
CNodesManager::propagateMessage()
{

}

void
CNodesManager::analyseMessage()
{

}

}
