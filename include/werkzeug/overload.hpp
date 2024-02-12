#ifndef WERKZEUG_GUARD_OVERLOAD_HPP
#define WERKZEUG_GUARD_OVERLOAD_HPP

#include <utility>


namespace werkzeug
{
	namespace detail::_overload
	{
		template<typename Callable>
		struct callable_wrapper : Callable
		{
			using Callable::operator();	
		};

		template<typename R, typename ... Args>
		struct callable_wrapper<R(*)(Args...)>
		{
			using fct_ptr_t = R(*)(Args...);
			const fct_ptr_t ptr;

			constexpr R operator()( Args ... args ) const
			{
				return ptr( std::forward<Args>(args) ... );
			}
		};

		template<typename R, typename C, typename ... Args>
		struct callable_wrapper<R(C::*)(Args...)>
		{
			using fct_ptr_t = R(C::*)(Args...);
			const fct_ptr_t ptr;

			constexpr R operator()( C& c, Args ... args ) const
			{
				return (c.*ptr)( std::forward<Args>(args) ... );
			}
		};

		template<typename R, typename C, typename ... Args>
		struct callable_wrapper<R(C::*)(Args...) const>
		{
			using fct_ptr_t = R(C::*)(Args...) const;
			const fct_ptr_t ptr;

			constexpr R operator()( const C& c, Args ... args ) const
			{
				return (c.*ptr)( std::forward<Args>(args) ... );
			}
		};

		template<typename C, typename T>
		struct callable_wrapper<T C::*>
		{
			static_assert( sizeof(C) and false );
			using fct_ptr_t = T C::*;

			const fct_ptr_t ptr;

			constexpr decltype(auto) operator()( C& c) const noexcept
			{
				return c.*ptr;
			}

			constexpr decltype(auto) operator()( const C& c) const noexcept
			{
				return c.*ptr;
			}
		};
	}

	/**
	 * @brief provides a means to create a visitor by aggregating callables, member function pointers and data member pointers
	 */
	template<typename ... Ts>
	class overload : detail::_overload::callable_wrapper<Ts> ...
	{
	public :
		constexpr overload( Ts ... ts )
			: detail::_overload::callable_wrapper<Ts>{std::move(ts)} ...
		{}
		using detail::_overload::callable_wrapper<Ts>::operator() ...;
	};


	namespace detail
	{
		template<typename ... Args>
		struct select_overload_t
		{
			template<typename R>
			constexpr auto operator()( R(*f)(Args...) )
			{
				return f;
			}

			template<typename R, typename C>
			constexpr auto operator()( R(C::*f)(Args...) )
			{
				return f;
			}

			template<typename R, typename C>
			constexpr auto operator()( R(C::*f)(Args...) const )
			{
				return f;
			}
		};
		template<typename R, typename ... Args>
		struct select_overload_t<R(Args...)>
		{
			constexpr auto operator()( R(*f)(Args...) )
			{
				return f;
			}

			template<typename C>
			constexpr auto operator()( R(C::*f)(Args...) )
			{
				return f;
			}

			template<typename C>
			constexpr auto operator()( R(C::*f)(Args...) const )
			{
				return f;
			}
		};
	}

	/**
	 * @brief allows to select an overload with a given signature from an overload set
	 * 
	 * @tparam Args... Argument types
	 */
	template<typename ... Args>
	detail::select_overload_t<Args...> select_overload;
}

#endif