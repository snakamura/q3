/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
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
	class MoveMessageDialog;
	class ProgressDialog;
	class RenameDialog;
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
	DefaultDialog(UINT nId);

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

class AccountDialog :
	public DefaultDialog,
	public qs::NotifyHandler
{
public:
	AccountDialog(Document* pDocument,
				  Account* pAccount,
				  SyncFilterManager* pSyncFilterManager,
				  qs::Profile* pProfile);
	virtual ~AccountDialog();

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

protected:
	virtual LRESULT onDestroy();
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

public:
	virtual LRESULT onNotify(NMHDR* pnmhdr,
							 bool* pbHandled);

private:
	LRESULT onAddAccount();
	LRESULT onAddSubAccount();
	LRESULT onRemove();
	LRESULT onRename();
	LRESULT onProperty();
	LRESULT onAccountSelChanged(NMHDR* pnmhdr,
								bool* pbHandled);

private:
	void update();
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

class AddressBookDialog :
	public DefaultDialog,
	public qs::NotifyHandler
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
	typedef std::vector<qs::WSTRING> CategoryNameList;

public:
	class Item
	{
	public:
		Item(qs::wstring_ptr wstrValue,
			 Type type);
		~Item();
	
	public:
		const WCHAR* getValue() const;
		qs::wstring_ptr releaseValue();
		Type getType() const;
	
	private:
		Item(const Item&);
		Item& operator=(const Item&);
	
	private:
		qs::wstring_ptr wstrValue_;
		Type type_;
	};

public:
	AddressBookDialog(AddressBook* pAddressBook,
					  qs::Profile* pProfile,
					  const WCHAR* pwszAddress[]);
	virtual ~AddressBookDialog();

public:
	const AddressList& getAddresses(Type type) const;

public:
	virtual INT_PTR dialogProc(UINT uMsg,
							   WPARAM wParam,
							   LPARAM lParam);

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

protected:
	virtual LRESULT onDestroy();
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

protected:
	virtual LRESULT onOk();

public:
	virtual LRESULT onNotify(NMHDR* pnmhdr,
							 bool* pbHandled);

protected:
	LRESULT onSize(UINT nFlags,
				   int cx,
				   int cy);

private:
	LRESULT onCategory();
	LRESULT onSelect(UINT nId);
	LRESULT onRemove();
#if !defined _WIN32_WCE || _WIN32_WCE < 300 || !defined _WIN32_WCE_PSPC
	LRESULT onFilterChange();
#endif
	LRESULT onAddressColumnClick(NMHDR* pnmhdr,
								 bool* pbHandled);
	LRESULT onAddressDblClk(NMHDR* pnmhdr,
							bool* pbHandled);

private:
	void update();
	void select(Type type);
	void remove();
	void layout();
	HMENU createCategoryMenu(const AddressBook::CategoryList& l,
							 CategoryNameList* pList);
	void setCurrentCategory(const WCHAR* pwszCategory);
	bool isCategory(const AddressBookAddress::CategoryList& listCategory) const;
	bool isMatchFilter(const AddressBookEntry* pEntry) const;
	bool isMatchFilter(const AddressBookAddress* pAddress) const;

private:
	static size_t getCategoryLevel(const WCHAR* pwszCategory);
	static qs::wstring_ptr getCategoryName(const WCHAR* pwszCategory,
										   size_t nLevel,
										   bool bFull);

private:
	AddressBookDialog(const AddressBookDialog&);
	AddressBookDialog& operator=(const AddressBookDialog&);

private:
	class AddressListWindow :
		public qs::WindowBase,
		public qs::DefaultWindowHandler
	{
	public:
		explicit AddressListWindow(AddressBookDialog* pDialog);
		virtual ~AddressListWindow();
	
	public:
		virtual LRESULT windowProc(UINT uMsg,
								   WPARAM wParam,
								   LPARAM lParam);
	
	protected:
		LRESULT onChar(UINT nChar,
					   UINT nRepeat,
					   UINT nFlags);
	
	private:
		AddressListWindow(const AddressListWindow&);
		AddressListWindow& operator=(const AddressListWindow&);
	
	private:
		AddressBookDialog* pDialog_;
	};
	friend class AddressListWindow;

private:
	struct CategoryLess :
		public std::binary_function<AddressBookCategory*, AddressBookCategory*, bool>
	{
		bool operator()(const AddressBookCategory* pLhs,
						const AddressBookCategory* pRhs);
	};

private:
	AddressBook* pAddressBook_;
	qs::Profile* pProfile_;
	unsigned int nSort_;
	qs::wstring_ptr wstrCategory_;
	qs::wstring_ptr wstrFilter_;
	AddressList listAddress_[3];
	AddressListWindow wndAddressList_;
};


/****************************************************************************
 *
 * AttachmentDialog
 *
 */

class AttachmentDialog :
	public DefaultDialog,
	qs::NotifyHandler
{
public:
	explicit AttachmentDialog(EditMessage::AttachmentList& listAttachment);
	virtual ~AttachmentDialog();

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

protected:
	virtual LRESULT onDestroy();
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

public:
	virtual LRESULT onNotify(NMHDR* pnmhdr,
							 bool* pbHandled);

private:
	LRESULT onAdd();
	LRESULT onRemove();
	LRESULT onAttachmentItemChanged(NMHDR* pnmhdr,
									bool* pbHandled);

private:
	void update();
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
	explicit CreateAccountDialog(qs::Profile* pProfile);
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
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

protected:
	virtual LRESULT onOk();

private:
	LRESULT onNameChange();
	LRESULT onClassChange();
	LRESULT onProtocolChange();
	LRESULT onTypeChange();

private:
	void updateProtocols();
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
	qs::wstring_ptr wstrName_;
	qs::wstring_ptr wstrClass_;
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
	CreateFolderDialog(Type type,
					   bool bAllowRemote);
	virtual ~CreateFolderDialog();

public:
	Type getType() const;
	const WCHAR* getName() const;

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

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
	qs::wstring_ptr wstrName_;
};


/****************************************************************************
 *
 * CreateSubAccountDialog
 *
 */

class CreateSubAccountDialog : public DefaultDialog
{
public:
	explicit CreateSubAccountDialog(Document* pDocument);
	virtual ~CreateSubAccountDialog();

public:
	const WCHAR* getName() const;

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

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
	qs::wstring_ptr wstrName_;
};


/****************************************************************************
 *
 * CustomFilterDialog
 *
 */

class CustomFilterDialog : public DefaultDialog
{
public:
	explicit CustomFilterDialog(const WCHAR* pwszMacro);
	virtual ~CustomFilterDialog();

public:
	const WCHAR* getMacro() const;

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

protected:
	virtual LRESULT onOk();

private:
	CustomFilterDialog(const CustomFilterDialog&);
	CustomFilterDialog& operator=(const CustomFilterDialog&);

private:
	qs::wstring_ptr wstrMacro_;
};


/****************************************************************************
 *
 * DetachDialog
 *
 */

class DetachDialog :
	public DefaultDialog,
	public qs::NotifyHandler
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
	DetachDialog(qs::Profile* pProfile,
				 List& list);
	virtual ~DetachDialog();

public:
	const WCHAR* getFolder() const;

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

protected:
	virtual LRESULT onDestroy();
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

protected:
	virtual LRESULT onOk();

public:
	virtual LRESULT onNotify(NMHDR* pnmhdr,
							 bool* pbHandled);

private:
	LRESULT onBrowse();
	LRESULT onRename();
	LRESULT onAttachmentEndLabelEdit(NMHDR* pnmhdr,
									 bool* pbHandled);
	LRESULT onAttachmentItemChanged(NMHDR* pnmhdr,
									bool* pbHandled);

private:
	void updateState();

private:
	DetachDialog(const DetachDialog&);
	DetachDialog& operator=(const DetachDialog&);

private:
	qs::Profile* pProfile_;
	List& list_;
	qs::wstring_ptr wstrFolder_;
};


/****************************************************************************
 *
 * DialupDialog
 *
 */

class DialupDialog : public DefaultDialog
{
public:
	DialupDialog(const WCHAR* pwszEntry,
				 const WCHAR* pwszUserName,
				 const WCHAR* pwszPassword,
				 const WCHAR* pwszDomain);
	virtual ~DialupDialog();

public:
	const WCHAR* getUserName() const;
	const WCHAR* getPassword() const;
	const WCHAR* getDomain() const;

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

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
	qs::wstring_ptr wstrEntry_;
	qs::wstring_ptr wstrUserName_;
	qs::wstring_ptr wstrPassword_;
	qs::wstring_ptr wstrDomain_;
};


/****************************************************************************
 *
 * ExportDialog
 *
 */

class ExportDialog : public DefaultDialog
{
public:
	explicit ExportDialog(bool bSingleMessage);
	virtual ~ExportDialog();

public:
	const WCHAR* getPath() const;
	bool isFilePerMessage() const;
	bool isExportFlags() const;
	const WCHAR* getTemplate() const;
	const WCHAR* getEncoding() const;

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

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
	qs::wstring_ptr wstrPath_;
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
	FindDialog(qs::Profile* pProfile,
			   bool bSupportRegex);
	virtual ~FindDialog();

public:
	const WCHAR* getFind() const;
	bool isMatchCase() const;
	bool isRegex() const;
	bool isPrev() const;

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

private:
	LRESULT onUpdateState(WPARAM wParam,
						  LPARAM lParam);

private:
	LRESULT onFind(UINT nId);
	LRESULT onFindChange();
	LRESULT onFindSelChange();
	LRESULT onRegexChange();

private:
	void updateState();

private:
	FindDialog(const FindDialog&);
	FindDialog& operator=(const FindDialog&);

private:
	enum {
		HISTORY_SIZE = 10
	};

private:
	qs::Profile* pProfile_;
	bool bSupportRegex_;
	qs::wstring_ptr wstrFind_;
	bool bMatchCase_;
	bool bRegex_;
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
	ImportDialog();
	virtual ~ImportDialog();

public:
	const WCHAR* getPath() const;
	bool isMultiple() const;
	unsigned int getFlags() const;

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

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
	qs::wstring_ptr wstrPath_;
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
	InputBoxDialog(bool bMultiLine,
				   const WCHAR* pwszMessage,
				   const WCHAR* pwszValue);
	virtual ~InputBoxDialog();

public:
	const WCHAR* getMessage() const;
	const WCHAR* getValue() const;

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

protected:
	virtual LRESULT onOk();

private:
	InputBoxDialog(const InputBoxDialog&);
	InputBoxDialog& operator=(const InputBoxDialog&);

private:
	bool bMultiLine_;
	qs::wstring_ptr wstrMessage_;
	qs::wstring_ptr wstrValue_;
};


/****************************************************************************
 *
 * InsertTextDialog
 *
 */

class InsertTextDialog : public DefaultDialog
{
public:
	InsertTextDialog();
	virtual ~InsertTextDialog();

public:
	const FixedFormText* getText() const;

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

protected:
	virtual LRESULT onOk();

private:
	InsertTextDialog(const InsertTextDialog&);
	InsertTextDialog& operator=(const InsertTextDialog&);

private:
	std::auto_ptr<FixedFormTextManager> pManager_;
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
	explicit MailFolderDialog(HINSTANCE hInstResource);
	virtual ~MailFolderDialog();

public:
	const WCHAR* getMailFolder() const;

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

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
	qs::wstring_ptr wstrMailFolder_;
};


/****************************************************************************
 *
 * MoveMessageDialog
 *
 */

class MoveMessageDialog :
	public DefaultDialog,
	public qs::NotifyHandler
{
public:
	MoveMessageDialog(Document* pDocument,
					  qs::Profile* pProfile);
	virtual ~MoveMessageDialog();

public:
	NormalFolder* getFolder() const;
	bool isCopy() const;

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

public:
	virtual LRESULT onNotify(NMHDR* pnmhdr,
							 bool* pbHandled);

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);
	virtual LRESULT onDestroy();

protected:
	virtual LRESULT onOk();

private:
	LRESULT onShowHidden();
	LRESULT onFolderSelChanged(NMHDR* pnmhdr,
							   bool* pbHandled);

private:
	bool update();
	bool insertAccount(Account* pAccount);
	bool insertFolders(HTREEITEM hItem,
					   Account* pAccount);
	void updateState();

private:
	MoveMessageDialog(const MoveMessageDialog&);
	MoveMessageDialog& operator=(const MoveMessageDialog&);

private:
	Document* pDocument_;
	qs::Profile* pProfile_;
	NormalFolder* pFolder_;
	bool bCopy_;
	bool bShowHidden_;
};


/****************************************************************************
 *
 * ProgressDialog
 *
 */

class ProgressDialog : public DefaultDialog
{
public:
	explicit ProgressDialog(UINT nTitleId);
	virtual ~ProgressDialog();

public:
	bool init(HWND hwnd);
	void term();
	bool isCanceled() const;
	void setTitle(UINT nId);
	void setMessage(UINT nId);
	void setRange(unsigned int nMin,
				  unsigned int nMax);
	void setPos(unsigned int n);

protected:
	virtual LRESULT onDestroy();
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

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
 * RenameDialog
 *
 */

class RenameDialog : public DefaultDialog
{
public:
	RenameDialog(const WCHAR* pwszName);
	virtual ~RenameDialog();

public:
	const WCHAR* getName() const;

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

protected:
	virtual LRESULT onOk();

private:
	void updateState();

private:
	LRESULT onNameChange();

private:
	RenameDialog(const RenameDialog&);
	RenameDialog& operator=(const RenameDialog&);

private:
	qs::wstring_ptr wstrName_;
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
	explicit ReplaceDialog(qs::Profile* pProfile);
	virtual ~ReplaceDialog();

public:
	const WCHAR* getFind() const;
	const WCHAR* getReplace() const;
	bool isMatchCase() const;
	bool isRegex() const;
	Type getType() const;

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

private:
	LRESULT onReplace(UINT nId);
	LRESULT onFindChange();
	LRESULT onFindSelChange();
	LRESULT onRegexChange();

private:
	void updateState();

private:
	ReplaceDialog(const ReplaceDialog&);
	ReplaceDialog& operator=(const ReplaceDialog&);

private:
	enum {
		HISTORY_SIZE = 10
	};

private:
	qs::Profile* pProfile_;
	qs::wstring_ptr wstrFind_;
	qs::wstring_ptr wstrReplace_;
	bool bMatchCase_;
	bool bRegex_;
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
	explicit SelectDialupEntryDialog(qs::Profile* pProfile);
	virtual ~SelectDialupEntryDialog();

public:
	const WCHAR* getEntry() const;

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

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
	qs::wstring_ptr wstrEntry_;
};


/****************************************************************************
 *
 * SelectSyncFilterDialog
 *
 */

class SelectSyncFilterDialog : public DefaultDialog
{
public:
	SelectSyncFilterDialog(SyncFilterManager* pManager,
						   Account* pAccount,
						   const WCHAR* pwszDefaultName);
	virtual ~SelectSyncFilterDialog();

public:
	const WCHAR* getName() const;

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

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
