/*
 * $Id: template.cpp,v 1.1.1.1 2003/04/29 08:07:31 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmmacro.h>
#include <qmtemplate.h>

#include <qsnew.h>
#include <qsstl.h>
#include <qsstream.h>

#include <algorithm>

#pragma warning(disable:4786)

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * Template
 *
 */

qm::Template::Template(const ValueList& l, QSTATUS* pstatus)
{
	assert(pstatus);
	
	DECLARE_QSTATUS();
	
	status = STLWrapper<ValueList>(listValue_).resize(l.size());
	CHECK_QSTATUS_SET(pstatus);
	std::copy(l.begin(), l.end(), listValue_.begin());
}

qm::Template::~Template()
{
	std::for_each(listValue_.begin(), listValue_.end(),
		unary_compose_fx_gx(string_free<WSTRING>(), deleter<Macro>()));
}

QSTATUS qm::Template::getValue(
	const TemplateContext& context, WSTRING* pwstrValue) const
{
	assert(pwstrValue);
	
	DECLARE_QSTATUS();
	
	*pwstrValue = 0;
	
	StringBuffer<WSTRING> buf(&status);
	CHECK_QSTATUS();
	
	ValueList::const_iterator it = listValue_.begin();
	while (it != listValue_.end()) {
		status = buf.append((*it).first);
		CHECK_QSTATUS();
		
		if ((*it).second) {
			MacroVariableHolder globalVariable(&status);
			CHECK_QSTATUS();
			MacroContext::Init init = {
				context.getMessageHolder(),
				context.getMessage(),
				context.getAccount(),
				context.getDocument(),
				context.getWindow(),
				context.getProfile(),
				false,
				context.getErrorHandler(),
				&globalVariable
			};
			MacroContext c(init, &status);
			CHECK_QSTATUS();
			MacroValue* pValue = 0;
			status = (*it).second->value(&c, &pValue);
			CHECK_QSTATUS();
			MacroValuePtr apValue(pValue);
			string_ptr<WSTRING> wstrValue;
			status = pValue->string(&wstrValue);
			CHECK_QSTATUS();
			status = buf.append(wstrValue.get());
			CHECK_QSTATUS();
		}
		
		++it;
	}
	
	*pwstrValue = buf.getString();
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * TemplateContext
 *
 */

qm::TemplateContext::TemplateContext(MessageHolderBase* pmh,
	Message* pMessage, Account* pAccount, Document* pDocument, HWND hwnd,
	Profile* pProfile, MacroErrorHandler* pErrorHandler, QSTATUS* pstatus) :
	pmh_(pmh),
	pMessage_(pMessage),
	pAccount_(pAccount),
	pDocument_(pDocument),
	hwnd_(hwnd),
	pProfile_(pProfile),
	pErrorHandler_(pErrorHandler)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qm::TemplateContext::~TemplateContext()
{
}

MessageHolderBase* qm::TemplateContext::getMessageHolder() const
{
	return pmh_;
}

Message* qm::TemplateContext::getMessage() const
{
	return pMessage_;
}

Account* qm::TemplateContext::getAccount() const
{
	return pAccount_;
}

Document* qm::TemplateContext::getDocument() const
{
	return pDocument_;
}

HWND qm::TemplateContext::getWindow() const
{
	return hwnd_;
}

Profile* qm::TemplateContext::getProfile() const
{
	return pProfile_;
}

MacroErrorHandler* qm::TemplateContext::getErrorHandler() const
{
	return pErrorHandler_;
}


/****************************************************************************
 *
 * TemplateParser
 *
 */

qm::TemplateParser::TemplateParser(QSTATUS* pstatus)
{
}

qm::TemplateParser::~TemplateParser()
{
}

QSTATUS qm::TemplateParser::parse(Reader* pReader, Template** ppTemplate)
{
	assert(pReader);
	assert(ppTemplate);
	
	DECLARE_QSTATUS();
	
	*ppTemplate = 0;
	
	Template::ValueList listValue;
	struct Deleter
	{
		Deleter(Template::ValueList&l ) : p_(&l) {}
		~Deleter()
		{
			if (p_) {
				std::for_each(p_->begin(), p_->end(),
					unary_compose_fx_gx(
						string_free<WSTRING>(), deleter<Macro>()));
				p_->clear();
			}
		}
		void release() { p_ = 0; }
		Template::ValueList* p_;
	} deleter(listValue);
	
	MacroParser parser(MacroParser::TYPE_TEMPLATE, &status);
	CHECK_QSTATUS();
	
	StringBuffer<WSTRING> bufText(&status);
	StringBuffer<WSTRING> bufMacro(&status);
	bool bMacro = false;
	while (true) {
		WCHAR c = L'\0';
		size_t nRead = 0;
		status = pReader->read(&c, 1, &nRead);
		CHECK_QSTATUS();
		if (nRead != 1)
			break;
		if (!bMacro) {
			if (c == L'\\') {
				status = pReader->read(&c, 1, &nRead);
				CHECK_QSTATUS();
				if (nRead != 1)
					return QSTATUS_FAIL;
				status = bufText.append(c);
				CHECK_QSTATUS();
			}
			else if (c == L'{') {
				bMacro = true;
			}
			else {
				status = bufText.append(c);
				CHECK_QSTATUS();
			}
		}
		else {
			if (c == L'\\') {
				status = pReader->read(&c, 1, &nRead);
				CHECK_QSTATUS();
				if (nRead != 1)
					return QSTATUS_FAIL;
				status = bufMacro.append(c);
				CHECK_QSTATUS();
			}
			else if (c == L'}') {
				Macro* pMacro = 0;
				status = parser.parse(bufMacro.getCharArray(), &pMacro);
				CHECK_QSTATUS();
				std::auto_ptr<Macro> apMacro(pMacro);
				string_ptr<WSTRING> wstrText(bufText.getString());
				status = STLWrapper<Template::ValueList>(listValue
					).push_back(std::make_pair(wstrText.get(), pMacro));
				CHECK_QSTATUS();
				wstrText.release();
				apMacro.release();
				bufText.remove();
				bufMacro.remove();
				bMacro = false;
			}
			else {
				status = bufMacro.append(c);
				CHECK_QSTATUS();
			}
		}
	}
	if (bufText.getLength() != 0) {
		string_ptr<WSTRING> wstrText(bufText.getString());
		status = STLWrapper<Template::ValueList>(listValue).push_back(
			std::make_pair(wstrText.get(), static_cast<Macro*>(0)));
		CHECK_QSTATUS();
		wstrText.release();
	}
	
	std::auto_ptr<Template> pTemplate;
	status = newQsObject(listValue, &pTemplate);
	CHECK_QSTATUS();
	deleter.release();
	
	*ppTemplate = pTemplate.release();
	
	return QSTATUS_SUCCESS;
}
