// Copyright (c) 2014-2015 DiMS dev-team
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "common/segmentFileStorage.h"

#include "monitor/copyStorageHandler.h"

namespace monitor
{

CCopyStorageHandler * CCopyStorageHandler::ms_instance = NULL;

CCopyStorageHandler*
CCopyStorageHandler::getInstance()
{
	if ( !ms_instance )
	{
		ms_instance = new CCopyStorageHandler();
	};
	return ms_instance;
}

bool
CCopyStorageHandler::createCopyRequest()
{
	boost::lock_guard<boost::mutex> lock( m_requestLock );
	if ( m_copyRequest )
		return false;

	m_copyRequest = true;

	return true;
}

bool
CCopyStorageHandler::copyCreated()
{
	return m_copyCreated;
}

CCopyStorageHandler::CCopyStorageHandler()
	: m_copyRequest( false )
	, m_copyCreated( false )
{
}

uint64_t
CCopyStorageHandler::getTimeStamp()
{
	return m_timeStamp;
}

uint64_t
CCopyStorageHandler::getDiscBlockSize() const
{
	return common::CSegmentFileStorage::getInstance()->calculateStoredBlockNumber();
}

uint64_t
CCopyStorageHandler::getSegmentHeaderSize() const
{
	return common::CSegmentFileStorage::getInstance()->getStoredHeaderCount();
}


void
CCopyStorageHandler::loop()
{

	while(1)
	{
		{
			boost::lock_guard<boost::mutex> lock( m_requestLock );
			if ( m_copyRequest )
			{
				m_timeStamp = GetTime();

				m_copyCreated = false;

				m_storageSize = common::CSegmentFileStorage::getInstance()->calculateStoredBlockNumber();
				m_headerSize = common::CSegmentFileStorage::getInstance()->getStoredHeaderCount();

				if ( m_headerSize )
					common::CSegmentFileStorage::getInstance()->copyHeader();

				if ( m_storageSize )
					common::CSegmentFileStorage::getInstance()->copyStorage();

				m_copyCreated = true;
				m_copyRequest = false;
			}
		}
		MilliSleep(1000);
	}
}

}
