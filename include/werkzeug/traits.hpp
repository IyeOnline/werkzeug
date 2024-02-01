#ifndef WERKZEUG_GUARD_TRAITS_HPP
#define WERKZEUG_GUARD_TRAITS_HPP

#include <iostream>
#include <type_traits>

#include "werkzeug/concepts.hpp"

namespace werkzeug
{
	namespace detail
	{
		template<bool Possible, bool Nothrow, bool Trivial = false>
		struct trait_base
		{
			constexpr static bool possible = Possible;
			constexpr static bool nothrow = Nothrow;
			constexpr static bool trivial = Trivial;
		};

		template<typename T>
		using default_construct_trait = trait_base< std::is_default_constructible_v<T>, std::is_nothrow_default_constructible_v<T>, std::is_trivially_default_constructible_v<T>>;
		template<typename T>
		using copy_construct_trait = trait_base< std::is_copy_constructible_v<T>, std::is_nothrow_copy_constructible_v<T>, std::is_trivially_copy_constructible_v<T>>;
		template<typename T>
		using move_construct_trait = trait_base< std::is_move_constructible_v<T>, std::is_nothrow_move_constructible_v<T>, std::is_trivially_move_constructible_v<T>>;
		template<typename T>
		using copy_assign_trait = trait_base< std::is_copy_assignable_v<T>, std::is_nothrow_copy_assignable_v<T>, std::is_trivially_copy_assignable_v<T>>;
		template<typename T>
		using move_assign_trait = trait_base< std::is_move_assignable_v<T>, std::is_nothrow_move_assignable_v<T>, std::is_trivially_move_assignable_v<T>>;
		template<typename T>
		using destroy_trait = trait_base< std::is_destructible_v<T>, std::is_nothrow_destructible_v<T>, std::is_trivially_destructible_v<T>>;

		template<typename T, typename ... Args>
		using construct_from_trait = trait_base< std::is_constructible_v<T,Args...>, std::is_nothrow_constructible_v<T,Args...>, std::is_constructible_v<T, Args...> and std::is_trivial_v<T>>;

		template<typename T>
		using equality_compare_trait = trait_base< equality_comparable<T>, nothrow_equality_comparable<T>>;
		template<typename T>
		using three_way_compare_trait = trait_base< std::three_way_comparable<T>, nothrow_three_way_comarable<T>>;

		template<typename T>
		using swap_trait = trait_base< swapable<T>, nothrow_swapable<T>, std::is_trivially_copy_constructible_v<T>>;
		template<typename T>
		using hash_trait = trait_base< hashable<T>, nothrow_hashable<T>>;

		template<typename Callable, typename ... Args>
		using invocable_trait = trait_base<std::is_invocable_v<Callable, Args...>, std::is_nothrow_invocable_v<Callable, Args...>>;

		template<typename T>
		using pre_increment_trait = trait_base<pre_increment<T>, nothrow_pre_increment<T>>;
		template<typename T>
		using pre_decrement_trait = trait_base<pre_decrement<T>, nothrow_pre_decrement<T>>;
		template<typename T>
		using post_increment_trait = trait_base<post_increment<T>, nothrow_post_increment<T>>;
		template<typename T>
		using post_decrement_trait = trait_base<post_decrement<T>, nothrow_post_decrement<T>>;

		template<typename T, typename U=T>
		using compound_add_trait = trait_base<compound_add<T, U>, nothrow_compound_add<T, U>>;
		template<typename T, typename U=T>
		using compound_sub_trait = trait_base<compound_sub<T, U>, nothrow_compound_sub<T, U>>;
		template<typename T, typename U=T>
		using compound_mul_trait = trait_base<compound_mul<T, U>, nothrow_compound_mul<T, U>>;
		template<typename T, typename U=T>
		using compound_div_trait = trait_base<compound_div<T, U>, nothrow_compound_div<T, U>>;

		template<typename T, typename U=T>
		using add_trait = trait_base<add<T, U>, nothrow_add<T, U>>;
		template<typename T, typename U=T>
		using sub_trait = trait_base<sub<T, U>, nothrow_sub<T, U>>;
		template<typename T, typename U=T>
		using mul_trait = trait_base<mul<T, U>, nothrow_mul<T, U>>;
		template<typename T, typename U=T>
		using div_trait = trait_base<div<T, U>, nothrow_div<T, U>>;

		template<typename T>
		using difference_trait = trait_base<requires( T t ){ { t-t } -> std::convertible_to<std::ptrdiff_t>; }, false, requires( T t ){ { t-t } noexcept; }>;

		template<typename T>
		using deref_trait = trait_base<deref<T>, nothrow_deref<T>>;
		template<typename T>
		using subscript_trait = trait_base<subscript<T>,nothrow_subscript<T>>;


		template<typename T>
		consteval static std::string_view get_name()
		{
	#if defined _WIN32
			constexpr std::string_view s = __FUNCTION__;
			const auto begin_search = s.find_first_of("<");
			const auto space = s.find(' ', begin_search);
			const auto begin_type = space != s.npos ? space+1 : begin_search+1;
			const auto end_type = s.find_last_of(">");
			return s.substr(begin_type, end_type-begin_type);
	#elif defined __GNUC__
			constexpr std::string_view s = __PRETTY_FUNCTION__;
			constexpr std::string_view t_equals = "T = ";
			const auto begin_type = s.find(t_equals) + t_equals.size();
			const auto end_type = s.find_first_of(";]", begin_type);
			return s.substr(begin_type, end_type - begin_type);
	#endif
		}
	}

	template<typename T>
	struct type_traits
	{
		constexpr static std::string_view name = detail::get_name<T>();
		constexpr static size_t size = sizeof(T);
		constexpr static size_t alignment = alignof(T);


		using default_construct = detail::default_construct_trait<T>;
		using copy_construct = detail::copy_construct_trait<T>;
		using move_construct = detail::move_construct_trait<T>;
		using copy_assign = detail::copy_assign_trait<T>;
		using move_assign = detail::move_assign_trait<T>;
		using destroy = detail::destroy_trait<T>;

		template<typename ... Args>
		using construct_from = detail::construct_from_trait<T,Args...>;

		using equality_compare = detail::equality_compare_trait<T>;
		using three_way_compare = detail::three_way_compare_trait<T>;

		using swap = detail::swap_trait<T>;
		using hash = detail::hash_trait<T>;

		template<typename ... Args>
		using invocable = detail::invocable_trait<T,Args...>;

		using pre_increment = detail::pre_increment_trait<T>;
		using pre_decrement = detail::pre_decrement_trait<T>;
		using post_increment = detail::post_increment_trait<T>;
		using post_decrement = detail::post_decrement_trait<T>;

		template<typename U=T>
		using compound_add = detail::compound_add_trait<T,U>;
		template<typename U=T>
		using compound_sub = detail::compound_sub_trait<T,U>;
		template<typename U=T>
		using compound_mul = detail::compound_mul_trait<T,U>;
		template<typename U=T>
		using compound_div = detail::compound_div_trait<T,U>;

		template<typename U=T>
		using add = detail::add_trait<T,U>;
		template<typename U=T>
		using sub = detail::sub_trait<T,U>;
		template<typename U=T>
		using mul = detail::mul_trait<T,U>;
		template<typename U=T>
		using div = detail::div_trait<T,U>;

		using deref = detail::deref_trait<T>;
		using subscript = detail::subscript_trait<T>;
	};


	template<range R>
	struct range_traits
	{
		using value_type = std::remove_cvref_t<decltype(*std::ranges::begin(std::declval<R>()))>;
	};


	template<typename Fct, typename Base, typename ... Children>
	struct common_invoke_result : std::common_type<std::invoke_result_t<Fct,Children>...>
	{ };

	template<typename Fct, typename Base, typename ... Children>
	concept has_common_invoke_result = requires
	{
		typename common_invoke_result<Fct, Base, Children ...>::type;
	};

	template<typename Fct, typename Base, typename ... Children>
	struct common_result_type_or_void
	{
		using type = typename std::conditional_t<
			has_common_invoke_result<Fct,Base,Children...>,
				common_invoke_result<Fct,Base,Children...>,
				std::type_identity<void>
			>::type;
	};
}

#endif