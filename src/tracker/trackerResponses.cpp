#include "trackerResponses.h"

namespace tracker
{

CMainRequestType::Enum const CDummy::m_requestType = CMainRequestType::None;

CMainRequestType::Enum const CAvailableCoins::m_requestType = CMainRequestType::BalanceInfoReq;

CAvailableCoins::CAvailableCoins( std::vector< CCoins > const & _availableCoins, uint256 const & _hash )
	: m_hash(_hash)
	, m_availableCoins( _availableCoins )
{
}


}