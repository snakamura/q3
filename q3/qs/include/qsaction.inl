/*
 * $Id: qsaction.inl,v 1.2 2003/05/07 07:25:22 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QSACTION_INL__
#define __QSACTION_INL__


/****************************************************************************
 *
 * InitAction1
 *
 */

template<class T, class Arg>
qs::InitAction1<T, Arg>::InitAction1(qs::ActionMap* pActionMap,
	unsigned int nId, const Arg& arg) :
	status_(QSTATUS_SUCCESS)
{
	std::auto_ptr<T> pAction;
	status_ = newQsObject(arg, &pAction);
	if (status_ == QSTATUS_SUCCESS) {
		status_ = pActionMap->addAction(nId, pAction.get());
		if (status_ == QSTATUS_SUCCESS)
			pAction.release();
	}
}

template<class T, class Arg>
qs::InitAction1<T, Arg>::operator unsigned int() const
{
	return status_;
}


/****************************************************************************
 *
 * InitAction2
 *
 */

template<class T, class Arg1, class Arg2>
qs::InitAction2<T, Arg1, Arg2>::InitAction2(qs::ActionMap* pActionMap,
	unsigned int nId, const Arg1& arg1, const Arg2& arg2) :
	status_(QSTATUS_SUCCESS)
{
	std::auto_ptr<T> pAction;
	status_ = newQsObject(arg1, arg2, &pAction);
	if (status_ == QSTATUS_SUCCESS) {
		status_ = pActionMap->addAction(nId, pAction.get());
		if (status_ == QSTATUS_SUCCESS)
			pAction.release();
	}
}

template<class T, class Arg1, class Arg2>
qs::InitAction2<T, Arg1, Arg2>::operator unsigned int() const
{
	return status_;
}


/****************************************************************************
 *
 * InitAction3
 *
 */

template<class T, class Arg1, class Arg2, class Arg3>
qs::InitAction3<T, Arg1, Arg2, Arg3>::InitAction3(qs::ActionMap* pActionMap,
	unsigned int nId, const Arg1& arg1, const Arg2& arg2, const Arg3& arg3) :
	status_(QSTATUS_SUCCESS)
{
	std::auto_ptr<T> pAction;
	status_ = newQsObject(arg1, arg2, arg3, &pAction);
	if (status_ == QSTATUS_SUCCESS) {
		status_ = pActionMap->addAction(nId, pAction.get());
		if (status_ == QSTATUS_SUCCESS)
			pAction.release();
	}
}

template<class T, class Arg1, class Arg2, class Arg3>
qs::InitAction3<T, Arg1, Arg2, Arg3>::operator unsigned int() const
{
	return status_;
}


/****************************************************************************
 *
 * InitAction4
 *
 */

template<class T, class Arg1, class Arg2, class Arg3, class Arg4>
qs::InitAction4<T, Arg1, Arg2, Arg3, Arg4>::InitAction4(qs::ActionMap* pActionMap,
	unsigned int nId, const Arg1& arg1, const Arg2& arg2, const Arg3& arg3, const Arg4& arg4) :
	status_(QSTATUS_SUCCESS)
{
	std::auto_ptr<T> pAction;
	status_ = newQsObject(arg1, arg2, arg3, arg4, &pAction);
	if (status_ == QSTATUS_SUCCESS) {
		status_ = pActionMap->addAction(nId, pAction.get());
		if (status_ == QSTATUS_SUCCESS)
			pAction.release();
	}
}

template<class T, class Arg1, class Arg2, class Arg3, class Arg4>
qs::InitAction4<T, Arg1, Arg2, Arg3, Arg4>::operator unsigned int() const
{
	return status_;
}


/****************************************************************************
 *
 * InitAction5
 *
 */

template<class T, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5>
qs::InitAction5<T, Arg1, Arg2, Arg3, Arg4, Arg5>::InitAction5(
	qs::ActionMap* pActionMap, unsigned int nId, const Arg1& arg1,
	const Arg2& arg2, const Arg3& arg3, const Arg4& arg4, const Arg5& arg5) :
	status_(QSTATUS_SUCCESS)
{
	std::auto_ptr<T> pAction;
	status_ = newQsObject(arg1, arg2, arg3, arg4, arg5, &pAction);
	if (status_ == QSTATUS_SUCCESS) {
		status_ = pActionMap->addAction(nId, pAction.get());
		if (status_ == QSTATUS_SUCCESS)
			pAction.release();
	}
}

template<class T, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5>
qs::InitAction5<T, Arg1, Arg2, Arg3, Arg4, Arg5>::operator unsigned int() const
{
	return status_;
}


/****************************************************************************
 *
 * InitAction6
 *
 */

template<class T, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5, class Arg6>
qs::InitAction6<T, Arg1, Arg2, Arg3, Arg4, Arg5, Arg6>::InitAction6(
	qs::ActionMap* pActionMap, unsigned int nId, const Arg1& arg1, const Arg2& arg2,
	const Arg3& arg3, const Arg4& arg4, const Arg5& arg5, const Arg6& arg6) :
	status_(QSTATUS_SUCCESS)
{
	std::auto_ptr<T> pAction;
	status_ = newQsObject(arg1, arg2, arg3, arg4, arg5, arg6, &pAction);
	if (status_ == QSTATUS_SUCCESS) {
		status_ = pActionMap->addAction(nId, pAction.get());
		if (status_ == QSTATUS_SUCCESS)
			pAction.release();
	}
}

template<class T, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5, class Arg6>
qs::InitAction6<T, Arg1, Arg2, Arg3, Arg4, Arg5, Arg6>::operator unsigned int() const
{
	return status_;
}


/****************************************************************************
 *
 * InitAction7
 *
 */

template<class T, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5, class Arg6, class Arg7>
qs::InitAction7<T, Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7>::InitAction7(
	qs::ActionMap* pActionMap, unsigned int nId, const Arg1& arg1,
	const Arg2& arg2, const Arg3& arg3, const Arg4& arg4, const Arg5& arg5,
	const Arg6& arg6, const Arg7& arg7) :
	status_(QSTATUS_SUCCESS)
{
	std::auto_ptr<T> pAction;
	status_ = newQsObject(arg1, arg2, arg3, arg4, arg5, arg6, arg7, &pAction);
	if (status_ == QSTATUS_SUCCESS) {
		status_ = pActionMap->addAction(nId, pAction.get());
		if (status_ == QSTATUS_SUCCESS)
			pAction.release();
	}
}

template<class T, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5, class Arg6, class Arg7>
qs::InitAction7<T, Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7>::operator unsigned int() const
{
	return status_;
}


/****************************************************************************
 *
 * InitAction8
 *
 */

template<class T, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5, class Arg6, class Arg7, class Arg8>
qs::InitAction8<T, Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7, Arg8>::InitAction8(
	qs::ActionMap* pActionMap, unsigned int nId, const Arg1& arg1,
	const Arg2& arg2, const Arg3& arg3, const Arg4& arg4, const Arg5& arg5,
	const Arg6& arg6, const Arg7& arg7, const Arg8& arg8) :
	status_(QSTATUS_SUCCESS)
{
	std::auto_ptr<T> pAction;
	status_ = newQsObject(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, &pAction);
	if (status_ == QSTATUS_SUCCESS) {
		status_ = pActionMap->addAction(nId, pAction.get());
		if (status_ == QSTATUS_SUCCESS)
			pAction.release();
	}
}

template<class T, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5, class Arg6, class Arg7, class Arg8>
qs::InitAction8<T, Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7, Arg8>::operator unsigned int() const
{
	return status_;
}


/****************************************************************************
 *
 * InitActionRange1
 *
 */

template<class T, class Arg>
qs::InitActionRange1<T, Arg>::InitActionRange1(qs::ActionMap* pActionMap,
	unsigned int nFrom, unsigned int nTo, const Arg& arg) :
	status_(QSTATUS_SUCCESS)
{
	std::auto_ptr<T> pAction;
	status_ = newQsObject(arg, &pAction);
	if (status_ == QSTATUS_SUCCESS) {
		status_ = pActionMap->addAction(nFrom, nTo, pAction.get());
		if (status_ == QSTATUS_SUCCESS)
			pAction.release();
	}
}

template<class T, class Arg>
qs::InitActionRange1<T, Arg>::operator unsigned int() const
{
	return status_;
}


/****************************************************************************
 *
 * InitActionRange2
 *
 */

template<class T, class Arg1, class Arg2>
qs::InitActionRange2<T, Arg1, Arg2>::InitActionRange2(qs::ActionMap* pActionMap,
	unsigned int nFrom, unsigned int nTo, const Arg1& arg1, const Arg2& arg2) :
	status_(QSTATUS_SUCCESS)
{
	std::auto_ptr<T> pAction;
	status_ = newQsObject(arg1, arg2, &pAction);
	if (status_ == QSTATUS_SUCCESS) {
		status_ = pActionMap->addAction(nFrom, nTo, pAction.get());
		if (status_ == QSTATUS_SUCCESS)
			pAction.release();
	}
}

template<class T, class Arg1, class Arg2>
qs::InitActionRange2<T, Arg1, Arg2>::operator unsigned int() const
{
	return status_;
}


/****************************************************************************
 *
 * InitActionRange3
 *
 */

template<class T, class Arg1, class Arg2, class Arg3>
qs::InitActionRange3<T, Arg1, Arg2, Arg3>::InitActionRange3(
	qs::ActionMap* pActionMap, unsigned int nFrom, unsigned int nTo,
	const Arg1& arg1, const Arg2& arg2, const Arg3& arg3) :
	status_(QSTATUS_SUCCESS)
{
	std::auto_ptr<T> pAction;
	status_ = newQsObject(arg1, arg2, arg3, &pAction);
	if (status_ == QSTATUS_SUCCESS) {
		status_ = pActionMap->addAction(nFrom, nTo, pAction.get());
		if (status_ == QSTATUS_SUCCESS)
			pAction.release();
	}
}

template<class T, class Arg1, class Arg2, class Arg3>
qs::InitActionRange3<T, Arg1, Arg2, Arg3>::operator unsigned int() const
{
	return status_;
}


/****************************************************************************
 *
 * InitActionRange4
 *
 */

template<class T, class Arg1, class Arg2, class Arg3, class Arg4>
qs::InitActionRange4<T, Arg1, Arg2, Arg3, Arg4>::InitActionRange4(
	qs::ActionMap* pActionMap, unsigned int nFrom, unsigned int nTo,
	const Arg1& arg1, const Arg2& arg2, const Arg3& arg3, const Arg4& arg4) :
	status_(QSTATUS_SUCCESS)
{
	std::auto_ptr<T> pAction;
	status_ = newQsObject(arg1, arg2, arg3, arg4, &pAction);
	if (status_ == QSTATUS_SUCCESS) {
		status_ = pActionMap->addAction(nFrom, nTo, pAction.get());
		if (status_ == QSTATUS_SUCCESS)
			pAction.release();
	}
}

template<class T, class Arg1, class Arg2, class Arg3, class Arg4>
qs::InitActionRange4<T, Arg1, Arg2, Arg3, Arg4>::operator unsigned int() const
{
	return status_;
}


/****************************************************************************
 *
 * InitActionRange5
 *
 */

template<class T, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5>
qs::InitActionRange5<T, Arg1, Arg2, Arg3, Arg4, Arg5>::InitActionRange5(
	qs::ActionMap* pActionMap, unsigned int nFrom, unsigned int nTo,
	const Arg1& arg1, const Arg2& arg2, const Arg3& arg3,
	const Arg4& arg4, const Arg5& arg5) :
	status_(QSTATUS_SUCCESS)
{
	std::auto_ptr<T> pAction;
	status_ = newQsObject(arg1, arg2, arg3, arg4, arg5, &pAction);
	if (status_ == QSTATUS_SUCCESS) {
		status_ = pActionMap->addAction(nFrom, nTo, pAction.get());
		if (status_ == QSTATUS_SUCCESS)
			pAction.release();
	}
}

template<class T, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5>
qs::InitActionRange5<T, Arg1, Arg2, Arg3, Arg4, Arg5>::operator unsigned int() const
{
	return status_;
}


/****************************************************************************
 *
 * InitActionRange6
 *
 */

template<class T, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5, class Arg6>
qs::InitActionRange6<T, Arg1, Arg2, Arg3, Arg4, Arg5, Arg6>::InitActionRange6(
	qs::ActionMap* pActionMap, unsigned int nFrom, unsigned int nTo,
	const Arg1& arg1, const Arg2& arg2, const Arg3& arg3,
	const Arg4& arg4, const Arg5& arg5, const Arg6& arg6) :
	status_(QSTATUS_SUCCESS)
{
	std::auto_ptr<T> pAction;
	status_ = newQsObject(arg1, arg2, arg3, arg4, arg5, arg6, &pAction);
	if (status_ == QSTATUS_SUCCESS) {
		status_ = pActionMap->addAction(nFrom, nTo, pAction.get());
		if (status_ == QSTATUS_SUCCESS)
			pAction.release();
	}
}

template<class T, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5, class Arg6>
qs::InitActionRange6<T, Arg1, Arg2, Arg3, Arg4, Arg5, Arg6>::operator unsigned int() const
{
	return status_;
}


/****************************************************************************
 *
 * InitActionRange7
 *
 */

template<class T, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5, class Arg6, class Arg7>
qs::InitActionRange7<T, Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7>::InitActionRange7(
	qs::ActionMap* pActionMap, unsigned int nFrom, unsigned int nTo,
	const Arg1& arg1, const Arg2& arg2, const Arg3& arg3,
	const Arg4& arg4, const Arg5& arg5, const Arg6& arg6, const Arg7& arg7) :
	status_(QSTATUS_SUCCESS)
{
	std::auto_ptr<T> pAction;
	status_ = newQsObject(arg1, arg2, arg3, arg4, arg5, arg6, arg7, &pAction);
	if (status_ == QSTATUS_SUCCESS) {
		status_ = pActionMap->addAction(nFrom, nTo, pAction.get());
		if (status_ == QSTATUS_SUCCESS)
			pAction.release();
	}
}

template<class T, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5, class Arg6, class Arg7>
qs::InitActionRange7<T, Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7>::operator unsigned int() const
{
	return status_;
}


/****************************************************************************
 *
 * InitActionRange8
 *
 */

template<class T, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5, class Arg6, class Arg7, class Arg8>
qs::InitActionRange8<T, Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7, Arg8>::InitActionRange8(
	qs::ActionMap* pActionMap, unsigned int nFrom, unsigned int nTo,
	const Arg1& arg1, const Arg2& arg2, const Arg3& arg3, const Arg4& arg4,
	const Arg5& arg5, const Arg6& arg6, const Arg7& arg7, const Arg8& arg8) :
	status_(QSTATUS_SUCCESS)
{
	std::auto_ptr<T> pAction;
	status_ = newQsObject(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, &pAction);
	if (status_ == QSTATUS_SUCCESS) {
		status_ = pActionMap->addAction(nFrom, nTo, pAction.get());
		if (status_ == QSTATUS_SUCCESS)
			pAction.release();
	}
}

template<class T, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5, class Arg6, class Arg7, class Arg8>
qs::InitActionRange8<T, Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7, Arg8>::operator unsigned int() const
{
	return status_;
}

#endif // __QSACTION_INL__
