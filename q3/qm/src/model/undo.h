/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
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
	class MessageUndoItem;
		class SetFlagsUndoItem;
		class SetLabelUndoItem;
	class MessageListUndoItem;
		class MoveUndoItem;
		class DeleteUndoItem;
	class GroupUndoItem;
class UndoExecutor;
	class AbstractUndoExecutor;
		class MessageUndoExecutor;
			class SetFlagsUndoExecutor;
			class SetLabelUndoExecutor;
		class MessageListUndoExecutor;
			class MoveUndoExecutor;
			class DeleteUndoExecutor;
	class EmptyUndoExecutor;
class UndoContext;

class Account;
class AccountManager;
class MessageHolderURI;
class NormalFolder;
class URIResolver;


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
	virtual std::auto_ptr<UndoExecutor> getExecutor(const UndoContext& context) = 0;
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
	virtual std::auto_ptr<UndoExecutor> getExecutor(const UndoContext& context);

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
 * MessageUndoItem
 *
 */

class MessageUndoItem : public UndoItem
{
public:
	class Item
	{
	protected:
		Item(std::auto_ptr<MessageHolderURI> pURI);
	
	public:
		virtual ~Item();
	
	public:
		const MessageHolderURI* getURI() const;
	
	private:
		Item(const Item&);
		Item& operator=(const Item&);
	
	private:
		std::auto_ptr<MessageHolderURI> pURI_;
	};

public:
	MessageUndoItem();
	virtual ~MessageUndoItem();

public:
	virtual std::auto_ptr<UndoExecutor> getExecutor(const UndoContext& context);

protected:
	void add(MessageHolder* pmh,
			 std::auto_ptr<Item> pItem);

private:
	virtual std::auto_ptr<MessageUndoExecutor> createExecutor(Account* pAccount) const = 0;

private:
	MessageUndoItem(const MessageUndoItem&);
	MessageUndoItem& operator=(const MessageUndoItem&);

private:
	typedef std::vector<Item*> ItemList;

private:
	ItemList listItem_;
	qs::wstring_ptr wstrAccount_;
#ifndef NDEBUG
	NormalFolder* pFolder_;
#endif
};


/****************************************************************************
 *
 * MessageUndoExecutor
 *
 */

class MessageUndoExecutor : public AbstractUndoExecutor
{
public:
	explicit MessageUndoExecutor(Account* pAccount);
	virtual ~MessageUndoExecutor();

public:
	void add(MessageHolder* pmh,
			 const MessageUndoItem::Item* pItem);

private:
	virtual bool execute(Account* pAccount);

private:
	virtual bool execute(Account* pAccount,
						 MessageHolder* pmh,
						 const MessageUndoItem::Item* pItem) = 0;

private:
	MessageUndoExecutor(const MessageUndoExecutor&);
	MessageUndoExecutor& operator=(const MessageUndoExecutor&);

private:
	struct Item
	{
		MessageHolder* pmh_;
		const MessageUndoItem::Item* pItem_;
	};

private:
	typedef std::vector<Item> ItemList;

private:
	ItemList listItem_;
};


/****************************************************************************
 *
 * SetFlagsUndoItem
 *
 */

class SetFlagsUndoItem : public MessageUndoItem
{
public:
	class Item : public MessageUndoItem::Item
	{
	public:
		Item(std::auto_ptr<MessageHolderURI> pURI,
			 unsigned int nFlags,
			 unsigned int nMask);
		virtual ~Item();
	
	public:
		unsigned int getFlags() const;
		unsigned int getMask() const;
	
	private:
		Item(const Item&);
		Item& operator=(const Item&);
	
	private:
		unsigned int nFlags_;
		unsigned int nMask_;
	};

public:
	SetFlagsUndoItem();
	virtual ~SetFlagsUndoItem();

public:
	void add(MessageHolder* pmh,
			 unsigned int nFlags,
			 unsigned int nMask);

private:
	virtual std::auto_ptr<MessageUndoExecutor> createExecutor(Account* pAccount) const;

private:
	SetFlagsUndoItem(const SetFlagsUndoItem&);
	SetFlagsUndoItem& operator=(const SetFlagsUndoItem&);
};


/****************************************************************************
 *
 * SetFlagsUndoExecutor
 *
 */

class SetFlagsUndoExecutor : public MessageUndoExecutor
{
public:
	explicit SetFlagsUndoExecutor(Account* pAccount);
	virtual ~SetFlagsUndoExecutor();

private:
	virtual bool execute(Account* pAccount,
						 MessageHolder* pmh,
						 const MessageUndoItem::Item* pItem);

private:
	SetFlagsUndoExecutor(const SetFlagsUndoExecutor&);
	SetFlagsUndoExecutor& operator=(const SetFlagsUndoExecutor&);
};


/****************************************************************************
 *
 * SetLabelUndoItem
 *
 */

class SetLabelUndoItem : public MessageUndoItem
{
public:
	class Item : public MessageUndoItem::Item
	{
	public:
		Item(std::auto_ptr<MessageHolderURI> pURI,
			 const WCHAR* pwszLabel);
		virtual ~Item();
	
	public:
		const WCHAR* getLabel() const;
	
	private:
		Item(const Item&);
		Item& operator=(const Item&);
	
	private:
		qs::wstring_ptr wstrLabel_;
	};

public:
	SetLabelUndoItem();
	virtual ~SetLabelUndoItem();

public:
	void add(MessageHolder* pmh,
			 const WCHAR* pwszLabel);

private:
	virtual std::auto_ptr<MessageUndoExecutor> createExecutor(Account* pAccount) const;

private:
	SetLabelUndoItem(const SetLabelUndoItem&);
	SetLabelUndoItem& operator=(const SetLabelUndoItem&);
};


/****************************************************************************
 *
 * SetLabelUndoExecutor
 *
 */

class SetLabelUndoExecutor : public MessageUndoExecutor
{
public:
	explicit SetLabelUndoExecutor(Account* pAccount);
	virtual ~SetLabelUndoExecutor();

private:
	virtual bool execute(Account* pAccount,
						 MessageHolder* pmh,
						 const MessageUndoItem::Item* pItem);

private:
	SetLabelUndoExecutor(const SetLabelUndoExecutor&);
	SetLabelUndoExecutor& operator=(const SetLabelUndoExecutor&);
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
	virtual std::auto_ptr<UndoExecutor> getExecutor(const UndoContext& context);

private:
	virtual std::auto_ptr<UndoExecutor> getExecutor(const UndoContext& context,
													Account* pAccount,
													NormalFolder* pFolder,
													MessageHolderList& l) = 0;

protected:
	static NormalFolder* getFolder(AccountManager* pAccountManager,
								   const WCHAR* pwszFolder);

private:
	MessageListUndoItem(const MessageListUndoItem&);
	MessageListUndoItem& operator=(const MessageListUndoItem&);

private:
	typedef std::vector<MessageHolderURI*> URIList;

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
	MessageListUndoExecutor(Account* pAccount,
							NormalFolder* pFolder,
							MessageHolderList& l);
	virtual ~MessageListUndoExecutor();

private:
	virtual bool execute(Account* pAccount);

private:
	virtual bool execute(Account* pAccount,
						 NormalFolder* pFolder,
						 const MessageHolderList& l) = 0;

private:
	MessageListUndoExecutor(const MessageListUndoExecutor&);
	MessageListUndoExecutor& operator=(const MessageListUndoExecutor&);

private:
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
	virtual std::auto_ptr<UndoExecutor> getExecutor(const UndoContext& context,
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
	MoveUndoExecutor(Account* pAccount,
					 NormalFolder* pFolderFrom,
					 NormalFolder* pFolderTo,
					 MessageHolderList& l);
	virtual ~MoveUndoExecutor();

private:
	virtual bool execute(Account* pAccount,
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
	virtual std::auto_ptr<UndoExecutor> getExecutor(const UndoContext& context,
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
	DeleteUndoExecutor(Account* pAccount,
					   NormalFolder* pFolder,
					   MessageHolderList& l);
	virtual ~DeleteUndoExecutor();

private:
	virtual bool execute(Account* pAccount,
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
	virtual std::auto_ptr<UndoExecutor> getExecutor(const UndoContext& context);

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


/****************************************************************************
 *
 * UndoContext
 *
 */

class UndoContext
{
public:
	UndoContext(AccountManager* pAccountManager,
				const URIResolver* pURIResolver);
	~UndoContext();

public:
	AccountManager* getAccountManager() const;
	const URIResolver* getURIResolver() const;

private:
	UndoContext(const UndoContext&);
	UndoContext& operator=(const UndoContext&);

private:
	AccountManager* pAccountManager_;
	const URIResolver* pURIResolver_;
};

}

#endif __UNDO_H__
