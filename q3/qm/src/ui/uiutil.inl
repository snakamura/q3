/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __UIUTIL_INL__
#define __UIUTIL_INL__


/****************************************************************************
 *
 * ProgressDialogMessageOperationCallbackBase
 *
 */

template<class Callback>
qm::ProgressDialogMessageOperationCallbackBase<Callback>::ProgressDialogMessageOperationCallbackBase(HWND hwnd,
																						   UINT nTitle,
																						   UINT nMessage) :
	hwnd_(hwnd),
	nTitle_(nTitle),
	nMessage_(nMessage),
	nCount_(-1),
	nPos_(0)
{
}

template<class Callback>
qm::ProgressDialogMessageOperationCallbackBase<Callback>::~ProgressDialogMessageOperationCallbackBase()
{
	if (pDialog_.get())
		pDialog_->term();
}

template<class Callback>
bool qm::ProgressDialogMessageOperationCallbackBase<Callback>::isCanceled()
{
	if (pDialog_.get())
		return pDialog_->isCanceled();
	else
		return false;
}

template<class Callback>
void qm::ProgressDialogMessageOperationCallbackBase<Callback>::setCount(unsigned int nCount)
{
	if (nCount_ == -1) {
		nCount_ = nCount;
		if (pDialog_.get())
			pDialog_->setRange(0, nCount);
	}
}

template<class Callback>
void qm::ProgressDialogMessageOperationCallbackBase<Callback>::step(unsigned int nStep)
{
	nPos_ += nStep;
	if (pDialog_.get())
		pDialog_->setPos(nPos_);
}

template<class Callback>
void qm::ProgressDialogMessageOperationCallbackBase<Callback>::show()
{
	if (!pDialog_.get()) {
		std::auto_ptr<ProgressDialog> pDialog(new ProgressDialog(nTitle_));
		pDialog->init(hwnd_);
		pDialog_ = pDialog;
		
		pDialog_->setMessage(nMessage_);
		
		if (nCount_ != -1) {
			pDialog_->setRange(0, nCount_);
			pDialog_->setPos(nPos_);
		}
	}
}

template<class Callback>
qm::ProgressDialog* qm::ProgressDialogMessageOperationCallbackBase<Callback>::getDialog() const
{
	return pDialog_.get();
}

#endif // __UIUTIL_INL__
