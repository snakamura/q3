/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QSSTL_H__
#define __QSSTL_H__

#include <qs.h>
#include <qsassert.h>

#include <vector>
#include <hash_map>

#pragma warning(disable:4284)
#pragma warning(disable:4786)

namespace qs {

/****************************************************************************
 *
 * auto_ptr_array
 *
 */

template<class T>
class auto_ptr_array
{
public:
	explicit auto_ptr_array(T* p = 0) : p_(p) {}
	auto_ptr_array(auto_ptr_array& a) : p_(a.release()) {}
	~auto_ptr_array() { delete[] p_; }
	auto_ptr_array& operator=(auto_ptr_array& a)
	{
		if (&a != this) {
			delete[] p_;
			p_ = a.release();
		}
		return *this;
	}
	T& operator*() const { return *p_; }
//	T* operator->() const { return p_; }
	T& operator[](size_t n) const { return p_[n]; }
	T* get() const { return p_; }
	T* release()
	{
		T* p = p_;
		p_ = 0;
		return p;
	}
	void reset(T* p = 0)
	{
		if (p != p_) {
			delete[] p_;
			p_ = p;
		}
	}

private:
	T* p_;
};


/****************************************************************************
 *
 * malloc_ptr
 *
 */

template<class T>
class malloc_ptr
{
public:
	explicit malloc_ptr(T* p = 0) : p_(p) {}
	malloc_ptr(malloc_ptr& a) : p_(a.release()) {}
	~malloc_ptr() { free(p_); }
	malloc_ptr& operator=(malloc_ptr& a)
	{
		if (&a != this) {
			free(p_);
			p_ = a.release();
		}
		return *this;
	}
	T& operator*() const { return *p_; }
	T* operator->() const { return p_; }
	T& operator[](size_t n) const { return p_[n]; }
	T* get() const { return p_; }
	T* release()
	{
		T* p = p_;
		p_ = 0;
		return p;
	}
	void reset(T* p = 0)
	{
		if (p != p_) {
			free(p_);
			p_ = p;
		}
	}

private:
	T* p_;
};


/****************************************************************************
 *
 * malloc_size_ptr
 *
 */

template<class T>
class malloc_size_ptr
{
public:
	malloc_size_ptr() : p_(0), nSize_(-1) {}
	malloc_size_ptr(T* p, size_t nSize) : p_(p), nSize_(nSize) {}
	malloc_size_ptr(malloc_ptr<T>& a, size_t nSize) : p_(a.release()), nSize_(nSize) {}
	malloc_size_ptr(malloc_size_ptr& a) : p_(a.release()), nSize_(a.nSize_) {}
	~malloc_size_ptr() { free(p_); }
	malloc_size_ptr& operator=(malloc_size_ptr& a)
	{
		if (&a != this) {
			free(p_);
			p_ = a.release();
			nSize_ = a.nSize_;
		}
		return *this;
	}
	T& operator*() const { return *p_; }
	T* operator->() const { return p_; }
	T& operator[](size_t n) const { return p_[n]; }
	T* get() const { return p_; }
	T* release()
	{
		T* p = p_;
		p_ = 0;
		return p;
	}
	void reset(T* p = 0)
	{
		if (p != p_) {
			free(p_);
			p_ = p;
		}
	}
	size_t size() const { return nSize_; }

private:
	T* p_;
	size_t nSize_;
};


/****************************************************************************
 *
 * deleter
 *
 */

template<class T>
struct deleter : public std::unary_function<T*, T*>
{
	T* operator()(T* p) const { delete p; return 0; }
};


/****************************************************************************
 *
 * container_deleter
 *
 */

template<class Container>
class container_deleter
{
public:
	container_deleter(Container& c) : p_(&c) {}
	~container_deleter()
	{
		if (p_) {
			Container::iterator it = p_->begin();
			while (it != p_->end())
				delete *it++;
			p_->clear();
		}
	}

public:
	void release() { p_ = 0; }

private:
	container_deleter(const container_deleter&);
	container_deleter& operator=(const container_deleter&);

private:
	Container* p_;
};


/****************************************************************************
 *
 * pair_less_t
 *
 */

template<class Op1, class Op2>
struct pair_less_t :
	public std::binary_function<
		std::pair<Op1::first_argument_type, Op2::first_argument_type>,
		std::pair<Op2::second_argument_type, Op2::second_argument_type>,
		bool>
{
	pair_less_t(const Op1& op1, const Op2& op2) :
		op1_(op1), op2_(op2) {}
	bool operator()(const first_argument_type& lhs,
		const second_argument_type& rhs) const
	{
		if (op1_(lhs.first, rhs.first))
			return true;
		else if (op1_(rhs.first, lhs.first))
			return false;
		else
			return op2_(lhs.second, rhs.second);
	}
	Op1 op1_;
	Op2 op2_;
};

template<class Op1, class Op2>
pair_less_t<Op1, Op2> pair_less(const Op1& op1, const Op2& op2)
{
	return pair_less_t<Op1, Op2>(op1, op2);
}


/****************************************************************************
 *
 * contains
 *
 */

template<class T>
struct contains : public std::binary_function<T, T, bool>
{
	bool operator()(const T& lhs, const T&rhs) const
		{ return (lhs & rhs) != 0; }
};


/****************************************************************************
 *
 * stdcall_pointer_to_unary_function
 *
 */

template<class Arg, class Result>
struct stdcall_pointer_to_unary_function :
	public std::unary_function<Arg, Result>
{
	stdcall_pointer_to_unary_function(Result (__stdcall* pfn)(Arg)) :
		pfn_(pfn) {}
	Result operator()(const Arg& arg) const { return (*pfn_)(arg); }
	Result (__stdcall* pfn_)(Arg);
};

template<class Arg, class Result>
stdcall_pointer_to_unary_function<Arg, Result> stdcall_ptr_fun(
	Result (__stdcall* pfn)(Arg))
{
	return stdcall_pointer_to_unary_function<Arg, Result>(pfn);
}


/****************************************************************************
 *
 * stdcall_pointer_to_binary_function
 *
 */

template<class Arg1, class Arg2, class Result>
struct stdcall_pointer_to_binary_function :
	public std::binary_function<Arg1, Arg2, Result>
{
	stdcall_pointer_to_binary_function(Result (__stdcall* pfn)(Arg1, Arg2)) :
		pfn_(pfn) {}
	Result operator()(const Arg1& arg1, const Arg2& arg2) const
		{ return (*pfn_)(arg1, arg2); }
	Result (__stdcall* pfn_)(Arg1, Arg2);
};

template<class Arg1, class Arg2, class Result>
stdcall_pointer_to_binary_function<Arg1, Arg2, Result> stdcall_ptr_fun(
	Result (__stdcall* pfn)(Arg1, Arg2))
{
	return stdcall_pointer_to_binary_function<Arg1, Arg2, Result>(pfn);
}


/****************************************************************************
 *
 * unary_compose_f_gx_t
 *
 */

template<class Op1, class Op2>
struct unary_compose_f_gx_t :
	public std::unary_function<typename Op2::argument_type, typename Op1::result_type>
{
	unary_compose_f_gx_t(const Op1& op1, const Op2& op2) : op1_(op1), op2_(op2) {}
	result_type operator()(const argument_type& x) const
		{ return op1_(op2_(x)); }
	Op1 op1_;
	Op2 op2_;
};

template<class Op1, class Op2>
unary_compose_f_gx_t<Op1, Op2> unary_compose_f_gx(const Op1& op1, const Op2& op2)
{
	return unary_compose_f_gx_t<Op1, Op2>(op1, op2);
}


/****************************************************************************
 *
 * unary_compose_f_gx_hy_t
 *
 */

template<class Op1, class Op2, class Op3>
struct unary_compose_f_gx_hy_t :
	public std::unary_function<typename Op2::argument_type, typename Op1::result_type>
{
	unary_compose_f_gx_hy_t(const Op1& op1, const Op2& op2, const Op3& op3) :
		op1_(op1), op2_(op2), op3_(op3) {}
	result_type operator()(const argument_type& x) const
		{ return op1_(op2_(x), op3_(x)); }
	Op1 op1_;
	Op2 op2_;
	Op3 op3_;
};

template<class Op1, class Op2, class Op3>
unary_compose_f_gx_hy_t<Op1, Op2, Op3> unary_compose_f_gx_hy(
	const Op1& op1, const Op2& op2, const Op3& op3)
{
	return unary_compose_f_gx_hy_t<Op1, Op2, Op3>(op1, op2, op3);
}


/****************************************************************************
 *
 * unary_compose_fx_gx
 *
 */

template<class Op1, class Op2>
struct unary_compose_fx_gx_t :
	public std::unary_function<
		std::pair<typename Op1::argument_type, typename Op2::argument_type>,
		typename Op2::result_type>
{
	unary_compose_fx_gx_t(const Op1& op1, const Op2& op2) : op1_(op1), op2_(op2) {}
	result_type operator()(const argument_type& x) const
		{ op1_(x.first); return op2_(x.second); }
	Op1 op1_;
	Op2 op2_;
};

template<class Op1, class Op2>
unary_compose_fx_gx_t<Op1, Op2> unary_compose_fx_gx(const Op1& op1, const Op2& op2)
{
	return unary_compose_fx_gx_t<Op1, Op2>(op1, op2);
}


/****************************************************************************
 *
 * binary_compose_f_gx_hy_t
 *
 */

template<class Op1, class Op2, class Op3>
struct binary_compose_f_gx_hy_t :
	public std::binary_function<typename Op2::argument_type,
		typename Op3::argument_type, typename Op1::result_type>
{
	binary_compose_f_gx_hy_t(const Op1& op1, const Op2& op2, const Op3& op3) :
		op1_(op1), op2_(op2), op3_(op3) {}
	result_type operator()(const first_argument_type& x,
		const second_argument_type& y) const
		{ return op1_(op2_(x), op3_(y)); }
	Op1 op1_;
	Op2 op2_;
	Op3 op3_;
};

template<class Op1, class Op2, class Op3>
binary_compose_f_gx_hy_t<Op1, Op2, Op3> binary_compose_f_gx_hy(
	const Op1& op1, const Op2& op2, const Op3& op3)
{
	return binary_compose_f_gx_hy_t<Op1, Op2, Op3>(op1, op2, op3);
}


/****************************************************************************
 *
 * binary_and_t
 *
 */

template<class T>
struct binary_and_t : public std::binary_function<T, T, bool>
{
	bool operator()(const T& lhs, const T& rhs) const
		{ return lhs && rhs; }
};


/****************************************************************************
 *
 * binary_or_t
 *
 */

template<class T>
struct binary_or_t : public std::binary_function<T, T, bool>
{
	bool operator()(const T& lhs, const T& rhs) const
		{ return lhs || rhs; }
};


/****************************************************************************
 *
 * mem_data_t
 *
 */

template<class T, class U>
struct mem_data_t : public std::unary_function<T, U>
{
	mem_data_t(U T::*p) : p_(p) {}
	U operator()(T* p) const { return p->*p_; }
	U T::*p_;
};

template<class T, class U>
mem_data_t<T, U> mem_data(U T::*p)
{
	return mem_data_t<T, U>(p);
}


/****************************************************************************
 *
 * mem_data_ref_t
 *
 */

template<class T, class U>
struct mem_data_ref_t : public std::unary_function<T, U>
{
	mem_data_ref_t(U T::*p) : p_(p) {}
	U operator()(const T& t) const { return t.*p_; }
	U T::*p_;
};

template<class T, class U>
mem_data_ref_t<T, U> mem_data_ref(U T::*p)
{
	return mem_data_ref_t<T, U>(p);
}


/****************************************************************************
 *
 * transform_if
 *
 */

template<class InIt, class OutIt, class Pred, class Trans>
OutIt transform_if(InIt first, InIt last, OutIt it, Pred pred, Trans trans)
{
	while (first != last) {
		if (pred(*first)) {
			*it = trans(*first);
			++it;
		}
		++first;
	}
	return it;
}


/****************************************************************************
 *
 * transform2
 *
 */

template<class InIt, class OutIt1, class OutIt2, class Pred, class Trans1, class Trans2>
std::pair<OutIt1, OutIt2> transform2(InIt first, InIt last,
	OutIt1 it1, OutIt2 it2, Pred pred, Trans1 trans1, Trans2 trans2)
{
	while (first != last) {
		if (pred(*first)) {
			*it1 = trans1(*first);
			++it1;
		}
		else {
			*it2 = trans2(*first);
			++it2;
		}
		++first;
	}
	return std::make_pair(it1, it2);
}

}

#pragma warning(default:4786)

#endif // __QSSTL_H__
