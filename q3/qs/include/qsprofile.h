/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QSPROFILE_H__
#define __QSPROFILE_H__

#include <qs.h>
#include <qsstring.h>

#include <map>


namespace qs {

class Profile;
	class RegistryProfile;
	class AbstractProfile;
		class TextProfile;
		class XMLProfile;


/****************************************************************************
 *
 * Profile
 *
 */

class QSEXPORTCLASS Profile
{
public:
	virtual ~Profile();

public:
	virtual QSTATUS getString(const WCHAR* pwszSection,
		const WCHAR* pwszKey, const WCHAR* pwszDefault,
		WSTRING* pwstrValue) = 0;
	virtual QSTATUS setString(const WCHAR* pwszSection,
		const WCHAR* pwszKey, const WCHAR* pwszValue) = 0;
	
	virtual QSTATUS getInt(const WCHAR* pwszSection,
		const WCHAR* pwszKey, int nDefault, int* pnValue) = 0;
	virtual QSTATUS setInt(const WCHAR* pwszSection,
		const WCHAR* pwszKey, int nValue) = 0;
	
	virtual QSTATUS getBinary(const WCHAR* pwszSection,
		const WCHAR* pwszKey, unsigned char* pValue, int* pnSize) = 0;
	virtual QSTATUS setBinary(const WCHAR* pwszSection,
		const WCHAR* pwszKey, const unsigned char* pValue, int nSize) = 0;
	
	virtual QSTATUS load() = 0;
	virtual QSTATUS save() const = 0;
	virtual QSTATUS deletePermanent() = 0;
	virtual QSTATUS rename(const WCHAR* pwszName) = 0;
};


/****************************************************************************
 *
 * RegistryProfile
 *
 */

class QSEXPORTCLASS RegistryProfile : public Profile
{
public:
	RegistryProfile(const WCHAR* pwszCompanyName,
		const WCHAR* pwszAppName, QSTATUS* pstatus);
	virtual ~RegistryProfile();

public:
	virtual QSTATUS getString(const WCHAR* pwszSection,
		const WCHAR* pwszKey, const WCHAR* pwszDefault,
		WSTRING* pwstrValue);
	virtual QSTATUS setString(const WCHAR* pwszSection,
		const WCHAR* pwszKey, const WCHAR* pwszValue);
	
	virtual QSTATUS getInt(const WCHAR* pwszSection,
		const WCHAR* pwszKey, int nDefault, int* pnValue);
	virtual QSTATUS setInt(const WCHAR* pwszSection,
		const WCHAR* pwszKey, int nValue);
	
	virtual QSTATUS getBinary(const WCHAR* pwszSection,
		const WCHAR* pwszKey, unsigned char* pValue, int* pnSize);
	virtual QSTATUS setBinary(const WCHAR* pwszSection,
		const WCHAR* pwszKey, const unsigned char* pValue, int nSize);
	
	virtual QSTATUS load();
	virtual QSTATUS save() const;
	virtual QSTATUS deletePermanent();
	virtual QSTATUS rename(const WCHAR* pwszName);

private:
	RegistryProfile(const RegistryProfile&);
	RegistryProfile& operator=(const RegistryProfile&);

private:
	struct RegistryProfileImpl* pImpl_;
};


/****************************************************************************
 *
 * AbstractProfile
 *
 */

class QSEXPORTCLASS AbstractProfile : public Profile
{
public:
	typedef std::map<WSTRING, WSTRING, string_less<WCHAR> > Map;

protected:
	AbstractProfile(const WCHAR* pwszPath, QSTATUS* pstatus);

public:
	virtual ~AbstractProfile();

public:
	virtual QSTATUS getString(const WCHAR* pwszSection,
		const WCHAR* pwszKey, const WCHAR* pwszDefault,
		WSTRING* pwstrValue);
	virtual QSTATUS setString(const WCHAR* pwszSection,
		const WCHAR* pwszKey, const WCHAR* pwszValue);
	
	virtual QSTATUS getInt(const WCHAR* pwszSection,
		const WCHAR* pwszKey, int nDefault, int* pnValue);
	virtual QSTATUS setInt(const WCHAR* pwszSection,
		const WCHAR* pwszKey, int nValue);
	
	virtual QSTATUS getBinary(const WCHAR* pwszSection,
		const WCHAR* pwszKey, unsigned char* pValue, int* pnSize);
	virtual QSTATUS setBinary(const WCHAR* pwszSection,
		const WCHAR* pwszKey, const unsigned char* pValue, int nSize);
	
	virtual QSTATUS load();
	virtual QSTATUS save() const;
	virtual QSTATUS deletePermanent();
	virtual QSTATUS rename(const WCHAR* pwszName);

protected:
	virtual QSTATUS loadImpl(const WCHAR* pwszPath) = 0;
	virtual QSTATUS saveImpl(const WCHAR* pwszPath) const = 0;

protected:
	Map* getMap() const;

public:
	void lock() const;
	void unlock() const;

private:
	AbstractProfile(const AbstractProfile&);
	AbstractProfile& operator=(const AbstractProfile&);

private:
	struct AbstractProfileImpl* pImpl_;
};


/****************************************************************************
 *
 * TextProfile
 *
 */

class QSEXPORTCLASS TextProfile : public AbstractProfile
{
public:
	TextProfile(const WCHAR* pwszPath, QSTATUS* pstatus);
	virtual ~TextProfile();

protected:
	virtual QSTATUS loadImpl(const WCHAR* pwszPath);
	virtual QSTATUS saveImpl(const WCHAR* pwszPath) const;

private:
	TextProfile(const TextProfile&);
	TextProfile& operator=(const TextProfile&);
};


/****************************************************************************
 *
 * XMLProfile
 *
 */

class QSEXPORTCLASS XMLProfile : public AbstractProfile
{
public:
	XMLProfile(const WCHAR* pwszPath, QSTATUS* pstatus);
	virtual ~XMLProfile();

protected:
	virtual QSTATUS loadImpl(const WCHAR* pwszPath);
	virtual QSTATUS saveImpl(const WCHAR* pwszPath) const;

private:
	XMLProfile(const XMLProfile&);
	XMLProfile& operator=(const XMLProfile&);
};

}

#endif // __QSPROFILE_H__
