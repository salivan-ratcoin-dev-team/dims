// Copyright (c) 2014 Ratcoin dev-team
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef REQUEST_VISITOR_H
#define REQUEST_VISITOR_H

namespace common
{

class CRequestVisitor
{
public:
	virtual void visit( CRequest* _request ){};
};


}

#endif // REQUEST_VISITOR_H