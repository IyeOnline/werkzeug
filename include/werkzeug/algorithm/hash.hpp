#ifndef WERKZEUG_GUARD_ALGORITHM_HASH_HPP
#define WERKZEUG_GUARD_ALGORITHM_HASH_HPP


#include <functional>
#include <tuple>
#include <utility>

#include "werkzeug/traits.hpp"

namespace werkzeug::hash
{
	template <class T>
	inline void combine( std::size_t& seed, const T& v ) noexcept( type_traits<T>::hash::nothrow )
	{
		std::hash<T> hasher;
		seed ^= hasher( v ) + 0x9e3779b9 + ( seed << 6 ) + ( seed >> 2 );
	}

	template <typename T1, typename T2>
	constexpr inline size_t hash( const std::pair<T1, T2>& p ) 
		noexcept( type_traits<T1>::hash::nothrow and type_traits<T2>::hash::nothrow )
	{
		size_t seed = 0;
		combine( seed, p.first );
		combine( seed, p.second );
		return seed;
	}

	namespace detail
	{
		template <size_t... Is, typename... Ts>
		size_t hash_tuple_impl( const std::tuple<Ts...>& tup, std::index_sequence< Is...> ) 
			noexcept( ( type_traits<Ts>::hash::nothrow && ... ) )
		{
			size_t seed = 0;
			( combine( seed, std::get<Is>( tup ) ), ... );
			return seed;
		}
	} // namespace detail

	template <typename... Ts>
	constexpr inline size_t hash( const std::tuple<Ts...>& p ) 
		noexcept( ( type_traits<Ts>::hash::nothrow && ... ) ) 
	{
		return detail::hash_tuple_impl( p, std::make_index_sequence<sizeof...( Ts )> {} );
	}
} // namespace werkzeug::hash

template <typename T1, typename T2>
struct std::hash<std::pair<T1, T2>>
{
	auto operator()( const std::pair<T1, T2>& p ) const 
		noexcept( ::werkzeug::type_traits<T1>::hash::nothrow and ::werkzeug::type_traits<T2>::hash::nothrow )
	{ return werkzeug::hash::hash( p ); }
};

template <typename... Ts>
struct std::hash<std::tuple<Ts...>>
{
	auto operator()( const std::tuple<Ts...>& t ) const 
		noexcept( ( ::werkzeug::type_traits<Ts>::hash::nothrow && ... ) ) 
	{ return werkzeug::hash::hash( t ); }
};

#endif