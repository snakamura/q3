/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QSACTION_H__
#define __QSACTION_H__

#include <qs.h>
#include <qsstring.h>

#include <vector>


namespace qs {

class Action;
class ActionEvent;
	class AbstractAction;
class ActionMap;


/****************************************************************************
 *
 * Action
 *
 */

class QSEXPORTCLASS Action
{
public:
	virtual ~Action();

public:
	virtual QSTATUS invoke(const ActionEvent& event) = 0;
	virtual QSTATUS isEnabled(const ActionEvent& event, bool* pbEnabled) = 0;
	virtual QSTATUS isChecked(const ActionEvent& event, bool* pbChecked) = 0;
	virtual QSTATUS getText(const ActionEvent& event, WSTRING* pwstrText) = 0;
};


/****************************************************************************
 *
 * ActionEvent
 *
 */

class QSEXPORTCLASS ActionEvent
{
public:
	enum Modifier {
		MODIFIER_SHIFT	= 0x01,
		MODIFIER_CTRL	= 0x02,
		MODIFIER_ALT	= 0x04
	};

public:
	ActionEvent(unsigned int nId, unsigned int nModifier);
	ActionEvent(unsigned int nId, unsigned int nModifier, void* pParam);
	~ActionEvent();

public:
	unsigned int getId() const;
	unsigned int getModifier() const;
	void* getParam() const;

private:
	ActionEvent(const ActionEvent&);
	ActionEvent& operator=(const ActionEvent&);

private:
	unsigned int nId_;
	unsigned int nModifier_;
	void* pParam_;
};


/****************************************************************************
 *
 * AbstractAction
 *
 */

class QSEXPORTCLASS AbstractAction : public Action
{
public:
	AbstractAction();
	virtual ~AbstractAction();

public:
	virtual QSTATUS isEnabled(const ActionEvent& event, bool* pbEnabled);
	virtual QSTATUS isChecked(const ActionEvent& event, bool* pbChecked);
	virtual QSTATUS getText(const ActionEvent& event, WSTRING* pwstrText);

private:
	AbstractAction(const AbstractAction&);
	AbstractAction& operator=(const AbstractAction&);
};


/****************************************************************************
 *
 * ActionMap
 *
 */

class QSEXPORTCLASS ActionMap
{
public:
	explicit ActionMap(QSTATUS* pstatus);
	~ActionMap();

public:
	Action* getAction(unsigned int nId) const;
	QSTATUS addAction(unsigned int nId, Action* pAction);
	QSTATUS addAction(unsigned int nIdFrom,
		unsigned int nIdTo, Action* pAction);

private:
	ActionMap(const ActionMap&);
	ActionMap& operator=(const ActionMap&);

private:
	struct ActionMapImpl* pImpl_;
};


/****************************************************************************
 *
 * InitAction1
 *
 */

template<class T, class Arg>
class InitAction1
{
public:
	InitAction1(ActionMap* pActionMap, unsigned int nId, const Arg& arg);
	operator unsigned int() const;

private:
	InitAction1(const InitAction1&);
	InitAction1& operator=(const InitAction1&);

private:
	unsigned int status_;
};


/****************************************************************************
 *
 * InitAction2
 *
 */

template<class T, class Arg1, class Arg2>
class InitAction2
{
public:
	InitAction2(ActionMap* pActionMap, unsigned int nId,
		const Arg1& arg1, const Arg2& arg2);
	operator unsigned int() const;

private:
	InitAction2(const InitAction2&);
	InitAction2& operator=(const InitAction2&);

private:
	unsigned int status_;
};


/****************************************************************************
 *
 * InitAction3
 *
 */

template<class T, class Arg1, class Arg2, class Arg3>
class InitAction3
{
public:
	InitAction3(ActionMap* pActionMap, unsigned int nId,
		const Arg1& arg1, const Arg2& arg2, const Arg3& arg3);
	operator unsigned int() const;

private:
	InitAction3(const InitAction3&);
	InitAction3& operator=(const InitAction3&);

private:
	unsigned int status_;
};


/****************************************************************************
 *
 * InitAction4
 *
 */

template<class T, class Arg1, class Arg2, class Arg3, class Arg4>
class InitAction4
{
public:
	InitAction4(ActionMap* pActionMap, unsigned int nId,
		const Arg1& arg1, const Arg2& arg2, const Arg3& arg3, const Arg4& arg4);
	operator unsigned int() const;

private:
	InitAction4(const InitAction4&);
	InitAction4& operator=(const InitAction4&);

private:
	unsigned int status_;
};


/****************************************************************************
 *
 * InitAction5
 *
 */

template<class T, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5>
class InitAction5
{
public:
	InitAction5(ActionMap* pActionMap, unsigned int nId,
		const Arg1& arg1, const Arg2& arg2, const Arg3& arg3,
		const Arg4& arg4, const Arg5& arg5);
	operator unsigned int() const;

private:
	InitAction5(const InitAction5&);
	InitAction5& operator=(const InitAction5&);

private:
	unsigned int status_;
};


/****************************************************************************
 *
 * InitAction6
 *
 */

template<class T, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5, class Arg6>
class InitAction6
{
public:
	InitAction6(ActionMap* pActionMap, unsigned int nId,
		const Arg1& arg1, const Arg2& arg2, const Arg3& arg3,
		const Arg4& arg4, const Arg5& arg5, const Arg6& arg6);
	operator unsigned int() const;

private:
	InitAction6(const InitAction6&);
	InitAction6& operator=(const InitAction6&);

private:
	unsigned int status_;
};


/****************************************************************************
 *
 * InitAction7
 *
 */

template<class T, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5, class Arg6, class Arg7>
class InitAction7
{
public:
	InitAction7(ActionMap* pActionMap, unsigned int nId,
		const Arg1& arg1, const Arg2& arg2, const Arg3& arg3,
		const Arg4& arg4, const Arg5& arg5, const Arg6& arg6, const Arg7& arg7);
	operator unsigned int() const;

private:
	InitAction7(const InitAction7&);
	InitAction7& operator=(const InitAction7&);

private:
	unsigned int status_;
};


/****************************************************************************
 *
 * InitAction8
 *
 */

template<class T, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5, class Arg6, class Arg7, class Arg8>
class InitAction8
{
public:
	InitAction8(ActionMap* pActionMap, unsigned int nId,
		const Arg1& arg1, const Arg2& arg2, const Arg3& arg3,
		const Arg4& arg4, const Arg5& arg5, const Arg6& arg6,
		const Arg7& arg7, const Arg8& arg8);
	operator unsigned int() const;

private:
	InitAction8(const InitAction8&);
	InitAction8& operator=(const InitAction8&);

private:
	unsigned int status_;
};


/****************************************************************************
 *
 * InitAction9
 *
 */

template<class T, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5, class Arg6, class Arg7, class Arg8, class Arg9>
class InitAction9
{
public:
	InitAction9(ActionMap* pActionMap, unsigned int nId,
		const Arg1& arg1, const Arg2& arg2, const Arg3& arg3,
		const Arg4& arg4, const Arg5& arg5, const Arg6& arg6,
		const Arg7& arg7, const Arg8& arg8, const Arg9& arg9);
	operator unsigned int() const;

private:
	InitAction9(const InitAction9&);
	InitAction9& operator=(const InitAction9&);

private:
	unsigned int status_;
};


/****************************************************************************
 *
 * InitActionRange1
 *
 */

template<class T, class Arg>
class InitActionRange1
{
public:
	InitActionRange1(ActionMap* pActionMap, unsigned int nFrom,
		unsigned int nTo, const Arg& arg);
	operator unsigned int() const;

private:
	InitActionRange1(const InitActionRange1&);
	InitActionRange1& operator=(const InitActionRange1&);

private:
	unsigned int status_;
};


/****************************************************************************
 *
 * InitActionRange2
 *
 */

template<class T, class Arg1, class Arg2>
class InitActionRange2
{
public:
	InitActionRange2(ActionMap* pActionMap, unsigned int nFrom,
		unsigned int nTo, const Arg1& arg1, const Arg2& arg2);
	operator unsigned int() const;

private:
	InitActionRange2(const InitActionRange2&);
	InitActionRange2& operator=(const InitActionRange2&);

private:
	unsigned int status_;
};


/****************************************************************************
 *
 * InitActionRange3
 *
 */

template<class T, class Arg1, class Arg2, class Arg3>
class InitActionRange3
{
public:
	InitActionRange3(ActionMap* pActionMap, unsigned int nFrom,
		unsigned int nTo, const Arg1& arg1, const Arg2& arg2, const Arg3& arg3);
	operator unsigned int() const;

private:
	InitActionRange3(const InitActionRange3&);
	InitActionRange3& operator=(const InitActionRange3&);

private:
	unsigned int status_;
};


/****************************************************************************
 *
 * InitActionRange4
 *
 */

template<class T, class Arg1, class Arg2, class Arg3, class Arg4>
class InitActionRange4
{
public:
	InitActionRange4(ActionMap* pActionMap, unsigned int nFrom,
		unsigned int nTo, const Arg1& arg1, const Arg2& arg2,
		const Arg3& arg3, const Arg4& arg4);
	operator unsigned int() const;

private:
	InitActionRange4(const InitActionRange4&);
	InitActionRange4& operator=(const InitActionRange4&);

private:
	unsigned int status_;
};


/****************************************************************************
 *
 * InitActionRange5
 *
 */

template<class T, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5>
class InitActionRange5
{
public:
	InitActionRange5(ActionMap* pActionMap, unsigned int nFrom,
		unsigned int nTo, const Arg1& arg1, const Arg2& arg2,
		const Arg3& arg3, const Arg4& arg4, const Arg5& arg5);
	operator unsigned int() const;

private:
	InitActionRange5(const InitActionRange5&);
	InitActionRange5& operator=(const InitActionRange5&);

private:
	unsigned int status_;
};


/****************************************************************************
 *
 * InitActionRange6
 *
 */

template<class T, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5, class Arg6>
class InitActionRange6
{
public:
	InitActionRange6(ActionMap* pActionMap, unsigned int nFrom,
		unsigned int nTo, const Arg1& arg1, const Arg2& arg2,
		const Arg3& arg3, const Arg4& arg4, const Arg5& arg5, const Arg6& arg6);
	operator unsigned int() const;

private:
	InitActionRange6(const InitActionRange6&);
	InitActionRange6& operator=(const InitActionRange6&);

private:
	unsigned int status_;
};


/****************************************************************************
 *
 * InitActionRange7
 *
 */

template<class T, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5, class Arg6, class Arg7>
class InitActionRange7
{
public:
	InitActionRange7(ActionMap* pActionMap, unsigned int nFrom,
		unsigned int nTo, const Arg1& arg1, const Arg2& arg2,
		const Arg3& arg3, const Arg4& arg4, const Arg5& arg5,
		const Arg6& arg6, const Arg7& arg7);
	operator unsigned int() const;

private:
	InitActionRange7(const InitActionRange7&);
	InitActionRange7& operator=(const InitActionRange7&);

private:
	unsigned int status_;
};


/****************************************************************************
 *
 * InitActionRange8
 *
 */

template<class T, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5, class Arg6, class Arg7, class Arg8>
class InitActionRange8
{
public:
	InitActionRange8(ActionMap* pActionMap, unsigned int nFrom,
		unsigned int nTo, const Arg1& arg1, const Arg2& arg2,
		const Arg3& arg3, const Arg4& arg4, const Arg5& arg5,
		const Arg6& arg6, const Arg7& arg7, const Arg8& arg8);
	operator unsigned int() const;

private:
	InitActionRange8(const InitActionRange8&);
	InitActionRange8& operator=(const InitActionRange8&);

private:
	unsigned int status_;
};


/****************************************************************************
 *
 * InitActionRange9
 *
 */

template<class T, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5, class Arg6, class Arg7, class Arg8, class Arg9>
class InitActionRange9
{
public:
	InitActionRange9(ActionMap* pActionMap, unsigned int nFrom,
		unsigned int nTo, const Arg1& arg1, const Arg2& arg2,
		const Arg3& arg3, const Arg4& arg4, const Arg5& arg5,
		const Arg6& arg6, const Arg7& arg7, const Arg8& arg8, const Arg9& arg9);
	operator unsigned int() const;

private:
	InitActionRange9(const InitActionRange9&);
	InitActionRange9& operator=(const InitActionRange9&);

private:
	unsigned int status_;
};


/****************************************************************************
 *
 * ActionItem
 *
 */

struct QSEXPORTCLASS ActionItem
{
	const WCHAR* pwszAction_;
	UINT nId_;
};

}

#include <qsaction.inl>

#endif // __QSACTION_H__
