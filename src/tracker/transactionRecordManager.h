#ifndef TRANSACTION_RECORD_MANAGER_H
#define TRANSACTION_RECORD_MANAGER_H


class CTransaction;

namespace self
{


class CTransactionRecordManager
{
public:
	CTransactionRecordManager();

	~CTransactionRecordManager();

	void addCoinbaseTransaction( CTransaction const & _tx );

	bool checkIfCoinsAvailable( CTransaction const & _tx ) const;

	void validateTransaction( CTransaction const & _tx ) const;
	//time  stamp or  something needed
	//
	//create  transaction view


	void handleTransactionBundle();
private:
	void synchronize();
	void askForTokens();
private:
	// mutex
	CCoinsViewCache * m_coinsViewCache;

	// mutex
	CTxMemPool * m_memPool;

//	transaction history  section
};


}

#endif // TRANSACTION_RECORD_MANAGER_H
