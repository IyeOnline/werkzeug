#ifndef WERKZEUG_GUARD_ALGORITHM_INVOKE_HPP
#define WERKZEUG_GUARD_ALGORITHM_INVOKE_HPP

#include <type_traits>
#include <utility>

namespace werkzeug
{
	template<typename F, typename ... Args>
	constexpr decltype(auto) invoke( F&& f, Args&& ... args )
	{
		return std::forward<F>(f)( std::forward<Args>(args) ... );
	}

	template<typename C, typename T>
	using member_data_pointer_t = T C::*;

	template<typename C, typename T>
	constexpr decltype(auto) invoke( member_data_pointer_t<std::remove_const_t<C>,T> ptr, C* obj ) noexcept
	{
		return obj->*ptr;
	}

	template<typename C, typename T>
	constexpr decltype(auto) invoke( member_data_pointer_t<std::remove_cvref_t<C>,T> ptr, C& obj ) noexcept
	{
		return obj.*ptr;
	}

	template<typename C, typename R, typename ... Args>
	using member_function_pointer_t = R(C::*)(Args...);

	template<typename C, typename R, typename ... Args>
	constexpr decltype(auto) invoke( member_function_pointer_t<C,R,Args...> ptr, C* obj, Args&& ... args )
	{
		return (obj->*ptr)( std::forward<Args>(args) ... );
	}

	template<typename C, typename R, typename ... Args>
	constexpr decltype(auto) invoke( member_function_pointer_t<C,R,Args...> ptr, C& obj, Args&& ... args )
	{
		return (obj.*ptr)( std::forward<Args>(args) ... );
	}

	namespace detail
	{
		template<typename T, bool valid>
		class constexpr_invoke_result
		{
			union 
			{
				char sentinel_ = '\0';
				T value_;
			};

		public :
			constexpr constexpr_invoke_result()
				requires ( not valid )
				= default;
			constexpr constexpr_invoke_result( T value )
				requires valid
				: value_( std::move(value) )
			{}

			constexpr ~constexpr_invoke_result()
				requires ( not valid or std::is_trivially_destructible_v<T> )
				= default;

			constexpr ~constexpr_invoke_result()
				requires ( valid and not std::is_trivially_destructible_v<T> )
			{
				value_.~T();
			}

			static constexpr bool has_value() noexcept
			{
				return valid;
			}

			constexpr T& value() noexcept
				requires valid
			{
				return value_;
			}
			
			constexpr const T& value() const noexcept
				requires valid
			{
				return value_;
			}
		};
	}

	template<class F, auto result = F{}()>
	constexpr auto try_constexpr_invoke( F )
	{ 
		return detail::constexpr_invoke_result<decltype(result),true>{ result };
	}

	constexpr auto try_constexpr_invoke(...) 
	{ 
		return detail::constexpr_invoke_result<std::false_type,false>{};
	}
}

#endif