/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */


/****************************************************************************
 *
 * TemplateMenuCreatorImpl
 *
 */

template<const WCHAR* pwszName, const WCHAR* pwszPrefix, UINT nBaseId, unsigned int nMax>
qm::TemplateMenuCreatorImpl<pwszName, pwszPrefix, nBaseId, nMax>::TemplateMenuCreatorImpl(const TemplateManager* pTemplateManager,
																						  AccountSelectionModel* pAccountSelectionModel,
																						  qs::ActionParamMap* pActionParamMap) :
	TemplateMenuCreator(pTemplateManager, pAccountSelectionModel, pActionParamMap)
{
}

template<const WCHAR* pwszName, const WCHAR* pwszPrefix, UINT nBaseId, unsigned int nMax>
qm::TemplateMenuCreatorImpl<pwszName, pwszPrefix, nBaseId, nMax>::~TemplateMenuCreatorImpl()
{
}

template<const WCHAR* pwszName, const WCHAR* pwszPrefix, UINT nBaseId, unsigned int nMax>
const WCHAR* qm::TemplateMenuCreatorImpl<pwszName, pwszPrefix, nBaseId, nMax>::getName() const
{
	return pwszName;
}

template<const WCHAR* pwszName, const WCHAR* pwszPrefix, UINT nBaseId, unsigned int nMax>
const WCHAR* qm::TemplateMenuCreatorImpl<pwszName, pwszPrefix, nBaseId, nMax>::getPrefix() const
{
	return pwszPrefix;
}

template<const WCHAR* pwszName, const WCHAR* pwszPrefix, UINT nBaseId, unsigned int nMax>
UINT qm::TemplateMenuCreatorImpl<pwszName, pwszPrefix, nBaseId, nMax>::getBaseId() const
{
	return nBaseId;
}

template<const WCHAR* pwszName, const WCHAR* pwszPrefix, UINT nBaseId, unsigned int nMax>
unsigned int qm::TemplateMenuCreatorImpl<pwszName, pwszPrefix, nBaseId, nMax>::getMax() const
{
	return nMax;
}
