/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
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
	/**
	 * Get string value.
	 *
	 * @param pwszSection [in] Section name.
	 * @param pwszKey [in] Key name.
	 * @param pwszDefault [in] Default value.
	 * @return String value. Default value if there is no value
	 *         or error occured. Can not be null.
	 * @exception std::bad_alloc Out of memory.
	 */
	virtual wstring_ptr getString(const WCHAR* pwszSection,
								  const WCHAR* pwszKey,
								  const WCHAR* pwszDefault) = 0;
	
	/**
	 * Set string value.
	 *
	 * @param pwszSection [in] Section name.
	 * @param pwszKey [in] Key name.
	 * @param pwszValue [in] Value.
	 * @exception std::bad_alloc Out of memory.
	 */
	virtual void setString(const WCHAR* pwszSection,
						   const WCHAR* pwszKey,
						   const WCHAR* pwszValue) = 0;
	
	/**
	 * Get int value.
	 *
	 * @param pwszSection [in] Section name.
	 * @param pwszKey [in] Key name.
	 * @param nDefault [in] Default value.
	 * @return Integer value. Default value if there is no value or error occured.
	 * @exception std::bad_alloc Out of memory.
	 */
	virtual int getInt(const WCHAR* pwszSection,
					   const WCHAR* pwszKey,
					   int nDefault) = 0;
	
	/**
	 * Set int value.
	 *
	 * @param pwszSection [in] Section name.
	 * @param pwszKey [in] Key name.
	 * @param nValue [in] Value.
	 * @exception std::bad_alloc Out of memory.
	 */
	virtual void setInt(const WCHAR* pwszSection,
						const WCHAR* pwszKey,
						int nValue) = 0;
	
	/**
	 * Get binary value.
	 *
	 * @param pwszSection [in] Section name.
	 * @param pwszKey [in] Key name.
	 * @param pValue [in] Buffer to be filled.
	 * @param nSize [in] Buffer size.
	 * @return Written size.
	 * @exception std::bad_alloc Out of memory.
	 */
	virtual size_t getBinary(const WCHAR* pwszSection,
							 const WCHAR* pwszKey,
							 unsigned char* pValue,
							 size_t nSize) = 0;
	
	/**
	 * Set binary value.
	 *
	 * @param pwszSection [in] Section name.
	 * @param pwszKey [in] Key name.
	 * @param pValue [in] Buffer.
	 * @param nSize [in] Buffer size.
	 * @exception std::bad_alloc Out of memory.
	 */
	virtual void setBinary(const WCHAR* pwszSection,
						   const WCHAR* pwszKey,
						   const unsigned char* pValue,
						   int nSize) = 0;
	
	/**
	 * Load.
	 *
	 * @return true if success, false otherwise.
	 * @exception std::bad_alloc Out of memory.
	 */
	virtual bool load() = 0;
	
	/**
	 * Save.
	 *
	 * @return true if success, false otherwise.
	 * @exception std::bad_alloc Out of memory.
	 */
	virtual bool save() const = 0;
	
	/**
	 * Delete profile permanently.
	 *
	 * @return true if success, false otherwise.
	 * @exception std::bad_alloc Out of memory.
	 */
	virtual bool deletePermanent() = 0;
	
	/**
	 * Rename.
	 *
	 * @param pwszName [in] New name.
	 * @return true if success, false otherwise.
	 * @exception std::bad_alloc Out of memory.
	 */
	virtual bool rename(const WCHAR* pwszName) = 0;
};


/****************************************************************************
 *
 * RegistryProfile
 *
 */

class QSEXPORTCLASS RegistryProfile : public Profile
{
public:
	/**
	 * Create instance.
	 *
	 * @param pwszCompanyName [in] Company name.
	 * @param pwszAppName [in] Application name.
	 * @exception std::bad_alloc Out of memory.
	 */
	RegistryProfile(const WCHAR* pwszCompanyName,
					const WCHAR* pwszAppName);
	
	virtual ~RegistryProfile();

public:
	virtual wstring_ptr getString(const WCHAR* pwszSection,
								  const WCHAR* pwszKey,
								  const WCHAR* pwszDefault);
	virtual void setString(const WCHAR* pwszSection,
						   const WCHAR* pwszKey,
						   const WCHAR* pwszValue);
	
	virtual int getInt(const WCHAR* pwszSection,
					   const WCHAR* pwszKey,
					   int nDefault);
	virtual void setInt(const WCHAR* pwszSection,
						const WCHAR* pwszKey,
						int nValue);
	
	virtual size_t getBinary(const WCHAR* pwszSection,
							 const WCHAR* pwszKey,
							 unsigned char* pValue,
							 size_t nSize);
	virtual void setBinary(const WCHAR* pwszSection,
						   const WCHAR* pwszKey,
						   const unsigned char* pValue,
						   int nSize);
	
	virtual bool load();
	virtual bool save() const;
	virtual bool deletePermanent();
	virtual bool rename(const WCHAR* pwszName);

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
	/**
	 * Create instance.
	 *
	 * @param pwszPath [in] Path to file.
	 * @exception std::bad_alloc Out of memory.
	 */
	AbstractProfile(const WCHAR* pwszPath);

public:
	virtual ~AbstractProfile();

public:
	virtual wstring_ptr getString(const WCHAR* pwszSection,
								  const WCHAR* pwszKey,
								  const WCHAR* pwszDefault);
	virtual void setString(const WCHAR* pwszSection,
						   const WCHAR* pwszKey,
						   const WCHAR* pwszValue);
	
	virtual int getInt(const WCHAR* pwszSection,
					   const WCHAR* pwszKey,
					   int nDefault);
	virtual void setInt(const WCHAR* pwszSection,
						const WCHAR* pwszKey,
						int nValue);
	
	virtual size_t getBinary(const WCHAR* pwszSection,
							 const WCHAR* pwszKey,
							 unsigned char* pValue,
							 size_t nSize);
	virtual void setBinary(const WCHAR* pwszSection,
						   const WCHAR* pwszKey,
						   const unsigned char* pValue,
						   int nSize);
	
	virtual bool load();
	virtual bool save() const;
	virtual bool deletePermanent();
	virtual bool rename(const WCHAR* pwszName);

protected:
	/**
	 * Called to load from the specified file.
	 *
	 * @param pwszPath [in] Path to file.
	 * @return true if success, false otherwise.
	 * @exception std::bad_alloc Out of memory.
	 */
	virtual bool loadImpl(const WCHAR* pwszPath) = 0;
	
	/**
	 * Called to save to the specified file.
	 *
	 * @param pwszPath [in] Path to file.
	 * @return true if success, false otherwise.
	 * @exception std::bad_alloc Out of memory.
	 */
	virtual bool saveImpl(const WCHAR* pwszPath) const = 0;

protected:
	Map& getMap() const;

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
	/**
	 * Create instance.
	 *
	 * @param pwszPath [in] Path to file.
	 * @exception std::bad_alloc Out of memory.
	 */
	TextProfile(const WCHAR* pwszPath);
	
	virtual ~TextProfile();

protected:
	virtual bool loadImpl(const WCHAR* pwszPath);
	virtual bool saveImpl(const WCHAR* pwszPath) const;

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
	/**
	 * Create instance.
	 *
	 * @param pwszPath [in] Path to file.
	 * @exception std::bad_alloc Out of memory.
	 */
	XMLProfile(const WCHAR* pwszPath);
	virtual ~XMLProfile();

protected:
	virtual bool loadImpl(const WCHAR* pwszPath);
	virtual bool saveImpl(const WCHAR* pwszPath) const;

private:
	XMLProfile(const XMLProfile&);
	XMLProfile& operator=(const XMLProfile&);
};

}

#endif // __QSPROFILE_H__
