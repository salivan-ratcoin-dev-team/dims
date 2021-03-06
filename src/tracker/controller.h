// Copyright (c) 2014-2015 DiMS dev-team
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TRACKER_CONTROLLER_H
#define TRACKER_CONTROLLER_H

#include <boost/statechart/state_machine.hpp>

#include "key.h"

#include "common/events.h"

namespace tracker
{
struct CInitialSynchronization;
struct CStandAlone;
struct CMonitorData
{
	CMonitorData( bool _isAdmitted = false, CPubKey _monitorPublicKey = CPubKey(), bool _allowAdmission = true, double _accepableRatio = 0 ):m_isAdmitted( _isAdmitted ), m_monitorPublicKey( _monitorPublicKey ), m_allowAdmission( _allowAdmission ), m_accepableRatio( _accepableRatio ){}
	bool m_isAdmitted;
	CPubKey m_monitorPublicKey;
	bool m_allowAdmission;
	double m_accepableRatio;// price / period
};

class CController : public boost::statechart::state_machine< CController, CStandAlone >
{
public:
	static CController* getInstance();

	unsigned int getPrice() const;
	void setPrice( unsigned int _price );

	bool isConnected() const;
	bool setConnected( bool _connected );

	// monitor related
	CMonitorData & acquireMonitorData()
	{
		return m_monitorData;
	}

	void setStatusMessage( std::string const & _statusMessage )
	{
		m_statusMessage = _statusMessage;
	}

	std::string getStatusMessage()
	{
		return m_statusMessage;
	}

	void setAutoRenewRegistration( bool _renewal )
	{
		m_autoRegistrationRenewal = _renewal;
	}

	bool autoRenewRegistration() const
	{
		return m_autoRegistrationRenewal;
	}

	void setRegistrationData( common::CRegistrationData const & _registrationData )
	{
		m_registrationData = _registrationData;
	}

	common::CRegistrationData getRegistrationData() const
	{
		return m_registrationData;
	}

	void setNetworkData( common::CNetworkRecognizedData _networkData )
	{
		m_networkData = _networkData;
	}

	common::CNetworkRecognizedData const & getNetworkData() const
	{
		return m_networkData;
	}
private:
	CController();

private:
	static CController * ms_instance;

	unsigned int m_price;

	bool m_connected;

	CMonitorData m_monitorData;

	float const m_deviation;

	std::string m_statusMessage;

	bool m_autoRegistrationRenewal;

	common::CRegistrationData m_registrationData;

	common::CNetworkRecognizedData m_networkData;
};


}

#endif // TRACKER_CONTROLLER_H
