#ifndef WERKZEUG_GUARD_ALGORITHM_STATIC_FOR_HPP
#define WERKZEUG_GUARD_ALGORITHM_STATIC_FOR_HPP

#include <type_traits>
#include <utility>

namespace werkzeug
{
	namespace detail
	{
		template<int Lower, int ... Indices,typename F>
		void static_for_impl( std::integer_sequence<int, Indices...>, F&& f )
		{
			( f( std::integral_constant<int,Lower+Indices>{} ) , ... );
		}
	}

	/**
	 * @brief a constexpr for that passes the callable 'F' a `std::integral_constant` to invoke. Works best with a lamba that has an 'auto' parameter. This can then be used in a template parameter or "decay"ed into a plain int
	 * 
	 * @tparam Lower Lower bound of the loop
	 * @tparam Upper Upper bound of the loop
	 * 
	 * @param f function to invoke. 
	 */
	template<int Lower, int Upper, typename F>
	void static_for( F&& f )
	{
		detail::static_for_impl<Lower>( std::make_integer_sequence<int,Upper-Lower>{}, std::forward<F>(f) );
	}
}

#endif