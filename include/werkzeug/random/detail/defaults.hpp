#ifndef WERKZEUG_GUARD_RANDOM_DEFAULTS_HPP
#define WERKZEUG_GUARD_RANDOM_DEFAULTS_HPP

#include <type_traits>
#include <limits>

namespace werkzeug::detail
{
    template<typename T>
	struct defaults
	{
		static_assert( std::is_floating_point_v<T> );
		constexpr static inline T min = T{0};
		constexpr static inline T max = T{1};
	};
}
#endif