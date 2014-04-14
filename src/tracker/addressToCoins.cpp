// Copyright (c) 2014 Ratcoin dev-team
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "addressToCoins.h"

#include <boost/foreach.hpp>
//#include <stdint.h>

namespace tracker
{

template <class K, class T, char _prefix >
class CBatchWrite
{
public:
	void insert( K _key, T _object )
	{
		m_batch.Write( std::make_pair('a', _key), _object );
	}
	CLevelDBBatch & getBatch(){ return m_batch; }
private:
	CLevelDBBatch m_batch;
};



CAddressToCoinsDatabase::CAddressToCoinsDatabase(size_t nCacheSize, bool fMemory, bool fWipe)
: db(GetDataDir() / "addressTocoins",
	nCacheSize,
	fMemory,
	fWipe)
{
}

bool CAddressToCoinsDatabase::getCoinsAmount(const uint160 &_keyId, uint64_t & _amount )
{
	return db.Read( std::make_pair('a', _keyId), _amount );
}

bool CAddressToCoinsDatabase::setCoinsAmount(uint160 const &_keyId, uint64_t const _amount)
{
	return db.Write( std::make_pair('a', _keyId), _amount );
}

bool CAddressToCoinsDatabase::getCoin(uint256 const &_keyId, uint256 &coins)
{
	return db.Read( std::make_pair('c', _keyId), coins );
}

bool CAddressToCoinsDatabase::setCoin(uint256 const &_keyId, uint256 const &coins)
{
	return db.Write( std::make_pair('c', _keyId), coins );
}

bool CAddressToCoinsDatabase::haveCoin(uint256 const &_keyId)
{
	return db.Exists( std::make_pair( 'c', _keyId ) );
}

CAddressToCoins::CAddressToCoins(size_t _cacheSize)
:CAddressToCoinsDatabase(_cacheSize)
{

}

bool
CAddressToCoins::getCoins( uint160 const &_keyId, std::vector< uint256 > &_coins )
{
	uint256 key ;
	key =_keyId;
	uint64_t amount;
	if ( !getCoinsAmount(_keyId, amount) )
		return false;

	while( amount-- )
	{
		uint256 coin;
		if ( haveCoin(++key) )
		{
			getCoin( key, coin );
			_coins.push_back( coin );
		}
	}
	return true;
}

bool
CAddressToCoins::setCoins( uint160 const &_keyId, uint256 const & _coin )
{
	uint64_t amount;
	if ( !getCoinsAmount(_keyId, amount) )
		amount = 0;

	if ( !setCoinsAmount(_keyId, amount ) )
		return false;

	uint256 key;
	key = _keyId;
	if ( !setCoin(++key, _coin ) )
	{
		setCoinsAmount(_keyId, --amount );
		return false;
	}
	return true;
}

bool
CAddressToCoins::batchWrite( std::multimap<uint160,uint256> const &mapCoins )
{
	CBatchWrite< uint160, uint64_t, 'a' > amountBatch;
	CBatchWrite< uint256, uint256, 'c' > coinsBatch;
	std::multimap<uint160,uint256>::const_iterator iterator = mapCoins.begin(), end;

	while( iterator != mapCoins.end() )
	{
		end = mapCoins.upper_bound( iterator->first );
		uint64_t amount = 0;
		uint256 key;
		key = iterator->first;
		while( iterator != end )
		{
			amount++;
			coinsBatch.insert( ++key, iterator->second );
		}
		amountBatch.insert( iterator->first, amount );

		iterator = end;
	}
	CAddressToCoinsDatabase::batchWrite( amountBatch );
	CAddressToCoinsDatabase::batchWrite( coinsBatch );
}

bool CAddressToCoinsViewCache::getCoins( uint160 const &_keyId, std::vector< uint256 > &_coins )
{
	std::map<uint160,uint256>::iterator it = fetchCoins(_keyId);
	assert(it != cacheCoins.end());
	while(it != cacheCoins.end() && it->first == _keyId)
	{
		_coins.push_back( it->second );
	}
	
	return !_coins.empty();
}

bool CAddressToCoinsViewCache::setCoins( uint160 const &_keyId, uint256 const & _coin )
{
	cacheCoins.insert( std::make_pair( _keyId, _coin ) );
	return true;
}

bool CAddressToCoinsViewCache::haveCoins(const uint160 &txid)
{
	return fetchCoins(txid) != cacheCoins.end();
}

std::map<uint160,uint256>::iterator
CAddressToCoinsViewCache::fetchCoins(const uint160 &_keyId)
{
	std::map<uint160,uint256>::iterator it = cacheCoins.lower_bound(_keyId);
	if (it != cacheCoins.end() && it->first == _keyId)
		return it;

	std::vector< uint256 > tmp;

	if ( !m_addressToCoins.getCoins(_keyId,tmp) )
		return cacheCoins.end();
	
	BOOST_FOREACH( uint256 & coin, tmp )
	{
		cacheCoins.insert(it, std::make_pair(_keyId, coin));
	}
	return fetchCoins(_keyId);
}

bool 
CAddressToCoinsViewCache::flush()
{
	bool ok = m_addressToCoins.batchWrite(cacheCoins);
	if (ok)
		cacheCoins.clear();
	return ok;
}

}