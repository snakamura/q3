/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmmacro.h>
#include <qmtemplate.h>

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

qm::Template::Template(const ValueList& l) :
	listValue_(l)
{
}

qm::Template::~Template()
{
	std::for_each(listValue_.begin(), listValue_.end(),
		unary_compose_fx_gx(string_free<WSTRING>(), deleter<Macro>()));
}

Template::Result qm::Template::getValue(const TemplateContext& context,
										wstring_ptr* pwstrValue) const
{
	assert(pwstrValue);
	
	pwstrValue->reset(0);
	
	StringBuffer<WSTRING> buf;
	
	MacroVariableHolder globalVariable;
	
	const TemplateContext::ArgumentList& l = context.getArgumentList();
	for (TemplateContext::ArgumentList::const_iterator itA = l.begin(); itA != l.end(); ++itA) {
		MacroValuePtr pValue(MacroValueFactory::getFactory().newString((*itA).pwszValue_));
		globalVariable.setVariable((*itA).pwszName_, pValue);
	}
	
	for (ValueList::const_iterator itV = listValue_.begin(); itV != listValue_.end(); ++itV) {
		buf.append((*itV).first);
		
		if ((*itV).second) {
			MacroContext c(context.getMessageHolder(), context.getMessage(),
				context.getSelectedMessageHolders(), context.getAccount(),
				context.getDocument(), context.getWindow(), context.getProfile(),
				false, context.getSecurityMode(), context.getErrorHandler(), &globalVariable);
			MacroValuePtr pValue((*itV).second->value(&c));
			if (!pValue.get()) {
				if (c.getReturnType() == MacroContext::RETURNTYPE_NONE)
					return RESULT_ERROR;
				else
					return RESULT_CANCEL;
			}
			wstring_ptr wstrValue(pValue->string());
			buf.append(wstrValue.get());
		}
	}
	
	*pwstrValue = buf.getString();
	
	return RESULT_SUCCESS;
}


/****************************************************************************
 *
 * TemplateContext
 *
 */

qm::TemplateContext::TemplateContext(MessageHolderBase* pmh,
									 Message* pMessage,
									 const MessageHolderList& listSelected,
									 Account* pAccount,
									 Document* pDocument,
									 HWND hwnd,
									 unsigned int nSecurityMode,
									 Profile* pProfile,
									 MacroErrorHandler* pErrorHandler,
									 const ArgumentList& listArgument) :
	pmh_(pmh),
	pMessage_(pMessage),
	listSelected_(listSelected),
	pAccount_(pAccount),
	pDocument_(pDocument),
	hwnd_(hwnd),
	nSecurityMode_(nSecurityMode),
	pProfile_(pProfile),
	pErrorHandler_(pErrorHandler),
	listArgument_(listArgument)
{
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

const MessageHolderList& qm::TemplateContext::getSelectedMessageHolders() const
{
	return listSelected_;
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

unsigned int qm::TemplateContext::getSecurityMode() const
{
	return nSecurityMode_;
}

Profile* qm::TemplateContext::getProfile() const
{
	return pProfile_;
}

MacroErrorHandler* qm::TemplateContext::getErrorHandler() const
{
	return pErrorHandler_;
}

const TemplateContext::ArgumentList& qm::TemplateContext::getArgumentList() const
{
	return listArgument_;
}


/****************************************************************************
 *
 * TemplateParser
 *
 */

qm::TemplateParser::TemplateParser()
{
}

qm::TemplateParser::~TemplateParser()
{
}

std::auto_ptr<Template> qm::TemplateParser::parse(Reader* pReader) const
{
	assert(pReader);
	
	Template::ValueList listValue;
	struct Deleter
	{
		Deleter(Template::ValueList&l ) :
			p_(&l)
		{
		}
		
		~Deleter()
		{
			if (p_) {
				std::for_each(p_->begin(), p_->end(),
					unary_compose_fx_gx(
						string_free<WSTRING>(), qs::deleter<Macro>()));
				p_->clear();
			}
		}
		
		void release()
		{
			p_ = 0;
		}

		Template::ValueList* p_;
	} deleter(listValue);
	
	MacroParser parser(MacroParser::TYPE_TEMPLATE);
	
	StringBuffer<WSTRING> bufText;
	StringBuffer<WSTRING> bufMacro;
	bool bMacro = false;
	WCHAR cNext = L'\0';
	while (true) {
		WCHAR c = L'\0';
		if (cNext) {
			c = cNext;
			cNext = L'\0';
		}
		else {
			size_t nRead = pReader->read(&c, 1);
			if (nRead == -1)
				return std::auto_ptr<Template>(0);
			else if (nRead != 1)
				break;
		}
		
		if (!bMacro) {
			if (c == L'{') {
				size_t nRead = pReader->read(&c, 1);
				if (nRead == -1) {
					return std::auto_ptr<Template>(0);
				}
				else if (nRead == 1 && c == L'{') {
					bufText.append(L'{');
				}
				else {
					bMacro = true;
					if (nRead == 1)
						cNext = c;
				}
			}
			else if (c == L'}') {
				bufText.append(L'}');
				size_t nRead = pReader->read(&c, 1);
				if (nRead == -1)
					return std::auto_ptr<Template>(0);
				else if (nRead == 1 && c != L'}')
					cNext = c;
			}
			else {
				bufText.append(c);
			}
		}
		else {
			if (c == L'{') {
				bufMacro.append(L'{');
				size_t nRead = pReader->read(&c, 1);
				if (nRead == -1)
					return std::auto_ptr<Template>(0);
				else if (nRead == 1 && c != L'{')
					cNext = c;
			}
			else if (c == L'}') {
				size_t nRead = pReader->read(&c, 1);
				if (nRead == -1) {
					return std::auto_ptr<Template>(0);
				}
				else if (nRead == 1 && c == L'}') {
					bufMacro.append(L'}');
				}
				else {
					std::auto_ptr<Macro> pMacro(parser.parse(bufMacro.getCharArray()));
					if (!pMacro.get())
						return std::auto_ptr<Template>(0);
					wstring_ptr wstrText(bufText.getString());
					listValue.push_back(std::make_pair(wstrText.get(), pMacro.get()));
					wstrText.release();
					pMacro.release();
					bufText.remove();
					bufMacro.remove();
					bMacro = false;
					if (nRead == 1)
						cNext = c;
				}
			}
			else {
				bufMacro.append(c);
			}
		}
	}
	if (bufText.getLength() != 0) {
		wstring_ptr wstrText(bufText.getString());
		listValue.push_back(Template::ValueList::value_type(wstrText.get(), 0));
		wstrText.release();
	}
	
	std::auto_ptr<Template> pTemplate(new Template(listValue));
	deleter.release();
	return pTemplate;
}
