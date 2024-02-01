#ifndef WERKZEUG_GUARD_MEMORY_INTERFACES_HPP
#define WERKZEUG_GUARD_MEMORY_INTERFACES_HPP


#include <iostream>
#include <iomanip>

#include "werkzeug/memory/common.hpp"
#include "werkzeug/memory/concepts.hpp"

namespace werkzeug::memory::interfaces
{
	template<concepts::memory_source Primary, concepts::memory_source Secondary>
		requires concepts::has_owns<Primary>
	class Fallback
	{
		using primary_ptr_t = Pointer_Type_of<Block_Type_of<Primary>>;
		using secondary_ptr_t = Pointer_Type_of<Block_Type_of<Secondary>>;
		using ptr_t = std::common_type_t<primary_ptr_t, secondary_ptr_t>;

		[[no_unique_address]] Primary primary_;
		[[no_unique_address]] Secondary secondary_;
	public :
		using Block = Block_Type<ptr_t>;

		Fallback() 
			requires std::is_default_constructible_v<Primary> && std::is_default_constructible_v<Secondary> 
		= default;

		Fallback( Primary p )
			requires std::is_default_constructible_v<Secondary>
			: primary_{ std::forward<Primary>(p) }
		{ }


		Fallback( Primary p , Secondary s )
			: primary_{ std::forward<Primary>(p) }, secondary_{ std::forward<Secondary>(s) }
		{ }

		[[nodiscard]] Primary& primary() noexcept
		{
			return primary_;
		}

		[[nodiscard]] Secondary& secondary() noexcept
		{
			return secondary_;
		}

		[[nodiscard]] Block allocate( std::size_t count ) noexcept
		{
			auto blk = primary_.allocate(count);
			if ( blk.ptr != nullptr )
			{	
				return block_cast<ptr_t>( blk );
			}
			return block_cast<ptr_t>( secondary_.allocate(count) );
		}

		[[nodiscard]] bool deallocate( Block blk ) noexcept
		{
			const auto b = block_cast<primary_ptr_t>( blk );
			if ( primary_.owns(b) )
			{
				return primary_.deallocate(b);
			}
			return secondary_.deallocate( block_cast<secondary_ptr_t>(blk) );
		}

		[[nodiscard]] bool can_deallocate( Block blk ) noexcept
			requires concepts::has_can_deallocate<Primary> && concepts::has_can_deallocate<Fallback>
		{
			return primary_.can_deallocate( blk ) || secondary_.can_deallocate( blk );
		}

		[[nodiscard]] Block resize( Block blk, std::size_t new_count ) noexcept
			requires concepts::has_resize<Primary> or concepts::has_resize<Secondary>
		{
			const auto b = block_cast<primary_ptr_t>( blk );
			if ( primary_.owns(b) )
			{
				if constexpr ( concepts::has_resize<Primary> )
				{
					return primary_.resize(b, new_count);
				}
				else
				{
					return Block{};
				}
			}
			if constexpr ( concepts::has_resize<Secondary> )
			{
				return secondary_.resize( block_cast<secondary_ptr_t>(blk), new_count );
			}
			return Block{};
		}

		[[nodiscard]] bool owns( Block blk ) const noexcept
			requires concepts::has_owns<Fallback>
		{
			return primary_.owns( block_cast<primary_ptr_t>(blk) ) || secondary_.owns( block_cast<secondary_ptr_t>(blk) );
		}
	};

	template<std::size_t Split, concepts::memory_source Larger, concepts::memory_source Smaller>
	class Segregator
	{
		using large_ptr_t = decltype(Larger::Block::ptr);
		using small_ptr_t = decltype(Smaller::Block::ptr);
		using ptr_t = std::common_type_t<void*,large_ptr_t, small_ptr_t>;

		[[no_unique_address]] Larger large;
		[[no_unique_address]] Smaller small;

	public :
		using Block = Block_Type<ptr_t>;

		Segregator() 
			requires std::is_default_constructible_v<Larger> && std::is_default_constructible_v<Smaller>
		{ }

		Segregator( Larger larger, Smaller smaller ) 
			: large( larger ), small(smaller )
		{ }

		[[nodiscard]] bool owns( Block blk ) const noexcept
			requires concepts::has_owns<Larger> and concepts::has_owns<Smaller>
		{
			if ( blk.size < Split )
			{
				return small.owns( block_cast<small_ptr_t>(blk) );
			}
			else
			{
				return large.owns( block_cast<large_ptr_t>(blk) );
			}
		}

		[[nodiscard]] bool can_deallocate( Block blk ) const noexcept
			requires concepts::has_can_deallocate<Larger> and concepts::has_can_deallocate<Smaller>
		{
			if ( blk.size < Split )
			{
				return small.can_deallocate( block_cast<small_ptr_t>(blk) );
			}
			else
			{
				return large.can_deallocate( block_cast<large_ptr_t>(blk) );
			}
		}

		[[nodiscard]] Block allocate( std::size_t count ) noexcept
		{
			if ( count< Split )
			{
				return block_cast<ptr_t>( small.allocate( count ) );
			}
			else
			{
				return block_cast<ptr_t>( large.allocate( count ) );
			}
		}

		[[nodiscard]] bool deallocate( Block blk ) noexcept
		{
			if ( blk.size < Split )
			{
				return small.deallocte( block_cast<small_ptr_t>(blk) );
			}
			else
			{
				return large.deallocate( block_cast<large_ptr_t>(blk) );
			}
		}

		[[nodiscard]] Block resize( Block blk, std::size_t new_count ) noexcept
			requires concepts::has_resize<Larger> and concepts::has_resize<Smaller>
		{
			if ( blk.size < Split and new_count < Split )
			{
				return  block_cast<ptr_t>( small.resize( block_cast<small_ptr_t>(blk), new_count ) );
			}
			else
			{
				return block_cast<ptr_t>( large.resize( block_cast<large_ptr_t>(blk), new_count ) );
			}
		}
	};
}

#endif