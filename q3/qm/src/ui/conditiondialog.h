/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __CONDITIONDIALOG_H__
#define __CONDITIONDIALOG_H__

#include "dialogs.h"
#include "../model/condition.h"


namespace qm {

class ConditionsDialog;
class ConditionDialog;


/****************************************************************************
 *
 * ConditionsDialog
 *
 */

class ConditionsDialog : public AbstractListDialog<Condition, ConditionList::List>
{
public:
	explicit ConditionsDialog(const WCHAR* pwszCondition);
	virtual ~ConditionsDialog();

public:
	const WCHAR* getCondition() const;

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

protected:
	virtual LRESULT onOk();

protected:
	virtual qs::wstring_ptr getLabel(const Condition* p) const;
	virtual std::auto_ptr<Condition> create() const;
	virtual Condition* edit(Condition* p) const;
	virtual void updateState();

private:
	LRESULT onCustom();
	LRESULT onMacroChange();
	LRESULT onType();

private:
	void setConditionList(const ConditionList* pConditionList);
	qs::wstring_ptr getMacroFromConditions();

private:
	ConditionsDialog(const ConditionsDialog&);
	ConditionsDialog& operator=(const ConditionsDialog&);

private:
	qs::wstring_ptr wstrCondition_;
};


/****************************************************************************
 *
 * ConditionDialog
 *
 */

class ConditionDialog :
	public DefaultDialog,
	public qs::NotifyHandler
{
public:
	ConditionDialog();
	explicit ConditionDialog(const Condition* pCondition);
	virtual ~ConditionDialog();

public:
	std::auto_ptr<Condition> getCondition() const;

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

protected:
	virtual LRESULT onDestroy();
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

protected:
	virtual LRESULT onOk();

public:
	virtual LRESULT onNotify(NMHDR* pnmhdr,
							 bool* pbHandled);

private:
	LRESULT onConditionSelChange();
	LRESULT onEdit();
	LRESULT onArgumentsDblClk(NMHDR* pnmhdr,
							  bool* pbHandled);
	LRESULT onArgumentsItemChanged(NMHDR* pnmhdr,
								   bool* pbHandled);

private:
	void edit();
	Condition* getCurrentCondition() const;
	void updateArguments();
	void updateState();

private:
	ConditionDialog(const ConditionDialog&);
	ConditionDialog& operator=(const ConditionDialog&);

private:
	std::auto_ptr<Condition> pCondition_;
};


/****************************************************************************
 *
 * ConditionArgumentDialog
 *
 */

class ConditionArgumentDialog : public DefaultDialog
{
public:
	ConditionArgumentDialog(const WCHAR* pwszName,
							Condition::Type type,
							const WCHAR* pwszValue);
	virtual ~ConditionArgumentDialog();

public:
	const WCHAR* getValue() const;

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

protected:
	virtual LRESULT onOk();

private:
	LRESULT onChange();
	LRESULT onFieldSelChange();

private:
	UINT getId() const;
	void updateState();

private:
	ConditionArgumentDialog(const ConditionArgumentDialog&);
	ConditionArgumentDialog& operator=(const ConditionArgumentDialog&);

private:
	qs::wstring_ptr wstrName_;
	Condition::Type type_;
	qs::wstring_ptr wstrValue_;
};

}

#endif // __CONDITIONDIALOG_H__
