#ifndef WERKZEUG_GUARD_PARAMETER_PACK_HPP
#define WERKZEUG_GUARD_PARAMETER_PACK_HPP

#include <type_traits>
#include <concepts>
#include <tuple>
#include <algorithm>
#include <array>

#include "werkzeug/traits.hpp"

namespace werkzeug
{
	template<typename ... >
	struct type_pack;

	template<typename ... Ts>
	struct type_pack
	{ 
		constexpr static std::size_t size = sizeof...(Ts);

		template<std::size_t I>
		using type_at = typename std::tuple_element<I, std::tuple<Ts...>>::type;

		template<typename T>
		consteval static std::size_t index_of_first() noexcept
		{
			constexpr bool found[size] = { std::same_as<T,Ts> ... };
			for ( std::size_t i = 0; i < size; ++i )
			{
				if ( found[i] )
				{
					return i;
				}
			}
			return size;
		}

		template<typename T>
		consteval static std::size_t unique_index_of() noexcept
		{
			constexpr bool found[size] = { std::same_as<T,Ts> ... };
			std::size_t last = size;
			for ( std::size_t i = 0; i < size; ++i )
			{
				if ( found[i] )
				{
					if ( last < size )
					{
						return size;
					}
					last = i;
				}
			}
			return last;
		}

		template<typename T>
		consteval static bool contains() noexcept
		{
			return ( std::same_as<T,Ts> || ... );
		}

		template<template<typename> class Transformer>
		using transform = type_pack<typename Transformer<Ts>::type ... >;

		using as_tuple = std::tuple<Ts...>;
	};

	namespace detail
	{
		template<auto A, auto B>
			requires std::same_as<decltype(A),decltype(B)>
		constexpr static bool same()
		{ return A==B; }

		template<auto A, auto B>
		constexpr static bool same()
		{ return false; }

		template<typename T>
		constexpr T& cast_if_bool( T& t ) noexcept
		{
			return t;
		}

		constexpr unsigned char cast_if_bool( bool b ) noexcept
		{
			return static_cast<unsigned char>( b );
		}

	}

	template<auto ... Vs>
	struct value_pack
	{ 
		constexpr static std::size_t size = sizeof...(Vs);

		template<std::size_t I>
		constexpr static auto value_at = std::get<I>( std::tuple{ Vs... } );

		template<auto V>
		constexpr static std::size_t index_of() noexcept
		{
			constexpr bool found[size] = { detail::same<V,Vs>() ... };
			std::size_t last = size;
			for ( std::size_t i = 0; i < size; ++i )
			{
				if ( found[i] )
				{
					if ( last < size )
					{
						return size;
					}
					last = i;
				}
			}
			return last;
		}

		template<auto V>
		consteval static bool contains () noexcept
		{
			return ( ( V == detail::cast_if_bool(Vs) ) || ... );
		}

		template<typename Transformer>
		using transform = value_pack<Transformer{}( Vs ) ...>;

		using type_pack_type = type_pack<decltype(Vs)...>;

		constexpr static std::tuple as_tuple = { Vs ... };
	};

	template<typename>
	struct is_parameter_pack
		: std::false_type
	{ };

	template<typename>
	struct is_type_pack
		: std::false_type
	{ }; 

	template<typename ... Ts>
	struct is_type_pack<type_pack<Ts...>>
		: std::true_type
	{ };

	template<typename P>
	concept type_pack_c = is_type_pack<P>::value;

	template<typename>
	struct is_value_pack
		: std::false_type
	{ };

	template<auto ... Vs>
	struct is_value_pack<value_pack<Vs...>>
		: std::true_type
	{ };

	template<typename P>
	concept value_pack_c = is_value_pack<P>::value;

	template<typename P>
	concept parameter_pack = type_pack_c<P> or value_pack_c<P>;

	template<typename P1, typename P2>
	concept same_pack_kind = ( type_pack_c<P1> and type_pack_c<P2> ) or ( value_pack_c<P1> and value_pack_c<P2> );


	/**
	* @brief type trait to get the nth type from a pack
	* 
	* @tparam index The index requested
	* @tparam Pack... the pack to index
	*/
	template <std::size_t index, typename... Pack>
	struct type_at_index;

	template <typename T0, typename... Ts>
	struct type_at_index<0, T0, Ts...>
		: std::type_identity<T0>
	{ };

	template <std::size_t N, typename T0, typename... Ts>
		requires ( N <= sizeof...(Ts) )
	struct type_at_index<N, T0, Ts...>
		: std::type_identity<typename type_at_index<N - 1, Ts...>::type>
	{ };

	template< std::size_t N, typename ... Ts>
	struct type_at_index<N,type_pack<Ts...>>
		: type_at_index<N, Ts...>
	{ };

	template<std::size_t N, typename ... Ts>
	using type_at_index_t = typename type_at_index<N, Ts ...>::type;


	/**
	* @brief Type trait to check if all types in a pack are unique. Requires at least two types
	* 
	* @tparam T0 first type
	* @tparam T1 second type
	* @tparam Ts... All the other types
	*/
	template<typename ... Ts>
	struct all_unique
		: std::false_type
	{ };

	template<>
	struct all_unique<>
		: std::true_type
	{ };

	template<typename T>
	struct all_unique<T>
		: std::true_type
	{ };

	template<typename T1, typename ... Ts>
	struct all_unique<T1,Ts...>
		: std::conditional_t<
			( not std::is_same_v<T1,Ts> && ... ) and all_unique<Ts...>::value,
			std::true_type,
			std::false_type
			>
	{ };

	template<typename ... Ts>
	struct all_unique<type_pack<Ts...>>
		: all_unique<Ts...>
	{ };

	template<typename ... Ts>
	constexpr bool all_unique_v = all_unique<Ts...>::value;



	template<parameter_pack P1, parameter_pack P2>
		requires same_pack_kind<P1,P2>
	struct pack_includes;

	template<parameter_pack P1, typename ... Ts>
	struct pack_includes<P1, type_pack<Ts...>>
		: std::conditional_t<
			( P1::template contains<Ts>() && ... ),
			std::true_type,
			std::false_type
		>
	{ };

	template<parameter_pack P1, auto ... Vs>
	struct pack_includes<P1, value_pack<Vs...>>
		: std::conditional_t<
			( P1::template contains<Vs>() && ... ),
			std::true_type,
			std::false_type
		>
	{ };

	template<parameter_pack P1, parameter_pack P2>
	constexpr static bool pack_includes_v = pack_includes<P1,P2>::value;


	template<typename T0, typename ... Ts>
	struct is_type_in_pack
		: std::conditional_t<
			type_pack<Ts...>::template contains<T0>(),
			std::true_type,
			std::false_type
		>
	{ };

	template<typename T0, type_pack_c P>
	struct is_type_in_pack<T0,P>
		: std::conditional_t<
			P::template contains<T0>(),
			std::true_type,
			std::false_type
		>
	{ };

	template<typename T0, typename ... Ts>
	constexpr bool is_type_in_pack_v = is_type_in_pack<T0,Ts...>::value;

	template<parameter_pack P1, typename>
	struct append;

	template<typename ... Ts1, typename T>
		requires ( not type_pack_c<T> )
	struct append<type_pack<Ts1...>, T>
		: std::type_identity<type_pack<Ts1...,T>>
	{ };

	template<typename ... Ts1, typename ... Ts2>
	struct append<type_pack<Ts1...>, type_pack<Ts2...>>
		: std::type_identity<type_pack<Ts1...,Ts2...>>
	{ };

	template<auto ... Vs1, auto ... Vs2>
	struct append<value_pack<Vs1...>, value_pack<Vs2...>>
		: std::type_identity<value_pack<Vs1...,Vs2...>>
	{ };

	template<parameter_pack P1, typename ... Ts>
	struct append_if_unique;

	template<parameter_pack P1>
	struct append_if_unique<P1>
		: std::type_identity<P1>
	{ };

	template<typename ... Ts1, typename T0, typename ... Ts2>
		requires ( not type_pack_c<T0> )
	struct append_if_unique<type_pack<Ts1...>, T0, Ts2...>
		: std::conditional<
			is_type_in_pack<T0, Ts1...>::value,
			typename append_if_unique<type_pack<Ts1...>,Ts2...>::type,
			typename append_if_unique<type_pack<Ts1...,T0>,Ts2...>::type
		>
	{ };

	template<parameter_pack P1, typename ... Ts2>
	struct append_if_unique<P1, type_pack<Ts2...>>
		: append_if_unique<P1, Ts2...>
	{ };


	template<typename ... Ts>
	struct reduce_to_unique;

	template<>
	struct reduce_to_unique<>
		: std::type_identity<type_pack<>>
	{ };

	template<typename T>
	struct reduce_to_unique<T>
		: std::type_identity<type_pack<T>>
	{ };

	template<typename T, typename ... Ts>
	struct reduce_to_unique<T,Ts...>
		: append_if_unique<typename reduce_to_unique<Ts...>::type,T>
	{ };

	template<typename ... Ts>
	struct reduce_to_unique<type_pack<Ts...>>
		: reduce_to_unique<Ts...>
	{ };


	template<typename ... Ts>
	using reduce_to_unique_t = typename reduce_to_unique<Ts...>::type;



	namespace detail
	{
		template<template<typename> class Ordering, typename ... Ts>
		struct sort_pack_impl
		{
			using input = type_pack<Ts...>;

			template<typename>
			struct Impl;

			template<std::size_t ... Is>
			struct Impl<std::index_sequence<Is...>>
			{
				using O = std::common_type_t<decltype(Ordering<Ts>::value)...>;

				struct element
				{ 
					O o;
					std::size_t i;
				};
				constexpr static auto sorted_indices = [](){
					std::array a{ element{ Ordering<Ts>::value, Is } ... };

					std::sort( std::begin(a), std::end(a), [](const element& l, const element& r){ return l.o < r.o; } );

					return std::array{ a[Is].i ... };
				}();

				using type = type_pack< typename input::template type_at<sorted_indices[Is]> ... >;
			};

			using type = typename Impl<std::make_index_sequence<sizeof...(Ts)>>::type;
		};

		template<template<typename> class Ordering>
		struct sort_pack_impl<Ordering>
		{
			using type = type_pack<>;
		};

		template<typename T>
		struct default_ordering
		{
			constexpr static std::string_view value = type_traits<T>::name;
		};
	}

	template<typename ... Ts>
	struct normalized;

	template<typename ... Ts>
	struct normalized
		: detail::sort_pack_impl<detail::default_ordering,Ts...>
	{ };

	template<typename ... Ts>
	struct normalized<type_pack<Ts...>>
		: detail::sort_pack_impl<detail::default_ordering,Ts...>
	{ };

	template<typename ... Ts>
	using normalized_t = typename normalized<Ts...>::type;


	namespace detail
	{
		template<value_pack_c Value_Pack, std::size_t ... Indices, typename F>
		void static_for_impl( std::integer_sequence<std::size_t, Indices...>, F&& f )
		{
			( f( std::integral_constant< decltype(Value_Pack::template value_at<Indices>), Value_Pack::template value_at<Indices> >{} ) , ... );
		}
	}

	/**
	 * @brief Invokes 'f' for every element in the pack
	 * 
	 * @tparam Value_Pack A pack of values
	 * @param f function to invoke for every value
	 */
	template<value_pack_c Value_Pack, typename F>
	void static_for( F&& f )
	{
		return detail::static_for_impl<Value_Pack>( std::make_index_sequence<Value_Pack::size>{}, std::forward<F>(f) );
	}
}

#endif