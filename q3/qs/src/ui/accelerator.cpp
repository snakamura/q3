/*
 * $Id: accelerator.cpp,v 1.2 2003/05/09 06:05:34 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsaccelerator.h>
#include <qserror.h>
#include <qsstl.h>
#include <qsnew.h>

#include <vector>
#include <utility>
#include <algorithm>

#pragma warning(disable:4786)

using namespace qs;


/****************************************************************************
 *
 * Accelerator
 *
 */

qs::Accelerator::~Accelerator()
{
}


/****************************************************************************
 *
 * AcceleratorFactory
 *
 */

qs::AcceleratorFactory::~AcceleratorFactory()
{
}


/****************************************************************************
 *
 * AbstractAcceleratorImpl
 *
 */

struct qs::AbstractAcceleratorImpl
{
	typedef std::vector<std::pair<unsigned int, unsigned int> > IdKeyMap;
	IdKeyMap mapIdToKey_;
};


/****************************************************************************
 *
 * AbstractAccelerator
 *
 */

qs::AbstractAccelerator::AbstractAccelerator(
	const ACCEL* pAccel, int nSize, QSTATUS* pstatus) :
	pImpl_(0)
{
	assert(pstatus);
	
	*pstatus = QSTATUS_SUCCESS;
	
	DECLARE_QSTATUS();
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	
	STLWrapper<AbstractAcceleratorImpl::IdKeyMap> wrapper(pImpl_->mapIdToKey_);
	for (int n = 0; n < nSize; ++n, ++pAccel) {
		status = wrapper.push_back(
			AbstractAcceleratorImpl::IdKeyMap::value_type(
				pAccel->cmd, MAKELONG(pAccel->key, pAccel->fVirt)));
		CHECK_QSTATUS_SET(pstatus);
	}
	std::sort(pImpl_->mapIdToKey_.begin(), pImpl_->mapIdToKey_.end(),
		binary_compose_f_gx_hy(
			std::less<AbstractAcceleratorImpl::IdKeyMap::value_type::first_type>(),
			std::select1st<AbstractAcceleratorImpl::IdKeyMap::value_type>(),
			std::select1st<AbstractAcceleratorImpl::IdKeyMap::value_type>()));
}

qs::AbstractAccelerator::~AbstractAccelerator()
{
	delete pImpl_;
	pImpl_ = 0;
}

QSTATUS qs::AbstractAccelerator::getKeyFromId(UINT nId, WSTRING* pwstr)
{
	assert(pwstr);
	
	DECLARE_QSTATUS();
	
	*pwstr = 0;
	
	AbstractAcceleratorImpl::IdKeyMap::const_iterator it = std::lower_bound(
		pImpl_->mapIdToKey_.begin(), pImpl_->mapIdToKey_.end(),
		AbstractAcceleratorImpl::IdKeyMap::value_type(nId, 0),
		binary_compose_f_gx_hy(
			std::less<AbstractAcceleratorImpl::IdKeyMap::value_type::first_type>(),
			std::select1st<AbstractAcceleratorImpl::IdKeyMap::value_type>(),
			std::select1st<AbstractAcceleratorImpl::IdKeyMap::value_type>()));
	if (it == pImpl_->mapIdToKey_.end() || (*it).first != nId)
		return QSTATUS_SUCCESS;
	
	StringBuffer<WSTRING> buffer(10, &status);
	CHECK_QSTATUS();
	
	static const struct
	{
		unsigned int nKey_;
		const WCHAR* pwsz_;
	} modifiers[] = {
		{ FALT,		L"Alt+"		},
		{ FCONTROL,	L"Ctrl+"	},
		{ FSHIFT,	L"Shift+"	}
	};
	unsigned int nVirt = HIWORD((*it).second);
	for (int n = 0; n < countof(modifiers); ++n) {
		if (nVirt & modifiers[n].nKey_) {
			status = buffer.append(modifiers[n].pwsz_);
			CHECK_QSTATUS();
		}
	}
	
	static const struct
	{
		unsigned int nKey_;
		const WCHAR* pwsz_;
	} keys[] = {
		{ VK_RETURN, 	L"Enter"	},
		{ VK_TAB,		L"Tab"		},
		{ VK_ESCAPE,	L"Esc"		},
		{ VK_SPACE,		L"Space"	},
		{ VK_BACK,		L"BS"		},
		{ VK_DELETE,	L"Del"		},
		{ VK_INSERT,	L"Ins"		},
		{ VK_HOME,		L"Home"		},
		{ VK_END,		L"End"		},
		{ VK_PRIOR,		L"PageUp"	},
		{ VK_NEXT,		L"PageDown"	},
		{ VK_UP,		L"Up"		},
		{ VK_DOWN,		L"Down"		},
		{ VK_LEFT,		L"Left"		},
		{ VK_RIGHT,		L"Right"	}
	};
	
	unsigned int nKey = LOWORD((*it).second);
	if (!(nVirt & FVIRTKEY)) {
		status = buffer.append(static_cast<WCHAR>(nKey));
		CHECK_QSTATUS();
	}
	else if (('A' <= nKey && nKey <= 'Z') || ('0' <= nKey && nKey <= '9')) {
		status = buffer.append(static_cast<WCHAR>(nKey));
		CHECK_QSTATUS();
	}
	else if (VK_F1 <= nKey && nKey <= VK_F12) {
		WCHAR wsz[8];
		swprintf(wsz, L"F%d", nKey - VK_F1);
		status = buffer.append(wsz);
		CHECK_QSTATUS();
	}
	else {
		for (int n = 0; n < countof(keys); ++n) {
			if (nKey == keys[n].nKey_) {
				status = buffer.append(keys[n].pwsz_);
				CHECK_QSTATUS();
				break;
			}
		}
	}
	
	*pwstr = buffer.getString();
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * SystemAccelerator
 *
 */

qs::SystemAccelerator::SystemAccelerator(
	const ACCEL* pAccel, int nSize, QSTATUS* pstatus) :
	AbstractAccelerator(pAccel, nSize, pstatus),
	haccel_(0)
{
	if (*pstatus != QSTATUS_SUCCESS)
		return;
	
	haccel_ = ::CreateAcceleratorTable(const_cast<ACCEL*>(pAccel), nSize);
	*pstatus = haccel_ ? QSTATUS_SUCCESS : QSTATUS_FAIL;
}

qs::SystemAccelerator::~SystemAccelerator()
{
	::DestroyAcceleratorTable(haccel_);
}

QSTATUS qs::SystemAccelerator::translateAccelerator(HWND hwnd,
	const MSG& msg, bool* pbProcessed)
{
	assert(pbProcessed);
	*pbProcessed = ::TranslateAccelerator(hwnd,
		haccel_, const_cast<MSG*>(&msg)) != 0;
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * SystemAcceleratorFactory
 *
 */

qs::SystemAcceleratorFactory::SystemAcceleratorFactory()
{
}

qs::SystemAcceleratorFactory::~SystemAcceleratorFactory()
{
}

QSTATUS qs::SystemAcceleratorFactory::createAccelerator(
	const ACCEL* pAccel, int nSize, Accelerator** ppAccelerator)
{
	assert(ppAccelerator);
	
	DECLARE_QSTATUS();
	
	SystemAccelerator* p = 0;
	status = newQsObject(pAccel, nSize, &p);
	*ppAccelerator = p;
	
	return status;
}


/****************************************************************************
 *
 * CustomAcceleratorImpl
 *
 */

struct qs::CustomAcceleratorImpl
{
	enum {
		ALT		= 0x00010000,
		CTRL	= 0x00020000,
		SHIFT	= 0x00040000,
		VIRTUAL	= 0x00080000,
	};
	
	typedef std::vector<std::pair<unsigned int, unsigned int> > AccelMap;
	AccelMap mapAccel_;
};


/****************************************************************************
 *
 * CustomAccelerator
 *
 */

qs::CustomAccelerator::CustomAccelerator(
	const ACCEL* pAccel, int nSize, QSTATUS* pstatus) :
	AbstractAccelerator(pAccel, nSize, pstatus),
	pImpl_(0)
{
	if (*pstatus != QSTATUS_SUCCESS)
		return;
	
	DECLARE_QSTATUS();
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus)
	
	STLWrapper<CustomAcceleratorImpl::AccelMap> wrapper(pImpl_->mapAccel_);
	for (int n = 0; n < nSize; ++n, ++pAccel) {
		unsigned int nKey = pAccel->key;
		BYTE b = pAccel->fVirt;
		if (b & FALT)
			nKey |= CustomAcceleratorImpl::ALT;
		if (b & FCONTROL)
			nKey |= CustomAcceleratorImpl::CTRL;
		if (b & FSHIFT)
			nKey |= CustomAcceleratorImpl::SHIFT;
		if (b & FVIRTKEY)
			nKey |= CustomAcceleratorImpl::VIRTUAL;
		status = wrapper.push_back(
			CustomAcceleratorImpl::AccelMap::value_type(nKey, pAccel->cmd));
		CHECK_QSTATUS_SET(pstatus);
	}
	std::sort(pImpl_->mapAccel_.begin(), pImpl_->mapAccel_.end(),
		binary_compose_f_gx_hy(
			std::less<CustomAcceleratorImpl::AccelMap::value_type::first_type>(),
			std::select1st<CustomAcceleratorImpl::AccelMap::value_type>(),
			std::select1st<CustomAcceleratorImpl::AccelMap::value_type>()));
}

qs::CustomAccelerator::~CustomAccelerator()
{
	delete pImpl_;
	pImpl_ = 0;
}

QSTATUS qs::CustomAccelerator::translateAccelerator(HWND hwnd,
	const MSG& msg, bool* pbProcessed)
{
	assert(pbProcessed);
	
	*pbProcessed = false;
	
	if (msg.message != WM_KEYDOWN &&
		msg.message != WM_SYSKEYDOWN &&
		msg.message != WM_CHAR &&
		msg.message != WM_SYSCHAR)
		return QSTATUS_SUCCESS;
	
	unsigned int nKey = msg.wParam;
	if (::GetKeyState(VK_MENU) < 0)
		nKey |= CustomAcceleratorImpl::ALT;
	if (::GetKeyState(VK_CONTROL) < 0)
		nKey |= CustomAcceleratorImpl::CTRL;
	if (msg.message == WM_KEYDOWN ||
		msg.message == WM_SYSKEYDOWN) {
		if (::GetKeyState(VK_SHIFT) < 0)
			nKey |= CustomAcceleratorImpl::SHIFT;
		nKey |= CustomAcceleratorImpl::VIRTUAL;
	}
	
	CustomAcceleratorImpl::AccelMap::iterator it = std::lower_bound(
		pImpl_->mapAccel_.begin(), pImpl_->mapAccel_.end(),
		CustomAcceleratorImpl::AccelMap::value_type(nKey, 0),
		binary_compose_f_gx_hy(
			std::less<CustomAcceleratorImpl::AccelMap::value_type::first_type>(),
			std::select1st<CustomAcceleratorImpl::AccelMap::value_type>(),
			std::select1st<CustomAcceleratorImpl::AccelMap::value_type>()));
	if (it == pImpl_->mapAccel_.end() || (*it).first != nKey)
		return QSTATUS_SUCCESS;
	::SendMessage(hwnd, WM_COMMAND, MAKEWPARAM((*it).second, 1), 0);
	*pbProcessed = true;
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * CustomAcceleratorFactory
 *
 */

qs::CustomAcceleratorFactory::CustomAcceleratorFactory()
{
}

qs::CustomAcceleratorFactory::~CustomAcceleratorFactory()
{
}

QSTATUS qs::CustomAcceleratorFactory::createAccelerator(
	const ACCEL* pAccel, int nSize, Accelerator** ppAccelerator)
{
	assert(ppAccelerator);
	
	DECLARE_QSTATUS();
	
	CustomAccelerator* p = 0;
	status = newQsObject(pAccel, nSize, &p);
	*ppAccelerator = p;
	
	return status;
}
