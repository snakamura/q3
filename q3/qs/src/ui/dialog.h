/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __DIALOG_H__
#define __DIALOG_H__

#include <qsdialog.h>
#include <qsinit.h>

#ifdef _WIN32_WCE_PSPC
#	include <dra.h>
#endif

#include "window.h"


/****************************************************************************
 *
 * DialogBaseImpl
 *
 */

class qs::DialogBaseImpl
{
public:
	typedef std::vector<CommandHandler*> CommandHandlerList;
	typedef std::vector<NotifyHandler*> NotifyHandlerList;
	typedef std::vector<OwnerDrawHandler*> OwnerDrawHandlerList;
	typedef ControllerMap<DialogBase> DialogMap;
	typedef std::vector<DialogBase*> ModelessList;

public:
	INT_PTR dialogProc(UINT uMsg,
					   WPARAM wParam,
					   LPARAM lParam);
	void destroy();

private:
	LRESULT notifyCommandHandlers(WORD wCode,
								  WORD wId) const;
	LRESULT notifyNotifyHandlers(NMHDR* pnmhdr,
								 bool* pbHandled) const;
	void notifyOwnerDrawHandlers(DRAWITEMSTRUCT* pDrawItem) const;
	void measureOwnerDrawHandlers(MEASUREITEMSTRUCT* pMeasureItem) const;
#ifdef _WIN32_WCE_PSPC
	void layout(bool bNotify);
#endif

public:
	static DialogMap* getDialogMap();
	
	static const ModelessList* getModelessList();
	static void addModelessDialog(DialogBase* pDialogBase);
	static void removeModelessDialog(DialogBase* pDialogBase);

private:
	DialogBase* pThis_;
#ifdef _WIN32_WCE_PSPC
	HINSTANCE hInstResource_;
	UINT nIdPortrait_;
	UINT nIdLandscape_;
#endif
	bool bDeleteThis_;
	DialogHandler* pDialogHandler_;
	bool bDeleteHandler_;
	CommandHandlerList listCommandHandler_;
	NotifyHandlerList listNotifyHandler_;
	OwnerDrawHandlerList listOwnerDrawHandler_;
	InitThread* pInitThread_;
#ifdef _WIN32_WCE_PSPC
	DRA::DisplayMode displayMode_;
#endif

private:
	static DialogMap* pMap__;
	static ThreadLocal<ModelessList*>* pModelessList__;
	static class InitializerImpl : public Initializer
	{
	public:
		InitializerImpl();
		virtual ~InitializerImpl();
	
	public:
		virtual bool init();
		virtual void term();
		virtual bool initThread();
		virtual void termThread();
	} init__;

friend class InitializerImpl;
friend class DialogBase;
};


#ifdef _WIN32_WCE

namespace qs {

/****************************************************************************
 *
 * FolderNameDialog
 *
 */

class FolderNameDialog : public DefaultDialog
{
public:
	FolderNameDialog();
	virtual ~FolderNameDialog();

public:
	const WCHAR* getName() const;

protected:
	virtual LRESULT onOk();

private:
	FolderNameDialog(const FolderNameDialog&);
	FolderNameDialog& operator=(const FolderNameDialog&);

private:
	wstring_ptr wstrName_;
};

}

#endif

#endif // __DIALOG_H__
