/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qm.h>

#include <qs.h>
#include <qstimer.h>
#include <qswindow.h>

#include <vector>

#include "messageviewmode.h"
#include "viewmodel.h"

#ifndef __MESSAGEMODEL_H__
#define __MESSAGEMODEL_H__


namespace qm {

class MessageModel;
	class AbstractMessageModel;
		class MessageMessageModel;
			class PreviewMessageModel;
class MessageModelHandler;
class MessageModelEvent;

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
	virtual MessagePtr getCurrentMessage() const = 0;
	virtual void setMessage(MessageHolder* pmh) = 0;
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
	public DefaultAccountHandler
{
protected:
	AbstractMessageModel();

public:
	virtual ~AbstractMessageModel();

public:
	virtual Account* getCurrentAccount() const;
	virtual void setCurrentAccount(Account* pAccount);
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

private:
	void fireMessageChanged(MessageHolder* pmh) const;

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
						bool bConnectToViewModel);
	virtual ~PreviewMessageModel();

public:
	void updateToViewModel();
	void connectToViewModel();
	void disconnectFromViewModel();
	bool isConnectedToViewModel() const;

public:
	virtual void itemStateChanged(const ViewModelEvent& event);

public:
	virtual void viewModelSelected(const ViewModelManagerEvent& event);

public:
	virtual void timerTimeout(unsigned int nId);

private:
	PreviewMessageModel(const PreviewMessageModel&);
	PreviewMessageModel& operator=(const PreviewMessageModel&);

private:
	enum {
		TIMER_ITEMSTATECHANGED	= 10,
		TIMEOUT					= 300
	};

private:
	ViewModelManager* pViewModelManager_;
	std::auto_ptr<qs::Timer> pTimer_;
	unsigned int nTimerId_;
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

}

#endif // __MESSAGEMODEL_H__
