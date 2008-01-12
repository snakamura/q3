/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
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

class MessageContext;


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
	virtual Folder* getCurrentFolder() const = 0;
	virtual MessageContext* getCurrentMessage() const = 0;
	virtual void setMessage(std::auto_ptr<MessageContext> pContext) = 0;
	virtual void clearMessage() = 0;
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
	AbstractMessageModel(MessageViewMode* pDefaultMessageViewMode);

public:
	virtual ~AbstractMessageModel();

public:
	virtual Account* getCurrentAccount() const;
	virtual Folder* getCurrentFolder() const;
	virtual MessageContext* getCurrentMessage() const;
	virtual void setMessage(std::auto_ptr<MessageContext> pContext);
	virtual void clearMessage();
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
	virtual void updateCurrentMessage() = 0;
	virtual MessageViewMode* getMessageViewMode(ViewModel* pViewModel) const = 0;

protected:
	void setCurrentAccount(Account* pAccount);
	void asyncExec(qs::Runnable* pRunnable);

protected:
	void fireMessageChanged(MessageContext* pContext) const;
	void fireUpdateRestoreInfo(ViewModel::RestoreInfo* pRestoreInfo) const;
	void fireApplyRestoreInfo(ViewModel::RestoreInfo* pRestoreInfo) const;

private:
	AbstractMessageModel(const AbstractMessageModel&);
	AbstractMessageModel& operator=(const AbstractMessageModel&);


private:
	typedef std::vector<MessageModelHandler*> HandlerList;

private:
	MessageViewMode* pDefaultMessageViewMode_;
	Account* pAccount_;
	std::auto_ptr<MessageContext> pContext_;
	HandlerList listHandler_;
	ViewModel* pViewModel_;
	qs::Synchronizer* pSynchronizer_;
};


/****************************************************************************
 *
 * MessageMessageModel
 *
 */

class MessageMessageModel : public AbstractMessageModel
{
public:
	explicit MessageMessageModel(MessageViewMode* pDefaultMessageViewMode);
	virtual ~MessageMessageModel();

public:
	virtual void reloadProfiles();

protected:
	virtual void updateCurrentMessage();
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
						bool bConnectToViewModel,
						MessageViewMode* pDefaultMessageViewMode);
	virtual ~PreviewMessageModel();

public:
	virtual void reloadProfiles();

public:
	void updateToViewModel(bool bClearIfChanged);
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
	virtual void updateCurrentMessage();
	virtual MessageViewMode* getMessageViewMode(ViewModel* pViewModel) const;

private:
	void killTimer();

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
	bool bUpdateAlways_;
	std::auto_ptr<qs::Timer> pTimer_;
	bool bTimer_;
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
					  MessageContext* pContext);
	~MessageModelEvent();

public:
	const MessageModel* getMessageModel() const;
	MessageContext* getMessageContext() const;

public:
	MessageModelEvent(const MessageModelEvent&);
	MessageModelEvent& operator=(const MessageModelEvent&);

private:
	const MessageModel* pModel_;
	MessageContext* pContext_;
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
