/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
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
	/**
	 * Translate message to an accelerator command.
	 *
	 * @param hwnd [in] Window handle.
	 * @param msg [in] Message.
	 *
	 * @return true if translated, false otherwise.
	 */
	virtual bool translateAccelerator(HWND hwnd,
									  const MSG& msg) = 0;
	
	/**
	 * Get string representation of the key combination of
	 * the specified id.
	 *
	 * @param nId [in] Id.
	 * @return String representation of the key combination.
	 *         Can not be null.
	 * @exception std::bad_alloc Out of memory.
	 */
	virtual wstring_ptr getKeyFromId(UINT nId) = 0;
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
	/**
	 * Create accelerator.
	 *
	 * @param pAccel [in] List of accelerators.
	 * @param nSize [in] Size of the list.
	 * @return Created accelerator. Can not be null.
	 * @exception std::bad_alloc Out of memory.
	 */
	virtual std::auto_ptr<Accelerator> createAccelerator(const ACCEL* pAccel,
														 size_t nSize) = 0;
};


/****************************************************************************
 *
 * AbstractAccelerator
 *
 */

class QSEXPORTCLASS AbstractAccelerator : public Accelerator
{
protected:
	AbstractAccelerator(const ACCEL* pAccel,
						size_t nSize);

public:
	virtual ~AbstractAccelerator();

public:
	virtual wstring_ptr getKeyFromId(UINT nId);

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
	SystemAccelerator(const ACCEL* pAccel,
					  size_t nSize);
	virtual ~SystemAccelerator();

public:
	virtual bool translateAccelerator(HWND hwnd,
									  const MSG& msg);

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
	virtual std::auto_ptr<Accelerator> createAccelerator(const ACCEL* pAccel,
														 size_t nSize);

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
	CustomAccelerator(const ACCEL* pAccel,
					  size_t nSize);
	virtual ~CustomAccelerator();

public:
	virtual bool translateAccelerator(HWND hwnd,
									  const MSG& msg);

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
	virtual std::auto_ptr<Accelerator> createAccelerator(const ACCEL* pAccel,
														 size_t nSize);

private:
	CustomAcceleratorFactory(const CustomAcceleratorFactory&);
	CustomAcceleratorFactory& operator=(const CustomAcceleratorFactory&);
};

}

#endif // __QSACCELERATOR_H__
