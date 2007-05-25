/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __MENUCREATORMACRO_H__
#define __MENUCREATORMACRO_H__

#define ADD_MENUCREATOR1(name, arg1) \
	do { \
		std::auto_ptr<qm::MenuCreator> p(new name(arg1, pUIManager_->getActionParamMap())); \
		pMenuCreatorList_->add(p); \
	} while (0)

#define ADD_MENUCREATOR2(name, arg1, arg2) \
	do { \
		std::auto_ptr<qm::MenuCreator> p(new name(arg1, arg2, pUIManager_->getActionParamMap())); \
		pMenuCreatorList_->add(p); \
	} while (0)

#define ADD_MENUCREATOR3(name, arg1, arg2, arg3) \
	do { \
		std::auto_ptr<qm::MenuCreator> p(new name(arg1, arg2, arg3, pUIManager_->getActionParamMap())); \
		pMenuCreatorList_->add(p); \
	} while (0)

#endif // __MENUCREATORMACRO_H__
