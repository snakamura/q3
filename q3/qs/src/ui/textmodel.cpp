/*
 * $Id: textmodel.cpp,v 1.1.1.1 2003/04/29 08:07:37 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qserror.h>
#include <qsnew.h>
#include <qsstream.h>
#include <qstextwindow.h>
#include <qstimer.h>

#include <algorithm>
#include <vector>

#pragma warning(disable:4786)

using namespace qs;


/****************************************************************************
 *
 * TextModel
 *
 */

qs::TextModel::~TextModel()
{
}


/****************************************************************************
 *
 * TextModel::Line
 *
 */

qs::TextModel::Line::Line(const WCHAR* pwszText, size_t nLen) :
	pwszText_(pwszText), nLen_(nLen)
{
}

qs::TextModel::Line::~Line()
{
}

const WCHAR* qs::TextModel::Line::getText() const
{
	return pwszText_;
}

size_t qs::TextModel::Line::getLength() const
{
	return nLen_;
}


/****************************************************************************
 *
 * AbstractTextModelImpl
 *
 */

struct qs::AbstractTextModelImpl
{
	QSTATUS fireEvent(QSTATUS (TextModelHandler::*pfn)(const TextModelEvent&),
		const TextModelEvent& event) const;
	
	typedef std::vector<TextModelHandler*> HandlerList;
	HandlerList listHandler_;
};

QSTATUS qs::AbstractTextModelImpl::fireEvent(
	QSTATUS (TextModelHandler::*pfn)(const TextModelEvent&),
	const TextModelEvent& event) const
{
	DECLARE_QSTATUS();
	
	HandlerList::const_iterator it = listHandler_.begin();
	while (it != listHandler_.end()) {
		status = ((*it)->*pfn)(event);
		CHECK_QSTATUS();
		++it;
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * AbstractTextModel
 *
 */

qs::AbstractTextModel::AbstractTextModel(QSTATUS* pstatus) :
	pImpl_(0)
{
	assert(pstatus);
	
	DECLARE_QSTATUS();
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
}

qs::AbstractTextModel::~AbstractTextModel()
{
	delete pImpl_;
}

QSTATUS qs::AbstractTextModel::addTextModelHandler(TextModelHandler* pHandler)
{
	return STLWrapper<AbstractTextModelImpl::HandlerList>(
		pImpl_->listHandler_).push_back(pHandler);
}

QSTATUS qs::AbstractTextModel::removeTextModelHandler(TextModelHandler* pHandler)
{
	AbstractTextModelImpl::HandlerList::iterator it = std::remove(
		pImpl_->listHandler_.begin(), pImpl_->listHandler_.end(), pHandler);
	pImpl_->listHandler_.erase(it, pImpl_->listHandler_.end());
	return QSTATUS_SUCCESS;
}

QSTATUS qs::AbstractTextModel::fireTextUpdated(unsigned int nStartLine,
	unsigned int nOldEndLine, unsigned int nNewEndLine)
{
	return pImpl_->fireEvent(&TextModelHandler::textUpdated,
		TextModelEvent(this, nStartLine, nOldEndLine, nNewEndLine));
}

QSTATUS qs::AbstractTextModel::fireTextSet()
{
	return pImpl_->fireEvent(&TextModelHandler::textSet,
		TextModelEvent(this, 0, 0, 0));
}


/****************************************************************************
 *
 * EditableTextModelImpl
 *
 */

struct qs::EditableTextModelImpl
{
public:
	class EditLine
	{
	public:
		EditLine(const WCHAR* pwsz, size_t nLen, QSTATUS* pstatus);
		~EditLine();
	
	public:
		const WCHAR* getText() const;
		size_t getLength() const;
		QSTATUS insertText(unsigned int nChar,
			const WCHAR* pwsz, size_t nLen);
		QSTATUS deleteText(unsigned int nChar, size_t nLen);
	
	private:
		EditLine(const EditLine&);
		EditLine& operator=(const EditLine&);
	
	private:
		StringBuffer<WSTRING> buf_;
	};

public:
	void clear();
	void clearLines(unsigned int nStart, unsigned int nEnd);

public:
	typedef std::vector<EditLine*> LineList;

public:
	EditableTextModel* pThis_;
	LineList listLine_;
};

void qs::EditableTextModelImpl::clear()
{
	std::for_each(listLine_.begin(),
		listLine_.end(), deleter<EditLine>());
	listLine_.clear();
}

void qs::EditableTextModelImpl::clearLines(unsigned int nStart, unsigned int nEnd)
{
	LineList::iterator begin = listLine_.begin() + nStart;
	LineList::iterator end = listLine_.begin() + nEnd;
	
	std::for_each(begin, end, deleter<EditLine>());
	listLine_.erase(begin, end);
}


/****************************************************************************
 *
 * EditableTextModelImpl::EditLine
 *
 */

qs::EditableTextModelImpl::EditLine::EditLine(
	const WCHAR* p, size_t nLen, QSTATUS* pstatus) :
	buf_(p, nLen, pstatus)
{
}

qs::EditableTextModelImpl::EditLine::~EditLine()
{
}

const WCHAR* qs::EditableTextModelImpl::EditLine::getText() const
{
	return buf_.getCharArray();
}

size_t qs::EditableTextModelImpl::EditLine::getLength() const
{
	return buf_.getLength();
}

QSTATUS qs::EditableTextModelImpl::EditLine::insertText(
	unsigned int nChar, const WCHAR* pwsz, size_t nLen)
{
	assert((nChar < buf_.getLength()) ||
		(nChar == buf_.getLength() &&
			(buf_.getLength() == 0 || buf_.get(nChar - 1) != L'\n')));
	
	DECLARE_QSTATUS();
	
	status = buf_.insert(nChar, pwsz, nLen);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::EditableTextModelImpl::EditLine::deleteText(
	unsigned int nChar, size_t nLen)
{
	return buf_.remove(nChar, nLen == -1 ? -1 : nChar + nLen);
}


/****************************************************************************
 *
 * EditableTextModel
 *
 */

qs::EditableTextModel::EditableTextModel(QSTATUS* pstatus) :
	AbstractTextModel(pstatus),
	pImpl_(0)
{
	if (*pstatus != QSTATUS_SUCCESS)
		return;
	
	DECLARE_QSTATUS();
	
	*pstatus = QSTATUS_SUCCESS;
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	pImpl_->pThis_ = this;
}

qs::EditableTextModel::~EditableTextModel()
{
	if (pImpl_) {
		pImpl_->clear();
		delete pImpl_;
	}
}

QSTATUS qs::EditableTextModel::getText(WSTRING* pwstrText) const
{
	assert(pwstrText);
	
	DECLARE_QSTATUS();
	
	*pwstrText = 0;
	
	StringBuffer<WSTRING> buf(&status);
	CHECK_QSTATUS();
	
	EditableTextModelImpl::LineList::const_iterator it = pImpl_->listLine_.begin();
	while (it != pImpl_->listLine_.end()) {
		status = buf.append((*it)->getText(), (*it)->getLength());
		CHECK_QSTATUS();
		++it;
	}
	
	*pwstrText = buf.getString();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::EditableTextModel::setText(const WCHAR* pwszText, size_t nLen)
{
	DECLARE_QSTATUS();
	
	pImpl_->clear();
	
	if (nLen == static_cast<size_t>(-1))
		nLen = wcslen(pwszText);
	
	const WCHAR* p = pwszText;
	for (size_t n = 0; n <= nLen; ++n) {
		if (n == nLen || *pwszText == L'\n') {
			std::auto_ptr<EditableTextModelImpl::EditLine> pLine;
			size_t nLineLen = pwszText - p + (n == nLen ? 0 : 1);
			status = newQsObject(p, nLineLen, &pLine);
			CHECK_QSTATUS();
			status = STLWrapper<EditableTextModelImpl::LineList>(
				pImpl_->listLine_).push_back(pLine.get());
			CHECK_QSTATUS();
			pLine.release();
			
			p = pwszText + 1;
		}
		++pwszText;
	}
	
	status = fireTextSet();
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

size_t qs::EditableTextModel::getLineCount() const
{
	return pImpl_->listLine_.size();
}

TextModel::Line qs::EditableTextModel::getLine(size_t nLine) const
{
	assert(nLine < pImpl_->listLine_.size());
	
	EditableTextModelImpl::EditLine* pLine = pImpl_->listLine_[nLine];
	return Line(pLine->getText(), pLine->getLength());
}

bool qs::EditableTextModel::isEditable() const
{
	return true;
}

QSTATUS qs::EditableTextModel::update(unsigned int nStartLine,
	unsigned int nStartChar, unsigned int nEndLine, unsigned int nEndChar,
	const WCHAR* pwsz, size_t nLen, unsigned int* pnLine, unsigned int* pnChar)
{
	assert(nStartLine < pImpl_->listLine_.size());
	assert(pnLine);
	assert(pnChar);
	
	DECLARE_QSTATUS();
	
	typedef EditableTextModelImpl::EditLine EditLine;
	
	if (nEndLine == -1) {
		nEndLine = nStartLine;
		nEndChar = nStartChar;
	}
	
	EditLine* pLine = pImpl_->listLine_[nStartLine];
	
	string_ptr<WSTRING> wstrLast;
	if (nEndLine == nStartLine) {
		assert(nStartChar <= nEndChar);
		if (nStartChar < nEndChar) {
			status = pLine->deleteText(nStartChar, nEndChar - nStartChar);
			CHECK_QSTATUS();
		}
	}
	else {
		assert(nStartLine < nEndLine);
		
		status = pImpl_->listLine_[nStartLine]->deleteText(nStartChar, -1);
		CHECK_QSTATUS();
		
		EditLine* pEndLine = pImpl_->listLine_[nEndLine];
		wstrLast.reset(allocWString(pEndLine->getText() + nEndChar,
			pEndLine->getLength() - nEndChar));
		if (!wstrLast.get())
			return QSTATUS_OUTOFMEMORY;
	}
	
	size_t n = 0;
	const WCHAR* p = pwsz;
	for (n = 0; n < nLen; ++n) {
		if (*p == L'\n')
			break;
		++p;
	}
	if (n == nLen) {
		status = pLine->insertText(nStartChar, pwsz, nLen);
		CHECK_QSTATUS();
		*pnLine = nStartLine;
		*pnChar = nStartChar + nLen;
		
		if (nStartLine != nEndLine) {
			if (wstrLast.get()) {
				status = pLine->insertText(nStartChar + nLen,
					wstrLast.get(), wcslen(wstrLast.get()));
				CHECK_QSTATUS();
			}
			pImpl_->clearLines(nStartLine + 1, nEndLine + 1);
		}
	}
	else {
		++p;
		++n;
		
		if (pLine->getLength() != nStartChar) {
			assert(!wstrLast.get());
			wstrLast.reset(allocWString(pLine->getText() + nStartChar,
				pLine->getLength() - nStartChar));
			if (!wstrLast.get())
				return QSTATUS_OUTOFMEMORY;
		}
		status = pLine->deleteText(nStartChar, -1);
		CHECK_QSTATUS();
		status = pLine->insertText(nStartChar, pwsz, n);
		CHECK_QSTATUS();
		
		typedef EditableTextModelImpl::LineList LineList;
		LineList l;
		struct Deleter
		{
			Deleter(LineList& l) : p_(&l) {}
			~Deleter()
			{
				if (p_)
					std::for_each(p_->begin(), p_->end(),
						deleter<EditableTextModelImpl::EditLine>());
			}
			void release() { p_ = 0; }
			LineList* p_;
		} deleter(l);
		
		const WCHAR* pBegin = p;
		while (n < nLen) {
			if (*p == L'\n') {
				std::auto_ptr<EditLine> pNewLine;
				status = newQsObject(pBegin, p - pBegin + 1, &pNewLine);
				CHECK_QSTATUS();
				status = STLWrapper<LineList>(l).push_back(pNewLine.get());
				CHECK_QSTATUS();
				pNewLine.release();
				
				pBegin = p + 1;
			}
			++p;
			++n;
		}
		
		std::auto_ptr<EditLine> pLastLine;
		status = newQsObject(pBegin, p - pBegin, &pLastLine);
		CHECK_QSTATUS();
		if (wstrLast.get()) {
			status = pLastLine->insertText(pLastLine->getLength(),
				wstrLast.get(), wcslen(wstrLast.get()));
			CHECK_QSTATUS();
		}
		status = STLWrapper<LineList>(l).push_back(pLastLine.get());
		CHECK_QSTATUS();
		pLastLine.release();
		
		LineList::const_iterator it = l.begin();
		unsigned int n = nStartLine + 1;
		if (nStartLine != nEndLine) {
			while (n <= nEndLine && it != l.end()) {
				delete pImpl_->listLine_[n];
				pImpl_->listLine_[n] = *it;
				++n;
				++it;
			}
			if (n <= nEndLine) {
				assert(it == l.end());
				pImpl_->clearLines(n, nEndLine + 1);
			}
		}
		
		if (it != l.end()) {
			status = STLWrapper<LineList>(pImpl_->listLine_).insert(
				pImpl_->listLine_.begin() + n, it, l.end());
			CHECK_QSTATUS();
		}
		deleter.release();
		
		*pnLine = nStartLine + l.size();
		*pnChar = p - pBegin;
	}
	
	status = fireTextUpdated(nStartLine, nEndLine, *pnLine);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * ReadOnlyTextModelImpl
 *
 */

class qs::ReadOnlyTextModelImpl : public TimerHandler
{
public:
	enum {
		TIMER_LOAD		= 1000,
	};
	enum {
		LOAD_INTERVAL		= 0,
		INITIAL_LOAD_LINES	= 100
	};

public:
	QSTATUS appendText(const WCHAR* pwszText, size_t nLen, bool bFireEvent);
	QSTATUS clearText(bool bFireEvent);

public:
	virtual QSTATUS timerTimeout(unsigned int nId);

private:
	QSTATUS updateLines(bool bClear, bool bFireEvent);

public:
	typedef std::vector<std::pair<size_t, size_t> > LineList;

public:
	ReadOnlyTextModel* pThis_;
	StringBuffer<WSTRING>* pBuffer_;
	LineList listLine_;
	
	Timer* pTimer_;
	Reader* pReader_;
	unsigned int nTimerLoad_;
};

QSTATUS qs::ReadOnlyTextModelImpl::appendText(
	const WCHAR* pwszText, size_t nLen, bool bFireEvent)
{
	DECLARE_QSTATUS();
	
	if (nLen == static_cast<size_t>(-1))
		nLen = wcslen(pwszText);
	
	size_t nOldLen = pBuffer_->getLength();
	status = pBuffer_->append(pwszText, nLen);
	CHECK_QSTATUS();
	status = updateLines(false, bFireEvent);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::ReadOnlyTextModelImpl::clearText(bool bFireEvent)
{
	DECLARE_QSTATUS();
	
	status = pBuffer_->remove();
	CHECK_QSTATUS();
	status = updateLines(true, bFireEvent);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::ReadOnlyTextModelImpl::timerTimeout(unsigned int nId)
{
	if (nId == nTimerLoad_) {
		assert(pReader_);
		
		bool bError = false;
		typedef std::vector<WCHAR> Buffer;
		Buffer buf;
		if (STLWrapper<Buffer>(buf).resize(10240) != QSTATUS_SUCCESS)
			bError = true;
		size_t nRead = 0;
		if (!bError) {
			if (pReader_->read(&buf[0], buf.size(), &nRead) != QSTATUS_SUCCESS)
				bError = true;
			if (!bError && nRead != static_cast<size_t>(-1)) {
				if (appendText(&buf[0], nRead, true) != QSTATUS_SUCCESS)
					bError = true;
			}
		}
		
		if (bError || nRead == static_cast<size_t>(-1)) {
			delete pReader_;
			pReader_ = 0;
			pTimer_->killTimer(nTimerLoad_);
			nTimerLoad_ = 0;
		}
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::ReadOnlyTextModelImpl::updateLines(bool bClear, bool bFireEvent)
{
	DECLARE_QSTATUS();
	
	unsigned int nStartLine = bClear ? 0 :
		listLine_.empty() ? 0 : listLine_.size() - 1;
	unsigned int nOldEndLine = bClear ?
		listLine_.empty() ? 0 : listLine_.size() - 1 : nStartLine;
	
	if (bClear)
		listLine_.clear();
	
	size_t nPos = 0;
	if (!listLine_.empty()) {
		nPos = listLine_.back().first;
		listLine_.pop_back();
	}
	
	const WCHAR* pBase = pBuffer_->getCharArray();
	const WCHAR* p = pBase + nPos;
	while (true) {
		if (!*p || *p == '\n') {
			size_t n = p - pBase;
			assert(n >= nPos);
			status = STLWrapper<LineList>(listLine_).push_back(
				std::make_pair(nPos, n - nPos + (*p ? 1 : 0)));
			CHECK_QSTATUS();
			nPos = n + 1;
		}
		if (!*p)
			break;
		++p;
	}
	
	if (bFireEvent) {
		if (bClear) {
			status = pThis_->fireTextSet();
			CHECK_QSTATUS();
		}
		else {
			status = pThis_->fireTextUpdated(nStartLine, nOldEndLine,
				listLine_.empty() ? 0 : listLine_.size() - 1);
			CHECK_QSTATUS();
		}
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * ReadOnlyTextModel
 *
 */

qs::ReadOnlyTextModel::ReadOnlyTextModel(QSTATUS* pstatus) :
	AbstractTextModel(pstatus),
	pImpl_(0)
{
	if (*pstatus != QSTATUS_SUCCESS)
		return;
	
	
	DECLARE_QSTATUS();
	
	*pstatus = QSTATUS_SUCCESS;
	
	std::auto_ptr<StringBuffer<WSTRING> > pBuffer;
	status = newQsObject(&pBuffer);
	CHECK_QSTATUS_SET(pstatus);
	
	std::auto_ptr<Timer> pTimer;
	status = newQsObject(&pTimer);
	CHECK_QSTATUS_SET(pstatus);
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	pImpl_->pThis_ = this;
	pImpl_->pBuffer_ = pBuffer.release();
	pImpl_->pTimer_ = pTimer.release();
	pImpl_->pReader_ = 0;
	pImpl_->nTimerLoad_ = 0;
}

qs::ReadOnlyTextModel::~ReadOnlyTextModel()
{
	if (pImpl_) {
		delete pImpl_->pTimer_;
		delete pImpl_->pBuffer_;
		delete pImpl_;
	}
}

QSTATUS qs::ReadOnlyTextModel::getText(const WCHAR** ppwszText, size_t* pnLen) const
{
	assert(ppwszText);
	assert(pnLen);
	
	*ppwszText = pImpl_->pBuffer_->getCharArray();
	*pnLen = pImpl_->pBuffer_->getLength();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::ReadOnlyTextModel::setText(const WCHAR* pwszText, size_t nLen)
{
	DECLARE_QSTATUS();
	
	status = pImpl_->clearText(false);
	CHECK_QSTATUS();
	status = pImpl_->appendText(pwszText, nLen, false);
	CHECK_QSTATUS();
	
	status = fireTextSet();
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::ReadOnlyTextModel::loadText(Reader* pReader, bool bAsync)
{
	assert(pReader);
	
	DECLARE_QSTATUS();
	
	status = cancelLoad();
	CHECK_QSTATUS();
	
	assert(!pImpl_->pReader_);
	
	status = pImpl_->clearText(false);
	CHECK_QSTATUS();
	
	WCHAR wsz[1024];
	size_t nRead = 0;
	while (nRead != static_cast<size_t>(-1) &&
		(!bAsync ||
		pImpl_->listLine_.size() < ReadOnlyTextModelImpl::INITIAL_LOAD_LINES)) {
		status = pReader->read(wsz, countof(wsz), &nRead);
		CHECK_QSTATUS();
		if (nRead != static_cast<size_t>(-1)) {
			status = pImpl_->appendText(wsz, nRead, false);
			CHECK_QSTATUS();
		}
	}
	
	status = fireTextSet();
	CHECK_QSTATUS();
	
	if (nRead != static_cast<size_t>(-1)) {
		pImpl_->pReader_ = pReader;
		pImpl_->nTimerLoad_ = ReadOnlyTextModelImpl::TIMER_LOAD;
		status = pImpl_->pTimer_->setTimer(&pImpl_->nTimerLoad_,
			ReadOnlyTextModelImpl::LOAD_INTERVAL, pImpl_);
		CHECK_QSTATUS();
	}
	else {
		delete pReader;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::ReadOnlyTextModel::cancelLoad()
{
	if (pImpl_->pReader_) {
		pImpl_->pTimer_->killTimer(pImpl_->nTimerLoad_);
		delete pImpl_->pReader_;
		pImpl_->pReader_ = 0;
	}
	
	return QSTATUS_SUCCESS;
}

size_t qs::ReadOnlyTextModel::getLineCount() const
{
	return pImpl_->listLine_.size();
}


TextModel::Line qs::ReadOnlyTextModel::getLine(size_t nLine) const
{
	assert(nLine < pImpl_->listLine_.size());
	
	return Line(
		pImpl_->pBuffer_->getCharArray() + pImpl_->listLine_[nLine].first,
		pImpl_->listLine_[nLine].second);
}

bool qs::ReadOnlyTextModel::isEditable() const
{
	return false;
}

QSTATUS qs::ReadOnlyTextModel::update(unsigned int nStartLine,
	unsigned int nStartChar, unsigned int nEndLine, unsigned int nEndChar,
	const WCHAR* pwsz, size_t nLen, unsigned int* pnLine, unsigned int* pnChar)
{
	assert(false);
	return QSTATUS_FAIL;
}


/****************************************************************************
 *
 * TextModelHandler
 *
 */

qs::TextModelHandler::~TextModelHandler()
{
}


/****************************************************************************
 *
 * TextModelEvent
 *
 */

qs::TextModelEvent::TextModelEvent(TextModel* pTextModel,
	unsigned int nStartLine, unsigned int nOldEndLine,
	unsigned int nNewEndLine) :
	pTextModel_(pTextModel),
	nStartLine_(nStartLine),
	nOldEndLine_(nOldEndLine),
	nNewEndLine_(nNewEndLine)
{
}

qs::TextModelEvent::~TextModelEvent()
{
}

TextModel* qs::TextModelEvent::getTextModel() const
{
	return pTextModel_;
}

unsigned int qs::TextModelEvent::getStartLine() const
{
	return nStartLine_;
}

unsigned int qs::TextModelEvent::getOldEndLine() const
{
	return nOldEndLine_;
}

unsigned int qs::TextModelEvent::getNewEndLine() const
{
	return nNewEndLine_;
}
