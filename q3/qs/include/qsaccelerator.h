/*
 * $Id: qsaccelerator.h,v 1.1.1.1 2003/04/29 08:07:34 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QSACCELERATOR_H__
#define __QSACCELERATOR_H__

#include <qs.h>
#include <qsstring.h>


namespace qs {

/****************************************************************************
 *
 * Accelerator
 *
 */

class QSEXPORTCLASS Accelerator
{
public:
	virtual ~Accelerator();

public:
	virtual QSTATUS translateAccelerator(HWND hwnd,
		const MSG& msg, bool* pbProcessed) = 0;
	virtual QSTATUS getKeyFromId(UINT nId, WSTRING* pwstr) = 0;
};


/****************************************************************************
 *
 * AcceleratorFactory
 *
 */

class QSEXPORTCLASS AcceleratorFactory
{
public:
	virtual ~AcceleratorFactory();

public:
	virtual QSTATUS createAccelerator(const ACCEL* pAccel, int nSize,
		Accelerator** ppAccelerator) = 0;
};


/****************************************************************************
 *
 * AbstractAccelerator
 *
 */

class QSEXPORTCLASS AbstractAccelerator : public Accelerator
{
protected:
	AbstractAccelerator(const ACCEL* pAccel, int nSize, QSTATUS* pstatus);

public:
	virtual ~AbstractAccelerator();

public:
	virtual QSTATUS getKeyFromId(UINT nId, WSTRING* pwstr);

private:
	AbstractAccelerator(const AbstractAccelerator&);
	AbstractAccelerator& operator=(const AbstractAccelerator&);

private:
	struct AbstractAcceleratorImpl* pImpl_;
};


/****************************************************************************
 *
 * SystemAccelerator
 *
 */

class QSEXPORTCLASS SystemAccelerator : public AbstractAccelerator
{
public:
	SystemAccelerator(const ACCEL* pAccel, int nSize, QSTATUS* pstatus);
	virtual ~SystemAccelerator();

public:
	virtual QSTATUS translateAccelerator(HWND hwnd,
		const MSG& msg, bool* pbProcessed);

private:
	SystemAccelerator(const SystemAccelerator&);
	SystemAccelerator& operator=(const SystemAccelerator&);

private:
	HACCEL haccel_;
};


/****************************************************************************
 *
 * SystemAcceleratorFactory
 *
 */

class QSEXPORTCLASS SystemAcceleratorFactory : public AcceleratorFactory
{
public:
	SystemAcceleratorFactory();
	virtual ~SystemAcceleratorFactory();

public:
	virtual QSTATUS createAccelerator(const ACCEL* pAccel, int nSize,
		Accelerator** ppAccelerator);

private:
	SystemAcceleratorFactory(const SystemAcceleratorFactory&);
	SystemAcceleratorFactory& operator=(const SystemAcceleratorFactory&);
};


/****************************************************************************
 *
 * CustomAccelerator
 *
 */

class QSEXPORTCLASS CustomAccelerator : public AbstractAccelerator
{
public:
	CustomAccelerator(const ACCEL* pAccel, int nSize, QSTATUS* pstatus);
	virtual ~CustomAccelerator();

public:
	virtual QSTATUS translateAccelerator(HWND hwnd,
		const MSG& msg, bool* pbProcessed);

private:
	CustomAccelerator(const CustomAccelerator&);
	CustomAccelerator& operator=(const CustomAccelerator&);

private:
	struct CustomAcceleratorImpl* pImpl_;
};


/****************************************************************************
 *
 * CustomAcceleratorFactory
 *
 */

class QSEXPORTCLASS CustomAcceleratorFactory : public AcceleratorFactory
{
public:
	CustomAcceleratorFactory();
	virtual ~CustomAcceleratorFactory();

public:
	virtual QSTATUS createAccelerator(const ACCEL* pAccel, int nSize,
		Accelerator** ppAccelerator);

private:
	CustomAcceleratorFactory(const CustomAcceleratorFactory&);
	CustomAcceleratorFactory& operator=(const CustomAcceleratorFactory&);
};

}

#endif // __QSACCELERATOR_H__
