/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __SECURITYMODEL_H__
#define __SECURITYMODEL_H__

#include <qm.h>
#include <qmsecurity.h>

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
	virtual unsigned int getSecurityMode() const = 0;
	virtual void setSecurityMode(SecurityMode mode,
								 bool b) = 0;
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
	explicit DefaultSecurityModel(unsigned int nMode);
	virtual ~DefaultSecurityModel();

public:
	virtual unsigned int getSecurityMode() const;
	virtual void setSecurityMode(SecurityMode mode,
								 bool b);
	virtual void addSecurityModelHandler(SecurityModelHandler* pHandler);
	virtual void removeSecurityModelHandler(SecurityModelHandler* pHandler);

private:
	void fireSecurityModeChanged();

private:
	DefaultSecurityModel(const DefaultSecurityModel&);
	DefaultSecurityModel& operator=(const DefaultSecurityModel&);

private:
	typedef std::vector<SecurityModelHandler*> HandlerList;

private:
	unsigned int nMode_;
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
	virtual void securityModeChanged(const SecurityModelEvent& event) = 0;
};


/****************************************************************************
 *
 * SecurityModelEvent
 *
 */

class SecurityModelEvent
{
public:
	explicit SecurityModelEvent(SecurityModel* pSecurityModel);
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
