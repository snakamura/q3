/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QSACTION_H__
#define __QSACTION_H__

#include <qs.h>
#include <qsstring.h>

#include <vector>


namespace qs {

class Action;
class ActionEvent;
	class AbstractAction;
class ActionMap;


/****************************************************************************
 *
 * Action
 *
 */

class QSEXPORTCLASS Action
{
public:
	virtual ~Action();

public:
	virtual void invoke(const ActionEvent& event) = 0;
	virtual bool isEnabled(const ActionEvent& event) = 0;
	virtual bool isChecked(const ActionEvent& event) = 0;
	virtual wstring_ptr getText(const ActionEvent& event) = 0;
};


/****************************************************************************
 *
 * ActionEvent
 *
 */

class QSEXPORTCLASS ActionEvent
{
public:
	enum Modifier {
		MODIFIER_SHIFT	= 0x01,
		MODIFIER_CTRL	= 0x02,
		MODIFIER_ALT	= 0x04
	};

public:
	ActionEvent(unsigned int nId,
				unsigned int nModifier);
	ActionEvent(unsigned int nId,
				unsigned int nModifier,
				void* pParam);
	~ActionEvent();

public:
	unsigned int getId() const;
	unsigned int getModifier() const;
	void* getParam() const;

private:
	ActionEvent(const ActionEvent&);
	ActionEvent& operator=(const ActionEvent&);

private:
	unsigned int nId_;
	unsigned int nModifier_;
	void* pParam_;
};


/****************************************************************************
 *
 * AbstractAction
 *
 */

class QSEXPORTCLASS AbstractAction : public Action
{
public:
	AbstractAction();
	virtual ~AbstractAction();

public:
	virtual bool isEnabled(const ActionEvent& event);
	virtual bool isChecked(const ActionEvent& event);
	virtual wstring_ptr getText(const ActionEvent& event);

private:
	AbstractAction(const AbstractAction&);
	AbstractAction& operator=(const AbstractAction&);
};


/****************************************************************************
 *
 * ActionMap
 *
 */

class QSEXPORTCLASS ActionMap
{
public:
	ActionMap();
	~ActionMap();

public:
	Action* getAction(unsigned int nId) const;
	void addAction(unsigned int nId,
				   std::auto_ptr<Action> pAction);
	void addAction(unsigned int nIdFrom,
				   unsigned int nIdTo,
				   std::auto_ptr<Action> pAction);

private:
	ActionMap(const ActionMap&);
	ActionMap& operator=(const ActionMap&);

private:
	struct ActionMapImpl* pImpl_;
};


/****************************************************************************
 *
 * ActionItem
 *
 */

struct QSEXPORTCLASS ActionItem
{
	const WCHAR* pwszAction_;
	UINT nId_;
};

}

#endif // __QSACTION_H__
