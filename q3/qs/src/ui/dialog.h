/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __DIALOG_H__
#define __DIALOG_H__

#include <qsdialog.h>
#include <qsinit.h>

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
	LRESULT notifyCommandHandlers(WORD wCode,
								  WORD wId) const;
	LRESULT notifyNotifyHandlers(NMHDR* pnmhdr,
								 bool* pbHandled) const;
	void notifyOwnerDrawHandlers(DRAWITEMSTRUCT* pDrawItem) const;
	void measureOwnerDrawHandlers(MEASUREITEMSTRUCT* pMeasureItem) const;
	INT_PTR dialogProc(UINT uMsg,
					   WPARAM wParam,
					   LPARAM lParam);
	void destroy();

public:
	static DialogMap* getDialogMap();
	
	static const ModelessList* getModelessList();
	static void addModelessDialog(DialogBase* pDialogBase);
	static void removeModelessDialog(DialogBase* pDialogBase);

private:
	DialogBase* pThis_;
	bool bDeleteThis_;
	DialogHandler* pDialogHandler_;
	bool bDeleteHandler_;
	CommandHandlerList listCommandHandler_;
	NotifyHandlerList listNotifyHandler_;
	OwnerDrawHandlerList listOwnerDrawHandler_;
	InitThread* pInitThread_;

private:
	static DialogMap* pMap__;
	static ThreadLocal* pModelessList__;
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

#endif // __DIALOG_H__
