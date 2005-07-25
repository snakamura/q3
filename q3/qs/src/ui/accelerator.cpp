/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#pragma warning(disable:4786)

#include <qsaccelerator.h>
#include <qsstl.h>

#include <algorithm>
#include <utility>
#include <vector>

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

qs::AbstractAccelerator::AbstractAccelerator(const ACCEL* pAccel,
											 size_t nSize) :
	pImpl_(0)
{
	pImpl_ = new AbstractAcceleratorImpl();
	
	for (size_t n = 0; n < nSize; ++n, ++pAccel)
		pImpl_->mapIdToKey_.push_back(
			AbstractAcceleratorImpl::IdKeyMap::value_type(
				pAccel->cmd, MAKELONG(pAccel->key, pAccel->fVirt)));
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

wstring_ptr qs::AbstractAccelerator::getKeyFromId(UINT nId)
{
	AbstractAcceleratorImpl::IdKeyMap::const_iterator it = std::lower_bound(
		pImpl_->mapIdToKey_.begin(), pImpl_->mapIdToKey_.end(),
		AbstractAcceleratorImpl::IdKeyMap::value_type(nId, 0),
		binary_compose_f_gx_hy(
			std::less<AbstractAcceleratorImpl::IdKeyMap::value_type::first_type>(),
			std::select1st<AbstractAcceleratorImpl::IdKeyMap::value_type>(),
			std::select1st<AbstractAcceleratorImpl::IdKeyMap::value_type>()));
	if (it == pImpl_->mapIdToKey_.end() || (*it).first != nId)
		return 0;
	
	StringBuffer<WSTRING> buffer(10);
	
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
		if (nVirt & modifiers[n].nKey_)
			buffer.append(modifiers[n].pwsz_);
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
		buffer.append(static_cast<WCHAR>(nKey));
	}
	else if (('A' <= nKey && nKey <= 'Z') || ('0' <= nKey && nKey <= '9')) {
		buffer.append(static_cast<WCHAR>(nKey));
	}
	else if (VK_F1 <= nKey && nKey <= VK_F12) {
		WCHAR wsz[8];
		swprintf(wsz, L"F%d", nKey - VK_F1 + 1);
		buffer.append(wsz);
	}
	else {
		for (int n = 0; n < countof(keys); ++n) {
			if (nKey == keys[n].nKey_) {
				buffer.append(keys[n].pwsz_);
				break;
			}
		}
	}
	
	return buffer.getString();
}


/****************************************************************************
 *
 * SystemAccelerator
 *
 */

qs::SystemAccelerator::SystemAccelerator(const ACCEL* pAccel,
										 size_t nSize) :
	AbstractAccelerator(pAccel, nSize),
	haccel_(0)
{
	haccel_ = ::CreateAcceleratorTable(
		const_cast<ACCEL*>(pAccel), static_cast<int>(nSize));
}

qs::SystemAccelerator::~SystemAccelerator()
{
	::DestroyAcceleratorTable(haccel_);
}

bool qs::SystemAccelerator::translateAccelerator(HWND hwnd,
												 const MSG& msg)
{
	return ::TranslateAccelerator(hwnd,
		haccel_, const_cast<MSG*>(&msg)) != 0;
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

std::auto_ptr<Accelerator> qs::SystemAcceleratorFactory::createAccelerator(const ACCEL* pAccel,
																		   size_t nSize)
{
	return std::auto_ptr<Accelerator>(new SystemAccelerator(pAccel, nSize));
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

qs::CustomAccelerator::CustomAccelerator(const ACCEL* pAccel,
										 size_t nSize) :
	AbstractAccelerator(pAccel, nSize),
	pImpl_(0)
{
	pImpl_ = new CustomAcceleratorImpl();
	
	for (size_t n = 0; n < nSize; ++n, ++pAccel) {
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
		pImpl_->mapAccel_.push_back(
			CustomAcceleratorImpl::AccelMap::value_type(nKey, pAccel->cmd));
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

bool qs::CustomAccelerator::translateAccelerator(HWND hwnd,
												 const MSG& msg)
{
	if (msg.message != WM_KEYDOWN &&
		msg.message != WM_SYSKEYDOWN &&
		msg.message != WM_CHAR &&
		msg.message != WM_SYSCHAR)
		return false;
	
	unsigned int nKey = static_cast<int>(msg.wParam);
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
		return false;
	::SendMessage(hwnd, WM_COMMAND, MAKEWPARAM((*it).second, 1), 0);
	
	return true;
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

std::auto_ptr<Accelerator> qs::CustomAcceleratorFactory::createAccelerator(const ACCEL* pAccel,
																		   size_t nSize)
{
	return std::auto_ptr<Accelerator>(new CustomAccelerator(pAccel, nSize));
}
