#ifndef WERKZEUG_GUARD_MEMORY_CONCEPTS_HPP
#define WERKZEUG_GUARD_MEMORY_CONCEPTS_HPP

#include <concepts>
#include <memory>
#include <type_traits>

#include "werkzeug/concepts.hpp"
#include "werkzeug/traits.hpp"

namespace werkzeug::memory
{
	namespace concepts
	{
		template<typename Block>
		concept block = requires ( Block blk )
		{
			{ blk.ptr } -> std::convertible_to<void*>;
			{ blk.size } -> std::convertible_to<std::size_t>;
		};
	}

	template<typename Resource>
	using Block_Type_of = typename std::remove_reference_t<Resource>::Block;

	namespace concepts
	{
		template<typename T>
		concept memory_source = 
			block<Block_Type_of<T>>
			and requires ( T r, std::size_t size, Block_Type_of<T> blk, std::size_t alignment )
		{
			{ blk } -> block;
			{ r.allocate( size, alignment ) } -> std::same_as<Block_Type_of<T>>;
			{ r.deallocate( blk ) } -> std::same_as<bool>;
		};
	}

	namespace detail
	{
		template<typename T>
		struct Pointer_Type_of_impl;

		template<concepts::block Block>
		struct Pointer_Type_of_impl<Block>
			: std::type_identity<decltype(Block::ptr)>
		{ };
		template<concepts::memory_source Resource>
		struct Pointer_Type_of_impl<Resource>
			: std::type_identity<typename Pointer_Type_of_impl<Block_Type_of<Resource>>::type >
		{ };
	}
	

	template<typename T>
	using Pointer_Type_of = typename detail::Pointer_Type_of_impl<T>::type;

	namespace concepts
	{
		template<typename T>
		concept has_nothrow_resize = requires (T t, Block_Type_of<T> b)
		{
			{ t.resize(b, 1 ) } noexcept;
		};

		template<typename T>
		concept has_owns = requires ( T r, Block_Type_of<T> blk )
		{
			{ r.owns( blk ) } -> std::same_as<bool>;
		};

		template<typename T>
		concept has_resize = memory_source<T> && requires ( T alloc, std::size_t size, Block_Type_of<T> blk )
		{
			{ alloc.resize( blk, size ) } -> std::same_as< Block_Type_of<T>>;
		};

		template<typename T>
		concept has_can_deallocate = memory_source<T> && requires ( T alloc,  Block_Type_of<T>& blk )
		{
			{ alloc.can_deallocate( blk ) } -> std::same_as<bool>;
		};

		template<typename Block, typename T>
		concept typed_block = requires ( Block blk )
		{
			{ blk.ptr } -> std::convertible_to<T*>;
		};

		WERKZEUG_MAKE_CONCEPT(allocate, typename R, (R r, std::size_t i), r.allocate(i), concepts::block );
		WERKZEUG_MAKE_CONCEPT(deallocate, typename R, (R r, Block_Type_of<R> b), r.deallocate(b), std::convertible_to<bool> );
		WERKZEUG_MAKE_CONCEPT(owns, typename R, (R r, Block_Type_of<R> b), r.owns(b), std::convertible_to<bool> );
		WERKZEUG_MAKE_CONCEPT(resize, typename R, (R r, Block_Type_of<R> b), r.resize(b), std::same_as<Block_Type_of<R>> );
		WERKZEUG_MAKE_CONCEPT(can_deallocate, typename R, (R r, Block_Type_of<R> b), r.can_deallocate(b), std::convertible_to<bool> );
	}

	namespace detail
	{
		template<typename R>
		using allocate_trait = ::werkzeug::detail::trait_base<concepts::allocate<R>,concepts::nothrow_allocate<R>>;
		template<typename R>
		using deallocate_trait = ::werkzeug::detail::trait_base<concepts::allocate<R>,concepts::nothrow_allocate<R>>;
		template<typename R>
		using owns_trait = ::werkzeug::detail::trait_base<concepts::owns<R>,concepts::nothrow_owns<R>>;
		template<typename R>
		using resize_trait = ::werkzeug::detail::trait_base<concepts::resize<R>,concepts::nothrow_resize<R>>;
		template<typename R>
		using can_deallocate_trait = ::werkzeug::detail::trait_base<concepts::can_deallocate<R>,concepts::nothrow_can_deallocate<R>>;
	}

	template<typename R>
	struct Resource_Traits
	{
		using allocate = typename detail::allocate_trait<R>;
		using deallocate = typename detail::deallocate_trait<R>;
		using owns = typename detail::owns_trait<R>;
		using resize = typename detail::resize_trait<R>;
		using can_deallocate = typename detail::can_deallocate_trait<R>;
	};
}

#endif