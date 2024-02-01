#ifndef WERKZEUG_GUARD_MEMORY_ALLOCATOR_HPP
#define WERKZEUG_GUARD_MEMORY_ALLOCATOR_HPP

#include "werkzeug/memory/common.hpp"
#include "werkzeug/memory/concepts.hpp"
#include "werkzeug/memory/resource.hpp"

namespace werkzeug::memory
{
	/**
	 * @brief The allocator implementation. Returns typed memory from a raw memory resource.
	 * 
	 * @tparam T the type to allocate
	 * @tparam R a resource to querry. This is held *by value* to allow for stateless reouces to be optimized out
	 */
	template<typename T, concepts::memory_source R>
	class Allocator
	{
		[[no_unique_address]] R r;
		using ptr_t = T*;
		using r_ptr_t = decltype(Block_Type_of<R>::ptr);

	public :
		constexpr static std::size_t alignment = alignof(T);

		constexpr static bool is_nothrow_alloc = noexcept(r.allocate(std::size_t{},std::size_t{}));
		constexpr static bool is_nothrow_dealloc = noexcept( r.deallocate( Block_Type_of<R>{} ) );
		constexpr static bool is_nothrow_resize = concepts::has_nothrow_resize<T>;

		constexpr static bool is_nothrow = is_nothrow_alloc and is_nothrow_dealloc and is_nothrow_resize;
		
		using Block = Block_Type<T*>;

		Allocator() = default;

		explicit Allocator( R r_ )
			: r(r_)
		{ }

		Block allocate( std::size_t size ) noexcept(is_nothrow_alloc)
		{
			return block_cast<T*>( r.allocate(size *sizeof(T), alignment ) );
		}

		T* allocate_single() noexcept(is_nothrow_alloc)
		{
			return reinterpret_cast<T*>( r.allocate(sizeof(T), alignment).ptr );
		}

		bool deallocate( Block blk ) noexcept(is_nothrow_dealloc)
		{
			return r.deallocate( block_cast<r_ptr_t>(blk), alignment );
		}

		bool deallocate_single( T* const ptr ) noexcept(is_nothrow_dealloc)
		{
			return r.deallocate( Block_Type_of<R>{ reinterpret_cast<r_ptr_t>(ptr), sizeof(T) }, alignment );
		}

		Block resize( Block blk, std::size_t size ) noexcept(is_nothrow_resize)
			requires concepts::has_resize<R>
		{
			return block_cast<ptr_t>( r.resize(block_cast<r_ptr_t>(blk), size*sizeof(T) ) );
		}

		bool owns( Block blk ) const noexcept
			requires concepts::has_owns<R>
		{
			return r.owns( block_cast<ptr_t>(blk) );
		}

		bool can_deallocate( Block blk ) const noexcept
			requires concepts::has_can_deallocate<R>
		{
			return r.can_deallocate( block_cast<ptr_t>(blk) );
		}
	};
};

#endif