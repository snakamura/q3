/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __AUTOCOMPLETE_H__
#define __AUTOCOMPLETE_H__

#include <qm.h>

#include <qsdevicecontext.h>
#include <qswindow.h>

#include <vector>


namespace qm {

class AutoComplete;
class AutoCompleteCallback;
class AutoCompleteEditSubclassWindow;
class AutoCompleteListWindow;


/****************************************************************************
 *
 * AutoComplete
 *
 */

class AutoComplete
{
public:
	AutoComplete(HWND hwnd,
				 qs::WindowBase* pParent,
				 AutoCompleteCallback* pCallback);
	~AutoComplete();

private:
	AutoComplete(const AutoComplete&);
	AutoComplete& operator=(const AutoComplete&);
};


/****************************************************************************
 *
 * AutoCompleteCallback
 *
 */

class AutoCompleteCallback
{
public:
	typedef std::vector<qs::WSTRING> CandidateList;

public:
	virtual ~AutoCompleteCallback();

public:
	virtual std::pair<size_t, size_t> getInput(const WCHAR* pwszText,
											   size_t nCaret) = 0;
	virtual void getCandidates(const WCHAR* pwszInput,
							   CandidateList* pList) = 0;
};


/****************************************************************************
 *
 * AutoCompleteEditSubclassWindow
 *
 */

class AutoCompleteEditSubclassWindow :
	public qs::WindowBase,
	public qs::DefaultWindowHandler,
	public qs::CommandHandler
{
public:
	AutoCompleteEditSubclassWindow(HWND hwnd,
								   qs::WindowBase* pParent,
								   AutoCompleteCallback* pCallback);
	virtual ~AutoCompleteEditSubclassWindow();

public:
	void fill(const WCHAR* pwszText);

public:
	virtual bool preTranslateAccelerator(const MSG& msg);
	virtual LRESULT windowProc(UINT uMsg,
							   WPARAM wParam,
							   LPARAM lParam);

protected:
	LRESULT onDestroy();
	LRESULT onKillFocus(HWND hwnd);
	LRESULT onTimer(UINT_PTR nId);

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

private:
	LRESULT onChange();

private:
	void showCandidates();
	void showCandidates(AutoCompleteCallback::CandidateList& listCandidate,
						const WCHAR* pwszInput);
	void hideCandidates();

private:
	AutoCompleteEditSubclassWindow(const AutoCompleteEditSubclassWindow&);
	AutoCompleteEditSubclassWindow& operator=(const AutoCompleteEditSubclassWindow&);

private:
	enum {
		TIMER_ID		= 1101,
		TIMER_INTERVAL	= 200
	};

private:
	qs::WindowBase* pParent_;
	AutoCompleteCallback* pCallback_;
	UINT nId_;
	UINT_PTR nTimerId_;
	std::pair<size_t, size_t> input_;
	AutoCompleteListWindow* pListWindow_;
};


/****************************************************************************
 *
 * AutoCompleteListWindow
 *
 */

class AutoCompleteListWindow :
	public qs::WindowBase,
	public qs::DefaultWindowHandler
{
public:
	enum Select {
		SELECT_PREV,
		SELECT_NEXT,
		SELECT_PREVPAGE,
		SELECT_NEXTPAGE
	};

public:
	typedef AutoCompleteCallback::CandidateList CandidateList;

public:
	AutoCompleteListWindow(AutoCompleteEditSubclassWindow* pEditWindow,
						   HFONT hfont);
	virtual ~AutoCompleteListWindow();

public:
	void showCandidates(CandidateList& listCandidate,
						const WCHAR* pwszInput);
	void select(Select select);
	void fill();

public:
	virtual LRESULT windowProc(UINT uMsg,
							   WPARAM wParam,
							   LPARAM lParam);

protected:
	LRESULT onActivate(UINT nFlags,
					   HWND hwnd,
					   bool bMinimized);
	LRESULT onCreate(CREATESTRUCT* pCreateStruct);
	LRESULT onDestroy();
	LRESULT onEraseBkgnd(HDC hdc);
	LRESULT onLButtonUp(UINT nFlags,
						const POINT& pt);
#ifndef _WIN32_WCE
	LRESULT onMouseActivate(HWND hwnd,
							UINT nHitTest,
							UINT uMsg);
#endif
	LRESULT onMouseMove(UINT nFlags,
						const POINT& pt);
#if !defined _WIN32_WCE || _WIN32_WCE >= 211
	LRESULT onMouseWheel(UINT nFlags,
						 short nDelta,
						 const POINT& pt);
#endif
	LRESULT onPaint();
	LRESULT onSize(UINT nFlags,
				   int cx,
				   int cy);
	LRESULT onVScroll(UINT nCode,
					  UINT nPos,
					  HWND hwnd);
#ifndef _WIN32_WCE
	LRESULT onWindowPosChanging(WINDOWPOS* pWindowPos);
#endif

private:
	int getItemFromPoint(const POINT& pt) const;
	void paintItem(qs::DeviceContext* pdc,
				   unsigned int n,
				   const RECT& rect,
				   bool bSelected);
	void updateScrollBar();
	void scroll(int nPos);

private:
	static void paintText(qs::DeviceContext* pdc,
						  const WCHAR* pwsz,
						  size_t nLen,
						  HFONT hfont,
						  RECT* pRect);

private:
	AutoCompleteListWindow(const AutoCompleteListWindow&);
	AutoCompleteListWindow& operator=(const AutoCompleteListWindow&);

private:
	AutoCompleteEditSubclassWindow* pEditWindow_;
	CandidateList listCandidate_;
	qs::wstring_ptr wstrInput_;
	int nSelect_;
	unsigned int nLineHeight_;
	HFONT hfont_;
	HFONT hfontBold_;
};

}

#endif // __AUTOCOMPLETE_H__
