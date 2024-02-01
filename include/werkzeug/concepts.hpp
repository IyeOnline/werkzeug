#ifndef WERKZEUG_GUARD_CONCEPTS_HPP
#define WERKZEUG_GUARD_CONCEPTS_HPP

#include <iterator>
#include <concepts>
#include <limits>
#include <type_traits>

namespace werkzeug
{
	template<typename>
	concept always = true;

	template<typename T>
	concept not_void = ( not std::is_same_v<T,void> );

	template<typename T>
	concept nothrow_three_way_comarable = std::three_way_comparable<T> and requires (T t)
	{
		{ t <=> t } noexcept;
	};

	template<typename T>
	concept reference = std::is_reference_v<T>;


	#define WERKZEUG_SINGLE_ARG(...) __VA_ARGS__
	#define WERKZEUG_MAKE_CONCEPT(NAME, TEMPLATE, IDENTIFIERS, EXPRESSION, VALUE_CONSTRAINT )\
	template<TEMPLATE>\
	concept NAME = requires IDENTIFIERS\
	{\
		{ EXPRESSION } -> VALUE_CONSTRAINT;\
	};\
	template<TEMPLATE>\
	concept nothrow_##NAME = requires IDENTIFIERS\
	{\
		{ EXPRESSION } noexcept;\
	}

	WERKZEUG_MAKE_CONCEPT(hashable,typename T, (T t), std::hash<T>{t}, std::convertible_to<std::size_t> );
	WERKZEUG_MAKE_CONCEPT(swapable,typename T, (T t), (std::swap(t,t)), always );
	WERKZEUG_MAKE_CONCEPT(equality_comparable, WERKZEUG_SINGLE_ARG(typename T, typename U=T), (T t, U u), (t == u), std::same_as<bool> );
	WERKZEUG_MAKE_CONCEPT(streamable, typename T, ( std::ostream& os, T t), ( os << t), always );
	WERKZEUG_MAKE_CONCEPT(range_streamable, typename T, ( std::ostream& os, T t), ( os << *std::begin(t) ), always );

	WERKZEUG_MAKE_CONCEPT(pre_increment,typename T, (T t), (++t), always);
	WERKZEUG_MAKE_CONCEPT(post_increment,typename T, (T t), (t++), always);
	WERKZEUG_MAKE_CONCEPT(pre_decrement,typename T, (T t), (--t), always);
	WERKZEUG_MAKE_CONCEPT(post_decrement,typename T, (T t), (t--), always);

	WERKZEUG_MAKE_CONCEPT(compound_add,WERKZEUG_SINGLE_ARG(typename T, typename U), (T t, U u), (t += u), always);
	WERKZEUG_MAKE_CONCEPT(compound_sub,WERKZEUG_SINGLE_ARG(typename T, typename U), (T t, U u), (t -= u), always);
	WERKZEUG_MAKE_CONCEPT(compound_mul,WERKZEUG_SINGLE_ARG(typename T, typename U), (T t, U u), (t *= u), always);
	WERKZEUG_MAKE_CONCEPT(compound_div,WERKZEUG_SINGLE_ARG(typename T, typename U), (T t, U u), (t /= u), always);
	
	WERKZEUG_MAKE_CONCEPT(add,WERKZEUG_SINGLE_ARG(typename T, typename U), (T t, U u), (t + u), not_void );
	WERKZEUG_MAKE_CONCEPT(sub,WERKZEUG_SINGLE_ARG(typename T, typename U), (T t, U u), (t - u), not_void );
	WERKZEUG_MAKE_CONCEPT(mul,WERKZEUG_SINGLE_ARG(typename T, typename U), (T t, U u), (t * u), not_void );
	WERKZEUG_MAKE_CONCEPT(div,WERKZEUG_SINGLE_ARG(typename T, typename U), (T t, U u), (t / u), not_void );

	WERKZEUG_MAKE_CONCEPT(deref,typename T, (T t), *t, reference );
	WERKZEUG_MAKE_CONCEPT(subscript,WERKZEUG_SINGLE_ARG(typename T, typename U=std::size_t), (T t, U u), t[u], reference );


	template<typename T>
	concept fundamental = std::integral<T> or std::floating_point<T>;

	template<typename T, typename U>
	concept lossless_fundamental_conversion = 
		fundamental<T>  
		and ( std::integral<T> == std::integral<U> )
		and ( std::numeric_limits<T>::lowest() <= std::numeric_limits<U>::lowest() )
		and ( std::numeric_limits<U>::max() >= std::numeric_limits<U>::max() );

	template<typename T, typename U>
	concept lossless_conversion_from = 
		lossless_fundamental_conversion<T,U> 
		or ( not fundamental<T> and std::assignable_from<T&,U> );

	template<typename It>
	concept forward_iterator = requires (It it )
	{
		{ *it } -> not_void;
		{ ++it } -> std::convertible_to<It>;
	};

	template<typename It,typename T>
	concept forward_iterator_of_type = requires (It it )
	{
		{ *it } -> std::convertible_to<const T&>;
		{ ++it } -> std::convertible_to<It>;
	};

	template<typename It, typename T>
	concept output_iterator_of_type = forward_iterator_of_type<It,T> && requires ( It it, T t )
	{
		*it = t;
	};

	template<typename EndIt, typename It>
	concept end_iterator_for_iterator = requires ( EndIt e, It it )
	{
		{ it != e } -> std::same_as<bool>;
	};

	template<typename ItBegin, typename ItEnd>
	concept has_distance = requires ( ItBegin begin, ItEnd end )
	{
		{ end - begin } -> std::integral;
	};

	template<typename It>
	concept random_access_iterator = requires ( It it )
	{
		{ it + 1 } -> std::same_as<It>;
		{ it - 1 } -> std::same_as<It>;
	};

	template<typename R,typename T>
	concept range_of_type = requires ( R r )
	{
		{ std::begin(r) } -> forward_iterator_of_type<T>;
		{ std::end(r) } -> end_iterator_for_iterator<decltype(std::begin(r))> ;
	};

	template<typename R>
	concept range = requires ( R r )
	{
		{ std::begin(r) } -> forward_iterator;
		{ std::end(r) } -> end_iterator_for_iterator<decltype(std::begin(r))> ;
	};

	template<typename T>
	concept iterable = requires (T t)
	{
		{ t.begin() } -> not_void;
		{ t.end() } -> not_void;
		t.begin() != t.end();
	};

	template<typename T>
	concept reverse_iterable = requires (T t)
	{
		{ t.rbegin() } -> not_void;
		{ t.rend() } -> not_void;
		t.rbegin() != t.rend();
	};

	namespace detail
	{
		template<typename>
		struct is_signature
			: std::false_type
		{ };

		template<typename R, typename ... Args>
		struct is_signature<R(Args...)>
			: std::true_type
		{ };
	}

	/** @brief tests whether T is of the form R(Args...)*/
	template<typename T>
	concept signature = detail::is_signature<T>::value;

	
	/** @brief concept to require that type 'T' inherits from 'Base' and itself overload the member function 'ptr'. Functions from derived classes will not be accepted
	*
	* @tparam Derived type to check
	* @tparam Base Base type
	* @tparam ptr member function pointer
	*/
	template<typename Derived, typename Base, auto ptr>
	concept derived_overloads = 
		std::is_member_function_pointer_v<decltype(ptr)> and
		(
			std::same_as<Derived,Base> or 
			(
				std::derived_from<Derived,Base> and
				not requires ( Base& obj )
				{
					(obj.*ptr)();
				}
			)
		);

}

#endif