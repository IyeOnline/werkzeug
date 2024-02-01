#ifndef WERKZEUG_GUARD_ALGORITHM_RANGES_HPP
#define WERKZEUG_GUARD_ALGORITHM_RANGES_HPP


#include <concepts>
#include <ranges>
#include <utility>
#include <functional>


namespace werkzeug
{
	namespace detail
	{
		template<std::ranges::forward_range R, typename M, typename ... Ms >
		auto flatten_nested_by_transform_impl( R r, M m, Ms ... members )
		{
			if constexpr ( sizeof...(Ms) > 0 )
			{
				return flatten_nested_by_transform_impl( r | std::views::transform( m ) | std::views::join, members ... );
			}
			else // is "last" member pointer
			{
				using transform_result_t = decltype( std::invoke( std::declval<M>(), std::declval<std::ranges::range_value_t<R>&>() ) );
				if constexpr ( std::ranges::range<transform_result_t> ) // if its a range itself
				{
					return r | std::views::transform( m ) | std::views::join;
				}
				else // if its just a simple member transform
				{
					return r | std::views::transform( m );
				}
			}
		}
	}

	/**
	 * @brief Iterates over range r applying the transforms `trafos...`. If the trafo results in a range itself, it is flattened. Only the mast trafo may result in a non-range type,
	 * 
	 * @param r range to iterate
	 * @param trafos... transformations to apply. Only the last transformation may result in a non-range type
	 * @return range object representing the transform flattened range
	 */
	template<std::ranges::forward_range R, typename ... Trafos >
	auto flatten_nested_by_transform( R&& r, Trafos ... trafos )
	{
		return flatten_nested_by_transform_impl( std::forward<R>(r) | std::views::all , trafos... );
	}
}

#endif