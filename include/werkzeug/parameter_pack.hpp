#ifndef WERKZEUG_GUARD_PARAMETER_PACK_HPP
#define WERKZEUG_GUARD_PARAMETER_PACK_HPP

#include <algorithm>
#include <array>
#include <concepts>
#include <tuple>
#include <type_traits>

#include "werkzeug/traits.hpp"

namespace werkzeug
{
	/**
	 * @brief type that contains a list of types
	 *
	 * @tparam ...Ts the types
	 */
	template <typename... Ts>
	struct type_pack
	{
		/** @brief number of elements in this pack */
		constexpr static std::size_t size = sizeof...( Ts );

		/** @brief the type at index I */
		template <std::size_t I>
		using type_at = typename std::tuple_element<I, std::tuple<Ts...>>::type;

		using first = type_at<0>;
		using last = type_at<size - 1>;

		/** @brief gets the index of first occurence of type 'T' */
		template <typename T>
		consteval static std::size_t index_of_first() noexcept
		{
			constexpr bool found[size] = { std::same_as<T, Ts>... };
			for ( std::size_t i = 0; i < size; ++i )
			{
				if ( found[i] )
				{
					return i;
				}
			}
			return size;
		}

		/** @brief gets the index of first occurence of type 'T' or 'size' otherwise */
		template <typename T>
		consteval static std::size_t unique_index_of() noexcept
		{
			constexpr bool found[size] = { std::same_as<T, Ts>... };
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

		/** @brief checks whether the pack contains the type 'T' */
		template <typename T>
		consteval static bool contains() noexcept
		{
			return ( std::same_as<T, Ts> || ... );
		}

		/** @brief Applies the type transformer 'Transformer' to every element and returns a new pack */
		template <template <typename> class Transformer>
		using transform = type_pack<typename Transformer<Ts>::type... >;

		/** @brief Invokes 'f' with a 'std::type_identity<T>' for every type in the pack */
		template <typename F>
		constexpr static void invoke_for_each( F&& f )
		{
			( f( std::type_identity<Ts> {} ), ... );
		}

		/** @brief A tuple type for the types in the pack */
		using as_tuple = std::tuple<Ts...>;
	};

	namespace detail
	{
		template <auto A, auto B>
			requires std::same_as<decltype( A ), decltype( B )>
		constexpr static bool same()
		{
			return A == B;
		}

		template <auto A, auto B>
		constexpr static bool same()
		{
			return false;
		}

		template <typename T>
		constexpr T& cast_if_bool( T& t ) noexcept
		{
			return t;
		}

		constexpr unsigned char cast_if_bool( bool b ) noexcept { return static_cast<unsigned char>( b ); }
	} // namespace detail

	/**
	 * @brief A pack of values
	 *
	 * @tparam ...Vs the values the pack holds
	 */
	template <auto... Vs>
	struct value_pack
	{
		/** @brief number of elements in this pack */
		constexpr static std::size_t size = sizeof...( Vs );

		/** @brief gets the value at index I */
		template <std::size_t I>
		constexpr static auto value_at = std::get<I>( std::tuple { Vs... } );

		/** @brief gets the index of first occurence of value 'V' */
		template <auto V>
		consteval static std::size_t index_of_first() noexcept
		{
			constexpr bool found[size] = { ( Vs == V )... };
			for ( std::size_t i = 0; i < size; ++i )
			{
				if ( found[i] )
				{
					return i;
				}
			}
			return size;
		}

		/** @brief gets the index of first occurence of value 'V' or 'size' otherwise */
		template <auto V>
		constexpr static std::size_t unique_index_of() noexcept
		{
			constexpr bool found[size] = { detail::same<V, Vs>()... };
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

		/** @brief checks whether the pack contains the value 'V' */
		template <auto V>
		consteval static bool contains() noexcept
		{
			return ( ( V == detail::cast_if_bool( Vs ) ) || ... );
		}

		/** @brief Applies the transformer 'Transformer' to every element and returns a new pack */
		template <typename Transformer>
		using transform = value_pack<Transformer {}( Vs )...>;

		/** @brief Invokes 'f' with a 'std::integral_constant<decltype(V),V>' for every value 'V' in the pack */
		template <typename F>
		constexpr static void invoke_for_each( F&& f )
		{
			( f( std::integral_constant<decltype( Vs ), Vs> {} ), ... );
		}

		/** @brief gets the 'type_pack' of the values */
		using type_pack_type = type_pack<decltype( Vs )...>;

		/** @brief a tuple of the values in the pack */
		constexpr static std::tuple as_tuple = { Vs... };
	};

	/** @brief type traits to check whether a type is a 'type_pack' */
	template <typename>
	struct is_type_pack : std::false_type
	{
	};

	template <typename... Ts>
	struct is_type_pack<type_pack<Ts...>> : std::true_type
	{
	};

	/** @brief concept to check whether a type is a parameter pack, i.e. 'type_pack' */
	template <typename P>
	concept type_pack_c = is_type_pack<P>::value;

	/** @brief type traits to check whether a type is a 'value_pack' */
	template <typename>
	struct is_value_pack : std::false_type
	{
	};

	template <auto... Vs>
	struct is_value_pack<value_pack<Vs...>> : std::true_type
	{
	};

	/** @brief concept to check whether a type is a parameter pack, i.e. 'value_pack' */
	template <typename P>
	concept value_pack_c = is_value_pack<P>::value;

	/** @brief concept to check whether a type is a parameter pack, i.e. 'type_pack' or 'value_pack' */
	template <typename P>
	concept parameter_pack = type_pack_c<P> or value_pack_c<P>;

	/** @brief type trait to check whether a type is a parameter pack, i.e. 'type_pack' or 'value_pack' */
	template <typename P>
	struct is_parameter_pack : std::conditional_t<parameter_pack<P>, std::true_type, std::false_type>
	{
	};

	/** @brief concept to check whether two types are packs and are of the same pack kind */
	template <typename P1, typename P2>
	concept same_pack_kind = (type_pack_c<P1> and type_pack_c<P2>) or ( value_pack_c<P1> and value_pack_c<P2> );


	/**
	 * @brief type trait to get the nth type from a pack. Works on both C++ parameter packs and specializations of 'type_pack'
	 *
	 * @tparam index The index requested
	 * @tparam Pack... the pack to index. May be either a C++ parameter pack, or an specialization of 'type_pack'
	 */
	template <std::size_t index, typename... Pack>
	struct type_at_index;

	template <typename T0, typename... Ts>
	struct type_at_index<0, T0, Ts...> : std::type_identity<T0>
	{
	};

	template <std::size_t N, typename T0, typename... Ts>
		requires( N <= sizeof...( Ts ) )
	struct type_at_index<N, T0, Ts...> : std::type_identity<typename type_at_index<N - 1, Ts...>::type>
	{
	};

	template < std::size_t N, typename... Ts>
	struct type_at_index<N, type_pack<Ts...>> : type_at_index<N, Ts...>
	{
	};

	template <std::size_t N, typename... Ts>
	using type_at_index_t = typename type_at_index<N, Ts...>::type;


	template <parameter_pack Pack, std::size_t Start, size_t End = Pack::size>
		requires( Start < End and End <= Pack::size )
	struct sub_pack;

	template <size_t Start, size_t End, typename... Ts>
	struct sub_pack<type_pack<Ts...>, Start, End> : std::type_identity<decltype( []<size_t... Is>( std::index_sequence<Is...> ) -> type_pack<typename type_pack<Ts...>::template type_at<Is + Start>...> { }( std::make_index_sequence<End - Start> {} ) )>
	{
	};

	template <size_t Start, size_t End, auto... Vs>
	struct sub_pack<value_pack<Vs...>, Start, End> : std::type_identity<decltype( []<size_t... Is>( std::index_sequence<Is...> ) -> value_pack<value_pack<Vs...>::template value_at<Is + Start>...> { }( std::make_index_sequence<End - Start> {} ) )>
	{
	};

	template <parameter_pack Pack, std::size_t Start, size_t End = Pack::size>
	using sub_pack_t = typename sub_pack<Pack, Start, End>::type;

	/**
	 * @brief Type trait to check if all types in a pack are unique. Requires at least two types
	 *
	 * @tparam T0 first type
	 * @tparam T1 second type
	 * @tparam Ts... All the other types
	 */
	template <typename... Ts>
	struct all_unique : std::false_type
	{
	};

	template <>
	struct all_unique<> : std::true_type
	{
	};

	template <typename T>
	struct all_unique<T> : std::true_type
	{
	};

	template <typename T1, typename... Ts>
	struct all_unique<T1, Ts...> : std::conditional_t< ( not std::is_same_v<T1, Ts> && ... ) and all_unique<Ts...>::value, std::true_type, std::false_type >
	{
	};

	template <typename... Ts>
	struct all_unique<type_pack<Ts...>> : all_unique<Ts...>
	{
	};

	/** @brief Helper to check if all elements in a C++ or type_pack are unique */
	template <typename... Ts>
	constexpr bool all_unique_v = all_unique<Ts...>::value;


	/** @brief type trait to check whether a pack includes another pack */
	template <parameter_pack P1, parameter_pack P2>
		requires same_pack_kind<P1, P2>
	struct pack_includes;

	template <parameter_pack P1, typename... Ts>
	struct pack_includes<P1, type_pack<Ts...>> : std::conditional_t< ( P1::template contains<Ts>() && ... ), std::true_type, std::false_type >
	{
	};

	template <parameter_pack P1, auto... Vs>
	struct pack_includes<P1, value_pack<Vs...>> : std::conditional_t< ( P1::template contains<Vs>() && ... ), std::true_type, std::false_type >
	{
	};

	/** @brief Helper to check if one parmeter pack includes another */
	template <parameter_pack P1, parameter_pack P2>
	constexpr static bool pack_includes_v = pack_includes<P1, P2>::value;

	/** @brief type trait to check whether a type is in a pack. Works with both C++ parameter packs and parameter pack types from this library*/
	template <typename T0, typename... Ts>
	struct is_type_in_pack : std::conditional_t< type_pack<Ts...>::template contains<T0>(), std::true_type, std::false_type >
	{
	};

	template <typename T0, type_pack_c P>
	struct is_type_in_pack<T0, P> : std::conditional_t< P::template contains<T0>(), std::true_type, std::false_type >
	{
	};

	/** @brief Helper check whether a type is in a pack. Works with both C++ parameter packs and parameter pack types from this library */
	template <typename T0, typename... Ts>
	constexpr bool is_type_in_pack_v = is_type_in_pack<T0, Ts...>::value;


	/** @brief type trait to append a type to a type_pack. Works with both C++ parameter packs and parameter pack types from this library */
	template <parameter_pack P1, typename... Ts>
	struct append;

	template <typename... Ts1, typename... Ts>
	struct append<type_pack<Ts1...>, Ts...> : std::type_identity<type_pack<Ts1..., Ts...>>
	{
	};

	template <typename... Ts1, typename T>
		requires( not type_pack_c<T> )
	struct append<type_pack<Ts1...>, T> : std::type_identity<type_pack<Ts1..., T>>
	{
	};

	template <typename... Ts1, typename... Ts2>
	struct append<type_pack<Ts1...>, type_pack<Ts2...>> : std::type_identity<type_pack<Ts1..., Ts2...>>
	{
	};

	template <auto... Vs1, auto... Vs2>
	struct append<value_pack<Vs1...>, value_pack<Vs2...>> : std::type_identity<value_pack<Vs1..., Vs2...>>
	{
	};


	/** @brief type trait to append a type to a type_pack, iff the type is not already in the pack. Works with both C++ parameter packs and parameter pack types from this library */
	template <type_pack_c P1, typename... Ts>
	struct append_if_unique;

	template <parameter_pack P1>
	struct append_if_unique<P1> : std::type_identity<P1>
	{
	};

	template <typename... Ts1, typename T0, typename... Ts2>
		requires( not type_pack_c<T0> )
	struct append_if_unique<type_pack<Ts1...>, T0, Ts2...> : std::conditional< is_type_in_pack<T0, Ts1...>::value, typename append_if_unique<type_pack<Ts1...>, Ts2...>::type, typename append_if_unique<type_pack<Ts1..., T0>, Ts2...>::type >
	{
	};

	template <parameter_pack P1, typename... Ts2>
	struct append_if_unique<P1, type_pack<Ts2...>> : append_if_unique<P1, Ts2...>
	{
	};

	/** @brief type trait to reduce a parmeter pack to only unique types. Works with both C++ parameter packs and parameter pack types from this library */
	template <typename... Ts>
	struct reduce_to_unique;

	template <>
	struct reduce_to_unique<> : std::type_identity<type_pack<>>
	{
	};

	template <typename T>
	struct reduce_to_unique<T> : std::type_identity<type_pack<T>>
	{
	};

	template <typename T, typename... Ts>
	struct reduce_to_unique<T, Ts...> : append_if_unique<typename reduce_to_unique<Ts...>::type, T>
	{
	};

	template <typename... Ts>
	struct reduce_to_unique<type_pack<Ts...>> : reduce_to_unique<Ts...>
	{
	};

	template <typename... Ts>
	using reduce_to_unique_t = typename reduce_to_unique<Ts...>::type;

	namespace detail
	{
		template <template <typename> class Value_Transform, typename... Ts>
		struct sort_pack_impl
		{
			using input = type_pack<Ts...>;

			template <typename>
			struct Impl;

			template <std::size_t... Is>
			struct Impl<std::index_sequence<Is...>>
			{
				using O = std::common_type_t<decltype( Value_Transform<Ts>::value )...>;

				struct element
				{
					O o;
					std::size_t i;
				};

				constexpr static auto sorted_indices = []()
				{
					std::array a {
						element { Value_Transform<Ts>::value, Is }
                         ...
                    };

					std::sort( std::begin( a ), std::end( a ), []( const element& l, const element& r ) { return l.o < r.o; } );

					return std::array { a[Is].i... };
				}();

				using type = type_pack< typename input::template type_at<sorted_indices[Is]>... >;
			};

			using type = typename Impl<std::make_index_sequence<sizeof...( Ts )>>::type;
		};

		template <template <typename> class Ordering>
		struct sort_pack_impl<Ordering>
		{
			using type = type_pack<>;
		};
	} // namespace detail

	/** @brief type trait that sorts a parameter pack by the values produced via 'Value_Transform<Ts>' */
	template <template <typename> class Value_Transform, typename... Ts>
	struct sort_pack : detail::sort_pack_impl<Value_Transform, Ts...>
	{
	};

	template <template <typename> class Value_Transform, typename... Ts>
	struct sort_pack<Value_Transform, type_pack<Ts...>> : detail::sort_pack_impl<Value_Transform, Ts...>
	{
	};

	/** @brief The default transform for type->value used to sort type_packs */
	template <typename T>
	struct default_value_transform
	{
		constexpr static std::string_view value = type_traits<T>::name;
	};

	/** @brief type trait that sorts a parameter pack by 'default_value_transform' */
	template <typename... Ts>
	struct normalized : detail::sort_pack_impl<default_value_transform, Ts...>
	{
	};

	template <typename... Ts>
	struct normalized<type_pack<Ts...>> : detail::sort_pack_impl<default_value_transform, Ts...>
	{
	};

	template <typename... Ts>
	using normalized_t = typename normalized<Ts...>::type;

	/**
	 * @brief Invokes 'f' for every element in the pack
	 *
	 * @tparam Value_Pack A pack of values
	 * @param f function to invoke for every value
	 */
	template <value_pack_c Value_Pack, typename F>
	void static_for( F&& f )
	{
		Value_Pack::invoke_for_each( std::forward<F>( f ) );
	}
} // namespace werkzeug

#endif