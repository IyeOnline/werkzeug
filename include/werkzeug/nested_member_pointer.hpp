#ifndef WERKZEUG_GUARD_NESTED_MEMBER_POINTER_HPP
#define WERKZEUG_GUARD_NESTED_MEMBER_POINTER_HPP

#include <tuple>
#include <concepts>
#include <type_traits>
#include <utility>

#include "werkzeug/parameter_pack.hpp"

namespace werkzeug
{
	namespace detail::nested_member_pointer
	{
		// https://en.cppreference.com/w/cpp/utility/forward_like
		template<class T, class U>
		[[nodiscard]] constexpr auto&& forward_like(U&& x) noexcept
		{
			constexpr bool is_adding_const = std::is_const_v<std::remove_reference_t<T>>;
			if constexpr (std::is_lvalue_reference_v<T&&>)
			{
				if constexpr (is_adding_const)
					return std::as_const(x);
				else
					return static_cast<U&>(x);
			}
			else
			{
				if constexpr (is_adding_const)
					return std::move(std::as_const(x));
				else
					return std::move(x);
			}
		}

		template<typename T, size_t I, size_t ... Is>
		decltype(auto) deref_impl( T&& obj, const auto& ptr, std::index_sequence<I, Is...> )
		{
			auto&& deref_one = obj.*std::get<I>(ptr.ptrs);

			if constexpr ( sizeof...(Is) > 0 )
			{
				return deref_impl( forward_like<T>( deref_one ), ptr, std::index_sequence<Is...>{} );
			}
			else
			{
				return forward_like<T>( deref_one );
			}
		}

		template<typename>
		struct member_pointer_traits;

		template<typename M, typename C>
		struct member_pointer_traits<M C::*>
		{
			using class_type = C;
			using member_type = M;
		};

		template<typename ... Ptrs>
		struct is_valid_pointer_chain;
		
		template<typename Ptr1, typename Ptr2, typename ... Rest>
		struct is_valid_pointer_chain<Ptr1,Ptr2,Rest...>
			: std::conditional_t<
				std::convertible_to< 
					typename member_pointer_traits<Ptr1>::member_type, 
					typename member_pointer_traits<Ptr2>::class_type 
				>,
				is_valid_pointer_chain<Ptr2,Rest...>,
				std::false_type
			>
		{ };

		template<typename Ptr>
		struct is_valid_pointer_chain<Ptr>
			: std::true_type
		{ };

		template<>
		struct is_valid_pointer_chain<>
			: std::false_type
		{ };
	}

	template<typename ... Ptrs>
		requires ( detail::nested_member_pointer::is_valid_pointer_chain<Ptrs...>::value )
	struct nested_member_pointer
	{
		std::tuple<Ptrs...> ptrs;

		using pointer_type_pack = type_pack<Ptrs...>;
		using class_type = typename detail::nested_member_pointer::member_pointer_traits<typename pointer_type_pack::first>::class_type;
		using value_type = typename detail::nested_member_pointer::member_pointer_traits<typename pointer_type_pack::last>::member_type;

		explicit(sizeof...(Ptrs) == 1) nested_member_pointer( Ptrs ... ptrs_ )
			: ptrs{ ptrs_ ... }
		{ }

		template<typename T>
			requires ( std::convertible_to<std::remove_cvref_t<T>&,class_type&> )
		[[nodiscard]] decltype(auto) operator()( T&& obj) const noexcept
		{
			return detail::nested_member_pointer::deref_impl( std::forward<T>(obj), *this, std::make_index_sequence<sizeof...(Ptrs)>{} );
		}
	};

	template<typename T, typename ... Ptrs>
	[[nodiscard]] decltype(auto) operator->*( T&& obj, const nested_member_pointer<Ptrs...>& ptr ) noexcept
	{
		return ptr( std::forward<T>(obj) );
	}

	template<typename T, typename ... Ptrs>
	[[nodiscard]] auto& operator->*( T* obj, const nested_member_pointer<Ptrs...>& ptr ) noexcept
	{
		return ptr( std::forward<T>(*obj) );
	}
}

#endif