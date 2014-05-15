// Copyright (c) 2014 Ratcoin dev-team
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "segmentFileStorage.h"

#include "simpleBuddy.h"
#include "util.h"
#include <boost/foreach.hpp>
#include "core.h"
#include "coins.h"

namespace tracker
{
/*
split to two files
how  to  append  new  content  to file 
does  the  serialisation  doing i t  right ??

fflush(file);
*/
size_t CSegmentFileStorage::m_lastSegmentIndex = 0;

const std::string
CSegmentFileStorage::ms_segmentFileName = "segments";

const std::string
CSegmentFileStorage::ms_headerFileName = "serheaders";

std::map< unsigned int, unsigned int > CDiskBlock::m_currentLastBlockIds;

#define assert(a) ;



CDiskBlock::CDiskBlock( CSimpleBuddy const & _simpleBuddy )
	:CSimpleBuddy( _simpleBuddy )
{
	for (unsigned int i = 0;i < MAX_BUCKET;i++)
	{
		m_currentLastBlockIds.insert( std::make_pair( i, 0 ) );
	}
	m_area = new unsigned char[ms_buddySize];
}

void *
CDiskBlock::translateToAddress( unsigned int _index )
{
	size_t baseUnit = ms_buddySize >> ms_buddyBaseLevel;
	return (void *)&m_area[ _index * baseUnit ];
}


CSegmentHeader::CSegmentHeader()
	: m_nextHeader(0)
{
}

void
CSegmentHeader::increaseUsageRecord( unsigned int _recordId )
{

}

void
CSegmentHeader::decreaseUsageRecord( unsigned int _recordId )
{

}

bool
CSegmentHeader::setNewRecord( unsigned int _bucked, CRecord const & _record )
{
	unsigned int maxNumber =  m_recordsNumber / m_maxBucket;

	for ( unsigned int i = 0; i < maxNumber; i++ )
	{
		CRecord & record = m_records[ _bucked + i * m_maxBucket ];
		if ( !record.m_isEmptySpace && record.m_blockNumber )
		{
			record = _record;
			return true;
		}
	}
	return false;
}

inline
bool
CSegmentHeader::isNextHeader() const
{
	return m_nextHeader;
}

unsigned int
CSegmentHeader::getNumberOfFullBucketSets()
{
	return m_recordsNumber/MAX_BUCKET;
}

int
CSegmentHeader::getIndexForBucket( unsigned int _bucket ) const
{
	for( int i = 0;i < m_recordsNumber/MAX_BUCKET; i++)
	{
		CRecord record = m_records[ i*(m_recordsNumber/MAX_BUCKET) + _bucket ];
		if ( record.m_blockNumber )
			return record.m_blockNumber;
	}
	return -1;
}

inline
bool
CSegmentHeader::givenRecordUsed(unsigned int _index ) const
{
	return m_records[_index].m_blockNumber;
}

unsigned int
CSegmentHeader::getUsedRecordNumber() const
{
	unsigned int usageCnt = 0;
	for ( unsigned i = 0; i < m_recordsNumber; ++i )
	{
		if ( givenRecordUsed(i) )
			usageCnt++;
	}
	return usageCnt;
}

inline
void
CSegmentHeader::setNextHeaderFlag()
{
	m_nextHeader = 1;
}


CRecord const &
CSegmentHeader::getRecord(unsigned int _bucket, unsigned int _index ) const
{
	assert( getNumberOfFullBucketSets() > _index );
	return m_records[ _index * MAX_BUCKET  + _bucket ];

}

CLocation::CLocation( unsigned int _bucket, unsigned int _position )
{
	m_location = CSegmentFileStorage::calculateLocation( _bucket, _position );
}
CLocation::CLocation( uint64_t const & _fullPosition )
{
	m_location = CSegmentFileStorage::calculateLocation( _fullPosition );
}

CSegmentFileStorage * CSegmentFileStorage::ms_instance = NULL;

CSegmentFileStorage*
CSegmentFileStorage::getInstance()
{
	if ( !ms_instance )
	{
		ms_instance = new CSegmentFileStorage();
	};
	return ms_instance;
}


CSegmentFileStorage::CSegmentFileStorage()
: m_recentlyStored(MAX_BUCKET)
{
	fillHeaderBuffor();
}

void
CSegmentFileStorage::includeTransaction( CTransaction const & _transaction, uint64_t const _timeStamp )
{
	boost::lock_guard<boost::mutex> lock(m_storeTransLock);

	std::vector< CTransaction > transactions;
	transactions.push_back( _transaction );

	m_transactionQueue.insert( std::make_pair( _timeStamp, transactions ) );
}

void
CSegmentFileStorage::includeTransactions( std::vector< CTransaction > const & _transactions, uint64_t const _timeStamp )
{
	boost::lock_guard<boost::mutex> lock(m_storeTransLock);
	m_transactionQueue.insert( std::make_pair( _timeStamp, _transactions ) );
}

CSegmentHeader &
CSegmentFileStorage::createNewHeader()
{
	if ( !m_headersCache.empty() )
		m_headersCache.back().setNextHeaderFlag();
	
	m_headersCache.push_back( CSegmentHeader() );
	
	return m_headersCache.back();
}

bool
CSegmentFileStorage::getBlock( unsigned int _index, CDiskBlock & _discBlock )
{
	return m_accessFile.loadSegmentFromFile< CDiskBlock >( _index, ms_segmentFileName, _discBlock );
}

bool
CSegmentFileStorage::getSegmentHeader( unsigned int _index, CSegmentHeader & _segmentHeader )
{
	return m_accessFile.loadSegmentFromFile< CSegmentHeader >( _index, ms_headerFileName, _segmentHeader );
}

void
CSegmentFileStorage::saveBlock( unsigned int _index, CSegmentHeader const & _header )
{
	m_accessFile.saveSegmentToFile( _index, ms_headerFileName, _header );
}

void
CSegmentFileStorage::saveBlock( unsigned int _index, CDiskBlock const & _block )
{
	m_accessFile.saveSegmentToFile( _index, ms_segmentFileName, _block );
}


unsigned int
CSegmentFileStorage::createRecordForBlock( unsigned int _bucket )
{
	std::vector< CSegmentHeader >::iterator iterator = m_headersCache.begin();

	while( iterator != m_headersCache.end() )
	{
		int index = iterator->getIndexForBucket( _bucket );
		if ( -1 != index )
		{
			iterator->setNewRecord( _bucket, CRecord(m_lastSegmentIndex,1));
		}
	}
}

uint64_t
CSegmentFileStorage::getPosition( CTransaction const & _transaction )
{
	unsigned int bucket = calculateBucket( _transaction.GetHash() );
	unsigned int reqLevel = CSimpleBuddy::getBuddyLevel( _transaction.GetSerializeSize(SER_DISK, CTransaction::CURRENT_VERSION) );

	std::map< unsigned int, unsigned int >::iterator iterator = m_usedBuddy.find( bucket );

	int index = -1, last = 0;
	if ( iterator != m_usedBuddy.end() )
	{
		last = iterator->second;

		for ( unsigned i = 0; i < iterator->second; ++i )
		{
			assert( m_transactionLocationToBuddy.find( calculateLocation( bucket, i ) ) != m_transactionLocationToBuddy.end() );
			TransactionLocationToBuddy::iterator buddy = m_transactionLocationToBuddy.find( calculateLocation( bucket, i ) );
			index = buddy->second->buddyAlloc( reqLevel );
			if ( index == -1 )
				continue;

			return createFullPosition( i, index, reqLevel, bucket );
		}
	}

	CSimpleBuddy * simpleBuddy = new CSimpleBuddy;

	index = simpleBuddy->buddyAlloc( reqLevel );

	CLocation location;

	if ( last )
	{
		m_usedBuddy.at( bucket )= ++last;
		m_transactionLocationToBuddy.insert( std::make_pair( CLocation( bucket, last - 1 ), simpleBuddy ) );
	}
	else
	{
		m_usedBuddy.insert( std::make_pair( bucket, 1 ) );
		m_transactionLocationToBuddy.insert( std::make_pair( CLocation( bucket, 0 ), simpleBuddy ) );
	}
	m_locationUsedFromLastUpdate.insert( location );

	return createFullPosition( last ? last - 1: 1, index, reqLevel, bucket );
}

unsigned int
CSegmentFileStorage::findDiscBlockInHeader( uint64_t const _location ) const
{
	unsigned int header = getPosition( _location )/ CSegmentHeader::getNumberOfFullBucketSets();
	std::vector< CSegmentHeader >::const_iterator iterator = m_headersCache.begin();

	std::advance( iterator , header );

	CRecord record = iterator->getRecord( getBucket( _location ), getPosition( _location ) % CSegmentHeader::getNumberOfFullBucketSets() );

	return record.m_blockNumber;
}

uint64_t
CSegmentFileStorage::calculateLocation( unsigned int _bucket, unsigned int _position )
{
	uint64_t location = _position;
	location <<= 32;
	location = location |_bucket;
	return location;
}

uint64_t
CSegmentFileStorage::calculateLocation( uint64_t const _fullPosition )
{
	return _fullPosition & 0xffffffff000000ff;
}

unsigned int
CSegmentFileStorage::getPosition( uint64_t const _fullPosition )
{
	return _fullPosition >> 32;
}

unsigned int
CSegmentFileStorage::getLevel( uint64_t const _fullPosition )
{
	return ( _fullPosition >> 8 ) & 0xff;
}

unsigned int
CSegmentFileStorage::getIndex( uint64_t const _fullPosition )
{
	return ( _fullPosition >> 16 ) & 0xffff;
}

unsigned int
CSegmentFileStorage::getBucket( uint64_t const _fullPosition )
{
	return _fullPosition & 0xff;
}

uint64_t
CSegmentFileStorage::createFullPosition( unsigned int _blockPosition, unsigned int _index, unsigned int _level, unsigned int _bucket )
{
	uint64_t fullPosition = _blockPosition;
	fullPosition <<= 16;
	fullPosition = fullPosition | _index;
	fullPosition <<= 8;
	fullPosition = fullPosition | _level;
	fullPosition <<= 8;
	fullPosition = fullPosition | _bucket;
	return fullPosition;
}

CDiskBlock*
CSegmentFileStorage::getDiscBlock( uint64_t const _location )
{
	int blockNumber = findDiscBlockInHeader( _location );

	CDiskBlock * diskBlock = new CDiskBlock;
	if ( blockNumber != -1 )
		getBlock( blockNumber, *diskBlock );

	return diskBlock;
}
// add logic  to limit  max amount of disc flushes. I don't want  to spend to much time in this logic
// it is a  shame that I can't use move construction from c++11
void 
CSegmentFileStorage::flushLoop()
{
	while(1)
	{
		{
			boost::lock_guard<boost::mutex> lock(m_cachelock);


			std::multimap< uint64_t, std::vector< CTransaction > >::iterator iterator = m_transactionQueue.begin();

			// separate  dump  logic  from disc cache  build  logic????????????
			while( iterator != m_transactionQueue.end() )
			{
				// save time stamp in database
				//
				BOOST_FOREACH( CTransaction const & transaction, iterator->second )
				{
					uint64_t location = transaction.m_location;
					std::map< CLocation,CDiskBlock* >::iterator usedBlock =  m_usedBlocks.find( location );
					if ( usedBlock == m_usedBlocks.end() )
					{
						mruset< CCacheElement >::iterator cacheIterator = m_discBlockCache.find( location );
						if ( cacheIterator != m_discBlockCache.end() )
							usedBlock = m_usedBlocks.insert( std::make_pair( location, cacheIterator->m_discBlock ) ).first;
						else
							usedBlock = m_usedBlocks.insert( std::make_pair( location, getDiscBlock( location ) ) ).first;
					}

					CBufferAsStream stream(
								(char *)usedBlock->second->translateToAddress( getIndex( transaction.m_location ) )
								, usedBlock->second->getBuddySize( getLevel( transaction.m_location ) )
								, SER_DISK
								, CLIENT_VERSION);

					stream << transaction;

					assert( m_discCache.find( location ) != m_discCache.end() );
					//	usedBlock->second = m_discCache.find( location )->second;
				}
			}

			BOOST_FOREACH( UsedBlocks::value_type const & _block, m_usedBlocks)
			{
					findDiscBlockInHeader( uint64_t const _location );
			_block.first
					createRecordForBlock( storeCandidate );
			saveBlock( m_lastSegmentIndex, _block );
*/
			}

			//save

			/*
					if ( iterator->second->m_blockPosition )
					{
						saveBlock( m_lastSegmentIndex, *iterator->second );
					}
					else
					{
						m_lastSegmentIndex++;

						createRecordForBlock( storeCandidate );
						saveBlock( m_lastSegmentIndex, *iterator->second );
						// update block  field

					}

					m_recentlyStored.insert( CStore(newStoretime,storeCandidate) );
				}

				m_accessFile.flush(ms_segmentFileName);
			}
		}
		{
			boost::lock_guard<boost::mutex> lock(m_headerCacheLock);

			unsigned int index = 0;
			BOOST_FOREACH( CSegmentHeader & header, m_headersCache )
			{
				index++;
				saveBlock(index,header);
			}

		}*/
			m_accessFile.flush(ms_headerFileName);

			//rebuild merkle and store it, in database
			boost::this_thread::interruption_point();
			//MilliSleep(40000);
			MilliSleep(400);//only for  debug such short period of time
		}
	}
}

void
CSegmentFileStorage::readTransactions( CCoinsViewCache * _coinsViewCache )
{
	boost::lock_guard<boost::mutex> lock(m_headerCacheLock);

	fillHeaderBuffor();

	std::vector< CSegmentHeader >::iterator iterator = m_headersCache.begin();

	for ( int i = 0; i < calculateStoredBlockNumber(); i++)
	{
		CDiskBlock discBlock;

		getBlock( i, discBlock );
		
		int level = CSimpleBuddy::ms_buddyBaseLevel;

		while(level)
		{
			std::list< int > transactionsInd = discBlock.getNotEmptyIndexes( level );

			std::list< int >::iterator iterator = transactionsInd.begin();

			BOOST_FOREACH( int index, transactionsInd )
			{

				CBufferAsStream stream(
					  (char *)discBlock.translateToAddress( index )
					, discBlock.getBuddySize( level )
					, SER_DISK
					, CLIENT_VERSION);

				CTransaction transaction;

				stream >> transaction;
				/* do  something  with  those transactions */
				//	_coinsViewCache->SetCoins(transaction->GetHash(), CCoins( *transaction,*iterator ));

			}
			level--;
		}
	}

}

void
CSegmentFileStorage::eraseTransaction( CTransaction const & _transaction )
{
}

void
CSegmentFileStorage::eraseTransaction( CCoins const & _coins )
{
/*	int index = -1;

	ToInclude toInclude;// = m_discCache.equal_range(_coins.m_bucket);

	unsigned short recordId;// = _coins.nHeight % CSegmentHeader::m_recordsNumber;

	unsigned short cacheId;// = (_coins.nHeight >> 16)/CSegmentHeader::m_recordsNumber;

	if ( toInclude.first != m_discCache.end() )
	{
		for ( CacheIterators cacheIterator = toInclude.first; cacheIterator!=toInclude.second; ++cacheIterator )
		{
			cacheId--;

			if ( cacheId == 0 )
				cacheIterator->second->buddyFree(_coins.nHeight & 0xff);
			//set  time
		}
	}*/
}


unsigned int
CSegmentFileStorage::calculateBucket( uint256 const & _coinsHash ) const
{
	return  _coinsHash.GetLow64() % MAX_BUCKET;
}

void *
CSegmentFileStorage::getNextFreeBlock()
{
	return 0;
}

void
CSegmentFileStorage::fillHeaderBuffor()
{
	CSegmentHeader header;

	if ( m_headersCache.size() > 0 )
	{
		header = m_headersCache.back();
	}
	else
	{
		if (! getSegmentHeader( 0, header ) )
		{
			m_headersCache.push_back(header);
			return;
		}
	}

	while(header.isNextHeader())
	{
		getSegmentHeader( m_headersCache.size(), header );
		m_headersCache.push_back(header);
	}
}

unsigned int
CSegmentFileStorage::calculateBlockIndex( void * _block )
{
	return 0;
}


unsigned int
CSegmentFileStorage::calculateStoredBlockNumber() const
{
	unsigned int blockCnt = 0;
	BOOST_FOREACH( CSegmentHeader header, m_headersCache )
	{
		blockCnt += header.getUsedRecordNumber();
	}
}

}
