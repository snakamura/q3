/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#ifndef __TEXTWINDOW_H__
#define __TEXTWINDOW_H__

#include <qs.h>

#include <vector>


namespace qs {

/****************************************************************************
 *
 * TextWindowRuler
 *
 */

class TextWindowRuler :
	public WindowBase,
	public DefaultWindowHandler
{
public:
	TextWindowRuler(TextWindowImpl* pImpl);
	virtual ~TextWindowRuler();

public:
	void update();

public:
	virtual LRESULT windowProc(UINT uMsg,
							   WPARAM wParam,
							   LPARAM lParam);

protected:
	LRESULT onEraseBkgnd(HDC hdc);
	LRESULT onPaint();

private:
	TextWindowRuler(const TextWindowRuler&);
	TextWindowRuler& operator=(const TextWindowRuler&);

private:
	TextWindowImpl* pImpl_;
	int nPos_;
};

}

#endif // __TEXTWINDOW_H__
