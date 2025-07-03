// Copyright (c) 2025 Denis Mikhailov
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_SMART_PTR_WEAKABLE_UNIQUE_PTR_HPP_INCLUDED
#define BOOST_SMART_PTR_WEAKABLE_UNIQUE_PTR_HPP_INCLUDED

#include <type_traits>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/smart_ptr/intrusive_ptr.hpp>
#include <boost/core/exchange.hpp>

namespace boost {
namespace detail {

template<class T>
class weakable_unique_ptr_control_block
{
    long ref_count;
    T* px; // TODO: should be type erased with `void* px`
public:
    explicit weakable_unique_ptr_control_block( T* p ) noexcept
        : ref_count( 0 ), px( p )
    {
    }

    weakable_unique_ptr_control_block( const weakable_unique_ptr_control_block& r ) = delete;
    weakable_unique_ptr_control_block& operator=( const weakable_unique_ptr_control_block& r ) = delete;

    void reset() noexcept
    {
        px = nullptr;
    }

    T * get() const noexcept
    {
        return px;
    }

    friend void intrusive_ptr_add_ref( weakable_unique_ptr_control_block *p ) {   
        ++p->ref_count;
    }

    friend void intrusive_ptr_release( weakable_unique_ptr_control_block *p ) {
        if ( --p->ref_count == 0 ) {
            delete p;
        }
    }
};
}

template<class T>
class unique_weak_ptr;

template<class T, class Deleter = std::default_delete<T>>
class weakable_unique_ptr
{
public:

    // TODO: implement support for arrays
    static_assert( !std::is_array<T>::value, "Arrays are not supported" );

    // TODO: implement support for reference deleter
    static_assert( !std::is_reference<T>::value, "Reference as deleter is not supported" );

    typedef Deleter deleter_type;
    typedef typename boost::detail::sp_element<T>::type element_type;
    typedef element_type* pointer;

private:

    typedef typename boost::detail::weakable_unique_ptr_control_block<element_type> control_block;

    pointer px;
    Deleter pd;                       // TODO: implement EBO
    intrusive_ptr<control_block> pc;  // TODO: initialize it lazily??

    friend class unique_weak_ptr<T>;

public:

    // destructor

    ~weakable_unique_ptr() noexcept
    {
        if( px )
        {
            pd( px );
        }
        if ( pc )
        {
            ( *pc ).reset();
        }
    }

    // constructors

    constexpr weakable_unique_ptr() noexcept
        : px( 0 ), pd( ), pc( )
    {
    }

    constexpr weakable_unique_ptr( std::nullptr_t ) noexcept
        : px( 0 ), pd( ), pc( )
    {
    }

    explicit weakable_unique_ptr( pointer p )
        : px( p ), pd( ), pc( new control_block( p ) )
    {
    }

    template<class D = deleter_type>
    weakable_unique_ptr( pointer p, const D& d,
        typename boost::detail::sp_enable_if_convertible<D, Deleter>::type = boost::detail::sp_empty() )
        : px( p ), pd( d ), pc( new control_block( p ) )
    {
        boost::detail::sp_assert_convertible< D, Deleter >();
    }

    template<class Y, class D = deleter_type>
    weakable_unique_ptr( pointer p, D&& d,
        typename boost::detail::sp_enable_if_convertible<D, Deleter>::type = boost::detail::sp_empty() )
        : px( p ), pd( std::forward<D>(d) ), pc( new control_block( p ) )
    {
        boost::detail::sp_assert_convertible< D, Deleter >();
    }

    // move constructor

    weakable_unique_ptr( weakable_unique_ptr && r ) noexcept
        : px( r.px ), pd( std::move( r.pd ) ), pc( std::move( r.pc ) )
    {
        r.px = 0;
    }

    // converting move constructor

    template<class Y, class E> weakable_unique_ptr( weakable_unique_ptr<Y, E> && r,
        typename boost::detail::sp_enable_if_convertible<Y, T>::type = boost::detail::sp_empty(),
        typename boost::detail::sp_enable_if_convertible<E, Deleter>::type = boost::detail::sp_empty() ) noexcept
        : px( r.px ), pd( std::move( r.pd ) ), pc( std::move( r.pc ) )
    {
        boost::detail::sp_assert_convertible< Y, T >();
        boost::detail::sp_assert_convertible< E, Deleter >();

        r.px = 0;
    }

    // copy constructor

    weakable_unique_ptr( weakable_unique_ptr const & r ) = delete;

    // construction from unique_ptr
    // TODO: implement this

    // assignment

    weakable_unique_ptr & operator=( weakable_unique_ptr && r ) noexcept
    {
        weakable_unique_ptr( std::move( r ) ).swap( *this );
        return *this;
    }

    template<class Y, class E>
    weakable_unique_ptr & operator=( weakable_unique_ptr<Y, E> && r ) noexcept
    {
        weakable_unique_ptr( std::move( r ) ).swap( *this );
        return *this;
    }

    weakable_unique_ptr & operator=( std::nullptr_t ) noexcept
    {
        weakable_unique_ptr().swap( *this );
        return *this;
    }

    weakable_unique_ptr & operator=( weakable_unique_ptr const & r ) = delete;

    // TODO: implement assignment from unique_ptr

    // release
    pointer release() noexcept
    {
        weakable_unique_ptr tmp;
        tmp.swap( *this );
        return boost::exchange( tmp.px, nullptr );
    }

    // reset

    void reset( pointer p = pointer() ) noexcept
    {
        weakable_unique_ptr( p ).swap( *this );
    }

    // accessors

    // TODO: must be unavailable for array version
    typename boost::detail::sp_dereference< T >::type operator* () const noexcept
    {
        // TODO: pointer might have a throwing operator*
        return *px;
    }

    // TODO: must be unavailable for array version
    typename boost::detail::sp_member_access< T >::type operator-> () const noexcept
    {
        return px;
    }

    // TODO: implement operator[] for array version

    element_type * get() const noexcept
    {
        return px;
    }

    Deleter& get_deleter() noexcept
    {
        return pd;
    }

    const Deleter& get_deleter() const noexcept
    {
        return pd;
    }

    explicit operator bool () const noexcept
    {
        return px != 0;
    }

    // conversions to shared_ptr

    // TODO: implement this

    // swap

    void swap( weakable_unique_ptr & r ) noexcept
    {
        std::swap( px, r.px );
        std::swap( pd, r.pd );
        std::swap( pc, r.pc );
    }
};

template<class T>
class unique_weak_ptr
{
public:

    typedef typename boost::detail::sp_element<T>::type element_type;

private:

    typedef typename boost::detail::weakable_unique_ptr_control_block<element_type> control_block;

    intrusive_ptr<control_block> pc;

public:
    ~unique_weak_ptr()
    {
    }

    constexpr unique_weak_ptr() noexcept : pc( )
    {
    }

    unique_weak_ptr( const unique_weak_ptr& r ) noexcept: pc( r.pc )
    {
    }

    template<class Y>
    unique_weak_ptr( const unique_weak_ptr<Y>& r,
        typename boost::detail::sp_enable_if_convertible<Y, T>::type = boost::detail::sp_empty() ) noexcept
        : pc( boost::static_pointer_cast<control_block>( r.pc ) )
    {
        boost::detail::sp_assert_convertible< Y, T >();
    }

    template<class Y>
    unique_weak_ptr( const weakable_unique_ptr<Y>& r,
        typename boost::detail::sp_enable_if_convertible<Y, T>::type = boost::detail::sp_empty() ) noexcept
        : pc( boost::static_pointer_cast<control_block>( r.pc ) )
    {
        boost::detail::sp_assert_convertible< Y, T >();
    }

    unique_weak_ptr( unique_weak_ptr&& r ) noexcept : pc( std::move( r.pc ) )
    {
    }

    template<class Y>
    unique_weak_ptr( unique_weak_ptr<Y>&& r,
        typename boost::detail::sp_enable_if_convertible<Y, T>::type = boost::detail::sp_empty() ) noexcept
        : pc( boost::static_pointer_cast<control_block>( std::move( r.pc ) ) )
    {
        boost::detail::sp_assert_convertible< Y, T >();
    }

    unique_weak_ptr& operator=( const unique_weak_ptr& r ) noexcept
    {
        unique_weak_ptr( r ).swap( *this );
        return *this;
    }

    template<class Y>
    unique_weak_ptr& operator=( const unique_weak_ptr<Y>& r ) noexcept
    {
        unique_weak_ptr( r ).swap( *this );
        return *this;
    }

    template<class Y>
    unique_weak_ptr& operator=( const weakable_unique_ptr<Y>& r ) noexcept
    {
        unique_weak_ptr( r ).swap( *this );
        return *this;
    }

    unique_weak_ptr& operator=( unique_weak_ptr&& r ) noexcept
    {
        unique_weak_ptr( std::move( r ) ).swap( *this );
        return *this;
    }

    template<class Y>
    unique_weak_ptr& operator=( unique_weak_ptr<Y>&& r ) noexcept
    {
        unique_weak_ptr( std::move( r ) ).swap( *this );
        return *this;
    }

    void reset() noexcept
    {
        unique_weak_ptr().swap( *this );
    }

    void swap( unique_weak_ptr& r ) noexcept
    {
        std::swap( pc, r.pc );
    }

    bool expired() const noexcept
    {
        return !( pc && pc->get() );
    }

    element_type * try_get() const noexcept
    {
        return pc ? pc->get() : nullptr;
    }
};
}

#endif  // #ifndef BOOST_SMART_PTR_WEAKABLE_UNIQUE_PTR_HPP_INCLUDED
