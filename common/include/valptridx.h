/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#pragma once

#include <cstddef>
#include <stdexcept>
#include "dxxsconf.h"
#include "compiler-type_traits.h"

#ifdef DXX_HAVE_BUILTIN_CONSTANT_P
#define DXX_VALPTRIDX_STATIC_CHECK(E,F,S)	\
	((void)(dxx_builtin_constant_p((E)) && !(E) &&	\
		(DXX_ALWAYS_ERROR_FUNCTION(F,S), 0)))
#else
#define DXX_VALPTRIDX_STATIC_CHECK(E,F,S)	\
	((void)0)
#endif

#ifdef DXX_HAVE_CXX11_REF_QUALIFIER
#define DXX_VALPTRIDX_REF_QUALIFIER_LVALUE &
#else
#define DXX_VALPTRIDX_REF_QUALIFIER_LVALUE
#endif

#define DXX_VALPTRIDX_CHECK(E,S,success,failure)	\
	(	\
		DXX_VALPTRIDX_STATIC_CHECK(E,dxx_trap_##failure,S),	\
		(E) ? success : throw failure(S)	\
	)

template <typename T>
void get_global_array(T *);

template <typename P, typename I>
class valbaseptridxutil_t
{
protected:
	struct null_pointer_exception : std::logic_error {
		null_pointer_exception(const char *s) : std::logic_error(s) {}
	};
	struct index_mismatch_exception : std::logic_error {
		index_mismatch_exception(const char *s) : std::logic_error(s) {}
	};
	struct index_range_exception : std::out_of_range {
		index_range_exception(const char *s) : std::out_of_range(s) {}
	};
	typedef P *pointer_type;
	typedef I index_type;
	static constexpr decltype(get_global_array(pointer_type())) get_array()
	{
		return get_global_array(pointer_type());
	}
	static pointer_type check_null_pointer(pointer_type p) __attribute_warn_unused_result
	{
		return DXX_VALPTRIDX_CHECK(p, "NULL pointer used", p, null_pointer_exception);
	}
	template <typename A>
		static index_type check_index_match(const A &a, pointer_type p, index_type s) __attribute_warn_unused_result;
	template <typename A>
		static index_type check_index_range(const A &a, index_type s) __attribute_warn_unused_result;
};

template <typename P, typename I>
class vvalptr_t;

template <typename P, typename I, template <I> class magic_constant>
class vvalidx_t;

template <typename P, typename I>
class valptr_t : protected valbaseptridxutil_t<P, I>
{
	typedef valbaseptridxutil_t<P, I> valutil;
protected:
	typedef typename tt::remove_const<P>::type Prc;
public:
	typedef typename valutil::pointer_type pointer_type;
	typedef P &reference;
	valptr_t() = delete;
	valptr_t(vvalptr_t<const P, I> &&) = delete;
	valptr_t(vvalptr_t<Prc, I> &&) = delete;
	valptr_t(const valptr_t<Prc, I> &t) :
		p(t.p)
	{
	}
	valptr_t(const vvalptr_t<Prc, I> &t);
	template <typename A>
		valptr_t(A &, pointer_type p) :
			p(p)
		{
		}
	valptr_t(pointer_type p) :
		p(p)
	{
	}
	pointer_type operator->() const DXX_VALPTRIDX_REF_QUALIFIER_LVALUE { return p; }
	operator pointer_type() const DXX_VALPTRIDX_REF_QUALIFIER_LVALUE { return p; }
#ifdef DXX_HAVE_CXX11_REF_QUALIFIER
	pointer_type operator->() const && = delete;
	operator pointer_type() const && = delete;
#endif
	/* Only vvalptr_t can implicitly convert to reference */
	operator reference() const = delete;
	bool operator==(const pointer_type &rhs) const { return p == rhs; }
	template <typename R>
		bool operator!=(const R &rhs) const
		{
			return !(*this == rhs);
		}
	bool operator==(const valptr_t &rhs) const { return p == rhs.p; }
	template <typename U>
		typename tt::enable_if<!tt::is_base_of<valptr_t, U>::value, bool>::type operator==(U) const = delete;
protected:
	pointer_type p;
};

template <typename P, typename I, template <I> class magic_constant>
class validx_t : protected valbaseptridxutil_t<P, I>
{
	typedef valbaseptridxutil_t<P, I> valutil;
protected:
	using valutil::check_index_match;
	using valutil::check_index_range;
public:
	typedef typename valutil::pointer_type pointer_type;
	typedef typename valutil::index_type index_type;
	typedef I integral_type;
	validx_t() = delete;
	template <integral_type v>
		validx_t(const magic_constant<v> &) :
			i(v)
	{
	}
	validx_t(vvalidx_t<P, I, magic_constant> &&) = delete;
	template <typename A>
		validx_t(A &a, pointer_type p, index_type s) :
			i(s != ~static_cast<integral_type>(0) ? check_index_match(a, p, check_index_range(a, s)) : s)
	{
	}
	template <typename A>
		validx_t(A &a, index_type s) :
			i(s != ~static_cast<integral_type>(0) ? check_index_range(a, s) : s)
	{
	}
	operator const index_type&() const { return i; }
	template <integral_type v>
		bool operator==(const magic_constant<v> &) const
		{
			return i == v;
		}
	bool operator==(const index_type &rhs) const { return i == rhs; }
	// Compatibility hack since some object numbers are in int, not
	// short
	bool operator==(const int &rhs) const { return i == rhs; }
	template <typename R>
		bool operator!=(const R &rhs) const
		{
			return !(*this == rhs);
		}
	bool operator==(const validx_t &rhs) const
	{
		return i == rhs.i;
	}
	template <typename U>
		typename tt::enable_if<!tt::is_base_of<validx_t, U>::value && !tt::is_base_of<U, validx_t>::value, bool>::type operator==(U) const = delete;
protected:
	validx_t(index_type s) :
		i(s)
	{
	}
	index_type i;
};

/*
 * A data type for passing both a pointer and its offset in an
 * agreed-upon array.  Useful for Segments, Objects.
 */
template <
	template <typename, typename> class VP0,
	template <typename, typename I, template <I> class> class VI0,
	template <typename, typename> class VP1,
	template <typename, typename I, template <I> class> class VI1,
	typename P, typename I, template <I> class magic_constant,
	typename Prc = typename tt::remove_const<P>::type>
class valptridx_template_t : protected VP0<P, I>, protected VI0<P, I, magic_constant>
{
public:
	typedef VP0<P, I> vptr_type;
	typedef VI0<P, I, magic_constant> vidx_type;
	typedef valptridx_template_t valptridx_type;
protected:
	using vidx_type::get_array;
public:
	typedef typename vptr_type::pointer_type pointer_type;
	typedef typename vptr_type::reference reference;
	typedef typename vidx_type::index_type index_type;
	typedef typename vidx_type::integral_type integral_type;
	operator const vptr_type &() const { return *this; }
	operator const vidx_type &() const { return *this; }
	using vptr_type::operator->;
	using vptr_type::operator pointer_type;
	using vptr_type::operator reference;
	using vptr_type::operator==;
	using vidx_type::operator==;
	using vidx_type::operator const index_type&;
	valptridx_template_t() = delete;
	valptridx_template_t(std::nullptr_t) = delete;
	/* Swapped V0/V1 matches rvalue construction to/from always-valid type */
	valptridx_template_t(valptridx_template_t<VP1, VI1, VP0, VI0, const P, I, magic_constant> &&) = delete;
	valptridx_template_t(valptridx_template_t<VP1, VI1, VP0, VI0, Prc, I, magic_constant> &&) = delete;
	/* Convenience conversion to/from always-valid.  Throws on attempt
	 * to make an always-valid from an invalid maybe-valid.
	 */
	valptridx_template_t(const valptridx_template_t<VP1, VI1, VP0, VI0, const P, I, magic_constant> &t) :
		vptr_type(t.operator const VP1<const P, I> &()),
		vidx_type(t.operator const VI1<const P, I, magic_constant> &())
	{
	}
	valptridx_template_t(const valptridx_template_t<VP1, VI1, VP0, VI0, Prc, I, magic_constant> &t) :
		vptr_type(t.operator const VP1<Prc, I> &()),
		vidx_type(t.operator const VI1<Prc, I, magic_constant> &())
	{
	}
	/* Copy construction from like type, possibly const-qualified */
	valptridx_template_t(const valptridx_template_t<VP0, VI0, VP1, VI1, const P, I, magic_constant> &t) :
		vptr_type(static_cast<const vptr_type &>(t)),
		vidx_type(static_cast<const vidx_type &>(t))
	{
	}
	valptridx_template_t(const valptridx_template_t<VP0, VI0, VP1, VI1, Prc, I, magic_constant> &t) :
		vptr_type(static_cast<const vptr_type &>(t)),
		vidx_type(static_cast<const vidx_type &>(t))
	{
	}
	template <integral_type v>
		valptridx_template_t(const magic_constant<v> &m) :
			vptr_type(static_cast<std::size_t>(v) < get_array().size() ? &get_array()[v] : NULL),
			vidx_type(m)
	{
	}
	template <typename A>
	valptridx_template_t(A &a, pointer_type p, index_type s) :
		vptr_type(a, p),
		vidx_type(a, p, s)
	{
	}
	template <typename A>
	valptridx_template_t(A &a, pointer_type p) :
		vptr_type(a, p),
		vidx_type(a, p, p-a)
	{
	}
	valptridx_template_t(pointer_type p) :
		vptr_type(p),
		vidx_type(get_array(), p, p-get_array())
	{
	}
	template <typename A>
	valptridx_template_t(A &a, index_type s) :
		vptr_type(a, static_cast<std::size_t>(s) < a.size() ? &a[s] : NULL),
		vidx_type(a, s)
	{
	}
	valptridx_template_t(index_type s) :
		vptr_type(get_array(), static_cast<std::size_t>(s) < get_array().size() ? &get_array()[s] : NULL),
		vidx_type(get_array(), s)
	{
	}
	template <
		template <typename, typename> class EP0,
		template <typename, typename EI, template <EI> class> class EI0,
		template <typename, typename> class EP1,
		template <typename, typename EI, template <EI> class> class EI1,
		typename EP>
	/* Reuse Prc to enforce is_same<remove_const<EP>::type,
	 * remove_const<P>::type>.
	 */
	bool operator==(const valptridx_template_t<EP0, EI0, EP1, EI1, EP, I, magic_constant, Prc> &rhs) const
	{
		typedef valptridx_template_t<EP0, EI0, EP1, EI1, EP, I, magic_constant, Prc> rhs_t;
		return vptr_type::operator==(rhs.operator const typename rhs_t::vptr_type &()) &&
			vidx_type::operator==(rhs.operator const typename rhs_t::vidx_type &());
	}
	template <typename R>
		bool operator!=(const R &rhs) const
		{
			return !(*this == rhs);
		}
};

/* Out of line since gcc chokes on template + inline + attribute */
template <typename P, typename I>
template <typename A>
typename valbaseptridxutil_t<P, I>::index_type valbaseptridxutil_t<P, I>::check_index_match(const A &a, pointer_type p, index_type s)
{
	return DXX_VALPTRIDX_CHECK(&a[s] == p, "pointer/index mismatch", s, index_mismatch_exception);
}

template <typename P, typename I>
template <typename A>
typename valbaseptridxutil_t<P, I>::index_type valbaseptridxutil_t<P, I>::check_index_range(const A &a, index_type s)
{
	return DXX_VALPTRIDX_CHECK(static_cast<std::size_t>(s) < a.size(), "invalid index used in array subscript", s, index_range_exception);
}

template <typename P, typename I>
class vvalptr_t : public valptr_t<P, I>
{
	typedef valptr_t<P, I> base_t;
	using base_t::check_null_pointer;
	typedef typename base_t::Prc Prc;
public:
	typedef typename base_t::pointer_type pointer_type;
	typedef typename base_t::reference reference;
	operator reference() const { return *static_cast<pointer_type>(*this); }
	vvalptr_t(std::nullptr_t) = delete;
	vvalptr_t(base_t &&) = delete;
	vvalptr_t(const base_t &t) :
		base_t(check_null_pointer(t))
	{
	}
	vvalptr_t(const vvalptr_t<Prc, I> &t) :
		base_t(t)
	{
	}
	template <typename A>
		vvalptr_t(A &a, pointer_type p) :
			base_t(a, check_null_pointer(p))
	{
	}
	vvalptr_t(pointer_type p) :
		base_t(check_null_pointer(p))
	{
	}
};

template <typename P, typename I>
valptr_t<P, I>::valptr_t(const vvalptr_t<typename tt::remove_const<P>::type, I> &t) :
	p(t.p)
{
}

template <typename P, typename I, template <I> class magic_constant>
class vvalidx_t : public validx_t<P, I, magic_constant>
{
	typedef validx_t<P, I, magic_constant> base_t;
protected:
	using base_t::get_array;
	using base_t::check_index_match;
	using base_t::check_index_range;
public:
	typedef typename base_t::index_type index_type;
	typedef typename base_t::integral_type integral_type;
	typedef typename base_t::pointer_type pointer_type;
	template <integral_type v>
		vvalidx_t(const magic_constant<v> &m) :
			base_t(check_constant_index(m))
	{
	}
	vvalidx_t(base_t &&) = delete;
	vvalidx_t(const base_t &t) :
		base_t(check_index_range(get_array(), t))
	{
	}
	template <typename A>
		vvalidx_t(A &a, index_type s) :
			base_t(check_index_range(a, s))
	{
	}
	template <typename A>
	vvalidx_t(A &a, pointer_type p, index_type s) :
		base_t(check_index_match(a, p, check_index_range(a, s)))
	{
	}
	vvalidx_t(index_type s) :
		base_t(check_index_range(get_array(), s))
	{
	}
	template <integral_type v>
		static constexpr const magic_constant<v> &check_constant_index(const magic_constant<v> &m)
		{
#ifndef constexpr
			static_assert(static_cast<std::size_t>(v) < get_array().size(), "invalid index used");
#endif
			return m;
		}
	using base_t::operator==;
	template <integral_type v>
		bool operator==(const magic_constant<v> &m) const
		{
			return base_t::operator==(check_constant_index(m));
		}
	template <typename R>
		bool operator!=(const R &rhs) const
		{
			return !(*this == rhs);
		}
};

#define _DEFINE_VALPTRIDX_SUBTYPE_USERTYPE(N,P,I,A,name,Pconst)	\
	static inline constexpr decltype(A) Pconst &get_global_array(P Pconst*) { return A; }	\
	\
	struct name : valptridx_template_t<valptr_t, validx_t, vvalptr_t, vvalidx_t, P Pconst, I, P##_magic_constant_t> {	\
		DXX_INHERIT_CONSTRUCTORS(name, valptridx_type);	\
	};	\
	\
	struct v##name : valptridx_template_t<vvalptr_t, vvalidx_t, valptr_t, validx_t, P Pconst, I, P##_magic_constant_t> {	\
		DXX_INHERIT_CONSTRUCTORS(v##name, valptridx_type);	\
	};	\
	\
	static inline name N(name) = delete;	\
	static inline v##name N(v##name) = delete;	\
	static inline v##name N(name::pointer_type o) {	\
		return {A, o};	\
	}	\
	\
	static inline v##name N(name::pointer_type o, name::index_type i) {	\
		return {A, o, i};	\
	}	\
	\
	static inline v##name operator-(P Pconst *o, decltype(A) &O)	\
	{	\
		return N(o, const_cast<const P *>(o) - &(const_cast<const decltype(A) &>(O).front()));	\
	}	\
	\
	name operator-(name, decltype(A) &) = delete;	\
	v##name operator-(v##name, decltype(A) &) = delete;	\

#define DEFINE_VALPTRIDX_SUBTYPE(N,P,I,A)	\
	_DEFINE_VALPTRIDX_SUBTYPE_USERTYPE(N,P,I,A,N##_t,);	\
	_DEFINE_VALPTRIDX_SUBTYPE_USERTYPE(N,P,I,A,c##N##_t,const);	\
	\
	static inline N##_t N(N##_t::index_type i) {	\
		return {A, i};	\
	}	\
	\
	static inline v##N##_t v##N(N##_t::index_type i) {	\
		return {A, i};	\
	}	\

