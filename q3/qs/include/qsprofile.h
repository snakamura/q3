/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QSPROFILE_H__
#define __QSPROFILE_H__

#include <qs.h>
#include <qsstring.h>

#include <map>
#include <vector>


namespace qs {

class Profile;
	class AbstractProfile;
		class RegistryProfile;
		class AbstractTextProfile;
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
	struct Default
	{
		const WCHAR* pwszSection_;
		const WCHAR* pwszKey_;
		const WCHAR* pwszValue_;
	};

public:
	typedef std::vector<WSTRING> StringList;

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
	
	virtual wstring_ptr getString(const WCHAR* pwszSection,
								  const WCHAR* pwszKey);
	
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
	
	virtual void getStringList(const WCHAR* pwszSection,
							   const WCHAR* pwszKey,
							   StringList* pListValue) = 0;
	
	virtual void setStringList(const WCHAR* pwszSection,
							   const WCHAR* pwszKey,
							   const StringList& listValue) = 0;
	
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
	
	virtual int getInt(const WCHAR* pwszSection,
					   const WCHAR* pwszKey);
	
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
	
	virtual void addDefault(const Default* pDefault,
							size_t nCount) = 0;
};


/****************************************************************************
 *
 * AbstractProfile
 *
 */

class QSEXPORTCLASS AbstractProfile : public Profile
{
protected:
	AbstractProfile(const Default* pDefault,
					size_t nCount);

public:
	virtual ~AbstractProfile();

public:
	virtual void addDefault(const Default* pDefault,
							size_t nCount);

protected:
	const WCHAR* getDefault(const WCHAR* pwszSection,
							const WCHAR* pwszKey,
							const WCHAR* pwszDefault) const;
	int getDefault(const WCHAR* pwszSection,
				   const WCHAR* pwszKey,
				   int nDefault) const;

private:
	AbstractProfile(const AbstractProfile&);
	AbstractProfile& operator=(const AbstractProfile&);

private:
	struct AbstractProfileImpl* pImpl_;
};


/****************************************************************************
 *
 * RegistryProfile
 *
 */

class QSEXPORTCLASS RegistryProfile : public AbstractProfile
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
					const WCHAR* pwszAppName,
					const Default* pDefault,
					size_t nCount);
	
	virtual ~RegistryProfile();

public:
	virtual wstring_ptr getString(const WCHAR* pwszSection,
								  const WCHAR* pwszKey,
								  const WCHAR* pwszDefault);
	virtual void setString(const WCHAR* pwszSection,
						   const WCHAR* pwszKey,
						   const WCHAR* pwszValue);
	
	virtual void getStringList(const WCHAR* pwszSection,
							   const WCHAR* pwszKey,
							   StringList* pListValue);
	virtual void setStringList(const WCHAR* pwszSection,
							   const WCHAR* pwszKey,
							   const StringList& listValue);
	
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
 * AbstractTextProfile
 *
 */

class QSEXPORTCLASS AbstractTextProfile : public AbstractProfile
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
	AbstractTextProfile(const WCHAR* pwszPath,
						const Default* pDefault,
						size_t nCount);

public:
	virtual ~AbstractTextProfile();

public:
	virtual wstring_ptr getString(const WCHAR* pwszSection,
								  const WCHAR* pwszKey,
								  const WCHAR* pwszDefault);
	virtual void setString(const WCHAR* pwszSection,
						   const WCHAR* pwszKey,
						   const WCHAR* pwszValue);
	
	virtual void getStringList(const WCHAR* pwszSection,
							   const WCHAR* pwszKey,
							   StringList* pListValue);
	virtual void setStringList(const WCHAR* pwszSection,
							   const WCHAR* pwszKey,
							   const StringList& listValue);
	
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
	const WCHAR* get(const WCHAR* pwszSection,
					 const WCHAR* pwszKey) const;

private:
	AbstractTextProfile(const AbstractTextProfile&);
	AbstractTextProfile& operator=(const AbstractTextProfile&);

private:
	struct AbstractTextProfileImpl* pImpl_;
};


/****************************************************************************
 *
 * TextProfile
 *
 */

class QSEXPORTCLASS TextProfile : public AbstractTextProfile
{
public:
	/**
	 * Create instance.
	 *
	 * @param pwszPath [in] Path to file.
	 * @exception std::bad_alloc Out of memory.
	 */
	TextProfile(const WCHAR* pwszPath,
				const Default* pDefault,
				size_t nCount);
	
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

class QSEXPORTCLASS XMLProfile : public AbstractTextProfile
{
public:
	/**
	 * Create instance.
	 *
	 * @param pwszPath [in] Path to file.
	 * @exception std::bad_alloc Out of memory.
	 */
	XMLProfile(const WCHAR* pwszPath,
			   const Default* pDefault,
			   size_t nCount);
	
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
