/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
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
	class AbstractAction;
class ActionEvent;
class ActionParam;
class ActionMap;
class ActionParamMap;
struct ActionItem;


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
				const ActionParam* pParam);
	~ActionEvent();

public:
	unsigned int getId() const;
	unsigned int getModifier() const;
	const ActionParam* getParam() const;

public:
	static unsigned int getSystemModifiers();

private:
	ActionEvent(const ActionEvent&);
	ActionEvent& operator=(const ActionEvent&);

private:
	unsigned int nId_;
	unsigned int nModifier_;
	const ActionParam* pParam_;
};


/****************************************************************************
 *
 * ActionParam
 *
 */

#pragma warning(push)
#pragma warning(disable:4251)

class QSEXPORTCLASS ActionParam
{
public:
	typedef std::vector<WSTRING> ValueList;

public:
	explicit ActionParam(unsigned int nBaseId);
	ActionParam(unsigned int nBaseId,
				const WCHAR* pwszValue);
	ActionParam(unsigned int nBaseId,
				const WCHAR* pwszValue,
				bool bParse);
	ActionParam(unsigned int nBaseId,
				const WCHAR** ppwszValue,
				size_t nCount);
	~ActionParam();

public:
	unsigned int getBaseId() const;
	size_t getCount() const;
	const WCHAR* getValue(size_t n) const;

public:
	unsigned int addRef();
	unsigned int release();

public:
	static void parse(const WCHAR* pwszValue,
					  ValueList* pList);

private:
	ActionParam(const ActionParam&);
	ActionParam& operator=(const ActionParam&);

private:
	unsigned int nBaseId_;
	ValueList listValue_;
	unsigned int nRef_;
};

bool operator==(const ActionParam& lhs,
				const ActionParam& rhs);
bool operator!=(const ActionParam& lhs,
				const ActionParam& rhs);

#pragma warning(pop)


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
	enum {
		ID_MIN = 40000,
		ID_MAX = 60000
	};

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
 * ActionParamMap
 *
 */

class QSEXPORTCLASS ActionParamMap
{
public:
	ActionParamMap();
	~ActionParamMap();

public:
	const ActionParam* getActionParam(unsigned int nId) const;
	unsigned int addActionParam(unsigned int nMaxParamCount,
								std::auto_ptr<ActionParam> pParam);
	void removeActionParam(unsigned int nId);

private:
	ActionParamMap(const ActionParamMap&);
	ActionParamMap& operator=(const ActionParamMap&);

private:
	struct ActionParamMapImpl* pImpl_;
};


/****************************************************************************
 *
 * ActionItem
 *
 */

struct QSEXPORTCLASS ActionItem
{
	enum Flag {
		FLAG_MENU			= 0x01,
		FLAG_TOOLBAR		= 0x02,
		FLAG_ACCELERATOR	= 0x04
	};
	
	const WCHAR* pwszAction_;
	unsigned int nId_;
	unsigned int nMaxParamCount_;
	unsigned int nFlags_;
};

}

#endif // __QSACTION_H__
