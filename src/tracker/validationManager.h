// Copyright (c) 2014-2015 DiMS dev-team
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef VALIDATION_MANAGER_H
#define VALIDATION_MANAGER_H

#include "core.h"
#include "common/transactionStatus.h"

namespace tracker
{

class CTransactionRecordManager;

class CValidationManager
{
public:
	void serviceTransaction( CTransaction const & _tx );

	common::TransactionsStatus::Enum pullCurrentTransactionStatus( CTransaction const & _tx );

	static CValidationManager* getInstance( );
private:
	CValidationManager();

	void workLoop();

//	void  passTransactionBundleToNetwork( std::vector< CTransaction > _transactionBundle , TransactionStatus::Enum _status );

	void addBundleToSendQueue( std::vector< CTransaction > _bundle );

	CTransactionRecordManager * m_transactionRecordManager;
private:
	static CValidationManager * ms_instance;

    mutable boost::mutex buffMutex;
	std::vector< CTransaction > m_transactionsCandidates;

    mutable boost::mutex processedMutex;

	//std::list< std::pair< TransactionStatus::Enum, std::vector< CTransaction > > > m_bundlePipe;

    mutable boost::mutex validateTransLock;
	std::map< uint256, std::vector< CTransaction > > m_relayedTransactions;

	std::set< uint256 > m_validated;
/*


	*/
};


}

#endif
