/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __DIALOG_H__
#define __DIALOG_H__

#include <qm.h>
#include <qmfolder.h>
#include <qmsyncfilter.h>

#include <qs.h>
#include <qsdialog.h>
#include <qsprofile.h>

#include "../model/addressbook.h"
#include "../model/editmessage.h"


namespace qm {

class DefaultDialog;
	class AccountDialog;
	class AddressBookDialog;
	class AttachmentDialog;
	class CreateAccountDialog;
	class CreateFolderDialog;
	class CreateSubAccountDialog;
	class CustomFilterDialog;
	class DetachDialog;
	class DialupDialog;
	class ExportDialog;
	class FindDialog;
	class ImportDialog;
	class InputBoxDialog;
	class InsertTextDialog;
	class MailFolderDialog;
	class ProgressDialog;
	class ReplaceDialog;
	class SelectDialupEntryDialog;
	class SelectSyncFilterDialog;

class FixedFormText;
class FixedFormTextManager;


/****************************************************************************
 *
 * DefaultDialog
 *
 */

class DefaultDialog : public qs::DefaultDialog
{
protected:
	DefaultDialog(UINT nId, qs::QSTATUS* pstatus);

public:
	virtual ~DefaultDialog();

private:
	DefaultDialog(const DefaultDialog&);
	DefaultDialog& operator=(const DefaultDialog&);
};


/****************************************************************************
 *
 * AccountDialog
 *
 */

class AccountDialog : public DefaultDialog, public qs::NotifyHandler
{
public:
	AccountDialog(Document* pDocument, Account* pAccount,
		SyncFilterManager* pSyncFilterManager,
		qs::Profile* pProfile, qs::QSTATUS* pstatus);
	virtual ~AccountDialog();

public:
	virtual LRESULT onCommand(WORD nCode, WORD nId);

protected:
	virtual LRESULT onDestroy();
	virtual LRESULT onInitDialog(HWND hwndFocus, LPARAM lParam);

public:
	virtual LRESULT onNotify(NMHDR* pnmhdr, bool* pbHandled);

private:
	LRESULT onAddAccount();
	LRESULT onAddSubAccount();
	LRESULT onRemove();
	LRESULT onRename();
	LRESULT onProperty();
	LRESULT onAccountSelChanged(NMHDR* pnmhdr, bool* pbHandled);

private:
	qs::QSTATUS update();
	void updateState();

private:
	AccountDialog(const AccountDialog&);
	AccountDialog& operator=(const AccountDialog&);

private:
	Document* pDocument_;
	SubAccount* pSubAccount_;
	SyncFilterManager* pSyncFilterManager_;
	qs::Profile* pProfile_;
};


/****************************************************************************
 *
 * AddressBookDialog
 *
 */

class AddressBookDialog : public DefaultDialog, public qs::NotifyHandler
{
public:
	enum Sort {
		SORT_NAME			= 0x01,
		SORT_ADDRESS		= 0x02,
		SORT_COMMENT		= 0x03,
		SORT_TYPE_MASK		= 0x0f,
		
		SORT_ASCENDING		= 0x10,
		SORT_DESCENDING		= 0x20,
		SORT_DIRECTION_MASK	= 0xf0
	};
	
	enum Type {
		TYPE_TO,
		TYPE_CC,
		TYPE_BCC
	};

public:
	typedef std::vector<qs::WSTRING> AddressList;

public:
	class Item
	{
	public:
		Item(qs::WSTRING wstrValue, Type type, qs::QSTATUS* pstatus);
		~Item();
	
	public:
		const WCHAR* getValue() const;
		qs::WSTRING releaseValue();
		Type getType() const;
	
	private:
		Item(const Item&);
		Item& operator=(const Item&);
	
	private:
		qs::WSTRING wstrValue_;
		Type type_;
	};

public:
	AddressBookDialog(AddressBook* pAddressBook, qs::Profile* pProfile,
		const WCHAR* pwszAddress[], qs::QSTATUS* pstatus);
	virtual ~AddressBookDialog();

public:
	const AddressList& getAddresses(Type type) const;

public:
	virtual INT_PTR dialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

public:
	virtual LRESULT onCommand(WORD nCode, WORD nId);

protected:
	virtual LRESULT onDestroy();
	virtual LRESULT onInitDialog(HWND hwndFocus, LPARAM lParam);

protected:
	virtual LRESULT onOk();

public:
	virtual LRESULT onNotify(NMHDR* pnmhdr, bool* pbHandled);

protected:
	LRESULT onSize(UINT nFlags, int cx, int cy);

private:
	LRESULT onCategory();
	LRESULT onSelect(UINT nId);
	LRESULT onRemove();
	LRESULT onAddressColumnClick(NMHDR* pnmhdr, bool* pbHandled);

private:
	qs::QSTATUS update();
	qs::QSTATUS select(Type type);
	qs::QSTATUS remove();
	qs::QSTATUS layout();
	qs::QSTATUS createCategoryMenu(
		const AddressBook::CategoryList& l, HMENU* phmenu);
	qs::QSTATUS setCurrentCategory(const WCHAR* pwszCategory);

private:
	AddressBookDialog(const AddressBookDialog&);
	AddressBookDialog& operator=(const AddressBookDialog&);

private:
	AddressBook* pAddressBook_;
	qs::Profile* pProfile_;
	unsigned int nSort_;
	qs::WSTRING wstrCategory_;
	AddressList listAddress_[3];
};


/****************************************************************************
 *
 * AttachmentDialog
 *
 */

class AttachmentDialog : public DefaultDialog, qs::NotifyHandler
{
public:
	AttachmentDialog(EditMessage::AttachmentList& listAttachment,
		qs::QSTATUS* pstatus);
	virtual ~AttachmentDialog();

public:
	virtual LRESULT onCommand(WORD nCode, WORD nId);

protected:
	virtual LRESULT onDestroy();
	virtual LRESULT onInitDialog(HWND hwndFocus, LPARAM lParam);

public:
	virtual LRESULT onNotify(NMHDR* pnmhdr, bool* pbHandled);

private:
	LRESULT onAdd();
	LRESULT onRemove();
	LRESULT onAttachmentItemChanged(NMHDR* pnmhdr, bool* pbHandled);

private:
	qs::QSTATUS update();
	void updateState();

private:
	AttachmentDialog(const AttachmentDialog&);
	AttachmentDialog& operator=(const AttachmentDialog&);

private:
	EditMessage::AttachmentList& listAttachment_;
};


/****************************************************************************
 *
 * CreateAccountDialog
 *
 */

class CreateAccountDialog : public DefaultDialog
{
public:
	CreateAccountDialog(qs::Profile* pProfile, qs::QSTATUS* pstatus);
	virtual ~CreateAccountDialog();

public:
	const WCHAR* getName() const;
	const WCHAR* getClass() const;
	const WCHAR* getReceiveProtocol() const;
	short getReceivePort() const;
	const WCHAR* getSendProtocol() const;
	short getSendPort() const;
	unsigned int getBlockSize() const;
	unsigned int getCacheBlockSize() const;

public:
	virtual LRESULT onCommand(WORD nCode, WORD nId);

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus, LPARAM lParam);

protected:
	virtual LRESULT onOk();

private:
	LRESULT onNameChange();
	LRESULT onClassChange();
	LRESULT onProtocolChange();
	LRESULT onTypeChange();

private:
	qs::QSTATUS updateProtocols();
	void clearProtocols();
	void updateState();

private:
	CreateAccountDialog(const CreateAccountDialog&);
	CreateAccountDialog& operator=(const CreateAccountDialog&);

private:
	struct Protocol
	{
		qs::WSTRING wstrName_;
		short nPort_;
	};

private:
	typedef std::vector<Protocol> ProtocolList;

private:
	qs::Profile* pProfile_;
	ProtocolList listReceiveProtocol_;
	ProtocolList listSendProtocol_;
	qs::WSTRING wstrName_;
	qs::WSTRING wstrClass_;
	int nReceiveProtocol_;
	int nSendProtocol_;
	unsigned int nBlockSize_;
	unsigned int nCacheBlockSize_;
};


/****************************************************************************
 *
 * CreateFolderDialog
 *
 */

class CreateFolderDialog : public DefaultDialog
{
public:
	enum Type {
		TYPE_LOCALFOLDER,
		TYPE_REMOTEFOLDER,
		TYPE_QUERYFOLDER
	};

public:
	CreateFolderDialog(Type type, bool bAllowRemote, qs::QSTATUS* pstatus);
	virtual ~CreateFolderDialog();

public:
	Type getType() const;
	const WCHAR* getName() const;
	const WCHAR* getMacro() const;

public:
	virtual LRESULT onCommand(WORD nCode, WORD nId);

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus, LPARAM lParam);

protected:
	virtual LRESULT onOk();

private:
	LRESULT onNameChange();
	LRESULT onTypeChange(UINT nId);

private:
	void updateState();

private:
	CreateFolderDialog(const CreateFolderDialog&);
	CreateFolderDialog& operator=(const CreateFolderDialog&);

private:
	Type type_;
	bool bAllowRemote_;
	qs::WSTRING wstrName_;
	qs::WSTRING wstrMacro_;
};


/****************************************************************************
 *
 * CreateSubAccountDialog
 *
 */

class CreateSubAccountDialog : public DefaultDialog
{
public:
	CreateSubAccountDialog(Document* pDocument, qs::QSTATUS* pstatus);
	virtual ~CreateSubAccountDialog();

public:
	const WCHAR* getName() const;

public:
	virtual LRESULT onCommand(WORD nCode, WORD nId);

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus, LPARAM lParam);

protected:
	virtual LRESULT onOk();

private:
	LRESULT onNameChanged();

private:
	void updateState();

private:
	CreateSubAccountDialog(const CreateSubAccountDialog&);
	CreateSubAccountDialog& operator=(const CreateSubAccountDialog&);

private:
	Document* pDocument_;
	qs::WSTRING wstrName_;
};


/****************************************************************************
 *
 * CustomFilterDialog
 *
 */

class CustomFilterDialog : public DefaultDialog
{
public:
	CustomFilterDialog(const WCHAR* pwszMacro, qs::QSTATUS* pstatus);
	virtual ~CustomFilterDialog();

public:
	const WCHAR* getMacro() const;

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus, LPARAM lParam);

protected:
	virtual LRESULT onOk();

private:
	CustomFilterDialog(const CustomFilterDialog&);
	CustomFilterDialog& operator=(const CustomFilterDialog&);

private:
	qs::WSTRING wstrMacro_;
};


/****************************************************************************
 *
 * DetachDialog
 *
 */

class DetachDialog : public DefaultDialog, public qs::NotifyHandler
{
public:
	struct Item
	{
		MessageHolder* pmh_;
		qs::WSTRING wstrName_;
		bool bSelected_;
	};

public:
	typedef std::vector<Item> List;

public:
	DetachDialog(qs::Profile* pProfile, List& list, qs::QSTATUS* pstatus);
	virtual ~DetachDialog();

public:
	const WCHAR* getFolder() const;

public:
	virtual LRESULT onCommand(WORD nCode, WORD nId);

protected:
	virtual LRESULT onDestroy();
	virtual LRESULT onInitDialog(HWND hwndFocus, LPARAM lParam);

protected:
	virtual LRESULT onOk();

public:
	virtual LRESULT onNotify(NMHDR* pnmhdr, bool* pbHandled);

private:
	LRESULT onBrowse();
	LRESULT onRename();
	LRESULT onAttachmentItemChanged(NMHDR* pnmhdr, bool* pbHandled);

private:
	void updateState();

private:
	DetachDialog(const DetachDialog&);
	DetachDialog& operator=(const DetachDialog&);

private:
	qs::Profile* pProfile_;
	List& list_;
	qs::WSTRING wstrFolder_;
};


/****************************************************************************
 *
 * DialupDialog
 *
 */

class DialupDialog : public DefaultDialog
{
public:
	DialupDialog(const WCHAR* pwszEntry, const WCHAR* pwszUsername,
		const WCHAR* pwszPassword, const WCHAR* pwszDomain, qs::QSTATUS* pstatus);
	virtual ~DialupDialog();

public:
	const WCHAR* getUsername() const;
	const WCHAR* getPassword() const;
	const WCHAR* getDomain() const;

public:
	virtual LRESULT onCommand(WORD nCode, WORD nId);

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus, LPARAM lParam);

protected:
	virtual LRESULT onOk();

private:
	LRESULT onDialProperty();

private:
	void updateLocation();

private:
	DialupDialog(const DialupDialog&);
	DialupDialog& operator=(const DialupDialog&);

private:
	qs::WSTRING wstrEntry_;
	qs::WSTRING wstrUsername_;
	qs::WSTRING wstrPassword_;
	qs::WSTRING wstrDomain_;
};


/****************************************************************************
 *
 * ExportDialog
 *
 */

class ExportDialog : public DefaultDialog
{
public:
	ExportDialog(bool bSingleMessage, qs::QSTATUS* pstatus);
	virtual ~ExportDialog();

public:
	const WCHAR* getPath() const;
	bool isFilePerMessage() const;
	bool isExportFlags() const;
	const WCHAR* getTemplate() const;
	const WCHAR* getEncoding() const;

public:
	virtual LRESULT onCommand(WORD nCode, WORD nId);

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus, LPARAM lParam);

protected:
	virtual LRESULT onOk();

private:
	LRESULT onBrowse();
	LRESULT onPathChange();

private:
	void updateState();

private:
	ExportDialog(const ExportDialog&);
	ExportDialog& operator=(const ExportDialog&);

private:
	enum Flag {
		FLAG_FILEPERMESSAGE	= 0x01,
		FLAG_EXPORTFLAGS	= 0x02
	};

private:
	bool bSingleMessage_;
	qs::WSTRING wstrPath_;
	unsigned int nFlags_;
};


/****************************************************************************
 *
 * FindDialog
 *
 */

class FindDialog : public DefaultDialog
{
public:
	FindDialog(qs::Profile* pProfile, qs::QSTATUS* pstatus);
	virtual ~FindDialog();

public:
	const WCHAR* getFind() const;
	bool isMatchCase() const;
	bool isPrev() const;

public:
	virtual LRESULT onCommand(WORD nCode, WORD nId);

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus, LPARAM lParam);

private:
	LRESULT onFind(UINT nId);

private:
	FindDialog(const FindDialog&);
	FindDialog& operator=(const FindDialog&);

private:
	enum {
		HISTORY_SIZE = 10
	};

private:
	qs::Profile* pProfile_;
	qs::WSTRING wstrFind_;
	bool bMatchCase_;
	bool bPrev_;
};


/****************************************************************************
 *
 * ImportDialog
 *
 */

class ImportDialog : public DefaultDialog
{
public:
	ImportDialog(qs::QSTATUS* pstatus);
	virtual ~ImportDialog();

public:
	const WCHAR* getPath() const;
	bool isMultiple() const;
	unsigned int getFlags() const;

public:
	virtual LRESULT onCommand(WORD nCode, WORD nId);

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus, LPARAM lParam);

protected:
	virtual LRESULT onOk();

private:
	LRESULT onBrowse();
	LRESULT onPathChange();

private:
	void updateState();

private:
	ImportDialog(const ImportDialog&);
	ImportDialog& operator=(const ImportDialog&);

private:
	qs::WSTRING wstrPath_;
	bool bMultiple_;
	unsigned int nFlags_;
};


/****************************************************************************
 *
 * InputBoxDialog
 *
 */

class InputBoxDialog : public DefaultDialog
{
public:
	InputBoxDialog(bool bMultiLine, const WCHAR* pwszMessage,
		const WCHAR* pwszValue, qs::QSTATUS* pstatus);
	virtual ~InputBoxDialog();

public:
	const WCHAR* getMessage() const;
	const WCHAR* getValue() const;

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus, LPARAM lParam);

protected:
	virtual LRESULT onOk();

private:
	InputBoxDialog(const InputBoxDialog&);
	InputBoxDialog& operator=(const InputBoxDialog&);

private:
	bool bMultiLine_;
	qs::WSTRING wstrMessage_;
	qs::WSTRING wstrValue_;
};


/****************************************************************************
 *
 * InsertTextDialog
 *
 */

class InsertTextDialog : public DefaultDialog
{
public:
	explicit InsertTextDialog(qs::QSTATUS* pstatus);
	virtual ~InsertTextDialog();

public:
	const FixedFormText* getText() const;

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus, LPARAM lParam);

protected:
	virtual LRESULT onOk();

private:
	InsertTextDialog(const InsertTextDialog&);
	InsertTextDialog& operator=(const InsertTextDialog&);

private:
	FixedFormTextManager* pManager_;
	FixedFormText* pText_;
};


/****************************************************************************
 *
 * MailFolderDialog
 *
 */

class MailFolderDialog : public qs::DefaultDialog
{
public:
	MailFolderDialog(HINSTANCE hInstResource, qs::QSTATUS* pstatus);
	virtual ~MailFolderDialog();

public:
	const WCHAR* getMailFolder() const;

public:
	virtual LRESULT onCommand(WORD nCode, WORD nId);

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus, LPARAM lParam);

protected:
	virtual LRESULT onOk();

private:
	LRESULT onMailFolderChange();
	LRESULT onBrowse();

private:
	void updateState();

private:
	MailFolderDialog(const MailFolderDialog&);
	MailFolderDialog& operator=(const MailFolderDialog&);

private:
	qs::WSTRING wstrMailFolder_;
};


/****************************************************************************
 *
 * ProgressDialog
 *
 */

class ProgressDialog : public DefaultDialog
{
public:
	ProgressDialog(UINT nTitleId, qs::QSTATUS* pstatus);
	virtual ~ProgressDialog();

public:
	qs::QSTATUS init();
	void term();
	bool isCanceled() const;
	qs::QSTATUS setTitle(UINT nId);
	qs::QSTATUS setMessage(UINT nId);
	qs::QSTATUS setRange(unsigned int nMin, unsigned int nMax);
	qs::QSTATUS setPos(unsigned int n);

protected:
	virtual LRESULT onDestroy();
	virtual LRESULT onInitDialog(HWND hwndFocus, LPARAM lParam);

protected:
	virtual LRESULT onCancel();

private:
	ProgressDialog(const ProgressDialog&);
	ProgressDialog& operator=(const ProgressDialog&);

private:
	UINT nTitleId_;
	bool bCanceled_;
};


/****************************************************************************
 *
 * ReplaceDialog
 *
 */

class ReplaceDialog : public DefaultDialog
{
public:
	enum Type {
		TYPE_PREV,
		TYPE_NEXT,
		TYPE_ALL
	};

public:
	ReplaceDialog(qs::Profile* pProfile, qs::QSTATUS* pstatus);
	virtual ~ReplaceDialog();

public:
	const WCHAR* getFind() const;
	const WCHAR* getReplace() const;
	bool isMatchCase() const;
	Type getType() const;

public:
	virtual LRESULT onCommand(WORD nCode, WORD nId);

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus, LPARAM lParam);

private:
	LRESULT onReplace(UINT nId);

private:
	ReplaceDialog(const ReplaceDialog&);
	ReplaceDialog& operator=(const ReplaceDialog&);

private:
	enum {
		HISTORY_SIZE = 10
	};

private:
	qs::Profile* pProfile_;
	qs::WSTRING wstrFind_;
	qs::WSTRING wstrReplace_;
	bool bMatchCase_;
	Type type_;
};


/****************************************************************************
 *
 * SelectDialupEntryDialog
 *
 */

class SelectDialupEntryDialog : public DefaultDialog
{
public:
	SelectDialupEntryDialog(qs::Profile* pProfile, qs::QSTATUS* pstatus);
	virtual ~SelectDialupEntryDialog();

public:
	const WCHAR* getEntry() const;

public:
	virtual LRESULT onCommand(WORD nCode, WORD nId);

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus, LPARAM lParam);

protected:
	virtual LRESULT onOk();

private:
	void updateState();

private:
	LRESULT onSelChange();

private:
	SelectDialupEntryDialog(const SelectDialupEntryDialog&);
	SelectDialupEntryDialog& operator=(const SelectDialupEntryDialog&);

private:
	qs::Profile* pProfile_;
	qs::WSTRING wstrEntry_;
};


/****************************************************************************
 *
 * SelectSyncFilterDialog
 *
 */

class SelectSyncFilterDialog : public DefaultDialog
{
public:
	SelectSyncFilterDialog(SyncFilterManager* pManager, Account* pAccount,
		const WCHAR* pwszDefaultName, qs::QSTATUS* pstatus);
	virtual ~SelectSyncFilterDialog();

public:
	const WCHAR* getName() const;

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus, LPARAM lParam);

protected:
	virtual LRESULT onOk();

private:
	SelectSyncFilterDialog(const SelectSyncFilterDialog&);
	SelectSyncFilterDialog& operator=(const SelectSyncFilterDialog&);

private:
	SyncFilterManager::FilterSetList list_;
	const WCHAR* pwszName_;
};

}

#endif // __DIALOG_H__
