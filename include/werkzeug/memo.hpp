#ifndef WERKZEUG_GUARD_MEMO_HPP
#define WERKZEUG_GUARD_MEMO_HPP

#include <type_traits>
#include <unordered_map>
#include <functional>

#include "werkzeug/concepts.hpp"
#include "werkzeug/algorithm/hash.hpp"
#include "werkzeug/parameter_pack.hpp"

namespace werkzeug
{
	template<typename T>
	struct function_traits;

	template<typename R, typename ... Args>
	struct function_traits<R(*)(Args...)>
	{
		using result = R;
		using arguments = type_pack<Args...>;
		using signature = R(Args...);
	};

	template<typename R, typename C, typename ... Args>
	struct function_traits<R (C::*)(Args...)>
	{
		using result = R;
		using arguments = type_pack<Args...>;
		using type = C;
		using signature = R(Args...);
	};

	template<typename R, typename C, typename ... Args>
	struct function_traits<R (C::*)(Args...) const>
	{
		using result = R;
		using arguments = type_pack<Args...>;
		using type = C;
		using signature = R(Args...);
	};

	template<typename F, typename R, typename ... Args>
	concept callable_of_signature = requires( F f, Args ... args )
	{
		{ f(args...) } -> std::same_as<R>;
	};

	template<typename Signature, template<typename, typename> class Map_Type = std::unordered_map>
	class Memoizing_Function;


	template<template<typename, typename> class Map_Type, typename R, typename ... Args>
	class Memoizing_Function<R(Args...),Map_Type>
	{
		using result_type = std::remove_reference_t<R>;
		using input_type = std::pair<std::remove_reference_t<Args>...>;

		std::function<R(Args...)> f_;
		Map_Type<input_type,result_type> mem_;
	public :
		template<callable_of_signature<R,Args...> F>
		Memoizing_Function( F f )
			: f_{ std::move(f) }
		{ }

		bool known( Args ... args ) const
		{
			input_type k = { std::forward<Args>(args) ... };
			const auto it = mem_.find( k );
			return it != mem_.end();
		}

		const result_type& operator()( Args ... args )
		{
			input_type k = { std::forward<Args>(args) ... };
			const auto it = mem_.find( k );
			if ( it != mem_.end() )
			{
				return it->second;
			}
			else
			{
				auto result = std::apply( f_, k );
				return mem_.try_emplace( k, std::move( result ) ).first->second;
			}
		}
	};

	template<typename F>
	Memoizing_Function( F ) -> Memoizing_Function<typename function_traits<F>::signature>;
}

#endif	