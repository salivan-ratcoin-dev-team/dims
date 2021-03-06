// Copyright (c) 2014-2015 DiMS dev-team
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/statechart/simple_state.hpp>
#include <boost/statechart/state.hpp>
#include <boost/statechart/transition.hpp>
#include <boost/statechart/custom_reaction.hpp>

#include "wallet.h"

#include "common/analyseTransaction.h"
#include "common/actionHandler.h"
#include "common/authenticationProvider.h"
#include "common/setResponseVisitor.h"
#include "common/requests.h"
#include "common/events.h"
#include "common/segmentFileStorage.h"
#include "common/mediumKinds.h"
#include "common/supportTransactionsDatabase.h"
#include "common/events.h"

#include "monitor/synchronizationAction.h"
#include "monitor/filters.h"
#include "monitor/copyStorageHandler.h"
#include "monitor/transactionRecordManager.h"
#include "monitor/trackOriginAddressAction.h"
#include "monitor/reputationControlAction.h"
#include "monitor/activityControllerAction.h"

namespace monitor
{

unsigned const SynchronisingGetInfoTime = 10000;//milisec

unsigned const SynchronisingWaitTime = 15000;

struct CSynchronizingGetInfo;
struct CSynchronizingHeaders;
struct CSynchronizedUninitialized;
struct CSynchronizedProvideCopy;
struct CSynchronized;
struct CSynchronizingAsk;
struct CGetBitcoinHeader;

namespace  // not safe in  general !!
{
unsigned int HeaderSize;
unsigned int StrageSize;
}


struct CSwitchToSynchronized : boost::statechart::event< CSwitchToSynchronized >
{
};

struct CSwitchToSynchronizing : boost::statechart::event< CSwitchToSynchronizing >
{
};

struct CUninitiatedSynchronization : boost::statechart::simple_state< CUninitiatedSynchronization, CSynchronizationAction >
{
	typedef boost::mpl::list<
	boost::statechart::transition< CSwitchToSynchronizing, CSynchronizingAsk >,
	boost::statechart::transition< CSwitchToSynchronized, CSynchronizedUninitialized >
	> reactions;
};


struct CSynchronizingAsk : boost::statechart::state< CSynchronizingAsk, CSynchronizationAction >
{
	CSynchronizingAsk( my_context ctx ) : my_base( ctx )
	{
		LogPrintf("synchronize action: %p ask \n", &context< CSynchronizationAction >() );
		context< CSynchronizationAction >().forgetRequests();

		context< CSynchronizationAction >().addRequest(
				new common::CSendMessageRequest(
					common::CPayloadKind::SynchronizationAsk
					, common::CSynchronizationAsk()
					, context< CSynchronizationAction >().getActionKey()
					, new CByKeyMediumFilter( context< CSynchronizationAction >().getPartnerKey() ) ) );

		context< CSynchronizationAction >().addRequest(
					new common::CTimeEventRequest(
						 SynchronisingGetInfoTime
						, new CMediumClassFilter( common::CMediumKinds::Time ) ) );
	}

	boost::statechart::result react( common::CTimeEvent const & _timeEvent )
	{
		context< CSynchronizationAction >().forgetRequests();
		context< CSynchronizationAction >().setResult( common::CSynchronizationResult( 0 ) );
		context< CSynchronizationAction >().setExit();
		return discard_event();
	}

	boost::statechart::result react( common::CAckEvent const & _promptAck )
	{
		return discard_event();
	}

	boost::statechart::result react( common::CMessageResult const & _messageResult )
	{
		common::CMessage orginalMessage;
		if ( !common::CommunicationProtocol::unwindMessage( _messageResult.m_message, orginalMessage, GetTime(), _messageResult.m_pubKey) )
			assert( !"service it somehow" );

		if ( orginalMessage.m_header.m_payloadKind == common::CPayloadKind::Result )
		{
			common::CResult result;

			common::convertPayload( orginalMessage, result );

			context< CSynchronizationAction >().forgetRequests();

			context< CSynchronizationAction >().addRequest(
						new common::CAckRequest(
							  context< CSynchronizationAction >().getActionKey()
							, _messageResult.m_message.m_header.m_id
							, new CByKeyMediumFilter( _messageResult.m_pubKey ) ) );

			if ( result.m_result )
			{
				CWallet::getInstance()->resetDatabase();

				CWallet::getInstance()->AddKeyPubKey(
							common::CAuthenticationProvider::getInstance()->getMyPrivKey()
							, common::CAuthenticationProvider::getInstance()->getMyKey());

				common::CSegmentFileStorage::getInstance()->setSynchronizationInProgress();

				return transit< CGetBitcoinHeader >();
			}
			context< CSynchronizationAction >().setResult( common::CSynchronizationResult( 0 ) );
			context< CSynchronizationAction >().setExit();
		}
		return discard_event();
	}

	typedef boost::mpl::list<
	boost::statechart::custom_reaction< common::CTimeEvent >,
	boost::statechart::custom_reaction< common::CMessageResult >,
	boost::statechart::custom_reaction< common::CAckEvent >
	> reactions;
};

struct CGetBitcoinHeader: boost::statechart::state< CGetBitcoinHeader, CSynchronizationAction >
{
	CGetBitcoinHeader( my_context ctx ) : my_base( ctx )
	{
		LogPrintf("synchronize action: %p fetch bitcoin header \n", &context< CSynchronizationAction >() );

		context< CSynchronizationAction >().forgetRequests();

		context< CSynchronizationAction >().addRequest(
				new common::CSendMessageRequest(
					common::CPayloadKind::InfoReq
					, common::CInfoRequestData( (int)common::CInfoKind::BitcoinHeaderAsk, std::vector<unsigned char>() )
					, context< CSynchronizationAction >().getActionKey()
					, new CByKeyMediumFilter( context< CSynchronizationAction >().getPartnerKey() ) ) );

		context< CSynchronizationAction >().addRequest(
					new common::CTimeEventRequest(
						 SynchronisingGetInfoTime
						, new CMediumClassFilter( common::CMediumKinds::Time ) ) );
	}


	boost::statechart::result react( common::CMessageResult const & _messageResult )
	{
		common::CMessage orginalMessage;
		if ( !common::CommunicationProtocol::unwindMessage( _messageResult.m_message, orginalMessage, GetTime(), _messageResult.m_pubKey) )
			assert( !"service it somehow" );

		if ( orginalMessage.m_header.m_payloadKind == common::CPayloadKind::SynchronizationBitcoinHeader )
		{
			common::CBitcoinHeader bitcoinHeader;

			common::convertPayload( orginalMessage, bitcoinHeader );

			CAutoFile file( OpenHeadFile(false), SER_DISK, CLIENT_VERSION );

			file << bitcoinHeader.m_bitcoinHeader;
			fflush(file);
			FileCommit(file);

			resetChains();
			context< CSynchronizationAction >().addRequest(
						new common::CAckRequest(
							  context< CSynchronizationAction >().getActionKey()
							, _messageResult.m_message.m_header.m_id
							, new CByKeyMediumFilter( _messageResult.m_pubKey ) ) );

			common::CActionHandler::getInstance()->executeAction( CTrackOriginAddressAction::createInstance() );

		}
		return transit< CSynchronizingGetInfo >();
	}

	boost::statechart::result react( common::CTimeEvent const & _timeEvent )
	{
		context< CSynchronizationAction >().forgetRequests();
		context< CSynchronizationAction >().setResult( common::CSynchronizationResult( 0 ) );
		context< CSynchronizationAction >().setExit();
		return discard_event();
	}

	boost::statechart::result react( common::CAckEvent const & _promptAck )
	{
		return discard_event();
	}

	typedef boost::mpl::list<
	boost::statechart::custom_reaction< common::CTimeEvent >,
	boost::statechart::custom_reaction< common::CMessageResult >,
	boost::statechart::custom_reaction< common::CAckEvent >
	> reactions;
};

struct CSynchronizingGetInfo : boost::statechart::state< CSynchronizingGetInfo, CSynchronizationAction >
{
	CSynchronizingGetInfo( my_context ctx ) : my_base( ctx )
	{
		LogPrintf("synchronize action: %p get synchronizing info \n", &context< CSynchronizationAction >() );
		context< CSynchronizationAction >().forgetRequests();

		context< CSynchronizationAction >().addRequest(
				new common::CSendMessageRequest(
					common::CPayloadKind::InfoReq
					, common::CInfoRequestData( (int)common::CInfoKind::StorageInfoAsk, 0 )
					, context< CSynchronizationAction >().getActionKey()
					, new CByKeyMediumFilter( context< CSynchronizationAction >().getPartnerKey() ) ) );
	}

	boost::statechart::result react( common::CMessageResult const & _messageResult )
	{
		common::CMessage orginalMessage;
		if ( !common::CommunicationProtocol::unwindMessage( _messageResult.m_message, orginalMessage, GetTime(), _messageResult.m_pubKey) )
			assert( !"service it somehow" );

		if ( orginalMessage.m_header.m_payloadKind == common::CPayloadKind::SynchronizationInfo )
		{
			common::CSynchronizationInfo synchronizationInfo;

			common::convertPayload( orginalMessage, synchronizationInfo );

			StrageSize = synchronizationInfo.m_strageSize;
			HeaderSize = synchronizationInfo.m_headerSize;

			if ( !StrageSize && !HeaderSize )
			{
				context< CSynchronizationAction >().onExit();
			}
			else
			{
				context< CSynchronizationAction >().addRequest(
							new common::CAckRequest(
								context< CSynchronizationAction >().getActionKey()
								, _messageResult.m_message.m_header.m_id
								, new CByKeyMediumFilter( _messageResult.m_pubKey ) ) );
			}
		}
		return transit< CSynchronizingHeaders >();
	}

	typedef boost::mpl::list<
	boost::statechart::custom_reaction< common::CMessageResult >
	> reactions;
};

struct CSynchronizingBlocks : boost::statechart::state< CSynchronizingBlocks, CSynchronizationAction >
{
	CSynchronizingBlocks( my_context ctx ) : my_base( ctx ), m_currentBlock( 0 )
	{
		LogPrintf("synchronize action: %p fetch blocks \n", &context< CSynchronizationAction >() );
		context< CSynchronizationAction >().forgetRequests();

		context< CSynchronizationAction >().addRequest(
				new common::CSendMessageRequest(
					common::CPayloadKind::SynchronizationGet
					, common::CSynchronizationGet( (int)common::CBlockKind::Segment, m_currentBlock )
					, context< CSynchronizationAction >().getActionKey()
					, new CByKeyMediumFilter( context< CSynchronizationAction >().getPartnerKey() ) ) );

		context< CSynchronizationAction >().addRequest(
					new common::CTimeEventRequest(
						  SynchronisingWaitTime
						, new CMediumClassFilter( common::CMediumKinds::Time ) ) );
	}

	boost::statechart::result react( common::CMessageResult const & _messageResult )
	{
		common::CMessage orginalMessage;
		if ( !common::CommunicationProtocol::unwindMessage( _messageResult.m_message, orginalMessage, GetTime(), _messageResult.m_pubKey) )
			assert( !"service it somehow" );

		if ( orginalMessage.m_header.m_payloadKind == common::CPayloadKind::SynchronizationBlock )
		{
			common::CSynchronizationBlock synchronizationBlock( new common::CDiskBlock(), -1 );

			common::convertPayload( orginalMessage, synchronizationBlock );
			context< CSynchronizationAction >().forgetRequests();

			std::vector< CTransaction > transactions;

			assert( synchronizationBlock.m_blockIndex == m_currentBlock );

			common::CSegmentFileStorage::getInstance()->setDiscBlock( *synchronizationBlock.m_diskBlock, synchronizationBlock.m_blockIndex, transactions );

			BOOST_FOREACH( CTransaction const & transaction, transactions )
			{
				common::findSelfCoinsAndAddToWallet( transaction );
				common::CSupportTransactionsDatabase::getInstance()->setTransactionLocation( transaction.GetHash(), transaction.m_location );
			}

			common::CSupportTransactionsDatabase::getInstance()->flush();

			context< CSynchronizationAction >().addRequest(
						new common::CAckRequest(
							  context< CSynchronizationAction >().getActionKey()
							, _messageResult.m_message.m_header.m_id
							, new CByKeyMediumFilter( context< CSynchronizationAction >().getPartnerKey() ) ) );

			if ( StrageSize > ++m_currentBlock )
			{

				context< CSynchronizationAction >().addRequest(
						new common::CSendMessageRequest(
							common::CPayloadKind::SynchronizationGet
							, common::CSynchronizationGet( (int)common::CBlockKind::Segment, m_currentBlock )
							, context< CSynchronizationAction >().getActionKey()
							, new CByKeyMediumFilter( _messageResult.m_pubKey ) ) );
			}
			else
			{
				context< CSynchronizationAction >().onExit();
			}
		}
		return discard_event();
	}

	boost::statechart::result react( common::CTimeEvent const & _timeEvent )
	{
		context< CSynchronizationAction >().forgetRequests();

		context< CSynchronizationAction >().addRequest(
				new common::CSendMessageRequest(
					common::CPayloadKind::SynchronizationGet
					, common::CSynchronizationGet( (int)common::CBlockKind::Segment, m_currentBlock )
					, context< CSynchronizationAction >().getActionKey()
					, new CByKeyMediumFilter( context< CSynchronizationAction >().getPartnerKey() ) ) );

		context< CSynchronizationAction >().addRequest(
					new common::CTimeEventRequest(
						  SynchronisingWaitTime
						, new CMediumClassFilter( common::CMediumKinds::Time ) ) );

		return discard_event();
	}

	boost::statechart::result react( common::CAckEvent const & _promptAck )
	{
		return discard_event();
	}

	~CSynchronizingBlocks()
	{
		common::CSegmentFileStorage::getInstance()->releaseSynchronizationInProgress();
	}

	typedef boost::mpl::list<
	boost::statechart::custom_reaction< common::CTimeEvent >,
	boost::statechart::custom_reaction< common::CAckEvent >,
	boost::statechart::custom_reaction< common::CMessageResult >
	> reactions;

	unsigned int m_currentBlock;
};


struct CSynchronizingHeaders : boost::statechart::state< CSynchronizingHeaders, CSynchronizationAction >
{
	CSynchronizingHeaders( my_context ctx ) : my_base( ctx ), m_currentBlock( 0 )
	{
		LogPrintf("synchronize action: %p fetch headers \n", &context< CSynchronizationAction >() );
		context< CSynchronizationAction >().forgetRequests();

		context< CSynchronizationAction >().addRequest(
				new common::CSendMessageRequest(
					common::CPayloadKind::SynchronizationGet
					, common::CSynchronizationGet( (int)common::CBlockKind::Header, m_currentBlock )
					, context< CSynchronizationAction >().getActionKey()
					, new CByKeyMediumFilter( context< CSynchronizationAction >().getPartnerKey() ) ) );

		context< CSynchronizationAction >().addRequest(
					new common::CTimeEventRequest(
						  SynchronisingWaitTime
						, new CMediumClassFilter( common::CMediumKinds::Time ) ) );
	}

	boost::statechart::result react( common::CMessageResult const & _messageResult )
	{
		common::CMessage orginalMessage;
		if ( !common::CommunicationProtocol::unwindMessage( _messageResult.m_message, orginalMessage, GetTime(), _messageResult.m_pubKey) )
			assert( !"service it somehow" );

		if ( orginalMessage.m_header.m_payloadKind == common::CPayloadKind::SynchronizationHeader )
		{
			common::CSynchronizationSegmentHeader synchronizationHeader( new common::CSegmentHeader(), -1 );

			common::convertPayload( orginalMessage, synchronizationHeader );
			context< CSynchronizationAction >().forgetRequests();

			common::CSegmentFileStorage::getInstance()->setDiscBlock( *synchronizationHeader.m_segmentHeader, synchronizationHeader.m_blockIndex );

			context< CSynchronizationAction >().addRequest(
						new common::CAckRequest(
							  context< CSynchronizationAction >().getActionKey()
							, _messageResult.m_message.m_header.m_id
							, new CByKeyMediumFilter( _messageResult.m_pubKey ) ) );

			if ( HeaderSize > ++m_currentBlock )
			{
				context< CSynchronizationAction >().addRequest(
						new common::CSendMessageRequest(
							common::CPayloadKind::SynchronizationGet
							, common::CSynchronizationGet( (int)common::CBlockKind::Header, m_currentBlock )
							, context< CSynchronizationAction >().getActionKey()
							, new CByKeyMediumFilter( _messageResult.m_pubKey ) ) );
			}
			else
			{
				return transit< CSynchronizingBlocks >();
			}

			assert( synchronizationHeader.m_blockIndex == m_currentBlock - 1 );
		}
		return discard_event();
	}

	boost::statechart::result react( common::CTimeEvent const & _timeEvent )
	{
		context< CSynchronizationAction >().forgetRequests();

		context< CSynchronizationAction >().addRequest(
				new common::CSendMessageRequest(
					common::CPayloadKind::SynchronizationGet
					, common::CSynchronizationGet( (int)common::CBlockKind::Header, m_currentBlock )
					, context< CSynchronizationAction >().getActionKey()
					, new CByKeyMediumFilter( context< CSynchronizationAction >().getPartnerKey() ) ) );

		context< CSynchronizationAction >().addRequest(
					new common::CTimeEventRequest(
						  SynchronisingWaitTime
						, new CMediumClassFilter( common::CMediumKinds::Time ) ) );

		return discard_event();
	}

	boost::statechart::result react( common::CAckEvent const & _promptAck )
	{
		return discard_event();
	}

	typedef boost::mpl::list<
	boost::statechart::custom_reaction< common::CTimeEvent >,
	boost::statechart::custom_reaction< common::CAckEvent >,
	boost::statechart::custom_reaction< common::CMessageResult >
	> reactions;

	unsigned int m_currentBlock;
};

struct CSynchronizedUninitialized : boost::statechart::state< CSynchronizedUninitialized, CSynchronizationAction >
{
	CSynchronizedUninitialized( my_context ctx ) : my_base( ctx )
	{
		LogPrintf("synchronize action: %p get synchronizied uninitiated \n", &context< CSynchronizationAction >() );

		context< CSynchronizationAction >().addRequest(
					new common::CTimeEventRequest(
						SynchronisingWaitTime
						, new CMediumClassFilter( common::CMediumKinds::Time ) ) );

		context< CSynchronizationAction >().addRequest(
					new common::CAckRequest(
						context< CSynchronizationAction >().getActionKey()
						, context< CSynchronizationAction >().getRequestKey()
						, new CByKeyMediumFilter( context< CSynchronizationAction >().getPartnerKey() ) ) );

		bool allowed = CReputationTracker::getInstance()->isSynchronizationAllowed( context< CSynchronizationAction >().getPartnerKey().GetID() );

		context< CSynchronizationAction >().addRequest(
				new common::CSendMessageRequest(
					common::CPayloadKind::Result
					, common::CResult( allowed ? 1 : 0 )
					, context< CSynchronizationAction >().getActionKey()
					, context< CSynchronizationAction >().getRequestKey()
					, new CByKeyMediumFilter( context< CSynchronizationAction >().getPartnerKey() ) ) );

		if ( !allowed )
			context< CSynchronizationAction >().setExit();
	}

	boost::statechart::result react( common::CMessageResult const & _messageResult )
	{
		common::CMessage orginalMessage;
		if ( !common::CommunicationProtocol::unwindMessage( _messageResult.m_message, orginalMessage, GetTime(), _messageResult.m_pubKey ) )
			assert( !"service it somehow" );

		if ( orginalMessage.m_header.m_payloadKind == common::CPayloadKind::InfoReq )
		{
			common::CInfoRequestData infoRequest;

			common::convertPayload( orginalMessage, infoRequest );

			if ( infoRequest.m_kind == common::CInfoKind::StorageInfoAsk )
			{
				context< CSynchronizationAction >().forgetRequests();

				context< CSynchronizationAction >().setRequestKey( _messageResult.m_message.m_header.m_id );

				context< CSynchronizationAction >().addRequest(
							new common::CAckRequest(
								context< CSynchronizationAction >().getActionKey()
								, _messageResult.m_message.m_header.m_id
								, new CByKeyMediumFilter( _messageResult.m_pubKey ) ) );

				context< CSynchronizationAction >().setRequestKey( _messageResult.m_message.m_header.m_id );

				return transit< CSynchronizedProvideCopy >();
			}
			else if ( infoRequest.m_kind == common::CInfoKind::BitcoinHeaderAsk )
			{
				context< CSynchronizationAction >().addRequest(
							new common::CAckRequest(
								context< CSynchronizationAction >().getActionKey()
								, _messageResult.m_message.m_header.m_id
								, new CByKeyMediumFilter( _messageResult.m_pubKey ) ) );

				CAutoFile file(OpenHeadFile(true), SER_DISK, CLIENT_VERSION);
				CBlockHeader header;
				file >> header;

				context< CSynchronizationAction >().addRequest(
						new common::CSendMessageRequest(
							common::CPayloadKind::SynchronizationBitcoinHeader
							, header
							, context< CSynchronizationAction >().getActionKey()
							, _messageResult.m_message.m_header.m_id
							, new CByKeyMediumFilter( _messageResult.m_pubKey ) ) );
			}
		}

		return discard_event();
	}

	boost::statechart::result react( common::CTimeEvent const & _timeEvent )
	{
		context< CSynchronizationAction >().setExit();
		return discard_event();
	}

	typedef boost::mpl::list<
	boost::statechart::custom_reaction< common::CMessageResult >,
	boost::statechart::custom_reaction< common::CTimeEvent >
	> reactions;
};

struct CSynchronizedProvideCopy : boost::statechart::state< CSynchronizedProvideCopy, CSynchronizationAction >
{
	CSynchronizedProvideCopy( my_context ctx ) : my_base( ctx ), m_copyRequestDone( false )
	{
		LogPrintf("synchronize action: %p provide copy \n", &context< CSynchronizationAction >() );

		context< CSynchronizationAction >().addRequest(
					new common::CTimeEventRequest(
						 100
						, new CMediumClassFilter( common::CMediumKinds::Time ) ) );
	}

	boost::statechart::result react( common::CTimeEvent const & _timeEvent )
	{
		if ( !m_copyRequestDone )
		{
			m_copyRequestDone = CCopyStorageHandler::getInstance()->createCopyRequest();
		}
		else
		{
			if ( CCopyStorageHandler::getInstance()->copyCreated() )
			{
				common::CSynchronizationInfo synchronizationInfo(
							 CCopyStorageHandler::getInstance()->getTimeStamp()
							 , CCopyStorageHandler::getInstance()->getSegmentHeaderSize()
							 , CCopyStorageHandler::getInstance()->getDiscBlockSize() );

				context< CSynchronizationAction >().addRequest(
						new common::CSendMessageRequest(
							common::CPayloadKind::SynchronizationInfo
							, synchronizationInfo
							, context< CSynchronizationAction >().getActionKey()
							, context< CSynchronizationAction >().getRequestKey()
							, new CByKeyMediumFilter( context< CSynchronizationAction >().getPartnerKey() ) ) );

				if ( !CCopyStorageHandler::getInstance()->getSegmentHeaderSize()
					 && !CCopyStorageHandler::getInstance()->getDiscBlockSize() )
				{

					//fix  this  case, I have  no  time  for  this  now
					context< CSynchronizationAction >().forgetRequests();
					context< CSynchronizationAction >().setExit();
				}
			}
		}

		context< CSynchronizationAction >().addRequest(
					new common::CTimeEventRequest(
						SynchronisingWaitTime
						, new CMediumClassFilter( common::CMediumKinds::Time ) ) );

		return discard_event();
	}

	boost::statechart::result react( common::CAckEvent const & )
	{
		context< CSynchronizationAction >().forgetRequests();

		if ( !CCopyStorageHandler::getInstance()->getSegmentHeaderSize()
			 && !CCopyStorageHandler::getInstance()->getDiscBlockSize() )
		{
			context< CSynchronizationAction >().onSynchronizedExit();
			return discard_event();
		}
		else
		{
			return transit< CSynchronized >();
		}
		return discard_event();
	}

	typedef boost::mpl::list<
	boost::statechart::custom_reaction< common::CTimeEvent >,
	boost::statechart::custom_reaction< common::CAckEvent >
	> reactions;

	bool m_copyRequestDone;
};

struct CSynchronized : boost::statechart::state< CSynchronized, CSynchronizationAction >
{
	CSynchronized( my_context ctx ) : my_base( ctx )
	{
		LogPrintf("synchronize action: %p synchronized \n", &context< CSynchronizationAction >() );
		m_storedBlocks = CCopyStorageHandler::getInstance()->getDiscBlockSize();

		m_storedHeaders = CCopyStorageHandler::getInstance()->getSegmentHeaderSize();

		assert( m_storedBlocks );

		m_diskBlock = new common::CDiskBlock;

		m_segmentHeader = new common::CSegmentHeader;

		m_exit = false;
	}

	void setBlock( unsigned int _blockNumber )
	{
		common::CSegmentFileStorage::getInstance()->getCopyBlock( _blockNumber, *m_diskBlock );

		context< CSynchronizationAction >().forgetRequests();

		common::CSynchronizationBlock synchronizationBlock( m_diskBlock, _blockNumber );

		context< CSynchronizationAction >().addRequest(
				new common::CSendMessageRequest(
					common::CPayloadKind::SynchronizationBlock
					, synchronizationBlock
					, context< CSynchronizationAction >().getActionKey()
					, m_id
					, new CByKeyMediumFilter( context< CSynchronizationAction >().getPartnerKey() ) ) );
	}

	void setHeaders( unsigned int _headerNumber )
	{
		common::CSegmentFileStorage::getInstance()->getCopySegmentHeader( _headerNumber, *m_segmentHeader );

		common::CSynchronizationSegmentHeader synchronizationSegmentHeader( m_segmentHeader, _headerNumber );

		context< CSynchronizationAction >().forgetRequests();

		context< CSynchronizationAction >().addRequest(
				new common::CSendMessageRequest(
					common::CPayloadKind::SynchronizationHeader
					, synchronizationSegmentHeader
					, context< CSynchronizationAction >().getActionKey()
					, m_id
					, new CByKeyMediumFilter( context< CSynchronizationAction >().getPartnerKey() ) ) );
	}

	boost::statechart::result react( common::CMessageResult const & _messageResult )
	{
		common::CMessage orginalMessage;
		if ( !common::CommunicationProtocol::unwindMessage( _messageResult.m_message, orginalMessage, GetTime(), _messageResult.m_pubKey ) )
			assert( !"service it somehow" );

		if ( orginalMessage.m_header.m_payloadKind == common::CPayloadKind::SynchronizationGet )
		{
			m_id = orginalMessage.m_header.m_id;

			common::CSynchronizationGet synchronizationGet;

			common::convertPayload( orginalMessage, synchronizationGet );

			context< CSynchronizationAction >().addRequest(
						new common::CAckRequest(
							context< CSynchronizationAction >().getActionKey()
							, m_id
							, new CByKeyMediumFilter( _messageResult.m_pubKey ) ) );

			if ( synchronizationGet.m_number < m_storedBlocks && synchronizationGet.m_kind == common::CBlockKind::Segment )
			{
				setBlock( synchronizationGet.m_number );
				m_exit = synchronizationGet.m_number == m_storedBlocks - 1;
			}
			else if ( synchronizationGet.m_number < m_storedHeaders && synchronizationGet.m_kind == common::CBlockKind::Header )
			{
				setHeaders( synchronizationGet.m_number );
			}
		}
		return discard_event();
	}

	boost::statechart::result react( common::CAckEvent const & )
	{
		context< CSynchronizationAction >().forgetRequests();
		if ( m_exit )
		{
			context< CSynchronizationAction >().onSynchronizedExit();
		}

		return discard_event();
	}

	~CSynchronized()
	{
		delete m_diskBlock;
		delete m_segmentHeader;
	}

	typedef boost::mpl::list<
	boost::statechart::custom_reaction< common::CMessageResult >,
	boost::statechart::custom_reaction< common::CAckEvent >
	> reactions;

	unsigned int m_storedBlocks;

	unsigned int m_storedHeaders;

	common::CDiskBlock * m_diskBlock;
	common::CSegmentHeader * m_segmentHeader;

	uint256 m_id;
	bool m_exit;
};

CSynchronizationAction::CSynchronizationAction( CPubKey const & _partnerKey )
	: m_partnerKey( _partnerKey )
{
	LogPrintf("synchronize action: %p synchronizing \n", this );
	initiate();
	process_event( CSwitchToSynchronizing() );

	setResult( common::CSynchronizationResult( 0 ) );
}

CSynchronizationAction::CSynchronizationAction( uint256 const & _id, uint256 const & _actionKey, CPubKey const & _partnerKey )
	: common::CScheduleAbleAction( _actionKey )
	, m_requestKey( _id )
	, m_partnerKey( _partnerKey )
{
	LogPrintf("synchronize action: %p synchronized \n", this );
	initiate();
	process_event( CSwitchToSynchronized() );

	setResult( common::CSynchronizationResult( 0 ) );
}

void
CSynchronizationAction::accept( common::CSetResponseVisitor & _visitor )
{
	_visitor.visit( *this );
}

void
CSynchronizationAction::clear()
{
}

bool
CSynchronizationAction::isRequestInitialized() const
{
	return !m_requests.empty();
}

void
CSynchronizationAction::onSynchronizedExit()
{
	setExit();

	CReputationTracker::getInstance()->setPresentNode( context< CSynchronizationAction >().getPartnerKey().GetID() );

	if ( CReputationTracker::getInstance()->isRegisteredTracker( context< CSynchronizationAction >().getPartnerKey().GetID() ) )
		CReputationTracker::getInstance()->setTrackerSynchronized( context< CSynchronizationAction >().getPartnerKey().GetID() );

	common::CRankingFullInfo rankingFullInfo(
				CReputationTracker::getInstance()->getAllyTrackers()
				, CReputationTracker::getInstance()->getAllyMonitors()
				, CReputationTracker::getInstance()->getTrackers()
				, CReputationTracker::getInstance()->getSynchronizedTrackers()
				, CReputationTracker::getInstance()->getMeasureReputationTime()
				, CReputationControlAction::getInstance()->getActionKey() );

	context< CSynchronizationAction >().addRequest(
				new common::CSendMessageRequest(
					common::CPayloadKind::FullRankingInfo
					, rankingFullInfo
					, context< CSynchronizationAction >().getActionKey()
					, new CMediumClassFilter( common::CMediumKinds::DimsNodes ) ) );

	CAddress address;
	if ( !CReputationTracker::getInstance()->getAddresFromKey( m_partnerKey.GetID(), address ) )
		assert( !"can't fail" );

	setResult( common::CSynchronizationResult( 1 ) );

	common::CActionHandler::getInstance()->executeAction( new CActivityControllerAction( m_partnerKey, address, CActivitySatatus::Active ) );
}

void
CSynchronizationAction::onExit()
{
	common::CSegmentFileStorage::getInstance()->resetState();
	common::CSegmentFileStorage::getInstance()->retriveState();

	if ( !CReputationTracker::getInstance()->isRegisteredTracker( m_partnerKey.GetID() ) )
		CReputationTracker::getInstance()->removeNodeFromSynch( m_partnerKey.GetID() );

	setResult( common::CSynchronizationResult( 1 ) );

	CReputationTracker::getInstance()->setPresentNode( context< CSynchronizationAction >().getPartnerKey().GetID() );
	setExit();
}

CSynchronizationAction::~CSynchronizationAction()
{
	LogPrintf("synchronization result %i \n", boost::get< common::CSynchronizationResult >( m_result ).m_result );
}

}

//struct CDiskBlock;
