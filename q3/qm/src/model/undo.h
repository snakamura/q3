/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __UNDO_H__
#define __UNDO_H__

#include <qm.h>
#include <qmmessageholder.h>

#include <memory>
#include <vector>


namespace qm {

class UndoManager;
class UndoItemList;
class UndoItem;
	class EmptyUndoItem;
	class SetFlagsUndoItem;
	class MessageListUndoItem;
		class MoveUndoItem;
		class DeleteUndoItem;
	class GroupUndoItem;
class UndoExecutor;
	class AbstractUndoExecutor;
		class SetFlagsUndoExecutor;
		class MessageListUndoExecutor;
			class MoveUndoExecutor;
			class DeleteUndoExecutor;
	class EmptyUndoExecutor;

class Account;
class Document;
class NormalFolder;
class URI;


/****************************************************************************
 *
 * UndoManager
 *
 */

class UndoManager
{
public:
	UndoManager();
	~UndoManager();

public:
	void pushUndoItem(std::auto_ptr<UndoItem> pUndoItem);
	std::auto_ptr<UndoItem> popUndoItem();
	bool hasUndoItem() const;
	void clear();

private:
	UndoManager(const UndoManager&);
	UndoManager& operator=(const UndoManager&);

private:
	std::auto_ptr<UndoItem> pUndoItem_;
};


/****************************************************************************
 *
 * UndoItemList
 *
 */

class UndoItemList
{
public:
	UndoItemList();
	~UndoItemList();

public:
	void add(std::auto_ptr<UndoItem> pItem);

public:
	std::auto_ptr<UndoItem> getUndoItem();

private:
	UndoItemList(const UndoItemList&);
	UndoItemList& operator=(const UndoItemList&);

private:
	typedef std::vector<UndoItem*> ItemList;

private:
	ItemList listItem_;
};


/****************************************************************************
 *
 * UndoItem
 *
 */

class UndoItem
{
public:
	virtual ~UndoItem();

public:
	virtual std::auto_ptr<UndoExecutor> getExecutor(Document* pDocument) = 0;
};


/****************************************************************************
 *
 * UndoExecutor
 *
 */

class UndoExecutor
{
public:
	virtual ~UndoExecutor();

public:
	virtual bool execute() = 0;
};


/****************************************************************************
 *
 * EmptyUndoItem
 *
 */

class EmptyUndoItem : public UndoItem
{
public:
	EmptyUndoItem();
	virtual ~EmptyUndoItem();

public:
	virtual std::auto_ptr<UndoExecutor> getExecutor(Document* pDocument);

private:
	EmptyUndoItem(const EmptyUndoItem&);
	EmptyUndoItem& operator=(const EmptyUndoItem&);
};


/****************************************************************************
 *
 * EmptyUndoExecutor
 *
 */

class EmptyUndoExecutor : public UndoExecutor
{
public:
	EmptyUndoExecutor();
	virtual ~EmptyUndoExecutor();

public:
	virtual bool execute();

private:
	EmptyUndoExecutor(const EmptyUndoExecutor&);
	EmptyUndoExecutor& operator=(const EmptyUndoExecutor&);
};


/****************************************************************************
 *
 * AbstractUndoExecutor
 *
 */

class AbstractUndoExecutor : public UndoExecutor
{
public:
	explicit AbstractUndoExecutor(Account* pAccount);
	virtual ~AbstractUndoExecutor();

public:
	virtual bool execute();

private:
	virtual bool execute(Account* pAccount) = 0;

private:
	AbstractUndoExecutor(const AbstractUndoExecutor&);
	AbstractUndoExecutor& operator=(const AbstractUndoExecutor&);

private:
	Account* pAccount_;
};


/****************************************************************************
 *
 * SetFlagsUndoItem
 *
 */

class SetFlagsUndoItem : public UndoItem
{
public:
	SetFlagsUndoItem();
	virtual ~SetFlagsUndoItem();

public:
	void add(MessageHolder* pmh,
			 unsigned int nFlags,
			 unsigned int nMask);

public:
	virtual std::auto_ptr<UndoExecutor> getExecutor(Document* pDocument);

private:
	SetFlagsUndoItem(const SetFlagsUndoItem&);
	SetFlagsUndoItem& operator=(const SetFlagsUndoItem&);

private:
	struct Item
	{
		URI* pURI_;
		unsigned int nFlags_;
		unsigned int nMask_;
	};

private:
	typedef std::vector<Item> ItemList;

private:
	ItemList listItem_;
	qs::wstring_ptr wstrAccount_;
#ifndef NDEBUG
	NormalFolder* pFolder_;
#endif
};


/****************************************************************************
 *
 * SetFlagsUndoExecutor
 *
 */

class SetFlagsUndoExecutor : public AbstractUndoExecutor
{
public:
	explicit SetFlagsUndoExecutor(Account* pAccount);
	virtual ~SetFlagsUndoExecutor();

public:
	void add(MessageHolder* pmh,
			 unsigned int nFlags,
			 unsigned int nMask);

private:
	virtual bool execute(Account* pAccount);

private:
	SetFlagsUndoExecutor(const SetFlagsUndoExecutor&);
	SetFlagsUndoExecutor& operator=(const SetFlagsUndoExecutor&);

private:
	struct Item
	{
		MessageHolder* pmh_;
		unsigned int nFlags_;
		unsigned int nMask_;
	};

private:
	typedef std::vector<Item> ItemList;

private:
	ItemList listItem_;
};


/****************************************************************************
 *
 * MessageListUndoItem
 *
 */

class MessageListUndoItem : public UndoItem
{
public:
	MessageListUndoItem(const MessageHolderList& l);
	virtual ~MessageListUndoItem();

public:
	virtual std::auto_ptr<UndoExecutor> getExecutor(Document* pDocument);

private:
	virtual std::auto_ptr<UndoExecutor> getExecutor(Document* pDocument,
													Account* pAccount,
													NormalFolder* pFolder,
													MessageHolderList& l) = 0;

protected:
	static NormalFolder* getFolder(Document* pDocument,
								   const WCHAR* pwszFolder);

private:
	MessageListUndoItem(const MessageListUndoItem&);
	MessageListUndoItem& operator=(const MessageListUndoItem&);

private:
	typedef std::vector<URI*> URIList;

private:
	URIList listURI_;
	qs::wstring_ptr wstrFolder_;
};


/****************************************************************************
 *
 * MessageListUndoExecutor
 *
 */

class MessageListUndoExecutor : public AbstractUndoExecutor
{
public:
	MessageListUndoExecutor(Document* pDocument,
							Account* pAccount,
							NormalFolder* pFolder,
							MessageHolderList& l);
	virtual ~MessageListUndoExecutor();

private:
	virtual bool execute(Account* pAccount);

private:
	virtual bool execute(Document* pDocument,
						 Account* pAccount,
						 NormalFolder* pFolder,
						 const MessageHolderList& l) = 0;

private:
	MessageListUndoExecutor(const MessageListUndoExecutor&);
	MessageListUndoExecutor& operator=(const MessageListUndoExecutor&);

private:
	Document* pDocument_;
	NormalFolder* pFolder_;
	MessageHolderList listMessageHolder_;
};


/****************************************************************************
 *
 * MoveUndoItem
 *
 */

class MoveUndoItem : public MessageListUndoItem
{
public:
	MoveUndoItem(const MessageHolderList& l,
				 NormalFolder* pFolderTo);
	virtual ~MoveUndoItem();

private:
	virtual std::auto_ptr<UndoExecutor> getExecutor(Document* pDocument,
													Account* pAccount,
													NormalFolder* pFolder,
													MessageHolderList& l);

private:
	MoveUndoItem(const MoveUndoItem&);
	MoveUndoItem& operator=(const MoveUndoItem&);

private:
	qs::wstring_ptr wstrFolderTo_;
};


/****************************************************************************
 *
 * MoveUndoExecutor
 *
 */

class MoveUndoExecutor : public MessageListUndoExecutor
{
public:
	MoveUndoExecutor(Document* pDocument,
					 Account* pAccount,
					 NormalFolder* pFolderFrom,
					 NormalFolder* pFolderTo,
					 MessageHolderList& l);
	virtual ~MoveUndoExecutor();

private:
	virtual bool execute(Document* pDocument,
						 Account* pAccount,
						 NormalFolder* pFolder,
						 const MessageHolderList& l);

private:
	MoveUndoExecutor(const MoveUndoExecutor&);
	MoveUndoExecutor& operator=(const MoveUndoExecutor&);

private:
	NormalFolder* pFolderTo_;
};


/****************************************************************************
 *
 * DeleteUndoItem
 *
 */

class DeleteUndoItem : public MessageListUndoItem
{
public:
	DeleteUndoItem(const MessageHolderList& l);
	virtual ~DeleteUndoItem();

private:
	virtual std::auto_ptr<UndoExecutor> getExecutor(Document* pDocument,
													Account* pAccount,
													NormalFolder* pFolder,
													MessageHolderList& l);

private:
	DeleteUndoItem(const DeleteUndoItem&);
	DeleteUndoItem& operator=(const DeleteUndoItem&);
};


/****************************************************************************
 *
 * DeleteUndoExecutor
 *
 */

class DeleteUndoExecutor : public MessageListUndoExecutor
{
public:
	DeleteUndoExecutor(Document* pDocument,
					   Account* pAccount,
					   NormalFolder* pFolder,
					   MessageHolderList& l);
	virtual ~DeleteUndoExecutor();

private:
	virtual bool execute(Document* pDocument,
						 Account* pAccount,
						 NormalFolder* pFolder,
						 const MessageHolderList& l);

private:
	DeleteUndoExecutor(const DeleteUndoExecutor&);
	DeleteUndoExecutor& operator=(const DeleteUndoExecutor&);
};


/****************************************************************************
 *
 * GroupUndoItem
 *
 */

class GroupUndoItem : public UndoItem
{
public:
	typedef std::vector<UndoItem*> ItemList;

public:
	explicit GroupUndoItem(ItemList& listItem);
	virtual ~GroupUndoItem();

public:
	virtual std::auto_ptr<UndoExecutor> getExecutor(Document* pDocument);

private:
	GroupUndoItem(const GroupUndoItem&);
	GroupUndoItem& operator=(const GroupUndoItem&);

private:
	ItemList listItem_;
};


/****************************************************************************
 *
 * GroupUndoExecutor
 *
 */

class GroupUndoExecutor : public UndoExecutor
{
public:
	GroupUndoExecutor();
	virtual ~GroupUndoExecutor();

public:
	void add(std::auto_ptr<UndoExecutor> pExecutor);

public:
	virtual bool execute();

private:
	GroupUndoExecutor(const GroupUndoExecutor&);
	GroupUndoExecutor& operator=(const GroupUndoExecutor&);

private:
	typedef std::vector<UndoExecutor*> ExecutorList;

private:
	ExecutorList listExecutor_;
};

}

#endif __UNDO_H__
