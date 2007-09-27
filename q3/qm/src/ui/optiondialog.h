/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __OPTIONDIALOG_H__
#define __OPTIONDIALOG_H__

#include <qm.h>
#include <qmgoround.h>
#include <qmsyncfilter.h>

#include <qsdialog.h>
#include <qsprofile.h>

#include "dialogs.h"
#include "uiutil.h"
#include "../model/color.h"
#include "../model/filter.h"
#include "../model/fixedformtext.h"
#include "../model/goround.h"
#include "../model/rule.h"
#include "../model/signature.h"
#include "../sync/autopilot.h"


namespace qm {

class TextColorDialog;
class OptionDialog;
class OptionDialogPanel;
	template<class Dialog> class AbstractOptionDialogPanel;
class OptionDialogContext;
class OptionDialogManager;
class OptionAddressBookDialog;
class OptionConfirmDialog;
class OptionFolderDialog;
class OptionHeaderDialog;
#ifndef _WIN32_WCE
class OptionJunkDialog;
#endif
class OptionListDialog;
class OptionMiscDialog;
class OptionMisc2Dialog;
class OptionSearchDialog;
class OptionSecurityDialog;
class OptionSyncDialog;
class AbstractOptionTextDialog;
	class OptionEditDialog;
	class OptionMessageDialog;
	class OptionPreviewDialog;
class SecurityDialog;
class OptionEdit2Dialog;
#ifdef QMTABWINDOW
class OptionTabDialog;
#endif
template<class T, class List, class Manager, class EditDialog> class RuleColorSetsDialog;
template<class T, class List, class Container, class EditDialog> class RulesColorsDialog;
class ColorSetsDialog;
class ColorsDialog;
class ColorDialog;
class RuleSetsDialog;
class RulesDialog;
class RuleDialog;
class CopyRuleTemplateDialog;
class ArgumentDialog;
class AutoPilotDialog;
class AutoPilotEntryDialog;
class FiltersDialog;
class FilterDialog;
class FixedFormTextsDialog;
class FixedFormTextDialog;
class GoRoundDialog;
class GoRoundCourseDialog;
class GoRoundEntryDialog;
class GoRoundDialupDialog;
class SignaturesDialog;
class SignatureDialog;
class SyncFilterSetsDialog;
class SyncFiltersDialog;
class SyncFilterDialog;
class LayoutUtil;

class AddressBook;
class AddressBookFrameWindowManager;
class Document;
class EditFrameWindowManager;
class FolderComboBox;
class FolderListWindow;
class FolderWindow;
class JunkFilter;
class ListWindow;
class MainWindow;
class MessageFrameWindowManager;
class MessageWindow;
class Recents;
#ifdef QMRECENTSWINDOW
class RecentsWindowManager;
#endif
class Security;
#ifdef QMTABWINDOW
class TabWindow;
#endif
class UpdateChecker;


/****************************************************************************
 *
 * TextColorDialog
 *
 */

class TextColorDialog : public DefaultDialog
{
public:
	class Data
	{
	public:
		Data(qs::Profile* pProfile,
			 const WCHAR* pwszSection,
			 bool bText);
		Data(const Data& data);
		~Data();
	
	public:
		Data& operator=(const Data& data);
	
	public:
		void save(qs::Profile* pProfile,
				  const WCHAR* pwszSection) const;
	
	private:
		bool bText_;
		bool bSystemColor_;
		COLORREF crForeground_;
		COLORREF crBackground_;
		qs::wstring_ptr wstrQuote_[2];
		COLORREF crQuote_[2];
		COLORREF crLink_;
		
		friend class TextColorDialog;
	};

public:
	explicit TextColorDialog(const Data& data);
	virtual ~TextColorDialog();

public:
	const Data& getData() const;

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
	LRESULT onCtlColorEdit(HDC hdc,
						   HWND hwnd);
	LRESULT onCtlColorStatic(HDC hdc,
							 HWND hwnd);

protected:
	virtual LRESULT onOk();

private:
	LRESULT onChoose(UINT nId);
	LRESULT onColor(UINT nId);

private:
	void updateState();
	void updateBackgroundBrush();

private:
	TextColorDialog(const TextColorDialog&);
	TextColorDialog& operator=(const TextColorDialog&);

private:
	Data data_;
	HBRUSH hbrBackground_;
};


/****************************************************************************
 *
 * OptionDialog
 *
 */

class OptionDialog :
	public DefaultDialog,
	public qs::NotifyHandler
{
public:
	enum Panel {
		PANEL_NONE			= -1,
		
		PANEL_FOLDER,
		PANEL_LIST,
		PANEL_PREVIEW,
		PANEL_MESSAGE,
		PANEL_HEADER,
		PANEL_EDIT,
		PANEL_EDIT2,
#ifdef QMTABWINDOW
		PANEL_TAB,
#endif
		PANEL_ADDRESSBOOK,
		PANEL_RULES,
		PANEL_COLORS,
		PANEL_GOROUND,
		PANEL_SIGNATURES,
		PANEL_FIXEDFORMTEXTS,
		PANEL_FILTERS,
		PANEL_SYNCFILTERS,
		PANEL_AUTOPILOT,
		PANEL_SYNC,
		PANEL_SEARCH,
#ifndef _WIN32_WCE
		PANEL_JUNK,
#endif
		PANEL_SECURITY,
		PANEL_CONFIRM,
		PANEL_MISC,
		PANEL_MISC2,
		
		MAX_PANEL
	};

public:
	OptionDialog(Document* pDocument,
				 GoRound* pGoRound,
				 FilterManager* pFilterManager,
				 ColorManager* pColorManager,
				 SyncFilterManager* pSyncFilterManager,
				 AutoPilotManager* pAutoPilotManager,
				 UpdateChecker* pUpdateChecker,
				 MainWindow* pMainWindow,
				 FolderWindow* pFolderWindow,
				 FolderComboBox* pFolderComboBox,
				 ListWindow* pListWindow,
				 FolderListWindow* pFolderListWindow,
				 MessageWindow* pPreviewWindow,
				 MessageFrameWindowManager* pMessageFrameWindowManager,
				 EditFrameWindowManager* pEditFrameWindowManager,
#ifdef QMTABWINDOW
				 TabWindow* pTabWindow,
#endif
#ifdef QMRECENTSWINDOW
				 RecentsWindowManager* pRecentsWindowManager,
#endif
				 AddressBookFrameWindowManager* pAddressBookFrameWindowManager,
				 qs::Profile* pProfile,
				 Account* pCurrentAccount,
				 Panel panel);
	~OptionDialog();

public:
	int doModal(HWND hwndParent);

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
	virtual LRESULT onCancel();

public:
	virtual LRESULT onNotify(NMHDR* pnmhdr,
							 bool* pbHandled);

protected:
	LRESULT onSize(UINT nFlags,
				   int cx,
				   int cy);

private:
#ifndef _WIN32_WCE_PSPC
	LRESULT onSelectorSelChanged(NMHDR* pnmhdr,
								 bool* pbHandled);
#else
	LRESULT onSelectorSelChange();
#endif

private:
	void layout();
	void setCurrentPanel(Panel panel,
						 bool bForce);

private:
	bool processDialogMessage(const MSG& msg);
	void processTab(bool bShift);
	bool isTabStop(HWND hwnd) const;
	void processMnemonic(char c);
	void setFocus(HWND hwnd);

private:
	static WCHAR getMnemonic(HWND hwnd);
	static WCHAR getMnemonic(WCHAR c);
	static void clearDefaultButton(HWND hwnd);

private:
	OptionDialog(const OptionDialog&);
	OptionDialog& operator=(const OptionDialog&);

private:
	typedef std::vector<OptionDialogPanel*> PanelList;

private:
	Document* pDocument_;
	GoRound* pGoRound_;
	FilterManager* pFilterManager_;
	ColorManager* pColorManager_;
	SyncFilterManager* pSyncFilterManager_;
	AutoPilotManager* pAutoPilotManager_;
	UpdateChecker* pUpdateChecker_;
	MainWindow* pMainWindow_;
	FolderWindow* pFolderWindow_;
	FolderComboBox* pFolderComboBox_;
	ListWindow* pListWindow_;
	FolderListWindow* pFolderListWindow_;
	MessageWindow* pPreviewWindow_;
	MessageFrameWindowManager* pMessageFrameWindowManager_;
	EditFrameWindowManager* pEditFrameWindowManager_;
#ifdef QMTABWINDOW
	TabWindow* pTabWindow_;
#endif
#ifdef QMRECENTSWINDOW
	RecentsWindowManager* pRecentsWindowManager_;
#endif
	AddressBookFrameWindowManager* pAddressBookFrameWindowManager_;
	qs::Profile* pProfile_;
	Account* pCurrentAccount_;
	Panel panel_;
	PanelList listPanel_;
	OptionDialogPanel* pCurrentPanel_;
	int nEnd_;
};


/****************************************************************************
 *
 * OptionDialogPanel
 *
 */

class OptionDialogPanel
{
public:
	virtual ~OptionDialogPanel();

public:
	virtual HWND getWindow() = 0;
	virtual bool save(OptionDialogContext* pContext) = 0;
};


/****************************************************************************
 *
 * AbstractOptionDialogPanel
 *
 */

template<class Dialog>
class AbstractOptionDialogPanel : public OptionDialogPanel
{
public:
	AbstractOptionDialogPanel();
	virtual ~AbstractOptionDialogPanel();

public:
	virtual HWND getWindow();
};


/****************************************************************************
 *
 * OptionDialogContext
 *
 */

class OptionDialogContext
{
public:
	enum Flag {
		FLAG_RELOADMAIN				= 0x00000001,
		FLAG_RELOADFOLDER			= 0x00000002,
		FLAG_RELOADLIST				= 0x00000004,
		FLAG_RELOADMESSAGE			= 0x00000008,
		FLAG_RELOADPREVIEW			= 0x00000010,
		FLAG_RELOADEDIT				= 0x00000020,
		FLAG_RELOADTAB				= 0x00000040,
		FLAG_RELOADADDRESSBOOK		= 0x00000080,
		FLAG_RELOADSECURITY			= 0x00000100,
		FLAG_RELOADRECENTS			= 0x00000200,
		
		FLAG_LAYOUTMAINWINDOW		= 0x00010000,
		FLAG_LAYOUTMESSAGEWINDOW	= 0x00020000,
		FLAG_LAYOUTEDITWINDOW		= 0x00040000
	};

public:
	OptionDialogContext();
	~OptionDialogContext();

public:
	unsigned int getFlags() const;
	void setFlags(unsigned int nFlags);
	void setFlags(unsigned int nFlags,
				  unsigned int nMask);

private:
	OptionDialogContext(const OptionDialogContext&);
	OptionDialogContext& operator=(const OptionDialogContext&);

private:
	unsigned int nFlags_;
};


/****************************************************************************
 *
 * OptionDialogManager
 *
 */

class OptionDialogManager
{
public:
	OptionDialogManager(Document* pDocument,
						GoRound* pGoRound,
						FilterManager* pFilterManager,
						ColorManager* pColorManager,
						SyncManager* pSyncManager,
						AutoPilotManager* pAutoPilotManager,
						UpdateChecker* pUpdateChecker,
						qs::Profile* pProfile);
	~OptionDialogManager();

public:
	void initUIs(MainWindow* pMainWindow,
				 FolderWindow* pFolderWindow,
				 FolderComboBox* pFolderComboBox,
				 ListWindow* pListWindow,
				 FolderListWindow* pFolderListWindow,
				 MessageWindow* pPreviewWindow,
				 MessageFrameWindowManager* pMessageFrameWindowManager,
				 EditFrameWindowManager* pEditFrameWindowManager,
#ifdef QMTABWINDOW
				 TabWindow* pTabWindow,
#endif
#ifdef QMRECENTSWINDOW
				 RecentsWindowManager* pRecentsWindowManager,
#endif
				 AddressBookFrameWindowManager* pAddressBookFrameWindowManager);
	int showDialog(HWND hwndParent,
				   Account* pCurrentAccount,
				   OptionDialog::Panel panel) const;
	bool canShowDialog() const;

private:
	OptionDialogManager(const OptionDialogManager&);
	OptionDialogManager& operator=(const OptionDialogManager&);

private:
	Document* pDocument_;
	GoRound* pGoRound_;
	FilterManager* pFilterManager_;
	ColorManager* pColorManager_;
	SyncManager* pSyncManager_;
	AutoPilotManager* pAutoPilotManager_;
	UpdateChecker* pUpdateChecker_;
	qs::Profile* pProfile_;
	MainWindow* pMainWindow_;
	FolderWindow* pFolderWindow_;
	FolderComboBox* pFolderComboBox_;
	ListWindow* pListWindow_;
	FolderListWindow* pFolderListWindow_;
	MessageWindow* pPreviewWindow_;
	MessageFrameWindowManager* pMessageFrameWindowManager_;
	EditFrameWindowManager* pEditFrameWindowManager_;
#ifdef QMTABWINDOW
	TabWindow* pTabWindow_;
#endif
#ifdef QMRECENTSWINDOW
	RecentsWindowManager* pRecentsWindowManager_;
#endif
	AddressBookFrameWindowManager* pAddressBookFrameWindowManager_;
};


/****************************************************************************
 *
 * OptionAddressBookDialog
 *
 */

class OptionAddressBookDialog :
	public DefaultDialog,
	public AbstractOptionDialogPanel<OptionAddressBookDialog>
{
public:
	OptionAddressBookDialog(AddressBook* pAddressBook,
							AddressBookFrameWindowManager* pAddressBookFrameWindowManager,
							qs::Profile* pProfile);
	~OptionAddressBookDialog();


public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

public:
	virtual bool save(OptionDialogContext* pContext);

private:
	LRESULT onFont();

private:
	OptionAddressBookDialog(const OptionAddressBookDialog&);
	OptionAddressBookDialog& operator=(const OptionAddressBookDialog&);

private:
	struct External {
		UINT nId_;
		const WCHAR* pwszName_;
	};

private:
	AddressBook* pAddressBook_;
	AddressBookFrameWindowManager* pAddressBookFrameWindowManager_;
	qs::Profile* pProfile_;
	LOGFONT lf_;

private:
	static External externals__[];
};


/****************************************************************************
 *
 * OptionConfirmDialog
 *
 */

class OptionConfirmDialog :
	public DefaultDialog,
	public AbstractOptionDialogPanel<OptionConfirmDialog>
{
public:
	explicit OptionConfirmDialog(qs::Profile* pProfile);
	virtual ~OptionConfirmDialog();

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

public:
	virtual bool save(OptionDialogContext* pContext);

private:
	OptionConfirmDialog(const OptionConfirmDialog&);
	OptionConfirmDialog& operator=(const OptionConfirmDialog&);

private:
	qs::Profile* pProfile_;

private:
	static DialogUtil::BoolProperty boolProperties__[];
};


/****************************************************************************
 *
 * OptionFolderDialog
 *
 */

class OptionFolderDialog :
	public DefaultDialog,
	public AbstractOptionDialogPanel<OptionFolderDialog>
{
public:
	OptionFolderDialog(FolderWindow* pFolderWindow,
					   FolderComboBox* pFolderComboBox,
					   qs::Profile* pProfile);
	virtual ~OptionFolderDialog();

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

public:
	virtual bool save(OptionDialogContext* pContext);

private:
	LRESULT onComboBoxFont();
#ifndef _WIN32_WCE
	LRESULT onWindowColors();
#endif
	LRESULT onWindowFont();

private:
	OptionFolderDialog(const OptionFolderDialog&);
	OptionFolderDialog& operator=(const OptionFolderDialog&);

private:
	FolderWindow* pFolderWindow_;
	FolderComboBox* pFolderComboBox_;
	qs::Profile* pProfile_;
	LOGFONT lfWindow_;
	LOGFONT lfComboBox_;
#ifndef _WIN32_WCE
	TextColorDialog::Data color_;
#endif

private:
	static DialogUtil::BoolProperty windowBoolProperties__[];
	static DialogUtil::BoolProperty comboBoxBoolProperties__[];
};


/****************************************************************************
 *
 * OptionHeaderDialog
 *
 */

class OptionHeaderDialog :
	public DefaultDialog,
	public AbstractOptionDialogPanel<OptionHeaderDialog>
{
public:
	OptionHeaderDialog(MessageFrameWindowManager* pMessageFrameWindowManager,
					   MessageWindow* pPreviewWindow,
					   qs::Profile* pProfile);
	virtual ~OptionHeaderDialog();

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

public:
	virtual bool save(OptionDialogContext* pContext);

private:
	LRESULT onFont();

private:
	OptionHeaderDialog(const OptionHeaderDialog&);
	OptionHeaderDialog& operator=(const OptionHeaderDialog&);

private:
	MessageFrameWindowManager* pMessageFrameWindowManager_;
	MessageWindow* pPreviewWindow_;
	qs::Profile* pProfile_;
	LOGFONT lf_;
};


#ifndef _WIN32_WCE
/****************************************************************************
 *
 * OptionJunkDialog
 *
 */

class OptionJunkDialog :
	public DefaultDialog,
	public AbstractOptionDialogPanel<OptionJunkDialog>
{
public:
	explicit OptionJunkDialog(JunkFilter* pJunkFilter);
	virtual ~OptionJunkDialog();

public:
	virtual INT_PTR dialogProc(UINT uMsg,
							   WPARAM wParam,
							   LPARAM lParam);

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

public:
	virtual bool save(OptionDialogContext* pContext);

protected:
	LRESULT onSize(UINT nFlags,
				   int cx,
				   int cy);

private:
	LRESULT onRepair();

private:
	void layout();

private:
	OptionJunkDialog(const OptionJunkDialog&);
	OptionJunkDialog& operator=(const OptionJunkDialog&);

private:
	JunkFilter* pJunkFilter_;
};
#endif


/****************************************************************************
 *
 * OptionListDialog
 *
 */

class OptionListDialog :
	public DefaultDialog,
	public AbstractOptionDialogPanel<OptionListDialog>
{
public:
	OptionListDialog(ListWindow* pListWindow,
					 FolderListWindow* pFolderListWindow,
					 qs::Profile* pProfile);
	virtual ~OptionListDialog();

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

public:
	virtual bool save(OptionDialogContext* pContext);

private:
	LRESULT onColors();
	LRESULT onFont();

private:
	OptionListDialog(const OptionListDialog&);
	OptionListDialog& operator=(const OptionListDialog&);

private:
	ListWindow* pListWindow_;
	FolderListWindow* pFolderListWindow_;
	qs::Profile* pProfile_;
	LOGFONT lf_;
	TextColorDialog::Data color_;

private:
	static DialogUtil::BoolProperty boolProperties__[];
};


/****************************************************************************
 *
 * OptionMiscDialog
 *
 */

class OptionMiscDialog :
	public DefaultDialog,
	public AbstractOptionDialogPanel<OptionMiscDialog>
{
public:
	explicit OptionMiscDialog(qs::Profile* pProfile);
	virtual ~OptionMiscDialog();

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

public:
	virtual bool save(OptionDialogContext* pContext);

private:
	LRESULT onBrowse();
	LRESULT onDropDown();

private:
	void updateDefaultEncodings();

private:
	OptionMiscDialog(const OptionMiscDialog&);
	OptionMiscDialog& operator=(const OptionMiscDialog&);

private:
	qs::Profile* pProfile_;

private:
	static DialogUtil::BoolProperty boolProperties__[];
};


/****************************************************************************
 *
 * OptionMisc2Dialog
 *
 */

class OptionMisc2Dialog :
	public DefaultDialog,
	public AbstractOptionDialogPanel<OptionMisc2Dialog>
{
public:
	OptionMisc2Dialog(UpdateChecker* pUpdateChecker,
					  qs::Profile* pProfile);
	virtual ~OptionMisc2Dialog();

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

public:
	virtual bool save(OptionDialogContext* pContext);

private:
	OptionMisc2Dialog(const OptionMisc2Dialog&);
	OptionMisc2Dialog& operator=(const OptionMisc2Dialog&);

private:
	UpdateChecker* pUpdateChecker_;
	qs::Profile* pProfile_;

private:
	static DialogUtil::BoolProperty boolProperties__[];
};


/****************************************************************************
 *
 * OptionSearchDialog
 *
 */

class OptionSearchDialog :
	public DefaultDialog,
	public AbstractOptionDialogPanel<OptionSearchDialog>
{
public:
	explicit OptionSearchDialog(qs::Profile* pProfile);
	virtual ~OptionSearchDialog();

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

public:
	virtual bool save(OptionDialogContext* pContext);

#ifndef _WIN32_WCE
private:
	LRESULT onClicked();
#endif

private:
	void updateState();

private:
	OptionSearchDialog(const OptionSearchDialog&);
	OptionSearchDialog& operator=(const OptionSearchDialog&);

private:
	qs::Profile* pProfile_;
};


/****************************************************************************
 *
 * OptionSecurityDialog
 *
 */

class OptionSecurityDialog :
	public DefaultDialog,
	public AbstractOptionDialogPanel<OptionSecurityDialog>
{
public:
	OptionSecurityDialog(Security* pSecurity,
						 qs::Profile* pProfile);
	virtual ~OptionSecurityDialog();

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

public:
	virtual bool save(OptionDialogContext* pContext);

private:
	OptionSecurityDialog(const OptionSecurityDialog&);
	OptionSecurityDialog& operator=(const OptionSecurityDialog&);

private:
	Security* pSecurity_;
	qs::Profile* pProfile_;

private:
#ifndef _WIN32_WCE
	static DialogUtil::BoolProperty globalBoolProperties__[];
#endif
	static DialogUtil::BoolProperty securityBoolProperties__[];
};


/****************************************************************************
 *
 * OptionSyncDialog
 *
 */

class OptionSyncDialog :
	public DefaultDialog,
	public AbstractOptionDialogPanel<OptionSyncDialog>
{
public:
	OptionSyncDialog(Recents* pRecents,
					 qs::Profile* pProfile);
	virtual ~OptionSyncDialog();

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

public:
	virtual bool save(OptionDialogContext* pContext);

private:
	LRESULT onBrowse();
	LRESULT onEdit();

private:
	OptionSyncDialog(const OptionSyncDialog&);
	OptionSyncDialog& operator=(const OptionSyncDialog&);

private:
	Recents* pRecents_;
	qs::Profile* pProfile_;

private:
	static DialogUtil::BoolProperty globalBoolProperties__[];
#ifdef QMRECENTSWINDOW
	static DialogUtil::BoolProperty recentsBoolProperties__[];
#endif
};


/****************************************************************************
 *
 * AbstractOptionTextDialog
 *
 */

class AbstractOptionTextDialog :
	public DefaultDialog,
	public AbstractOptionDialogPanel<AbstractOptionTextDialog>
{
public:
	AbstractOptionTextDialog(UINT nIdPortrait,
							 UINT nIdLandscape,
							 qs::Profile* pProfile,
							 const WCHAR* pwszSection);
	virtual ~AbstractOptionTextDialog();

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

public:
	virtual bool save(OptionDialogContext* pContext) = 0;

protected:
	virtual void updateState();

private:
	LRESULT onColors();
	LRESULT onFont();
	LRESULT onWrapChange(UINT nId);

private:
	AbstractOptionTextDialog(const AbstractOptionTextDialog&);
	AbstractOptionTextDialog& operator=(const AbstractOptionTextDialog&);

private:
	qs::Profile* pProfile_;
	const WCHAR* pwszSection_;
	LOGFONT lf_;
	TextColorDialog::Data color_;

private:
	static DialogUtil::BoolProperty boolProperties__[];
	static DialogUtil::IntProperty intProperties__[];
};


/****************************************************************************
 *
 * OptionEditDialog
 *
 */

class OptionEditDialog : public AbstractOptionTextDialog
{
public:
	OptionEditDialog(EditFrameWindowManager* pEditFrameWindowManager,
					 qs::Profile* pProfile);
	virtual ~OptionEditDialog();

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

public:
	virtual bool save(OptionDialogContext* pContext);

private:
	LRESULT onHeaderFont();

private:
	OptionEditDialog(const OptionEditDialog&);
	OptionEditDialog& operator=(const OptionEditDialog&);

private:
	EditFrameWindowManager* pEditFrameWindowManager_;
	qs::Profile* pProfile_;
	LOGFONT lfHeader_;

private:
	static DialogUtil::BoolProperty boolProperties__[];
};


/****************************************************************************
 *
 * SecurityDialog
 *
 */

class SecurityDialog : public DefaultDialog
{
public:
	explicit SecurityDialog(unsigned int nMessageSecurity);
	virtual ~SecurityDialog();

public:
	unsigned int getMessageSecurity() const;

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

protected:
	virtual LRESULT onOk();

private:
	SecurityDialog(const SecurityDialog&);
	SecurityDialog& operator=(const SecurityDialog&);

private:
	struct Item
	{
		UINT nId_;
		MessageSecurity security_;
	};

private:
	unsigned int nMessageSecurity_;

private:
	static Item items__[];
};


/****************************************************************************
 *
 * OptionEdit2Dialog
 *
 */

class OptionEdit2Dialog :
	public DefaultDialog,
	public AbstractOptionDialogPanel<OptionEdit2Dialog>
{
public:
	OptionEdit2Dialog(EditFrameWindowManager* pEditFrameWindowManager,
					  qs::Profile* pProfile);
	virtual ~OptionEdit2Dialog();

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

public:
	virtual bool save(OptionDialogContext* pContext);

private:
	LRESULT onBrowse();
	LRESULT onSecurity();

private:
	OptionEdit2Dialog(const OptionEdit2Dialog&);
	OptionEdit2Dialog& operator=(const OptionEdit2Dialog&);

private:
	EditFrameWindowManager* pEditFrameWindowManager_;
	qs::Profile* pProfile_;
	unsigned int nMessageSecurity_;

private:
	static DialogUtil::BoolProperty globalBoolProperties__[];
	static DialogUtil::BoolProperty editBoolProperties__[];
	static DialogUtil::IntProperty intProperties__[];
};


/****************************************************************************
 *
 * OptionMessageDialog
 *
 */

class OptionMessageDialog : public AbstractOptionTextDialog
{
public:
	OptionMessageDialog(MessageFrameWindowManager* pMessageFrameWindowManager,
						qs::Profile* pProfile);
	virtual ~OptionMessageDialog();

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

public:
	virtual bool save(OptionDialogContext* pContext);

private:
	OptionMessageDialog(const OptionMessageDialog&);
	OptionMessageDialog& operator=(const OptionMessageDialog&);

private:
	MessageFrameWindowManager* pMessageFrameWindowManager_;
	qs::Profile* pProfile_;

private:
	static DialogUtil::BoolProperty boolProperties__[];
};


/****************************************************************************
 *
 * OptionPreviewDialog
 *
 */

class OptionPreviewDialog : public AbstractOptionTextDialog
{
public:
	OptionPreviewDialog(MessageWindow* pPreviewWindow,
						qs::Profile* pProfile);
	virtual ~OptionPreviewDialog();

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

public:
	virtual bool save(OptionDialogContext* pContext);

private:
	OptionPreviewDialog(const OptionPreviewDialog&);
	OptionPreviewDialog& operator=(const OptionPreviewDialog&);

private:
	MessageWindow* pPreviewWindow_;
	qs::Profile* pProfile_;

private:
	static DialogUtil::BoolProperty boolProperties__[];
	static DialogUtil::IntProperty intProperties__[];
};


#ifdef QMTABWINDOW
/****************************************************************************
 *
 * OptionTabDialog
 *
 */

class OptionTabDialog :
	public DefaultDialog,
	public AbstractOptionDialogPanel<OptionTabDialog>
{
public:
	OptionTabDialog(TabWindow* pTabWindow,
					qs::Profile* pProfile);
	virtual ~OptionTabDialog();

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

public:
	virtual bool save(OptionDialogContext* pContext);

private:
	LRESULT onFont();

private:
	OptionTabDialog(const OptionTabDialog&);
	OptionTabDialog& operator=(const OptionTabDialog&);

private:
	TabWindow* pTabWindow_;
	qs::Profile* pProfile_;
	LOGFONT lf_;

private:
	static DialogUtil::BoolProperty boolProperties__[];
};
#endif // QMTABWINDOW


/****************************************************************************
 *
 * RuleColorSetsDialog
 *
 */

template<class T, class List, class Manager, class EditDialog>
class RuleColorSetsDialog :
	public AbstractListDialog<T, List>,
	public AbstractOptionDialogPanel<RuleColorSetsDialog<T, List, Manager, EditDialog> >
{
public:
	typedef const List& (Manager::*PFN_GET)();
	typedef void (Manager::*PFN_SET)(List&);

public:
	RuleColorSetsDialog(Manager* pManager,
						AccountManager* pAccountManager,
						qs::Profile* pProfile,
						Account* pCurrentAccount,
						UINT nTitleId,
						PFN_GET pfnGet,
						PFN_SET pfnSet);
	virtual ~RuleColorSetsDialog();

public:
	virtual INT_PTR dialogProc(UINT uMsg,
							   WPARAM wParam,
							   LPARAM lParam);

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

protected:
	virtual qs::wstring_ptr getLabel(const T* p) const;
	virtual std::auto_ptr<T> create() const;
	virtual T* edit(T* p) const;

public:
	virtual bool save(OptionDialogContext* pContext);

protected:
	LRESULT onSize(UINT nFlags,
				   int cx,
				   int cy);

private:
	void layout();

private:
	RuleColorSetsDialog(const RuleColorSetsDialog&);
	RuleColorSetsDialog& operator=(const RuleColorSetsDialog&);

private:
	Manager* pManager_;
	AccountManager* pAccountManager_;
	qs::Profile* pProfile_;
	Account* pCurrentAccount_;
	UINT nTitleId_;
	PFN_SET pfnSet_;
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
					  AccountManager* pAccountManager,
					  qs::Profile* pProfile,
					  Account* pCurrentAccount,
					  UINT nTitleId,
					  PFN_GET pfnGet,
					  PFN_SET pfnSet);
	virtual ~RulesColorsDialog();

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

protected:
	virtual qs::wstring_ptr getLabel(const T* p) const;
	virtual std::auto_ptr<T> create() const;
	virtual T* edit(T* p) const;
	virtual void updateState();

private:
	virtual const WCHAR* getName() const = 0;
	virtual qs::wstring_ptr getLabelPrefix(const T* p) const = 0;

protected:
	LRESULT onSize(UINT nFlags,
				   int cx,
				   int cy);

private:
	LRESULT onAccountEditChange();
	LRESULT onAccountSelChange();

private:
	void updateFolder(Account* pAccount);
	void layout();

private:
	RulesColorsDialog(const RulesColorsDialog&);
	RulesColorsDialog& operator=(const RulesColorsDialog&);

private:
	Container* pContainer_;
	AccountManager* pAccountManager_;
	qs::Profile* pProfile_;
	Account* pCurrentAccount_;
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
					AccountManager* pAccountManager,
					qs::Profile* pProfile,
					Account* pCurrentAccount);
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
				 AccountManager* pAccountManager,
				 qs::Profile* pProfile,
				 Account* pCurrentAccount);

private:
	virtual const WCHAR* getName() const;
	virtual qs::wstring_ptr getLabelPrefix(const ColorEntry* p) const;
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
				AccountManager* pAccountManager,
				Account* pCurrentAccount);
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
	LRESULT onColor();
	LRESULT onEdit();
	LRESULT onChoose(UINT nId);
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
 * RuleSetsDialog
 *
 */

class RuleSetsDialog : public RuleColorSetsDialog<RuleSet, RuleManager::RuleSetList, RuleManager, RulesDialog>
{
public:
	RuleSetsDialog(RuleManager* pRuleManager,
				   AccountManager* pAccountManager,
				   qs::Profile* pProfile,
				   Account* pCurrentAccount);
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
				AccountManager* pAccountManager,
				qs::Profile* pProfile,
				Account* pCurrentAccount);

private:
	virtual const WCHAR* getName() const;
	virtual qs::wstring_ptr getLabelPrefix(const Rule* p) const;
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
			   AccountManager* pAccountManager,
			   Account* pCurrentAccount);
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
	AccountManager* pAccountManager_;
	Account* pCurrentAccount_;
	qs::wstring_ptr wstrTemplate_;
	CopyRuleAction::ArgumentList listArgument_;
	bool bInit_;
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
 * AutoPilotDialog
 *
 */

class AutoPilotDialog :
	public AbstractListDialog<AutoPilotEntry, AutoPilotManager::EntryList>,
	public AbstractOptionDialogPanel<AutoPilotDialog>
{
public:
	AutoPilotDialog(AutoPilotManager* pManager,
					GoRound* pGoRound,
					qs::Profile* pProfile);
	virtual ~AutoPilotDialog();

public:
	virtual INT_PTR dialogProc(UINT uMsg,
							   WPARAM wParam,
							   LPARAM lParam);

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

protected:
	virtual qs::wstring_ptr getLabel(const AutoPilotEntry* p) const;
	virtual std::auto_ptr<AutoPilotEntry> create() const;
	virtual AutoPilotEntry* edit(AutoPilotEntry* p) const;

public:
	virtual bool save(OptionDialogContext* pContext);

protected:
	LRESULT onSize(UINT nFlags,
				   int cx,
				   int cy);

private:
	void layout();

private:
	AutoPilotDialog(const AutoPilotDialog&);
	AutoPilotDialog& operator=(const AutoPilotDialog&);

private:
	AutoPilotManager* pManager_;
	GoRound* pGoRound_;
	qs::Profile* pProfile_;
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
 * FiltersDialog
 *
 */

class FiltersDialog :
	public AbstractListDialog<Filter, FilterManager::FilterList>,
	public AbstractOptionDialogPanel<FiltersDialog>
{
public:
	explicit FiltersDialog(FilterManager* pManager);
	virtual ~FiltersDialog();

public:
	virtual INT_PTR dialogProc(UINT uMsg,
							   WPARAM wParam,
							   LPARAM lParam);

protected:
	virtual qs::wstring_ptr getLabel(const Filter* p) const;
	virtual std::auto_ptr<Filter> create() const;
	virtual Filter* edit(Filter* p) const;

public:
	virtual bool save(OptionDialogContext* pContext);

protected:
	LRESULT onSize(UINT nFlags,
				   int cx,
				   int cy);

private:
	void layout();

private:
	FiltersDialog(const FiltersDialog&);
	FiltersDialog& operator=(const FiltersDialog&);

private:
	FilterManager* pManager_;
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
 * FixedFormTextsDialog
 *
 */

class FixedFormTextsDialog :
	public AbstractListDialog<FixedFormText, FixedFormTextManager::TextList>,
	public AbstractOptionDialogPanel<FixedFormTextsDialog>
{
public:
	FixedFormTextsDialog(FixedFormTextManager* pManager,
						 qs::Profile* pProfile);
	virtual ~FixedFormTextsDialog();

public:
	virtual INT_PTR dialogProc(UINT uMsg,
							   WPARAM wParam,
							   LPARAM lParam);

protected:
	virtual qs::wstring_ptr getLabel(const FixedFormText* p) const;
	virtual std::auto_ptr<FixedFormText> create() const;
	virtual FixedFormText* edit(FixedFormText* p) const;

public:
	virtual bool save(OptionDialogContext* pContext);

protected:
	LRESULT onSize(UINT nFlags,
				   int cx,
				   int cy);

private:
	void layout();

private:
	FixedFormTextsDialog(const FixedFormTextsDialog&);
	FixedFormTextsDialog& operator=(const FixedFormTextsDialog&);

private:
	FixedFormTextManager* pManager_;
	qs::Profile* pProfile_;
};


/****************************************************************************
 *
 * FixedFormTextDialog
 *
 */

class FixedFormTextDialog : public DefaultDialog
{
public:
	FixedFormTextDialog(FixedFormText* pText,
						qs::Profile* pProfile);
	virtual ~FixedFormTextDialog();

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

protected:
	LRESULT onSize(UINT nFlags,
				   int cx,
				   int cy);

private:
	LRESULT onNameChange();

private:
	void updateState();
	void layout();

private:
	FixedFormTextDialog(const FixedFormTextDialog&);
	FixedFormTextDialog& operator=(const FixedFormTextDialog&);

private:
	FixedFormText* pText_;
	qs::Profile* pProfile_;
};


/****************************************************************************
 *
 * GoRoundDialog
 *
 */

class GoRoundDialog :
	public AbstractListDialog<GoRoundCourse, GoRound::CourseList>,
	public AbstractOptionDialogPanel<GoRoundDialog>
{
public:
	GoRoundDialog(GoRound* pGoRound,
				  AccountManager* pAccountManager,
				  SyncFilterManager* pSyncFilterManager,
				  qs::Profile* pProfile);
	virtual ~GoRoundDialog();

public:
	virtual INT_PTR dialogProc(UINT uMsg,
							   WPARAM wParam,
							   LPARAM lParam);

protected:
	virtual qs::wstring_ptr getLabel(const GoRoundCourse* p) const;
	virtual std::auto_ptr<GoRoundCourse> create() const;
	virtual GoRoundCourse* edit(GoRoundCourse* p) const;

public:
	virtual bool save(OptionDialogContext* pContext);

protected:
	LRESULT onSize(UINT nFlags,
				   int cx,
				   int cy);

private:
	void layout();

private:
	GoRoundDialog(const GoRoundDialog&);
	GoRoundDialog& operator=(const GoRoundDialog&);

private:
	GoRound* pGoRound_;
	AccountManager* pAccountManager_;
	SyncFilterManager* pSyncFilterManager_;
	qs::Profile* pProfile_;
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
						AccountManager* pAccountManager,
						SyncFilterManager* pSyncFilterManager,
						qs::Profile* pProfile);
	virtual ~GoRoundCourseDialog();

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

protected:
	virtual qs::wstring_ptr getLabel(const GoRoundEntry* p) const;
	virtual std::auto_ptr<GoRoundEntry> create() const;
	virtual GoRoundEntry* edit(GoRoundEntry* p) const;
	virtual void updateState();

protected:
	LRESULT onSize(UINT nFlags,
				   int cx,
				   int cy);

private:
	LRESULT onDialup();
	LRESULT onNameChange();

private:
	void layout();

private:
	GoRoundCourseDialog(const GoRoundCourseDialog&);
	GoRoundCourseDialog& operator=(const GoRoundCourseDialog&);

private:
	GoRoundCourse* pCourse_;
	AccountManager* pAccountManager_;
	SyncFilterManager* pSyncFilterManager_;
	qs::Profile* pProfile_;
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
					   AccountManager* pAccountManager,
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
	LRESULT onClicked();

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
	AccountManager* pAccountManager_;
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
 * SignaturesDialog
 *
 */

class SignaturesDialog :
	public AbstractListDialog<Signature, SignatureManager::SignatureList>,
	public AbstractOptionDialogPanel<SignaturesDialog>
{
public:
	SignaturesDialog(SignatureManager* pSignatureManager,
					 AccountManager* pAccountManager,
					 qs::Profile* pProfile);
	virtual ~SignaturesDialog();

public:
	virtual INT_PTR dialogProc(UINT uMsg,
							   WPARAM wParam,
							   LPARAM lParam);

protected:
	virtual qs::wstring_ptr getLabel(const Signature* p) const;
	virtual std::auto_ptr<Signature> create() const;
	virtual Signature* edit(Signature* p) const;

public:
	virtual bool save(OptionDialogContext* pContext);

protected:
	LRESULT onSize(UINT nFlags,
				   int cx,
				   int cy);

private:
	void layout();

private:
	SignaturesDialog(const SignaturesDialog&);
	SignaturesDialog& operator=(const SignaturesDialog&);

private:
	SignatureManager* pSignatureManager_;
	AccountManager* pAccountManager_;
	qs::Profile* pProfile_;
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
					AccountManager* pAccountManager,
					qs::Profile* pProfile);
	virtual ~SignatureDialog();

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
	LRESULT onSize(UINT nFlags,
				   int cx,
				   int cy);

protected:
	virtual LRESULT onOk();

private:
	LRESULT onNameChange();

private:
	void updateState();
	void layout();

private:
	SignatureDialog(const SignatureDialog&);
	SignatureDialog& operator=(const SignatureDialog&);

private:
	Signature* pSignature_;
	AccountManager* pAccountManager_;
	qs::Profile* pProfile_;
};


/****************************************************************************
 *
 * SyncFilterSetsDialog
 *
 */

class SyncFilterSetsDialog :
	public AbstractListDialog<SyncFilterSet, SyncFilterManager::FilterSetList>,
	public AbstractOptionDialogPanel<SyncFilterSetsDialog>
{
public:
	SyncFilterSetsDialog(SyncFilterManager* pSyncFilterManager,
						 qs::Profile* pProfile);
	virtual ~SyncFilterSetsDialog();

public:
	virtual INT_PTR dialogProc(UINT uMsg,
							   WPARAM wParam,
							   LPARAM lParam);

protected:
	virtual qs::wstring_ptr getLabel(const SyncFilterSet* p) const;
	virtual std::auto_ptr<SyncFilterSet> create() const;
	virtual SyncFilterSet* edit(SyncFilterSet* p) const;

public:
	virtual bool save(OptionDialogContext* pContext);

protected:
	LRESULT onSize(UINT nFlags,
				   int cx,
				   int cy);

private:
	void layout();

private:
	SyncFilterSetsDialog(const SyncFilterSetsDialog&);
	SyncFilterSetsDialog& operator=(const SyncFilterSetsDialog&);

private:
	SyncFilterManager* pSyncFilterManager_;
	qs::Profile* pProfile_;
};


/****************************************************************************
 *
 * SyncFiltersDialog
 *
 */

class SyncFiltersDialog : public AbstractListDialog<SyncFilter, SyncFilterSet::FilterList>
{
public:
	SyncFiltersDialog(SyncFilterSet* pSyncFilterSet,
					  qs::Profile* pProfile);
	virtual ~SyncFiltersDialog();

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

protected:
	virtual qs::wstring_ptr getLabel(const SyncFilter* p) const;
	virtual std::auto_ptr<SyncFilter> create() const;
	virtual SyncFilter* edit(SyncFilter* p) const;
	virtual void updateState();

protected:
	LRESULT onSize(UINT nFlags,
				   int cx,
				   int cy);

private:
	LRESULT onNameChange();

private:
	void layout();

private:
	SyncFiltersDialog(const SyncFiltersDialog&);
	SyncFiltersDialog& operator=(const SyncFiltersDialog&);

private:
	SyncFilterSet* pSyncFilterSet_;
	qs::Profile* pProfile_;
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
 * LayoutUtil
 *
 */

class LayoutUtil
{
public:
	static void layout(qs::Window* pWindow,
					   UINT nId);
	static HDWP layout(qs::Window* pWindow,
					   UINT nId,
					   HDWP hdwp,
					   int nTopMargin,
					   int nBottomMargin);
};

}

#include "optiondialog.inl"

#endif // __OPTIONDIALOG_H__
