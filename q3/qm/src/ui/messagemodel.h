/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qm.h>

#include <qs.h>
#include <qstimer.h>
#include <qswindow.h>

#include <vector>

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
	virtual qs::QSTATUS setCurrentAccount(Account* pAccount) = 0;
	virtual MessagePtr getCurrentMessage() const = 0;
	virtual qs::QSTATUS setMessage(MessageHolder* pmh) = 0;
	virtual qs::QSTATUS addMessageModelHandler(MessageModelHandler* pHandler) = 0;
	virtual qs::QSTATUS removeMessageModelHandler(MessageModelHandler* pHandler) = 0;
};


/****************************************************************************
 *
 * AbstractMessageModel
 *
 */

class AbstractMessageModel :
	public MessageModel,
	public ViewModelHolder,
	public DefaultViewModelHandler,
	public DefaultAccountHandler
{
protected:
	AbstractMessageModel(qs::QSTATUS* pstatus);

public:
	virtual ~AbstractMessageModel();

public:
	virtual Account* getCurrentAccount() const;
	virtual qs::QSTATUS setCurrentAccount(Account* pAccount);
	virtual MessagePtr getCurrentMessage() const;
	virtual qs::QSTATUS setMessage(MessageHolder* pmh);
	virtual qs::QSTATUS addMessageModelHandler(MessageModelHandler* pHandler);
	virtual qs::QSTATUS removeMessageModelHandler(MessageModelHandler* pHandler);

public:
	virtual ViewModel* getViewModel() const;
	virtual qs::QSTATUS setViewModel(ViewModel* pViewModel);

public:
	virtual qs::QSTATUS itemRemoved(const ViewModelEvent& event);
	virtual qs::QSTATUS destroyed(const ViewModelEvent& event);

public:
	virtual qs::QSTATUS accountDestroyed(const AccountEvent& event);

private:
	qs::QSTATUS fireMessageChanged(MessageHolder* pmh) const;

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
	MessageMessageModel(qs::QSTATUS* pstatus);
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
		bool bConnectToViewModel, qs::QSTATUS* pstatus);
	virtual ~PreviewMessageModel();

public:
	qs::QSTATUS updateToViewModel();
	qs::QSTATUS connectToViewModel();
	qs::QSTATUS disconnectFromViewModel();
	bool isConnectedToViewModel() const;

public:
	virtual qs::QSTATUS itemStateChanged(const ViewModelEvent& event);

public:
	virtual qs::QSTATUS viewModelSelected(const ViewModelManagerEvent& event);

public:
	virtual qs::QSTATUS timerTimeout(unsigned int nId);

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
	qs::Timer* pTimer_;
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
	virtual qs::QSTATUS messageChanged(const MessageModelEvent& event) = 0;
};


/****************************************************************************
 *
 * MessageModelEvent
 *
 */

class MessageModelEvent
{
public:
	MessageModelEvent(const MessageModel* pModel, MessageHolder* pmh);
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
