/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
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
#include "../model/color.h"
#include "../model/filter.h"
#include "../model/fixedformtext.h"
#include "../model/goround.h"
#include "../model/rule.h"
#include "../model/signature.h"
#include "../sync/autopilot.h"


namespace qm {

class OptionDialog;
class OptionDialogPanel;
class OptionDialogManager;
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

class Document;


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
		
		PANEL_RULES,
		PANEL_COLORS,
		PANEL_GOROUND,
		PANEL_SIGNATURES,
		PANEL_FIXEDFORMTEXTS,
		PANEL_FILTERS,
		PANEL_SYNCFILTERS,
		PANEL_AUTOPILOT,
		
		MAX_PANEL
	};

public:
	OptionDialog(Document* pDocument,
				 GoRound* pGoRound,
				 FilterManager* pFilterManager,
				 ColorManager* pColorManager,
				 SyncFilterManager* pSyncFilterManager,
				 AutoPilotManager* pAutoPilotManager,
				 qs::Profile* pProfile,
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
	void setCurrentPanel(Panel panel);

private:
	bool processDialogMessage(const MSG& msg);
	void processTab(bool bShift);
	bool isTabStop(HWND hwnd) const;
	void processMnemonic(char c);
	WCHAR getMnemonic(HWND hwnd);
	void setFocus(HWND hwnd);
	void clearDefaultButton(HWND hwnd);

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
	qs::Profile* pProfile_;
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
	virtual bool save() = 0;
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
						qs::Profile* pProfile);
	~OptionDialogManager();

public:
	int showDialog(HWND hwndParent,
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
	qs::Profile* pProfile_;
};


/****************************************************************************
 *
 * RuleColorSetsDialog
 *
 */

template<class T, class List, class Manager, class EditDialog>
class RuleColorSetsDialog :
	public AbstractListDialog<T, List>,
	public AbstractOptionDialogPanel<RuleColorSetsDialog>
{
public:
	typedef const List& (Manager::*PFN_GET)();
	typedef void (Manager::*PFN_SET)(List&);

public:
	RuleColorSetsDialog(Manager* pManager,
						AccountManager* pAccountManager,
						qs::Profile* pProfile,
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
	virtual bool edit(T* p) const;

public:
	virtual bool save();

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
	virtual bool edit(T* p) const;
	virtual void updateState();

private:
	virtual const WCHAR* getName() = 0;

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
					qs::Profile* pProfile);
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
				 qs::Profile* pProfile);

private:
	virtual const WCHAR* getName();
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
				AccountManager* pAccountManager);
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
 * RuleSetsDialog
 *
 */

class RuleSetsDialog : public RuleColorSetsDialog<RuleSet, RuleManager::RuleSetList, RuleManager, RulesDialog>
{
public:
	RuleSetsDialog(RuleManager* pRuleManager,
				   AccountManager* pAccountManager,
				   qs::Profile* pProfile);
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
				qs::Profile* pProfile);

private:
	virtual const WCHAR* getName();
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
			   AccountManager* pAccountManager);
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
					GoRound* pGoRound);
	virtual ~AutoPilotDialog();

public:
	virtual INT_PTR dialogProc(UINT uMsg,
							   WPARAM wParam,
							   LPARAM lParam);

protected:
	virtual qs::wstring_ptr getLabel(const AutoPilotEntry* p) const;
	virtual std::auto_ptr<AutoPilotEntry> create() const;
	virtual bool edit(AutoPilotEntry* p) const;

public:
	virtual bool save();

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
	virtual bool edit(Filter* p) const;

public:
	virtual bool save();

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
	virtual bool edit(FixedFormText* p) const;

public:
	virtual bool save();

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
	virtual bool edit(GoRoundCourse* p) const;

public:
	virtual bool save();

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
	virtual bool edit(GoRoundEntry* p) const;
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
	virtual bool edit(Signature* p) const;

public:
	virtual bool save();

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
	virtual bool edit(SyncFilterSet* p) const;

public:
	virtual bool save();

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
	virtual bool edit(SyncFilter* p) const;
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