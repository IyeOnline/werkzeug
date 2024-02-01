#ifndef WERKZEUG_GUARD_MEMORY_OPERATOR_NEW_HPP
#define WERKZEUG_GUARD_MEMORY_OPERATOR_NEW_HPP

#include <iostream>
#include <cstddef>
#include <new>

namespace werkzeug::memory
{
	namespace detail
	{
		template<typename T, typename ... Args>
		constexpr inline static auto operator_new_impl = static_cast<void*(*)(Args...)>( ::operator new );

		template<typename T, typename ... Args>
			requires requires ( Args ... args )
			{
				T::operator new( args ... );
			}
		constexpr inline static auto operator_new_impl<T,Args...> = static_cast<void*(*)(Args...)>( T::operator new );

		template<typename T, typename ... Args>
		constexpr inline static auto operator_new_impl<T[],Args...> = static_cast<void*(*)(Args...)>( ::operator new[] );

		template<typename T, typename ... Args>
			requires requires ( Args ... args )
			{
				T::operator new[]( args... );
			}
		constexpr inline static auto operator_new_impl<T[],Args...> = static_cast<void*(*)(Args...)>( T::operator new[] );



		template<typename T, typename ... Args>
		constexpr inline static auto operator_delete_impl = static_cast<void(*)(Args...)>( ::operator delete );

		template<typename T, typename ... Args>
			requires requires ( Args ... args )
			{
				T::operator delete( args... );
			}
		constexpr inline static auto operator_delete_impl<T,Args...> = static_cast<void(*)(Args...)>( T::operator delete );

		template<typename T, typename ... Args>
		constexpr inline static auto operator_delete_impl<T[], Args ...> = static_cast<void(*)(Args...)>( ::operator delete[] );

		template<typename T, typename ... Args>
			requires requires ( Args ... args )
			{
				T::operator delete[]( args... );
			}
		constexpr inline static auto operator_delete_impl<T[],Args...> = static_cast<void(*)(Args...)>( T::operator delete[] );
	}


	/** @brief properly maps to `T::operator new or `::operator new` based on whether `T` has overloaded the operator */
	template<typename T, typename ... Args>
	auto operator_new_for( std::size_t count, Args ... args )
	{
		return detail::operator_new_impl<T,std::size_t, Args...>( count, args ... );
	}


	/** @brief properly maps to `T::operator delete or `::operator delete` based on whether `T` has overloaded the operator */
	template<typename T, typename ... Args>
	auto operator_delete_for( void* ptr, Args ... args )
	{
		return detail::operator_delete_impl<T,void*,Args...>( ptr, args ... );
	}
}


#endif