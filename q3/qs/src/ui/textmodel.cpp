/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

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

qs::TextModel::Line::Line(const WCHAR* pwszText,
						  size_t nLen) :
	pwszText_(pwszText),
	nLen_(nLen)
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
	void fireEvent(void (TextModelHandler::*pfn)(const TextModelEvent&),
				   const TextModelEvent& event) const;
	
	typedef std::vector<TextModelHandler*> HandlerList;
	HandlerList listHandler_;
};

void qs::AbstractTextModelImpl::fireEvent(void (TextModelHandler::*pfn)(const TextModelEvent&),
										  const TextModelEvent& event) const
{
	HandlerList::const_iterator it = listHandler_.begin();
	while (it != listHandler_.end()) {
		((*it)->*pfn)(event);
		++it;
	}
}


/****************************************************************************
 *
 * AbstractTextModel
 *
 */

qs::AbstractTextModel::AbstractTextModel() :
	pImpl_(0)
{
	pImpl_ = new AbstractTextModelImpl();
}

qs::AbstractTextModel::~AbstractTextModel()
{
	delete pImpl_;
}

void qs::AbstractTextModel::addTextModelHandler(TextModelHandler* pHandler)
{
	pImpl_->listHandler_.push_back(pHandler);
}

void qs::AbstractTextModel::removeTextModelHandler(TextModelHandler* pHandler)
{
	AbstractTextModelImpl::HandlerList::iterator it = std::remove(
		pImpl_->listHandler_.begin(), pImpl_->listHandler_.end(), pHandler);
	pImpl_->listHandler_.erase(it, pImpl_->listHandler_.end());
}

void qs::AbstractTextModel::fireTextUpdated(unsigned int nStartLine,
											unsigned int nOldEndLine,
											unsigned int nNewEndLine)
{
	pImpl_->fireEvent(&TextModelHandler::textUpdated,
		TextModelEvent(this, nStartLine, nOldEndLine, nNewEndLine));
}

void qs::AbstractTextModel::fireTextSet()
{
	pImpl_->fireEvent(&TextModelHandler::textSet, TextModelEvent(this, 0, 0, 0));
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
		EditLine(const WCHAR* pwsz,
				 size_t nLen);
		~EditLine();
	
	public:
		const WCHAR* getText() const;
		size_t getLength() const;
		void insertText(unsigned int nChar,
						const WCHAR* pwsz,
						size_t nLen);
		void deleteText(unsigned int nChar,
						size_t nLen);
	
	private:
		EditLine(const EditLine&);
		EditLine& operator=(const EditLine&);
	
	private:
		StringBuffer<WSTRING> buf_;
	};

public:
	void clear();
	void clearLines(unsigned int nStart,
					unsigned int nEnd);

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

void qs::EditableTextModelImpl::clearLines(unsigned int nStart,
										   unsigned int nEnd)
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

qs::EditableTextModelImpl::EditLine::EditLine(const WCHAR* p,
											  size_t nLen) :
	buf_(p, nLen)
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

void qs::EditableTextModelImpl::EditLine::insertText(unsigned int nChar,
													 const WCHAR* pwsz,
													 size_t nLen)
{
	assert((nChar < buf_.getLength()) ||
		(nChar == buf_.getLength() &&
			(buf_.getLength() == 0 || buf_.get(nChar - 1) != L'\n')));
	buf_.insert(nChar, pwsz, nLen);
}

void qs::EditableTextModelImpl::EditLine::deleteText(unsigned int nChar,
													 size_t nLen)
{
	buf_.remove(nChar, nLen == -1 ? -1 : nChar + nLen);
}


/****************************************************************************
 *
 * EditableTextModel
 *
 */

qs::EditableTextModel::EditableTextModel() :
	pImpl_(0)
{
	pImpl_ = new EditableTextModelImpl();
	pImpl_->pThis_ = this;
}

qs::EditableTextModel::~EditableTextModel()
{
	if (pImpl_) {
		pImpl_->clear();
		delete pImpl_;
	}
}

wxstring_ptr qs::EditableTextModel::getText() const
{
	XStringBuffer<WXSTRING> buf;
	
	for (EditableTextModelImpl::LineList::const_iterator it = pImpl_->listLine_.begin(); it != pImpl_->listLine_.end(); ++it) {
		if (!buf.append((*it)->getText(), (*it)->getLength()))
			return 0;
	}
	
	return buf.getXString();
}

bool qs::EditableTextModel::setText(const WCHAR* pwszText,
									size_t nLen)
{
	QTRY {
		pImpl_->clear();
		
		if (nLen == static_cast<size_t>(-1))
			nLen = wcslen(pwszText);
		
		const WCHAR* p = pwszText;
		for (size_t n = 0; n <= nLen; ++n) {
			if (n == nLen || *pwszText == L'\n') {
				size_t nLineLen = pwszText - p + (n == nLen ? 0 : 1);
				std::auto_ptr<EditableTextModelImpl::EditLine> pLine(
					new EditableTextModelImpl::EditLine(p, nLineLen));
				pImpl_->listLine_.push_back(pLine.get());
				pLine.release();
				p = pwszText + 1;
			}
			++pwszText;
		}
		
		fireTextSet();
	}
	QCATCH_ALL() {
		return false;
	}
	
	return true;
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

void qs::EditableTextModel::update(unsigned int nStartLine,
								   unsigned int nStartChar,
								   unsigned int nEndLine,
								   unsigned int nEndChar,
								   const WCHAR* pwsz,
								   size_t nLen,
								   unsigned int* pnLine,
								   unsigned int* pnChar)
{
	assert(nStartLine < pImpl_->listLine_.size());
	assert(pnLine);
	assert(pnChar);
	
	typedef EditableTextModelImpl::EditLine EditLine;
	
	if (nEndLine == -1) {
		nEndLine = nStartLine;
		nEndChar = nStartChar;
	}
	
	EditLine* pLine = pImpl_->listLine_[nStartLine];
	
	wstring_ptr wstrLast;
	if (nEndLine == nStartLine) {
		assert(nStartChar <= nEndChar);
		if (nStartChar < nEndChar)
			pLine->deleteText(nStartChar, nEndChar - nStartChar);
	}
	else {
		assert(nStartLine < nEndLine);
		
		pImpl_->listLine_[nStartLine]->deleteText(nStartChar, -1);
		
		EditLine* pEndLine = pImpl_->listLine_[nEndLine];
		wstrLast = allocWString(pEndLine->getText() + nEndChar,
			pEndLine->getLength() - nEndChar);
	}
	
	size_t n = 0;
	const WCHAR* p = pwsz;
	for (n = 0; n < nLen; ++n) {
		if (*p == L'\n')
			break;
		++p;
	}
	if (n == nLen) {
		pLine->insertText(nStartChar, pwsz, nLen);
		*pnLine = nStartLine;
		*pnChar = nStartChar + nLen;
		
		if (nStartLine != nEndLine) {
			if (wstrLast.get())
				pLine->insertText(nStartChar + nLen,
					wstrLast.get(), wcslen(wstrLast.get()));
			pImpl_->clearLines(nStartLine + 1, nEndLine + 1);
		}
	}
	else {
		++p;
		++n;
		
		if (pLine->getLength() != nStartChar) {
			assert(!wstrLast.get());
			wstrLast = allocWString(pLine->getText() + nStartChar,
				pLine->getLength() - nStartChar);
		}
		pLine->deleteText(nStartChar, -1);
		pLine->insertText(nStartChar, pwsz, n);
		
		typedef EditableTextModelImpl::LineList LineList;
		LineList l;
		struct Deleter
		{
			Deleter(LineList& l) :
				p_(&l)
			{
			}
			
			~Deleter()
			{
				if (p_)
					std::for_each(p_->begin(), p_->end(),
						qs::deleter<EditableTextModelImpl::EditLine>());
			}
			
			void release() { p_ = 0; }
			
			LineList* p_;
		} deleter(l);
		
		const WCHAR* pBegin = p;
		while (n < nLen) {
			if (*p == L'\n') {
				std::auto_ptr<EditLine> pNewLine(new EditLine(pBegin, p - pBegin + 1));
				l.push_back(pNewLine.get());
				pNewLine.release();
				pBegin = p + 1;
			}
			++p;
			++n;
		}
		
		std::auto_ptr<EditLine> pLastLine(new EditLine(pBegin, p - pBegin));
		if (wstrLast.get())
			pLastLine->insertText(pLastLine->getLength(),
				wstrLast.get(), wcslen(wstrLast.get()));
		l.push_back(pLastLine.get());
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
		
		if (it != l.end())
			pImpl_->listLine_.insert(pImpl_->listLine_.begin() + n, it,
				static_cast<LineList::const_iterator>(l.end()));
		deleter.release();
		
		*pnLine = nStartLine + l.size();
		*pnChar = p - pBegin;
	}
	
	fireTextUpdated(nStartLine, nEndLine, *pnLine);
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
	bool appendText(const WCHAR* pwszText,
					size_t nLen,
					bool bFireEvent);
	void clearText(bool bFireEvent);

public:
	virtual void timerTimeout(unsigned int nId);

private:
	void updateLines(bool bClear,
					 bool bFireEvent);

public:
	typedef std::vector<std::pair<size_t, size_t> > LineList;

public:
	ReadOnlyTextModel* pThis_;
	std::auto_ptr<StringBuffer<WSTRING> > pBuffer_;
	LineList listLine_;
	
	std::auto_ptr<Timer> pTimer_;
	std::auto_ptr<Reader> pReader_;
	unsigned int nTimerLoad_;
};

bool qs::ReadOnlyTextModelImpl::appendText(const WCHAR* pwszText,
										   size_t nLen,
										   bool bFireEvent)
{
	if (nLen == static_cast<size_t>(-1))
		nLen = wcslen(pwszText);
	
	size_t nOldLen = pBuffer_->getLength();
	QTRY {
		pBuffer_->append(pwszText, nLen);
		updateLines(false, bFireEvent);
	}
	QCATCH_ALL() {
		return false;
	}
	return true;
}

void qs::ReadOnlyTextModelImpl::clearText(bool bFireEvent)
{
	pBuffer_->remove();
	updateLines(true, bFireEvent);
}

void qs::ReadOnlyTextModelImpl::timerTimeout(unsigned int nId)
{
	if (nId == nTimerLoad_) {
		assert(pReader_.get());
		
		bool bError = false;
		typedef std::vector<WCHAR> Buffer;
		Buffer buf;
		buf.resize(10240);
		
		size_t nRead = pReader_->read(&buf[0], buf.size());
		if (nRead == -1)
			bError = true;
		if (!bError && nRead != 0) {
			if (!appendText(&buf[0], nRead, true))
				bError = true;
		}
		
		if (bError || nRead == 0) {
			pReader_.reset(0);
			pTimer_->killTimer(nTimerLoad_);
			nTimerLoad_ = 0;
		}
	}
}

void qs::ReadOnlyTextModelImpl::updateLines(bool bClear,
											bool bFireEvent)
{
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
			listLine_.push_back(std::make_pair(nPos, n - nPos + (*p ? 1 : 0)));
			nPos = n + 1;
		}
		if (!*p)
			break;
		++p;
	}
	
	if (bFireEvent) {
		if (bClear)
			pThis_->fireTextSet();
		else
			pThis_->fireTextUpdated(nStartLine, nOldEndLine,
				listLine_.empty() ? 0 : listLine_.size() - 1);
	}
}


/****************************************************************************
 *
 * ReadOnlyTextModel
 *
 */

qs::ReadOnlyTextModel::ReadOnlyTextModel() :
	pImpl_(0)
{
	std::auto_ptr<StringBuffer<WSTRING> > pBuffer(new StringBuffer<WSTRING>());
	std::auto_ptr<Timer> pTimer(new Timer());
	
	pImpl_ = new ReadOnlyTextModelImpl();
	pImpl_->pThis_ = this;
	pImpl_->pBuffer_ = pBuffer;
	pImpl_->pTimer_ = pTimer;
	pImpl_->nTimerLoad_ = 0;
}

qs::ReadOnlyTextModel::~ReadOnlyTextModel()
{
	delete pImpl_;
}

std::pair<const WCHAR*, size_t> qs::ReadOnlyTextModel::getText() const
{
	return std::make_pair(pImpl_->pBuffer_->getCharArray(),
		pImpl_->pBuffer_->getLength());
}

bool qs::ReadOnlyTextModel::setText(const WCHAR* pwszText,
									size_t nLen)
{
	pImpl_->clearText(false);
	if (!pImpl_->appendText(pwszText, nLen, false))
		return false;
	fireTextSet();
	return true;
}

bool qs::ReadOnlyTextModel::loadText(std::auto_ptr<Reader> pReader,
									 bool bAsync)
{
	assert(pReader.get());
	
	cancelLoad();
	
	assert(!pImpl_->pReader_.get());
	
	pImpl_->clearText(false);
	
	WCHAR wsz[1024];
	size_t nRead = -1;
	while ((!bAsync || pImpl_->listLine_.size() < ReadOnlyTextModelImpl::INITIAL_LOAD_LINES)) {
		nRead = pReader->read(wsz, countof(wsz));
		if (nRead == -1)
			return false;
		else if (nRead == 0)
			break;
		if (!pImpl_->appendText(wsz, nRead, false))
			return false;
	}
	
	fireTextSet();
	
	if (nRead != 0) {
		pImpl_->pReader_ = pReader;
		pImpl_->nTimerLoad_ = pImpl_->pTimer_->setTimer(
			ReadOnlyTextModelImpl::TIMER_LOAD,
			ReadOnlyTextModelImpl::LOAD_INTERVAL, pImpl_);
	}
	
	return true;
}

void qs::ReadOnlyTextModel::cancelLoad()
{
	if (pImpl_->pReader_.get()) {
		pImpl_->pTimer_->killTimer(pImpl_->nTimerLoad_);
		pImpl_->pReader_.reset(0);
	}
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

void qs::ReadOnlyTextModel::update(unsigned int nStartLine,
								   unsigned int nStartChar,
								   unsigned int nEndLine,
								   unsigned int nEndChar,
								   const WCHAR* pwsz,
								   size_t nLen,
								   unsigned int* pnLine,
								   unsigned int* pnChar)
{
	assert(false);
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
								   unsigned int nStartLine,
								   unsigned int nOldEndLine,
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
