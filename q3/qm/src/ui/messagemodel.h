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
class MessageModelHandler;
class MessageModelEvent;

class MessageHolder;
class Message;


/****************************************************************************
 *
 * MessageModel
 *
 */

class MessageModel :
	public ViewModelManagerHandler,
	public DefaultViewModelHandler,
	public qs::TimerHandler
{
public:
	MessageModel(ViewModelManager* pViewModelManager,
		bool bConnectToViewModel, qs::QSTATUS* pstatus);
	MessageModel(qs::QSTATUS* pstatus);
	~MessageModel();

public:
	Account* getCurrentAccount() const;
	MessagePtr getCurrentMessage() const;
	qs::QSTATUS setMessage(MessageHolder* pmh);
	ViewModel* getViewModel() const;
	qs::QSTATUS setViewModel(ViewModel* pViewModel);
	qs::QSTATUS updateToViewModel();
	qs::QSTATUS connectToViewModel();
	qs::QSTATUS disconnectFromViewModel();
	bool isConnectedToViewModel() const;
	
	qs::QSTATUS addMessageModelHandler(MessageModelHandler* pHandler);
	qs::QSTATUS removeMessageModelHandler(MessageModelHandler* pHandler);

public:
	virtual qs::QSTATUS viewModelSelected(const ViewModelManagerEvent& event);

public:
	virtual qs::QSTATUS itemRemoved(const ViewModelEvent& event);
	virtual qs::QSTATUS itemStateChanged(const ViewModelEvent& event);
	virtual qs::QSTATUS destroyed(const ViewModelEvent& event);

public:
	virtual qs::QSTATUS timerTimeout(unsigned int nId);

private:
	qs::QSTATUS fireMessageChanged(MessageHolder* pmh) const;

private:
	MessageModel(const MessageModel&);
	MessageModel& operator=(const MessageModel&);

private:
	enum {
		TIMER_ITEMSTATECHANGED	= 10,
		TIMEOUT					= 300
	};

private:
	typedef std::vector<MessageModelHandler*> HandlerList;

private:
	bool bPreview_;
	Account* pAccount_;
	MessagePtr ptr_;
	ViewModelManager* pViewModelManager_;
	ViewModel* pViewModel_;
	HandlerList listHandler_;
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
