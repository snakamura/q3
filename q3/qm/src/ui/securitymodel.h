/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __SECURITYMODEL_H__
#define __SECURITYMODEL_H__

#include <qm.h>

#include <vector>

namespace qm {

class SecurityModel;
	class DefaultSecurityModel;
class SecurityModelHandler;
class SecurityModelEvent;


/****************************************************************************
 *
 * SecurityModel
 *
 */

class SecurityModel
{
public:
	virtual ~SecurityModel();

public:
	virtual bool isDecryptVerify() = 0;
	virtual void setDecryptVerify(bool bDecryptVerify) = 0;
	virtual void addSecurityModelHandler(SecurityModelHandler* pHandler) = 0;
	virtual void removeSecurityModelHandler(SecurityModelHandler* pHandler) = 0;
};


/****************************************************************************
 *
 * DefaultSecurityModel
 *
 */

class DefaultSecurityModel : public SecurityModel
{
public:
	DefaultSecurityModel(bool bDecryptVerify);
	virtual ~DefaultSecurityModel();

public:
	virtual bool isDecryptVerify();
	virtual void setDecryptVerify(bool bDecryptVerify);
	virtual void addSecurityModelHandler(SecurityModelHandler* pHandler);
	virtual void removeSecurityModelHandler(SecurityModelHandler* pHandler);

private:
	void fireDecryptVerifyChanged();

private:
	DefaultSecurityModel(const DefaultSecurityModel&);
	DefaultSecurityModel& operator=(const DefaultSecurityModel&);

private:
	typedef std::vector<SecurityModelHandler*> HandlerList;

private:
	bool bDecryptVerify_;
	HandlerList listHandler_;
};


/****************************************************************************
 *
 * SecurityModelHandler
 *
 */

class SecurityModelHandler
{
public:
	virtual ~SecurityModelHandler();

public:
	virtual void decryptVerifyChanged(const SecurityModelEvent& event) = 0;
};


/****************************************************************************
 *
 * SecurityModelEvent
 *
 */

class SecurityModelEvent
{
public:
	SecurityModelEvent(SecurityModel* pSecurityModel);
	~SecurityModelEvent();

public:
	SecurityModel* getSecurityModel();

private:
	SecurityModelEvent(const SecurityModelEvent&);
	SecurityModelEvent& operator=(const SecurityModelEvent&);

private:
	SecurityModel* pSecurityModel_;
};

}

#endif // __SECURITYMODEL_H__
