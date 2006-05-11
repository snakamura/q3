/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __MESSAGEMODEL_H__
#define __MESSAGEMODEL_H__

#include <qm.h>

#include <qs.h>
#include <qstimer.h>
#include <qswindow.h>

#include <vector>

#include "messageviewmode.h"
#include "viewmodel.h"


namespace qm {

class MessageModel;
	class AbstractMessageModel;
		class MessageMessageModel;
			class PreviewMessageModel;
class MessageModelHandler;
class MessageModelEvent;
class MessageModelRestoreEvent;

class MessageHolder;
class Message;


/****************************************************************************
 *
 * MessageModel
 *
 */

class MessageModel
{
public:
	virtual ~MessageModel();

public:
	virtual Account* getCurrentAccount() const = 0;
	virtual void setCurrentAccount(Account* pAccount) = 0;
	virtual Folder* getCurrentFolder() const = 0;
	virtual MessagePtr getCurrentMessage() const = 0;
	virtual void setMessage(MessageHolder* pmh) = 0;
	virtual void reloadProfiles() = 0;
	virtual void addMessageModelHandler(MessageModelHandler* pHandler) = 0;
	virtual void removeMessageModelHandler(MessageModelHandler* pHandler) = 0;
};


/****************************************************************************
 *
 * AbstractMessageModel
 *
 */

class AbstractMessageModel :
	public MessageModel,
	public ViewModelHolder,
	public AbstractMessageViewModeHolder,
	public DefaultViewModelHandler,
	public DefaultAccountHandler,
	public DefaultMessageHolderHandler
{
protected:
	AbstractMessageModel();

public:
	virtual ~AbstractMessageModel();

public:
	virtual Account* getCurrentAccount() const;
	virtual void setCurrentAccount(Account* pAccount);
	virtual Folder* getCurrentFolder() const;
	virtual MessagePtr getCurrentMessage() const;
	virtual void setMessage(MessageHolder* pmh);
	virtual void addMessageModelHandler(MessageModelHandler* pHandler);
	virtual void removeMessageModelHandler(MessageModelHandler* pHandler);

public:
	virtual ViewModel* getViewModel() const;
	virtual void setViewModel(ViewModel* pViewModel);

public:
	virtual MessageViewMode* getMessageViewMode();

public:
	virtual void itemRemoved(const ViewModelEvent& event);
	virtual void destroyed(const ViewModelEvent& event);

public:
	virtual void accountDestroyed(const AccountEvent& event);

public:
	virtual void messageHolderKeysChanged(const MessageHolderEvent& event);

protected:
	virtual MessageViewMode* getMessageViewMode(ViewModel* pViewModel) const = 0;

protected:
	void fireMessageChanged(MessageHolder* pmh) const;
	void fireUpdateRestoreInfo(ViewModel::RestoreInfo* pRestoreInfo) const;
	void fireApplyRestoreInfo(ViewModel::RestoreInfo* pRestoreInfo) const;

private:
	AbstractMessageModel(const AbstractMessageModel&);
	AbstractMessageModel& operator=(const AbstractMessageModel&);


private:
	typedef std::vector<MessageModelHandler*> HandlerList;

private:
	Account* pAccount_;
	MessagePtr ptr_;
	HandlerList listHandler_;
	ViewModel* pViewModel_;
};


/****************************************************************************
 *
 * MessageMessageModel
 *
 */

class MessageMessageModel : public AbstractMessageModel
{
public:
	MessageMessageModel();
	virtual ~MessageMessageModel();

public:
	virtual void reloadProfiles();

protected:
	virtual MessageViewMode* getMessageViewMode(ViewModel* pViewModel) const;

private:
	MessageMessageModel(const MessageMessageModel&);
	MessageMessageModel& operator=(const MessageMessageModel&);
};


/****************************************************************************
 *
 * PreviewMessageModel
 *
 */

class PreviewMessageModel :
	public AbstractMessageModel,
	public ViewModelManagerHandler,
	public qs::TimerHandler
{
public:
	PreviewMessageModel(ViewModelManager* pViewModelManager,
						qs::Profile* pProfile,
						bool bConnectToViewModel);
	virtual ~PreviewMessageModel();

public:
	virtual void reloadProfiles();

public:
	void updateToViewModel();
	void connectToViewModel();
	void disconnectFromViewModel();
	bool isConnectedToViewModel() const;
	void save() const;

public:
	virtual void itemStateChanged(const ViewModelEvent& event);
	virtual void updated(const ViewModelEvent& event);

public:
	virtual void viewModelSelected(const ViewModelManagerEvent& event);

public:
	virtual void timerTimeout(qs::Timer::Id nId);

protected:
	virtual MessageViewMode* getMessageViewMode(ViewModel* pViewModel) const;

private:
	void updateToViewModel(bool bClearMessage);

private:
	PreviewMessageModel(const PreviewMessageModel&);
	PreviewMessageModel& operator=(const PreviewMessageModel&);

private:
	enum {
		TIMER_ITEMSTATECHANGED	= 10
	};

private:
	ViewModelManager* pViewModelManager_;
	qs::Profile* pProfile_;
	unsigned int nDelay_;
	std::auto_ptr<qs::Timer> pTimer_;
	qs::Timer::Id nTimerId_;
	bool bConnectedToViewModel_;
};


/****************************************************************************
 *
 * MessageModelHandler
 *
 */

class MessageModelHandler
{
public:
	virtual ~MessageModelHandler();

public:
	virtual void messageChanged(const MessageModelEvent& event) = 0;
	virtual void updateRestoreInfo(const MessageModelRestoreEvent& event) = 0;
	virtual void applyRestoreInfo(const MessageModelRestoreEvent& event) = 0;
};


/****************************************************************************
 *
 * MessageModelEvent
 *
 */

class MessageModelEvent
{
public:
	MessageModelEvent(const MessageModel* pModel,
					  MessageHolder* pmh);
	~MessageModelEvent();

public:
	const MessageModel* getMessageModel() const;
	MessageHolder* getMessageHolder() const;

public:
	MessageModelEvent(const MessageModelEvent&);
	MessageModelEvent& operator=(const MessageModelEvent&);

private:
	const MessageModel* pModel_;
	MessageHolder* pmh_;
};


/****************************************************************************
 *
 * MessageModelRestoreEvent
 *
 */

class MessageModelRestoreEvent
{
public:
	MessageModelRestoreEvent(const MessageModel* pModel,
							 ViewModel::RestoreInfo* pRestoreInfo);
	~MessageModelRestoreEvent();

public:
	const MessageModel* getMessageModel() const;
	ViewModel::RestoreInfo* getRestoreInfo() const;

private:
	MessageModelRestoreEvent(const MessageModelRestoreEvent&);
	MessageModelRestoreEvent& operator=(const MessageModelRestoreEvent&);

private:
	const MessageModel* pModel_;
	ViewModel::RestoreInfo* pRestoreInfo_;
};

}

#endif // __MESSAGEMODEL_H__
