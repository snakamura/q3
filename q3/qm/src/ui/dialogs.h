/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __DIALOGS_H__
#define __DIALOGS_H__

#include <qm.h>
#include <qmfolder.h>
#include <qmpassword.h>

#include <qs.h>
#include <qsdialog.h>
#include <qsprofile.h>

#include "resourceinc.h"
#include "../model/editmessage.h"
#include "../uimodel/viewmodel.h"


namespace qm {

class DefaultDialog;
	template<class T, class List> class AbstractListDialog;
#ifndef _WIN32_WCE
	class ArchiveDialog;
#endif
	class AttachmentDialog;
	class CertificateDialog;
	class ConfirmSendDialog;
	class CustomFilterDialog;
	class DetachDialog;
	class DialupDialog;
	class ExportDialog;
	class FindDialog;
	class ImportDialog;
	class InputBoxDialog;
		class SingleLineInputBoxDialog;
		class MultiLineInputBoxDialog;
	class LabelDialog;
	class MailFolderDialog;
	class MoveMessageDialog;
	class PasswordDialog;
	class ProgressDialog;
	class RenameDialog;
	class ReplaceDialog;
	class ResourceDialog;
	class SelectBoxDialog;
	class SelectDialupEntryDialog;
	class SelectSyncFilterDialog;
#ifdef TABWINDOW
	class TabTitleDialog;
#endif
	class ViewsColumnDialog;
	class ViewsDialog;

class Account;
class SyncFilterManager;
class TemplateManager;


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
 * AbstractListDialog
 *
 */

template<class T, class List>
class AbstractListDialog : public DefaultDialog
{
protected:
	AbstractListDialog(UINT nId,
					   UINT nListId,
					   bool bFocus);
	virtual ~AbstractListDialog();

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

protected:
	List& getList();
	void refresh();

protected:
	virtual qs::wstring_ptr getLabel(const T* p) const = 0;
	virtual std::auto_ptr<T> create() const = 0;
	virtual T* edit(T* p) const = 0;
	virtual void updateState();

private:
	LRESULT onAdd();
	LRESULT onRemove();
	LRESULT onEdit();
	LRESULT onUp();
	LRESULT onDown();
	LRESULT onSelChange();

private:
	AbstractListDialog(const AbstractListDialog&);
	AbstractListDialog& operator=(const AbstractListDialog&);

private:
	UINT nListId_;
	bool bFocus_;
	List list_;
};


#ifndef _WIN32_WCE
/****************************************************************************
 *
 * ArchiveDialog
 *
 */

class ArchiveDialog : public DefaultDialog
{
public:
	ArchiveDialog(const WCHAR* pwszFileName);
	virtual ~ArchiveDialog();

public:
	const WCHAR* getFileName() const;

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

protected:
	virtual LRESULT onOk();

private:
	LRESULT onFileNameChange();

private:
	void updateState();

private:
	ArchiveDialog(const ArchiveDialog&);
	ArchiveDialog& operator=(const ArchiveDialog&);

private:
	qs::wstring_ptr wstrFileName_;
};
#endif


/****************************************************************************
 *
 * AttachmentDialog
 *
 */

class AttachmentDialog :
	public DefaultDialog,
	public qs::NotifyHandler
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
 * CertificateDialog
 *
 */

class CertificateDialog : public DefaultDialog
{
public:
	explicit CertificateDialog(const WCHAR* pwszCertificate);
	virtual ~CertificateDialog();

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

private:
	CertificateDialog(const CertificateDialog&);
	CertificateDialog& operator=(const CertificateDialog&);

private:
	qs::wstring_ptr wstrCertificate_;
};


/****************************************************************************
 *
 * ConfirmSendDialog
 *
 */

class ConfirmSendDialog : public DefaultDialog
{
public:
	enum ID {
		ID_SEND		= 1000,
		ID_SAVE		= 1001,
		ID_DISCARD	= 1002
	};

public:
	ConfirmSendDialog();
	virtual ~ConfirmSendDialog();

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

private:
	LRESULT onSend();
	LRESULT onSave();
	LRESULT onDiscard();

private:
	ConfirmSendDialog(const ConfirmSendDialog&);
	ConfirmSendDialog& operator=(const ConfirmSendDialog&);
};


/****************************************************************************
 *
 * CustomFilterDialog
 *
 */

class CustomFilterDialog : public DefaultDialog
{
public:
	explicit CustomFilterDialog(const WCHAR* pwszCondition);
	virtual ~CustomFilterDialog();

public:
	const WCHAR* getCondition() const;

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

protected:
	virtual LRESULT onOk();

private:
	LRESULT onEdit();

private:
	CustomFilterDialog(const CustomFilterDialog&);
	CustomFilterDialog& operator=(const CustomFilterDialog&);

private:
	qs::wstring_ptr wstrCondition_;
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
	bool isOpenFolder() const;

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
	bool bOpenFolder_;
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
	ExportDialog(Account* pAccount,
				 const TemplateManager* pTemplateManager,
				 qs::Profile* pProfile,
				 bool bSingleMessage);
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
	LRESULT onTemplateSelChange();
	LRESULT onEncodingEditChange();
	LRESULT onEncodingSelChange();

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
	Account* pAccount_;
	const TemplateManager* pTemplateManager_;
	qs::Profile* pProfile_;
	bool bSingleMessage_;
	qs::wstring_ptr wstrPath_;
	unsigned int nFlags_;
	qs::wstring_ptr wstrTemplate_;
	qs::wstring_ptr wstrEncoding_;
};


/****************************************************************************
 *
 * FindDialog
 *
 */

class FindDialog : public DefaultDialog
{
public:
	class Callback
	{
	public:
		virtual ~Callback();
	
	public:
		virtual void statusChanged(const WCHAR* pwszFind,
								   bool bMatchCase,
								   bool bRegex) = 0;
	};

public:
	FindDialog(qs::Profile* pProfile,
			   bool bSupportRegex,
			   Callback* pCallback);
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
	LRESULT onFind(UINT nId);
	LRESULT onFindChange();
	LRESULT onFindSelChange();
	LRESULT onMatchCaseChange();
	LRESULT onRegexChange();

private:
	void updateState();
	void notifyCallback();

private:
	FindDialog(const FindDialog&);
	FindDialog& operator=(const FindDialog&);

private:
	qs::Profile* pProfile_;
	bool bSupportRegex_;
	Callback* pCallback_;
	qs::ImeWindow wndFind_;
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
	ImportDialog(const WCHAR* pwszPath,
				 qs::Profile* pProfile);
	virtual ~ImportDialog();

public:
	const WCHAR* getPath() const;
	bool isMultiple() const;
	const WCHAR* getEncoding() const;
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
	qs::Profile* pProfile_;
	qs::wstring_ptr wstrPath_;
	bool bMultiple_;
	qs::wstring_ptr wstrEncoding_;
	unsigned int nFlags_;
};


/****************************************************************************
 *
 * InputBoxDialog
 *
 */

class InputBoxDialog : public DefaultDialog
{
protected:
	InputBoxDialog(UINT nId,
				   const WCHAR* pwszTitle,
				   const WCHAR* pwszMessage,
				   const WCHAR* pwszValue,
				   bool bAllowEmpty);

public:
	virtual ~InputBoxDialog();

public:
	const WCHAR* getValue() const;

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

protected:
	virtual LRESULT onOk();

protected:
	virtual qs::wstring_ptr normalizeValue(const WCHAR* pwszValue) const;
	virtual qs::wstring_ptr unnormalizeValue(const WCHAR* pwszValue) const;

private:
	LRESULT onValueChange();

private:
	void updateState();

private:
	InputBoxDialog(const InputBoxDialog&);
	InputBoxDialog& operator=(const InputBoxDialog&);

private:
	qs::wstring_ptr wstrTitle_;
	qs::wstring_ptr wstrMessage_;
	qs::wstring_ptr wstrValue_;
	bool bAllowEmpty_;
};


/****************************************************************************
 *
 * SingleLineInputBoxDialog
 *
 */

class SingleLineInputBoxDialog : public InputBoxDialog
{
public:
	SingleLineInputBoxDialog(const WCHAR* pwszTitle,
							 const WCHAR* pwszMessage,
							 const WCHAR* pwszValue,
							 bool bAllowEmpty);
	virtual ~SingleLineInputBoxDialog();

private:
	SingleLineInputBoxDialog(const SingleLineInputBoxDialog&);
	SingleLineInputBoxDialog& operator=(const SingleLineInputBoxDialog&);
};


/****************************************************************************
 *
 * MultiLineInputBoxDialog
 *
 */

class MultiLineInputBoxDialog : public InputBoxDialog
{
public:
	MultiLineInputBoxDialog(const WCHAR* pwszTitle,
							const WCHAR* pwszMessage,
							const WCHAR* pwszValue,
							bool bAllowEmpty,
							qs::Profile* pProfile,
							const WCHAR* pwszSection);
	virtual ~MultiLineInputBoxDialog();

public:
	virtual INT_PTR dialogProc(UINT uMsg,
							   WPARAM wParam,
							   LPARAM lParam);

protected:
	virtual LRESULT onDestroy();
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

protected:
	LRESULT onSize(UINT nFlags,
				   int cx,
				   int cy);

private:
	void layout();

protected:
	virtual qs::wstring_ptr normalizeValue(const WCHAR* pwszValue) const;
	virtual qs::wstring_ptr unnormalizeValue(const WCHAR* pwszValue) const;

private:
	MultiLineInputBoxDialog(const MultiLineInputBoxDialog&);
	MultiLineInputBoxDialog& operator=(const MultiLineInputBoxDialog&);

private:
	qs::Profile* pProfile_;
	const WCHAR* pwszSection_;
};


/****************************************************************************
 *
 * LabelDialog
 *
 */

class LabelDialog : public DefaultDialog
{
public:
	LabelDialog(const WCHAR* pwszLabel,
				qs::Profile* pProfile);
	virtual ~LabelDialog();

public:
	const WCHAR* getLabel() const;

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

protected:
	virtual LRESULT onOk();

private:
	LabelDialog(const LabelDialog&);
	LabelDialog& operator=(const LabelDialog&);

private:
	qs::wstring_ptr wstrLabel_;
	qs::Profile* pProfile_;
};


/****************************************************************************
 *
 * MailFolderDialog
 *
 */

class MailFolderDialog : public qs::DefaultDialog
{
public:
	MailFolderDialog(HINSTANCE hInstResource,
					 const WCHAR* pwszMailFolder);
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
	MoveMessageDialog(AccountManager* pAccountManager,
					  Account* pAccount,
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
	bool update(Folder* pFolderSelected);
	bool insertAccount(HWND hwnd,
					   Account* pAccount,
					   Folder* pFolderSelected);
	bool insertFolders(HWND hwnd,
					   HTREEITEM hItem,
					   Account* pAccount,
					   Folder* pFolderSelected);
	void updateState();

private:
	MoveMessageDialog(const MoveMessageDialog&);
	MoveMessageDialog& operator=(const MoveMessageDialog&);

private:
	AccountManager* pAccountManager_;
	Account* pAccount_;
	qs::Profile* pProfile_;
	NormalFolder* pFolder_;
	bool bCopy_;
	bool bShowHidden_;
};


/****************************************************************************
 *
 * PasswordDialog
 *
 */

class PasswordDialog : public DefaultDialog
{
public:
	explicit PasswordDialog(const WCHAR* pwszHint);
	virtual ~PasswordDialog();

public:
	const WCHAR* getPassword() const;
	PasswordState getState() const;

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

protected:
	virtual LRESULT onOk();

private:
	LRESULT onPasswordChange();

private:
	void updateState();

private:
	PasswordDialog(const PasswordDialog&);
	PasswordDialog& operator=(const PasswordDialog&);

private:
	qs::wstring_ptr wstrHint_;
	qs::wstring_ptr wstrPassword_;
	PasswordState state_;
};


/****************************************************************************
 *
 * ProgressDialog
 *
 */

class ProgressDialog : public DefaultDialog
{
public:
	ProgressDialog();
	virtual ~ProgressDialog();

public:
	bool init(HWND hwnd);
	void term();
	bool isCanceled();
	void setCancelable(bool bCancelable);
	void setTitle(UINT nId);
	void setMessage(UINT nId);
	void setMessage(const WCHAR* pwszMessage);
	void setRange(size_t nMin,
				  size_t nMax);
	void setPos(size_t n);
	void setStep(size_t n);
	void step();

protected:
	virtual LRESULT onDestroy();
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

protected:
	virtual LRESULT onCancel();

private:
	void pumpMessage(HWND hwnd);

private:
	ProgressDialog(const ProgressDialog&);
	ProgressDialog& operator=(const ProgressDialog&);

private:
	bool bCancelable_;
	bool bCanceled_;
	size_t nLastMessagePumpPos_;
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
	LRESULT onNameChange();

private:
	void updateState();

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
 * ResourceDialog
 *
 */

class ResourceDialog : public DefaultDialog
{
public:
	typedef std::vector<std::pair<qs::WSTRING, bool> > ResourceList;

public:
	explicit ResourceDialog(ResourceList& listResource);
	virtual ~ResourceDialog();

public:
	bool isBackup() const;

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

protected:
	virtual LRESULT onOk();

private:
	LRESULT onCheckAll();
	LRESULT onClearAll();

private:
	ResourceDialog(const ResourceDialog&);
	ResourceDialog& operator=(const ResourceDialog&);

private:
	ResourceList& listResource_;
	bool bBackup_;
};


/****************************************************************************
 *
 * SelectBoxDialog
 *
 */

class SelectBoxDialog : public DefaultDialog
{
public:
	enum Type {
		TYPE_LIST,
		TYPE_DROPDOWNLIST,
		TYPE_DROPDOWN
	};

public:
	typedef std::vector<const WCHAR*> CandidateList;

public:
	SelectBoxDialog(Type type,
					const WCHAR* pwszMessage,
					const CandidateList& listCandidate,
					const WCHAR* pwszValue);
	virtual ~SelectBoxDialog();

public:
	const WCHAR* getValue() const;

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

protected:
	virtual LRESULT onOk();

private:
	HWND getList();

private:
	SelectBoxDialog(const SelectBoxDialog&);
	SelectBoxDialog& operator=(const SelectBoxDialog&);

private:
	Type type_;
	qs::wstring_ptr wstrMessage_;
	const CandidateList& listCandidate_;
	qs::wstring_ptr wstrValue_;
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
	SyncFilterManager* pManager_;
	qs::wstring_ptr wstrName_;
};


#ifdef QMTABWINDOW
/****************************************************************************
 *
 * TabTitleDialog
 *
 */

class TabTitleDialog : public DefaultDialog
{
public:
	explicit TabTitleDialog(const WCHAR* pwszTitle);
	virtual ~TabTitleDialog();

public:
	const WCHAR* getTitle() const;

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

protected:
	virtual LRESULT onOk();

private:
	TabTitleDialog(const TabTitleDialog&);
	TabTitleDialog& operator=(const TabTitleDialog&);

private:
	qs::wstring_ptr wstrTitle_;
};
#endif


/****************************************************************************
 *
 * ViewsColumnDialog
 *
 */

class ViewsColumnDialog : public DefaultDialog
{
public:
	explicit ViewsColumnDialog(ViewColumn* pColumn);
	virtual ~ViewsColumnDialog();

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

protected:
	virtual LRESULT onOk();

private:
	LRESULT onTypeSelChange();

private:
	void updateState();

private:
	ViewsColumnDialog(const ViewsColumnDialog&);
	ViewsColumnDialog& operator=(const ViewsColumnDialog&);

private:
	ViewColumn* pColumn_;
};


/****************************************************************************
 *
 * ViewsDialog
 *
 */

class ViewsDialog :
	public DefaultDialog,
	public qs::NotifyHandler
{
public:
	ViewsDialog(ViewModelManager* pViewModelManager,
				ViewModel* pViewModel);
	virtual ~ViewsDialog();

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

public:
	virtual LRESULT onNotify(NMHDR* pnmhdr,
							 bool* pbHandled);

protected:
	virtual LRESULT onDestroy();
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

protected:
	virtual LRESULT onOk();

private:
	LRESULT onAdd();
	LRESULT onRemove();
	LRESULT onEdit();
	LRESULT onUp();
	LRESULT onDown();
	LRESULT onAsDefault();
	LRESULT onApplyDefault();
	LRESULT onInherit();
	LRESULT onApplyToAll();
	LRESULT onApplyToChildren();
	LRESULT onColumnsDblClk(NMHDR* pnmhdr,
							bool* pbHandled);
	LRESULT onColumnsItemChanged(NMHDR* pnmhdr,
								 bool* pbHandled);

private:
	void update();
	void updateState();
	void setColumns(const ViewColumnList& listColumn);
	void applyToViewModel(ViewModel* pViewModel);
	void retrieveFromViewModel(const ViewModel* pViewModel);
	ViewDataItem* getDefaultItem();

private:
	static void cloneColumns(const ViewColumnList& listColumn,
							 ViewColumnList* pListColumn);

private:
	ViewsDialog(const ViewsDialog&);
	ViewsDialog& operator=(const ViewsDialog&);

private:
	ViewModelManager* pViewModelManager_;
	ViewModel* pViewModel_;
	ViewColumnList listColumn_;
	unsigned int nSort_;
	ViewDataItem::Mode mode_[ViewModel::MODETYPE_COUNT];
};

}

#include "dialogs.inl"

#endif // __DIALOGS_H__
