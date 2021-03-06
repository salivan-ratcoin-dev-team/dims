// Copyright (c) 2014-2015 DiMS dev-team
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef APP_CLIENT_H
#define APP_CLIENT_H

#include <QLocalSocket>

class QApplication;
class QMainWindow;
namespace dims
{

class CAppClient : public QObject
{
	Q_OBJECT
public:
	CAppClient();
	~CAppClient();

	void connectServer();

	bool isOpen();

	void send( QByteArray const & _message );

	bool checkApp( QApplication * _application, QMainWindow *_window );

private slots:
	void what(QLocalSocket::LocalSocketError);
	void readSocket();
	void discardSocket();
private:
	QLocalSocket* conection;
};


}

#endif // APP_CLIENT_H
