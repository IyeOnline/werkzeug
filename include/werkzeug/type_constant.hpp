#ifndef WERKGZEUG_GUARD_INTEGRAL_CONSTANT_HPP
#define WERKGZEUG_GUARD_INTEGRAL_CONSTANT_HPP

#include <algorithm>
#include <charconv>
#include <cstddef>
#include <string_view>
#include <type_traits>

namespace werkzeug
{
	namespace detail
	{
		template<char...digits>
		auto type_helper_for()
		{
			constexpr static char chars[] = { digits ... };
			constexpr static std::string_view v{ chars, sizeof(chars) };

			if constexpr ( v.find_first_of(".") != v.npos )
			{
				return (long double){};
			}
			else
			{
				return (size_t){};
			}
		}


		template<char ... Cs>
		consteval auto from_chars_c()
		{
			constexpr char chars[] = { Cs ... };
			

			decltype(type_helper_for<Cs...>()) result;
			
			std::from_chars( chars , chars+sizeof(chars), result );

			return result;
		}
	}

	template<typename T, T Value>
	struct type_constant
	{
		using type = type_constant;
		using value_type = T;
		constexpr static value_type value = Value;

		[[nodiscard]] consteval operator value_type() const noexcept
		{
			return value;
		}
		[[nodiscard]] consteval value_type operator()() const noexcept
		{
			return value;
		}
	};

	template<char ... Cs>
	consteval auto operator""_tc() 
	{
		return type_constant<decltype(detail::type_helper_for<Cs...>()),detail::from_chars_c<Cs...>() >{};
	}
}

#endif