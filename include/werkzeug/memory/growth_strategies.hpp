#ifndef WERKZEUG_GUARD_MEMORY_GROWTH_STRATEGIES_HPP
#define WERKZEUG_GUARD_MEMORY_GROWTH_STRATEGIES_HPP

#include <algorithm>
#include <concepts>

namespace werkzeug::memory::growth_strategy
{
	template<typename S>
	concept strategy = requires ( std::size_t current_size )
	{
		{ S::grow(current_size) } -> std::same_as<std::size_t>;
	};
	

	template<std::size_t split, strategy strat_low, strategy strat_high>
	struct segregator
	{
		constexpr static std::size_t grow( const std::size_t current_size ) noexcept
		{
			if ( current_size > split )
			{
				return strat_high::grow(current_size);
			}
			else
			{
				return strat_low::grow(current_size);
			}
		}
	};

	template<auto Num, auto Den>
	struct ratio
	{
		constexpr static auto Numerator = Num;
		constexpr static auto Denominator = Den;

		constexpr static auto multiply( auto value ) noexcept
		{
			return ( Num * value ) / Den;
		}
	};

	template<ratio factor>
	struct multiplicative
	{
		constexpr static std::size_t grow( const std::size_t current_size ) noexcept
		{
			return static_cast<size_t>( factor.multiply( current_size ) );
		}
	};

	template<std::size_t min, strategy S>
	struct minimum
	{
		constexpr static std::size_t grow( const std::size_t current_size ) noexcept
		{
			return std::max( min, S::grow(current_size) );
		}
	};

	using default_strategy = minimum<1,
		segregator< 100,
			multiplicative<ratio<2, 1>{}>,
			multiplicative<ratio<3, 2>{}>
		>>;
};

#endif