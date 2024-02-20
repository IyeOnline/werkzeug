#ifndef WERKZEUG_GUARD_ALGORITHM_CONSTEXPR_MATH_HPP
#define WERKZEUG_GUARD_ALGORITHM_CONSTEXPR_MATH_HPP

#include <limits>

namespace werkzeug::math
{
	/**
	 * @brief Raises a value to a power
	 * 
	 * @tparam Exponent The exponent
	 * @param value the value to be raised
	 * 
	 * @return v^Exponent
	 */
	template<int Exponent, typename T>
	[[nodiscard]] constexpr inline T pow( const T value ) noexcept
	{
		T result{ 1 };

		constexpr auto exp_abs = Exponent > 0 ? Exponent : -Exponent;
		for ( int i=0; i < exp_abs; ++i )
		{
			result *= value;
		}

		if ( Exponent < 0 )
		{
			return T{1}/result;
		}

		return result;
	}

	/**
	 * @brief Raises a value to a power
	 * 
	 * @param value the value to be raised
	 * @param Exponent The exponent
	 * 
	 * @return v^Exponent
	 */
	template<typename T>
	[[nodiscard]] constexpr inline T pow( const T value, const int Exponent ) noexcept
	{
		T result{ 1 };

		const auto exp_abs = Exponent > 0 ? Exponent : -Exponent;
		for ( int i=0; i < exp_abs; ++i )
		{
			result *= value;
		}

		if ( Exponent < 0 )
		{
			return T{1}/result;
		}

		return result;
	}

	/**
	 * @brief safe abs function
	 * 
	 * @param value 
	 * @return -value if value < 0; limits::max if value == limits::lowest
	 */
	template<typename T>
	[[nodiscard]] constexpr T abs( const T value ) noexcept
	{
		if ( value >= T{0} )
		{
			return value;
		}
		else
		{
			if constexpr ( std::is_integral_v<T> && std::is_signed_v<T> )
			{
				if ( value == std::numeric_limits<T>::lowest() )			
				{
					return std::numeric_limits<T>::max();
				}
			}
			return -value;
		}
	}

	/** @brief gets the sign of val */
	template <typename T>
	[[nodiscard]] constexpr int signum( const T val ) noexcept
	{
		static_assert( std::is_arithmetic_v<T>, "Pretty please");
		return val > 0 ? 1 : (val < 0 ? -1 : 0);
	}

	/** @brief calculates the numerical derivative of F via 1st order central difference */
	template<typename T, typename F>
	[[nodiscard]] T derivative_at( F&& f, T x ) noexcept
	{
		const auto h = std::cbrt( std::numeric_limits<T>::epsilon() );
		const auto fxl = f(x-h);
		const auto fxr = f(x+h);

		return ( fxr - fxl ) / (2 * h );
	}

}

#endif