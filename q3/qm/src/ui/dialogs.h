/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __DIALOG_H__
#define __DIALOG_H__

#include <qm.h>
#include <qmfolder.h>
#include <qmgoround.h>
#include <qmpassword.h>
#include <qmsyncfilter.h>

#include <qs.h>
#include <qsdialog.h>
#include <qsprofile.h>

#include "resourceinc.h"
#include "../model/addressbook.h"
#include "../model/color.h"
#include "../model/editmessage.h"
#include "../model/filter.h"
#include "../model/fixedformtext.h"
#include "../model/goround.h"
#include "../model/rule.h"
#include "../model/signature.h"
#include "../sync/autopilot.h"
#include "../uimodel/viewmodel.h"


namespace qm {

class DefaultDialog;
	class AccountDialog;
	class AddAddressDialog;
	class AddressBookAddressDialog;
	class AddressBookEntryDialog;
	class ArgumentDialog;
	class AttachmentDialog;
	class AutoPilotEntryDialog;
	class ColorDialog;
	class ConditionDialog;
	class ConfirmSendDialog;
	class CopyRuleTemplateDialog;
	class CreateAccountDialog;
	class CreateFolderDialog;
	class CreateSubAccountDialog;
	class CustomFilterDialog;
	class DetachDialog;
	class DialupDialog;
	class ExportDialog;
	class FilterDialog;
	class FindDialog;
	class FixedFormTextDialog;
	class GoRoundDialupDialog;
	class GoRoundEntryDialog;
	class ImportDialog;
	class InputBoxDialog;
	class MailFolderDialog;
	class MoveMessageDialog;
	class PasswordDialog;
	class ParameterDialog;
	class ProgressDialog;
	class RenameDialog;
	class ReplaceDialog;
	class ResourceDialog;
	class RuleDialog;
	class SelectAddressDialog;
	class SelectDialupEntryDialog;
	class SelectSyncFilterDialog;
	class SignatureDialog;
	class SyncFilterDialog;
#ifdef TABWINDOW
	class TabTitleDialog;
#endif
	class ViewsColumnDialog;
	class ViewsDialog;
	template<class T, class List> class AbstractListDialog;
		class AutoPilotDialog;
		class FiltersDialog;
		class FixedFormTextsDialog;
		class GoRoundDialog;
		class GoRoundCourseDialog;
		template<class T, class List, class Manager, class EditDialog> class RuleColorSetsDialog;
			class ColorSetsDialog;
			class RuleSetsDialog;
		template<class T, class List, class Container, class EditDialog> class RulesColorsDialog;
			class ColorsDialog;
			class RulesDialog;
		class SignaturesDialog;
		class SyncFiltersDialog;
		class SyncFilterSetsDialog;

class Account;
class Document;
class PasswordManager;
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
					   UINT nListId);
	virtual ~AbstractListDialog();

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

protected:
	List& getList();

protected:
	virtual qs::wstring_ptr getLabel(const T* p) const = 0;
	virtual std::auto_ptr<T> create() const = 0;
	virtual bool edit(T* p) const = 0;
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
	List list_;
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
				  PasswordManager* pPasswordManager,
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
	static void initProfileForClass(const WCHAR* pwszClass,
									qs::Profile* pProfile);

private:
	AccountDialog(const AccountDialog&);
	AccountDialog& operator=(const AccountDialog&);

private:
	Document* pDocument_;
	SubAccount* pSubAccount_;
	PasswordManager* pPasswordManager_;
	SyncFilterManager* pSyncFilterManager_;
	qs::Profile* pProfile_;
};


/****************************************************************************
 *
 * AddAddressDialog
 *
 */

class AddAddressDialog : public DefaultDialog
{
public:
	enum Type {
		TYPE_NEWENTRY,
		TYPE_NEWADDRESS
	};

public:
	explicit AddAddressDialog(AddressBook* pAddressBook);
	virtual ~AddAddressDialog();

public:
	Type getType() const;
	AddressBookEntry* getEntry() const;

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

protected:
	virtual LRESULT onOk();

private:
	LRESULT onNewEntry();
	LRESULT onNewAddress();
	LRESULT onEntriesSelChange();

private:
	void updateState();

private:
	AddAddressDialog(const AddAddressDialog&);
	AddAddressDialog& operator=(const AddAddressDialog&);

private:
	AddressBook* pAddressBook_;
	Type type_;
	AddressBookEntry* pEntry_;
};


/****************************************************************************
 *
 * AddressBookAddressDialog
 *
 */

class AddressBookAddressDialog : public DefaultDialog
{
public:
	AddressBookAddressDialog(AddressBook* pAddressBook,
							 AddressBookAddress* pAddress);
	virtual ~AddressBookAddressDialog();

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

protected:
	virtual LRESULT onOk();

private:
	LRESULT onAddressChange();

private:
	void updateState();

private:
	AddressBookAddressDialog(const AddressBookAddressDialog&);
	AddressBookAddressDialog& operator=(const AddressBookAddressDialog&);

private:
	AddressBook* pAddressBook_;
	AddressBookAddress* pAddress_;
};


/****************************************************************************
 *
 * AddressBookEntryDialog
 *
 */

class AddressBookEntryDialog : public AbstractListDialog<AddressBookAddress, AddressBookEntry::AddressList>
{
public:
	AddressBookEntryDialog(AddressBook* pAddressBook,
						   AddressBookEntry* pEntry);
	virtual ~AddressBookEntryDialog();

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

protected:
	virtual LRESULT onOk();

protected:
	virtual qs::wstring_ptr getLabel(const AddressBookAddress* p) const;
	virtual std::auto_ptr<AddressBookAddress> create() const;
	virtual bool edit(AddressBookAddress* p) const;
	virtual void updateState();

private:
	LRESULT onNameChange();

private:
	AddressBookEntryDialog(const AddressBookEntryDialog&);
	AddressBookEntryDialog& operator=(const AddressBookEntryDialog&);

private:
	AddressBook* pAddressBook_;
	AddressBookEntry* pEntry_;
};


/****************************************************************************
 *
 * ArgumentDialog
 *
 */

class ArgumentDialog : public DefaultDialog
{
public:
	ArgumentDialog(const WCHAR* pwszName,
				   const WCHAR* pwszValue);
	virtual ~ArgumentDialog();

public:
	const WCHAR* getName() const;
	const WCHAR* getValue() const;

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
	ArgumentDialog(const ArgumentDialog&);
	ArgumentDialog& operator=(const ArgumentDialog&);

private:
	qs::wstring_ptr wstrName_;
	qs::wstring_ptr wstrValue_;
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
 * AutoPilotDialog
 *
 */

class AutoPilotDialog : public AbstractListDialog<AutoPilotEntry, AutoPilotManager::EntryList>
{
public:
	AutoPilotDialog(AutoPilotManager* pManager,
					GoRound* pGoRound);
	virtual ~AutoPilotDialog();

protected:
	virtual LRESULT onOk();

protected:
	virtual qs::wstring_ptr getLabel(const AutoPilotEntry* p) const;
	virtual std::auto_ptr<AutoPilotEntry> create() const;
	virtual bool edit(AutoPilotEntry* p) const;

private:
	AutoPilotDialog(const AutoPilotDialog&);
	AutoPilotDialog& operator=(const AutoPilotDialog&);

private:
	AutoPilotManager* pManager_;
	GoRound* pGoRound_;
};


/****************************************************************************
 *
 * AutoPilotEntryDialog
 *
 */

class AutoPilotEntryDialog : public DefaultDialog
{
public:
	AutoPilotEntryDialog(AutoPilotEntry* pEntry,
						 GoRound* pGoRound);
	virtual ~AutoPilotEntryDialog();

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

protected:
	virtual LRESULT onOk();

private:
	LRESULT onCourseEditChange();
	LRESULT onCourseSelChange();
	LRESULT onIntervalChange();

private:
	void updateState();

private:
	AutoPilotEntryDialog(const AutoPilotEntryDialog&);
	AutoPilotEntryDialog& operator=(const AutoPilotEntryDialog&);

private:
	AutoPilotEntry* pEntry_;
	GoRound* pGoRound_;
};


/****************************************************************************
 *
 * ColorDialog
 *
 */

class ColorDialog : public DefaultDialog
{
public:
	ColorDialog(ColorEntry* pColor,
				Document* pDocument);
	virtual ~ColorDialog();

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
	LRESULT onChoose();
	LRESULT onConditionChange();
	LRESULT onColorChange();

private:
	void updateState();

private:
	ColorDialog(const ColorDialog&);
	ColorDialog& operator=(const ColorDialog&);

private:
	ColorEntry* pColor_;
};


/****************************************************************************
 *
 * ConditionDialog
 *
 */

class ConditionDialog : public DefaultDialog
{
public:
	explicit ConditionDialog(const WCHAR* pwszCondition);
	virtual ~ConditionDialog();

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
	ConditionDialog(const ConditionDialog&);
	ConditionDialog& operator=(const ConditionDialog&);

private:
	qs::wstring_ptr wstrCondition_;
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
 * CopyRuleTemplateDialog
 *
 */

class CopyRuleTemplateDialog :
	public DefaultDialog,
	public qs::NotifyHandler
{
public:
	CopyRuleTemplateDialog(const WCHAR* pwszName,
						   CopyRuleAction::ArgumentList* pListArgument);
	virtual ~CopyRuleTemplateDialog();

public:
	const WCHAR* getName() const;

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
	LRESULT onAdd();
	LRESULT onRemove();
	LRESULT onEdit();
	LRESULT onArgumentItemChanged(NMHDR* pnmhdr,
								  bool* pbHandled);

private:
	void updateState();

private:
	CopyRuleTemplateDialog(const CopyRuleTemplateDialog&);
	CopyRuleTemplateDialog& operator=(const CopyRuleTemplateDialog&);

private:
	qs::wstring_ptr wstrName_;
	CopyRuleAction::ArgumentList* pListArgument_;
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
	unsigned int getIndexBlockSize() const;

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
	unsigned int nIndexBlockSize_;
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
	
	enum Flag {
		FLAG_ALLOWREMOTE	= 0x01,
		FLAG_ALLOWLOCALSYNC	= 0x02
	};

public:
	CreateFolderDialog(Type type,
					   unsigned int nFlags);
	virtual ~CreateFolderDialog();

public:
	Type getType() const;
	const WCHAR* getName() const;
	bool isSyncable() const;

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
	unsigned int nFlags_;
	qs::wstring_ptr wstrName_;
	bool bSyncable_;
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
 * FilterDialog
 *
 */

class FilterDialog : public DefaultDialog
{
public:
	explicit FilterDialog(Filter* pFilter);
	virtual ~FilterDialog();

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
	LRESULT onConditionChange();
	LRESULT onNameChange();

private:
	void updateState();

private:
	FilterDialog(const FilterDialog&);
	FilterDialog& operator=(const FilterDialog&);

private:
	Filter* pFilter_;
};


/****************************************************************************
 *
 * FiltersDialog
 *
 */

class FiltersDialog : public AbstractListDialog<Filter, FilterManager::FilterList>
{
public:
	explicit FiltersDialog(FilterManager* pManager);
	virtual ~FiltersDialog();

protected:
	virtual LRESULT onOk();

protected:
	virtual qs::wstring_ptr getLabel(const Filter* p) const;
	virtual std::auto_ptr<Filter> create() const;
	virtual bool edit(Filter* p) const;

private:
	FiltersDialog(const FiltersDialog&);
	FiltersDialog& operator=(const FiltersDialog&);

private:
	FilterManager* pManager_;
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
	qs::Profile* pProfile_;
	bool bSupportRegex_;
	qs::wstring_ptr wstrFind_;
	bool bMatchCase_;
	bool bRegex_;
	bool bPrev_;
};


/****************************************************************************
 *
 * FixedFormTextDialog
 *
 */

class FixedFormTextDialog : public DefaultDialog
{
public:
	explicit FixedFormTextDialog(FixedFormText* pText);
	virtual ~FixedFormTextDialog();

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
	FixedFormTextDialog(const FixedFormTextDialog&);
	FixedFormTextDialog& operator=(const FixedFormTextDialog&);

private:
	FixedFormText* pText_;
};


/****************************************************************************
 *
 * FixedFormTextsDialog
 *
 */

class FixedFormTextsDialog : public AbstractListDialog<FixedFormText, FixedFormTextManager::TextList>
{
public:
	explicit FixedFormTextsDialog(FixedFormTextManager* pManager);
	virtual ~FixedFormTextsDialog();

protected:
	virtual LRESULT onOk();

protected:
	virtual qs::wstring_ptr getLabel(const FixedFormText* p) const;
	virtual std::auto_ptr<FixedFormText> create() const;
	virtual bool edit(FixedFormText* p) const;

private:
	FixedFormTextsDialog(const FixedFormTextsDialog&);
	FixedFormTextsDialog& operator=(const FixedFormTextsDialog&);

private:
	FixedFormTextManager* pManager_;
};


/****************************************************************************
 *
 * GoRoundDialog
 *
 */

class GoRoundDialog : public AbstractListDialog<GoRoundCourse, GoRound::CourseList>
{
public:
	GoRoundDialog(GoRound* pGoRound,
				  Document* pDocument,
				  SyncFilterManager* pSyncFilterManager);
	virtual ~GoRoundDialog();

protected:
	virtual LRESULT onOk();

protected:
	virtual qs::wstring_ptr getLabel(const GoRoundCourse* p) const;
	virtual std::auto_ptr<GoRoundCourse> create() const;
	virtual bool edit(GoRoundCourse* p) const;

private:
	GoRoundDialog(const GoRoundDialog&);
	GoRoundDialog& operator=(const GoRoundDialog&);

private:
	GoRound* pGoRound_;
	Document* pDocument_;
	SyncFilterManager* pSyncFilterManager_;
};


/****************************************************************************
 *
 * GoRoundCourseDialog
 *
 */

class GoRoundCourseDialog : public AbstractListDialog<GoRoundEntry, GoRoundCourse::EntryList>
{
public:
	GoRoundCourseDialog(GoRoundCourse* pCourse,
						Document* pDocument,
						SyncFilterManager* pSyncFilterManager);
	virtual ~GoRoundCourseDialog();

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

protected:
	virtual LRESULT onOk();

protected:
	virtual qs::wstring_ptr getLabel(const GoRoundEntry* p) const;
	virtual std::auto_ptr<GoRoundEntry> create() const;
	virtual bool edit(GoRoundEntry* p) const;
	virtual void updateState();

private:
	LRESULT onDialup();
	LRESULT onNameChange();

private:
	GoRoundCourseDialog(const GoRoundCourseDialog&);
	GoRoundCourseDialog& operator=(const GoRoundCourseDialog&);

private:
	GoRoundCourse* pCourse_;
	Document* pDocument_;
	SyncFilterManager* pSyncFilterManager_;
};


/****************************************************************************
 *
 * GoRoundDialupDialog
 *
 */

class GoRoundDialupDialog : public DefaultDialog
{
public:
	GoRoundDialupDialog(GoRoundDialup* pDialup,
						bool bNoDialup);
	virtual ~GoRoundDialupDialog();

public:
	bool isNoDialup() const;

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

protected:
	virtual LRESULT onOk();

private:
	LRESULT onTypeSelect(UINT nId);

private:
	void updateState();

private:
	GoRoundDialupDialog(const GoRoundDialupDialog&);
	GoRoundDialupDialog& operator=(const GoRoundDialupDialog&);

private:
	GoRoundDialup* pDialup_;
	bool bNoDialup_;
};


/****************************************************************************
 *
 * GoRoundEntryDialog
 *
 */

class GoRoundEntryDialog : public DefaultDialog
{
public:
	GoRoundEntryDialog(GoRoundEntry* pEntry,
					   Document* pDocument,
					   SyncFilterManager* pSyncFilterManager);
	virtual ~GoRoundEntryDialog();

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
	LRESULT onAccountEditChange();
	LRESULT onAccountSelChange();
	LRESULT onSelectFolderClicked();

private:
	void updateState();
	void updateSubAccount(Account* pAccount);
	void updateFolder(Account* pAccount);
	void updateFilter();

private:
	GoRoundEntryDialog(const GoRoundEntryDialog&);
	GoRoundEntryDialog& operator=(const GoRoundEntryDialog&);

private:
	GoRoundEntry* pEntry_;
	Document* pDocument_;
	SyncFilterManager* pSyncFilterManager_;
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
	MoveMessageDialog(Document* pDocument,
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
	Document* pDocument_;
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
 * ParameterDialog
 *
 */

class ParameterDialog : public DefaultDialog
{
public:
	ParameterDialog(const WCHAR* pwszName,
					const WCHAR* pwszValue);
	virtual ~ParameterDialog();

public:
	const WCHAR* getValue() const;

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

protected:
	virtual LRESULT onOk();

private:
	ParameterDialog(const ParameterDialog&);
	ParameterDialog& operator=(const ParameterDialog&);

private:
	qs::wstring_ptr wstrName_;
	qs::wstring_ptr wstrValue_;
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
	bool isCanceled();
	void setTitle(UINT nId);
	void setMessage(UINT nId);
	void setMessage(const WCHAR* pwszMessage);
	void setRange(unsigned int nMin,
				  unsigned int nMax);
	void setPos(unsigned int n);
	void setStep(unsigned int n);
	void step();

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
 * RuleDialog
 *
 */

class RuleDialog : public DefaultDialog
{
public:
	RuleDialog(Rule* pRule,
			   Document* pDocument);
	virtual ~RuleDialog();

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
	LRESULT onTemplate();
	LRESULT onActionSelChange();
	LRESULT onAccountEditChange();
	LRESULT onAccountSelChange();
	LRESULT onConditionChange();
	LRESULT onFolderEditChange();
	LRESULT onFolderSelChange();
	LRESULT onMacroChange();

private:
	void updateState(bool bUpdateFolder);
	void updateFolder(Account* pAccount);

private:
	RuleDialog(const RuleDialog&);
	RuleDialog& operator=(const RuleDialog&);

private:
	Rule* pRule_;
	Document* pDocument_;
	qs::wstring_ptr wstrTemplate_;
	CopyRuleAction::ArgumentList listArgument_;
	bool bInit_;
};


/****************************************************************************
 *
 * RuleColorSetsDialog
 *
 */

template<class T, class List, class Manager, class EditDialog>
class RuleColorSetsDialog : public AbstractListDialog<T, List>
{
public:
	typedef const List& (Manager::*PFN_GET)();
	typedef void (Manager::*PFN_SET)(List&);

public:
	RuleColorSetsDialog(Manager* pManager,
						Document* pDocument,
						UINT nTitleId,
						PFN_GET pfnGet,
						PFN_SET pfnSet);
	virtual ~RuleColorSetsDialog();

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

protected:
	virtual LRESULT onOk();

protected:
	virtual qs::wstring_ptr getLabel(const T* p) const;
	virtual std::auto_ptr<T> create() const;
	virtual bool edit(T* p) const;

private:
	RuleColorSetsDialog(const RuleColorSetsDialog&);
	RuleColorSetsDialog& operator=(const RuleColorSetsDialog&);

private:
	Manager* pManager_;
	Document* pDocument_;
	UINT nTitleId_;
	PFN_SET pfnSet_;
};


/****************************************************************************
 *
 * ColorSetsDialog
 *
 */

class ColorSetsDialog : public RuleColorSetsDialog<ColorSet, ColorManager::ColorSetList, ColorManager, ColorsDialog>
{
public:
	ColorSetsDialog(ColorManager* pColorManager,
					Document* pDocument);
};


/****************************************************************************
 *
 * RuleSetsDialog
 *
 */

class RuleSetsDialog : public RuleColorSetsDialog<RuleSet, RuleManager::RuleSetList, RuleManager, RulesDialog>
{
public:
	RuleSetsDialog(RuleManager* pRuleManager,
				   Document* pDocument);
};


/****************************************************************************
 *
 * RulesColorsDialog
 *
 */

template<class T, class List, class Container, class EditDialog>
class RulesColorsDialog : public AbstractListDialog<T, List>
{
public:
	typedef const List& (Container::*PFN_GET)() const;
	typedef void (Container::*PFN_SET)(List&);

public:
	RulesColorsDialog(Container* pContainer,
					  Document* pDocument,
					  UINT nTitleId,
					  PFN_GET pfnGet,
					  PFN_SET pfnSet);
	virtual ~RulesColorsDialog();

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

protected:
	virtual LRESULT onOk();

protected:
	virtual qs::wstring_ptr getLabel(const T* p) const;
	virtual std::auto_ptr<T> create() const;
	virtual bool edit(T* p) const;
	virtual void updateState();

private:
	LRESULT onAccountEditChange();
	LRESULT onAccountSelChange();

private:
	void updateFolder(Account* pAccount);

private:
	RulesColorsDialog(const RulesColorsDialog&);
	RulesColorsDialog& operator=(const RulesColorsDialog&);

private:
	Container* pContainer_;
	Document* pDocument_;
	UINT nTitleId_;
	PFN_SET pfnSet_;
};


/****************************************************************************
 *
 * ColorsDialog
 *
 */

class ColorsDialog : public RulesColorsDialog<ColorEntry, ColorSet::ColorList, ColorSet, ColorDialog>
{
public:
	ColorsDialog(ColorSet* pColorSet,
				 Document* pDocument);
};


/****************************************************************************
 *
 * RulesDialog
 *
 */

class RulesDialog : public RulesColorsDialog<Rule, RuleSet::RuleList, RuleSet, RuleDialog>
{
public:
	RulesDialog(RuleSet* pRuleSet,
				Document* pDocument);
};


/****************************************************************************
 *
 * SelectAddressDialog
 *
 */

class SelectAddressDialog :
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
		void setType(Type type);
	
	private:
		Item(const Item&);
		Item& operator=(const Item&);
	
	private:
		qs::wstring_ptr wstrValue_;
		Type type_;
	};

public:
	SelectAddressDialog(AddressBook* pAddressBook,
						qs::Profile* pProfile,
						const WCHAR* pwszAddress[]);
	virtual ~SelectAddressDialog();

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
	SelectAddressDialog(const SelectAddressDialog&);
	SelectAddressDialog& operator=(const SelectAddressDialog&);

private:
	class AddressListWindow :
		public qs::WindowBase,
		public qs::DefaultWindowHandler
	{
	public:
		explicit AddressListWindow(SelectAddressDialog* pDialog);
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
		SelectAddressDialog* pDialog_;
	};
	friend class AddressListWindow;
	
	class SelectedAddressListWindow :
		public qs::WindowBase,
		public qs::DefaultWindowHandler,
		public qs::NotifyHandler
	{
	public:
		explicit SelectedAddressListWindow(SelectAddressDialog* pDialog);
		virtual ~SelectedAddressListWindow();
	
	public:
		virtual bool preSubclassWindow();
		virtual LRESULT windowProc(UINT uMsg,
								   WPARAM wParam,
								   LPARAM lParam);
	
	protected:
		LRESULT onContextMenu(HWND hwnd,
							  const POINT& pt);
		LRESULT onDestroy();
		LRESULT onLButtonDown(UINT nFlags,
							  const POINT& pt);
	
	public:
		virtual LRESULT onNotify(NMHDR* pnmhdr,
								 bool* pbHandled);
	
	private:
#if defined _WIN32_WCE && _WIN32_WCE >= 400 && defined _WIN32_WCE_PSPC
		LRESULT onRecognizeGesture(NMHDR* pnmhdr,
								   bool* pbHandled);
#endif
	
	private:
		SelectedAddressListWindow(const SelectedAddressListWindow&);
		SelectedAddressListWindow& operator=(const SelectedAddressListWindow&);
	
	private:
		SelectAddressDialog* pDialog_;
	};
	friend class SelectedAddressListWindow;

private:
	AddressBook* pAddressBook_;
	qs::Profile* pProfile_;
	unsigned int nSort_;
	qs::wstring_ptr wstrCategory_;
	qs::wstring_ptr wstrFilter_;
	AddressList listAddress_[3];
	AddressListWindow wndAddressList_;
	SelectedAddressListWindow wndSelectedAddressList_;
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


/****************************************************************************
 *
 * SignatureDialog
 *
 */

class SignatureDialog : public DefaultDialog
{
public:
	SignatureDialog(Signature* pSignature,
					Document* pDocument);
	virtual ~SignatureDialog();

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
	SignatureDialog(const SignatureDialog&);
	SignatureDialog& operator=(const SignatureDialog&);

private:
	Signature* pSignature_;
	Document* pDocument_;
};


/****************************************************************************
 *
 * SignaturesDialog
 *
 */

class SignaturesDialog : public AbstractListDialog<Signature, SignatureManager::SignatureList>
{
public:
	SignaturesDialog(SignatureManager* pSignatureManager,
					 Document* pDocument);
	virtual ~SignaturesDialog();

protected:
	virtual LRESULT onOk();

protected:
	virtual qs::wstring_ptr getLabel(const Signature* p) const;
	virtual std::auto_ptr<Signature> create() const;
	virtual bool edit(Signature* p) const;

private:
	SignaturesDialog(const SignaturesDialog&);
	SignaturesDialog& operator=(const SignaturesDialog&);

private:
	SignatureManager* pSignatureManager_;
	Document* pDocument_;
};


/****************************************************************************
 *
 * SyncFilterDialog
 *
 */

class SyncFilterDialog : public DefaultDialog
{
public:
	explicit SyncFilterDialog(SyncFilter* pSyncFilter);
	virtual ~SyncFilterDialog();

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
	LRESULT onActionSelChange();

private:
	void updateState();

private:
	SyncFilterDialog(const SyncFilterDialog&);
	SyncFilterDialog& operator=(const SyncFilterDialog&);

private:
	SyncFilter* pSyncFilter_;
};


/****************************************************************************
 *
 * SyncFiltersDialog
 *
 */

class SyncFiltersDialog : public AbstractListDialog<SyncFilter, SyncFilterSet::FilterList>
{
public:
	explicit SyncFiltersDialog(SyncFilterSet* pSyncFilterSet);
	virtual ~SyncFiltersDialog();

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

protected:
	virtual LRESULT onOk();

protected:
	virtual qs::wstring_ptr getLabel(const SyncFilter* p) const;
	virtual std::auto_ptr<SyncFilter> create() const;
	virtual bool edit(SyncFilter* p) const;
	virtual void updateState();

private:
	LRESULT onNameChange();

private:
	SyncFiltersDialog(const SyncFiltersDialog&);
	SyncFiltersDialog& operator=(const SyncFiltersDialog&);

private:
	SyncFilterSet* pSyncFilterSet_;
};


/****************************************************************************
 *
 * SyncFilterSetsDialog
 *
 */

class SyncFilterSetsDialog : public AbstractListDialog<SyncFilterSet, SyncFilterManager::FilterSetList>
{
public:
	explicit SyncFilterSetsDialog(SyncFilterManager* pSyncFilterManager);
	virtual ~SyncFilterSetsDialog();

protected:
	virtual LRESULT onOk();

protected:
	virtual qs::wstring_ptr getLabel(const SyncFilterSet* p) const;
	virtual std::auto_ptr<SyncFilterSet> create() const;
	virtual bool edit(SyncFilterSet* p) const;

private:
	SyncFilterSetsDialog(const SyncFilterSetsDialog&);
	SyncFilterSetsDialog& operator=(const SyncFilterSetsDialog&);

private:
	SyncFilterManager* pSyncFilterManager_;
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
	void cloneColumns(const ViewColumnList& listColumn,
					  ViewColumnList* pListColumn);
	ViewDataItem* getDefaultItem();

private:
	ViewsDialog(const ViewsDialog&);
	ViewsDialog& operator=(const ViewsDialog&);

private:
	ViewModelManager* pViewModelManager_;
	ViewModel* pViewModel_;
	ViewColumnList listColumn_;
};

}

#include "dialogs.inl"

#endif // __DIALOG_H__
