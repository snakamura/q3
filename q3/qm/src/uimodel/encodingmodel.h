/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __ENCODINGMODEL_H__
#define __ENCODINGMODEL_H__

#include <qm.h>

#include <qsstring.h>

#include <vector>


namespace qm {

class EncodingModel;
class EncodingModelHandler;
class EncodingModelEvent;


/****************************************************************************
 *
 * EncodingModel
 *
 */

class EncodingModel
{
public:
	virtual ~EncodingModel();

public:
	virtual const WCHAR* getEncoding() const = 0;
	virtual void setEncoding(const WCHAR* pwszEncoding) = 0;
	virtual void addEncodingModelHandler(EncodingModelHandler* pHandler) = 0;
	virtual void removeEncodingModelHandler(EncodingModelHandler* pHandler) = 0;
};


/****************************************************************************
 *
 * DefaultEncodingModel
 *
 */

class DefaultEncodingModel : public EncodingModel
{
public:
	DefaultEncodingModel();
	virtual ~DefaultEncodingModel();

public:
	virtual const WCHAR* getEncoding() const;
	virtual void setEncoding(const WCHAR* pwszEncoding);
	virtual void addEncodingModelHandler(EncodingModelHandler* pHandler);
	virtual void removeEncodingModelHandler(EncodingModelHandler* pHandler);

private:
	void fireEncodingChanged(const WCHAR* pwszEncoding);

private:
	DefaultEncodingModel(const DefaultEncodingModel&);
	DefaultEncodingModel& operator=(const DefaultEncodingModel&);

private:
	typedef std::vector<EncodingModelHandler*> HandlerList;

private:
	qs::wstring_ptr wstrEncoding_;
	HandlerList listHandler_;
};


/****************************************************************************
 *
 * EncodingModelHandler
 *
 */

class EncodingModelHandler
{
public:
	virtual ~EncodingModelHandler();

public:
	virtual void encodingChanged(const EncodingModelEvent& event) = 0;
};


/****************************************************************************
 *
 * EncodingModelEvent
 *
 */

class EncodingModelEvent
{
public:
	EncodingModelEvent(EncodingModel* pEncodingModel,
					   const WCHAR* pwszEncoding);
	~EncodingModelEvent();

public:
	EncodingModel* getEncodingModel() const;
	const WCHAR* getEncoding() const;

private:
	EncodingModelEvent(const EncodingModelEvent&);
	EncodingModelEvent& operator=(const EncodingModelEvent&);

private:
	EncodingModel* pEncodingModel_;
	const WCHAR* pwszEncoding_;
};

}


#endif // __ENCODINGMODEL_H__
