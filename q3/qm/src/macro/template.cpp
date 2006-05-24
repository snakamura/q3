/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#pragma warning(disable:4786)

#include <qmmacro.h>
#include <qmtemplate.h>

#include <qsinit.h>
#include <qsstl.h>
#include <qsstream.h>

#include <algorithm>

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * Template
 *
 */

namespace qm {

template<class Buffer>
bool append(Buffer& buf,
			const WCHAR* pwsz)
{
	return buf.append(pwsz);
}

template<>
bool append(StringBuffer<WSTRING>& buf,
			const WCHAR* pwsz)
{
	buf.append(pwsz);
	return true;
}

template<class Buffer>
Template::Result getTemplateValue(const Template::ValueList& listValue,
								  const TemplateContext& context,
								  Buffer& buffer)
{
	MacroVariableHolder globalVariable;
	
	const TemplateContext::ArgumentList& l = context.getArgumentList();
	for (TemplateContext::ArgumentList::const_iterator itA = l.begin(); itA != l.end(); ++itA) {
		MacroValuePtr pValue(MacroValueFactory::getFactory().newString((*itA).pwszValue_));
		globalVariable.setVariable((*itA).pwszName_, pValue);
	}
	
	for (Template::ValueList::const_iterator itV = listValue.begin(); itV != listValue.end(); ++itV) {
		if (!append(buffer, (*itV).first))
			return Template::RESULT_ERROR;
		
		if ((*itV).second) {
			MacroContext c(context.getMessageHolder(), context.getMessage(),
				context.getAccount(), context.getSelectedMessageHolders(),
				context.getFolder(), context.getDocument(), context.getWindow(),
				context.getProfile(), context.getBodyCharset(), context.getMacroFlags(),
				context.getSecurityMode(), context.getErrorHandler(), &globalVariable);
			MacroValuePtr pValue((*itV).second->value(&c));
			if (!pValue.get()) {
				if (c.getReturnType() == MacroContext::RETURNTYPE_NONE)
					return Template::RESULT_ERROR;
				else
					return Template::RESULT_CANCEL;
			}
			if (!append(buffer, pValue->string().get()))
				return Template::RESULT_ERROR;
		}
	}
	
	return Template::RESULT_SUCCESS;
}

}

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
	Result result = getTemplateValue(listValue_, context, buf);
	*pwstrValue = buf.getString();
	return result;
}

Template::Result qm::Template::getValue(const TemplateContext& context,
										wxstring_size_ptr* pwstrValue) const
{
	assert(pwstrValue);
	
	pwstrValue->reset(0, -1);
	
	XStringBuffer<WXSTRING> buf;
	Result result = getTemplateValue(listValue_, context, buf);
	*pwstrValue = buf.getXStringSize();
	return result;
}


/****************************************************************************
 *
 * TemplateContext
 *
 */

qm::TemplateContext::TemplateContext(MessageHolderBase* pmh,
									 Message* pMessage,
									 const MessageHolderList& listSelected,
									 Folder* pFolder,
									 Account* pAccount,
									 Document* pDocument,
									 HWND hwnd,
									 const WCHAR* pwszBodyCharset,
									 unsigned int nMacroFlags,
									 unsigned int nSecurityMode,
									 Profile* pProfile,
									 MacroErrorHandler* pErrorHandler,
									 const ArgumentList& listArgument) :
	pmh_(pmh),
	pMessage_(pMessage),
	listSelected_(listSelected),
	pFolder_(pFolder),
	pAccount_(pAccount),
	pDocument_(pDocument),
	hwnd_(hwnd),
	nMacroFlags_(nMacroFlags),
	nSecurityMode_(nSecurityMode),
	pProfile_(pProfile),
	pErrorHandler_(pErrorHandler),
	listArgument_(listArgument)
{
	if (pwszBodyCharset)
		wstrBodyCharset_ = allocWString(pwszBodyCharset);
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

Folder* qm::TemplateContext::getFolder() const
{
	return pFolder_;
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

const WCHAR* qm::TemplateContext::getBodyCharset() const
{
	return wstrBodyCharset_.get();
}

unsigned int qm::TemplateContext::getMacroFlags() const
{
	return nMacroFlags_;
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

std::auto_ptr<Template> qm::TemplateParser::parse(Reader* pReader,
												  const WCHAR* pwszName) const
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
	
	MacroParser parser;
	
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
				return error(L"Could not read from the reader", pwszName);
			else if (nRead != 1)
				break;
		}
		
		if (!bMacro) {
			if (c == L'{') {
				size_t nRead = pReader->read(&c, 1);
				if (nRead == -1) {
					return error(L"Could not read from the reader", pwszName);
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
					return error(L"Could not read from the reader", pwszName);
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
					return error(L"Could not read from the reader", pwszName);
				else if (nRead == 1 && c != L'{')
					cNext = c;
			}
			else if (c == L'}') {
				size_t nRead = pReader->read(&c, 1);
				if (nRead == -1) {
					return error(L"Could not read from the reader", pwszName);
				}
				else if (nRead == 1 && c == L'}') {
					bufMacro.append(L'}');
				}
				else {
					std::auto_ptr<Macro> pMacro(parser.parse(bufMacro.getCharArray()));
					if (!pMacro.get())
						return error(L"Error occured while parsing macro: %s.", bufMacro.getCharArray());
					
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

std::auto_ptr<Template> qm::TemplateParser::error(const WCHAR* pwszLog,
												  const WCHAR* pwszArg) const
{
	if (!pwszArg)
		pwszArg = L"";
	
	Log log(InitThread::getInitThread().getLogger(), L"qm::TemplateParser");
	log.errorf(pwszLog, pwszArg);
	
	return std::auto_ptr<Template>(0);
}
