/**
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __ACTIONMACRO_H__
#define __ACTIONMACRO_H__

#define ADD_ACTION1(name, id, arg1) \
	do { \
		std::auto_ptr< name > p(new name(arg1)); \
		pActionMap_->addAction(id, p); \
	} while(0)

#define ADD_ACTION2(name, id, arg1, arg2) \
	do { \
		std::auto_ptr< name > p(new name(arg1, arg2)); \
		pActionMap_->addAction(id, p); \
	} while (0)

#define ADD_ACTION3(name, id, arg1, arg2, arg3) \
	do { \
		std::auto_ptr< name > p(new name(arg1, arg2, arg3)); \
		pActionMap_->addAction(id, p); \
	} while (0)

#define ADD_ACTION4(name, id, arg1, arg2, arg3, arg4) \
	do { \
		std::auto_ptr< name > p(new name(arg1, arg2, arg3, arg4)); \
		pActionMap_->addAction(id, p); \
	} while (0)

#define ADD_ACTION5(name, id, arg1, arg2, arg3, arg4, arg5) \
	do { \
		std::auto_ptr< name > p(new name(arg1, arg2, arg3, arg4, arg5)); \
		pActionMap_->addAction(id, p); \
	} while (0)

#define ADD_ACTION6(name, id, arg1, arg2, arg3, arg4, arg5, arg6) \
	do { \
		std::auto_ptr< name > p(new name(arg1, arg2, arg3, arg4, arg5, arg6)); \
		pActionMap_->addAction(id, p); \
	} while (0)

#define ADD_ACTION7(name, id, arg1, arg2, arg3, arg4, arg5, arg6, arg7) \
	do { \
		std::auto_ptr< name > p(new name(arg1, arg2, arg3, arg4, arg5, arg6, arg7)); \
		pActionMap_->addAction(id, p); \
	} while (0)

#define ADD_ACTION8(name, id, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8) \
	do { \
		std::auto_ptr< name > p(new name(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8)); \
		pActionMap_->addAction(id, p); \
	} while (0)

#define ADD_ACTION9(name, id, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9) \
	do { \
		std::auto_ptr< name > p(new name(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9)); \
		pActionMap_->addAction(id, p); \
	} while (0)

#define ADD_ACTION10(name, id, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10) \
	do { \
		std::auto_ptr< name > p(new name(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10)); \
		pActionMap_->addAction(id, p); \
	} while (0)

#define ADD_ACTION11(name, id, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11) \
	do { \
		std::auto_ptr< name > p(new name(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11)); \
		pActionMap_->addAction(id, p); \
	} while (0)

#define ADD_ACTION_RANGE1(name, from, to, arg1) \
	do { \
		std::auto_ptr< name > p(new name(arg1)); \
		pActionMap_->addAction(from, to, p); \
	} while (0)

#define ADD_ACTION_RANGE2(name, from, to, arg1, arg2) \
	do { \
		std::auto_ptr< name > p(new name(arg1, arg2)); \
		pActionMap_->addAction(from, to, p); \
	} while (0)

#define ADD_ACTION_RANGE3(name, from, to, arg1, arg2, arg3) \
	do { \
		std::auto_ptr< name > p(new name(arg1, arg2, arg3)); \
		pActionMap_->addAction(from, to, p); \
	} while (0)

#define ADD_ACTION_RANGE4(name, from, to, arg1, arg2, arg3, arg4) \
	do { \
		std::auto_ptr< name > p(new name(arg1, arg2, arg3, arg4)); \
		pActionMap_->addAction(from, to, p); \
	} while (0)

#define ADD_ACTION_RANGE5(name, from, to, arg1, arg2, arg3, arg4, arg5) \
	do { \
		std::auto_ptr< name > p(new name(arg1, arg2, arg3, arg4, arg5)); \
		pActionMap_->addAction(from, to, p); \
	} while (0)

#define ADD_ACTION_RANGE6(name, from, to, arg1, arg2, arg3, arg4, arg5, arg6) \
	do { \
		std::auto_ptr< name > p(new name(arg1, arg2, arg3, arg4, arg5, arg6)); \
		pActionMap_->addAction(from, to, p); \
	} while (0)

#define ADD_ACTION_RANGE7(name, from, to, arg1, arg2, arg3, arg4, arg5, arg6, arg7) \
	do { \
		std::auto_ptr< name > p(new name(arg1, arg2, arg3, arg4, arg5, arg6, arg7)); \
		pActionMap_->addAction(from, to, p); \
	} while (0)

#define ADD_ACTION_RANGE8(name, from, to, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8) \
	do { \
		std::auto_ptr< name > p(new name(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8)); \
		pActionMap_->addAction(from, to, p); \
	} while (0)

#define ADD_ACTION_RANGE9(name, from, to, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9) \
	do { \
		std::auto_ptr< name > p(new name(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9)); \
		pActionMap_->addAction(from, to, p); \
	} while (0)

#define ADD_ACTION_RANGE10(name, from, to, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10) \
	do { \
		std::auto_ptr< name > p(new name(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10)); \
		pActionMap_->addAction(from, to, p); \
	} while (0)

#define ADD_ACTION_RANGE11(name, from, to, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11) \
	do { \
		std::auto_ptr< name > p(new name(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11)); \
		pActionMap_->addAction(from, to, p); \
	} while (0)

#endif __ACTIONMACRO_H__
