#ifndef WERKZEUG_GUARD_MEMORY_ACTIONS_HPP
#define WERKZEUG_GUARD_MEMORY_ACTIONS_HPP


#include <iostream>
#include <iomanip>

#include "werkzeug/memory/common.hpp"
#include "werkzeug/memory/concepts.hpp"

namespace werkzeug::memory::actions
{
	struct Logging
	{
		void on_owns( concepts::block auto blk, bool result ) const noexcept
		{
			std::cout << "owns request for " << blk << ". success: " << result << '\n';
		}

		void on_can_deallocate( concepts::block auto blk, bool result ) const noexcept
		{
			std::cout << "Can deallocate request for " << blk << " success: " << result << '\n';
		}

		void on_allocate( std::size_t count, std::size_t alignment, concepts::block auto result ) noexcept
		{
			std::cout << "allocate request for " << count << '|'<< alignment;
			if ( result )
			{
				std::cout << " success: " << result << '\n';
			}
			else
			{
				std::cout << " failed!\n";
			}
		}

		void on_deallocate( concepts::block auto blk, bool success ) noexcept
		{
			std::cout << "deallocate request for " << blk;
			if ( success )
			{
				std::cout << " success!\n";
			}
			else
			{
				std::cout << " failure!!\n";
			}
		}

		void on_resize( concepts::block auto blk, std::size_t new_count, concepts::block auto result ) noexcept
		{
			std::cout << "deallocate request for " << blk << " to " << new_count;
			if ( result )
			{
				std::cout << " success: " << result << '\n';
			}
			else
			{
				std::cout << " failed!\n";
			}
		}
	};

	struct Statistics
	{
	protected :
		struct Stats
		{
			using unsigned_type = std::size_t;
			using signed_type = std::make_signed_t<std::size_t>;

			unsigned_type alloc_calls = 0;
			unsigned_type resize_calls = 0;
			unsigned_type dealloc_calls = 0;
			unsigned_type alloc_size = 0;
			signed_type resize_size = 0;
			unsigned_type dealloc_size = 0;
			unsigned_type alloc_success = 0;
			unsigned_type resize_success = 0;
			unsigned_type dealloc_success = 0;

			friend std::ostream& operator << ( std::ostream& os, const Stats& stats )
			{
				constexpr auto sign = []( auto value )
				{ 
					return value >= 0 ? '+' : '-';
				};


				const auto alloc_delta = stats.delta();
				os << "---------------------  resource stats ----------------------\n";
				os << "         " << std::setw(10) << "calls" << std::setw(10) << "success" << std::setw(17) << "size" << '\n';
				os << "  alloc :" << std::setw(10) << stats.alloc_calls << std::setw(10) << stats.alloc_success << "      +" << std::setw(10) << stats.alloc_size << '\n';
				os << " resize :" << std::setw(10) << stats.resize_calls << std::setw(10) << stats.resize_success << "      " << sign(stats.resize_size) << std::setw(10) << stats.resize_size << '\n';
				os << "dealloc :" << std::setw(10) << stats.dealloc_calls << std::setw(10) << stats.dealloc_success << "      -" << std::setw(10) << stats.dealloc_size << '\n';
				os << "______________________________________________\n";
				os << "  delta :" << std::setw(27) << sign(alloc_delta) << std::setw(10) << alloc_delta << '\n';
				os << "-------------------- end resource stats --------------------\n";
				return os;
			}

			signed_type delta() const noexcept
			{
				return static_cast<signed_type>(alloc_size - dealloc_size) + resize_size;
			}
		};
		Stats stats_;
	public :

		[[nodiscard]] const Stats& stats() const noexcept
		{ return stats_; }

		void reset_stats() noexcept
		{
			stats_ = {};
		}

		void on_allocate( std::size_t count, std::size_t alignment, bool success ) noexcept
		{
			(void)alignment;
			(void)success;
			stats_.alloc_calls += 1;
			if ( success )
			{
				stats_.alloc_success += 1;
				stats_.alloc_size += count;
			}
		}

		void on_deallocate( concepts::block auto blk, bool success ) noexcept
		{
			stats_.dealloc_calls += 1;
			if ( success )
			{
				stats_.dealloc_size += blk.size;
				stats_.dealloc_success += 1;
			}
		}

		void on_resize( concepts::block auto blk, std::size_t new_count, bool result ) noexcept
		{
			stats_.resize_calls += 1;
			if ( result )
			{
				stats_.resize_size += new_count - blk.size;
				stats_.resize_success += 1;
			}
		}
	};


	template<typename T>
	concept memory_action = requires ( T t, Block_Type<> block )
	{
		t.on_allocate( std::size_t{}, std::size_t{}, block );
		t.on_deallocate( block, bool{} );
	};

	template<typename T>
	concept has_on_owns = requires ( T t, Block_Type<> block, bool result )
	{
		t.on_owns( block, result );
	};

	template<typename T>
	concept has_on_can_deallocate = requires ( T t, Block_Type<> block, bool result )
	{
		t.on_can_deallocate( block, result );
	};

	template<typename T>
	concept has_on_resize = requires ( T t, Block_Type<> block, size_t new_size, Block_Type<> result )
	{
		t.on_resize( block, new_size, result );
	};

	template<concepts::memory_source Resource, memory_action ... Actions>
	class Action_Interface : public Actions ...
	{
		Resource resource_;

	public :
		using Block = Block_Type_of<Resource>;
		static constexpr inline std::size_t default_alignment = __STDCPP_DEFAULT_NEW_ALIGNMENT__;

		Action_Interface() 
			requires std::is_default_constructible_v<Resource>
		{}

		template<typename ... Action_Init>
			requires ( std::same_as<std::remove_cvref_t<Action_Init>,Actions> && ... )
		Action_Interface( Resource resource, Action_Init&& ... actions )
			: resource_( std::forward<Resource>( resource) ), Actions{ std::forward<Action_Init>(actions) } ...
		{}

		[[nodiscard]] bool owns( Block blk ) const noexcept
			requires concepts::has_owns<Resource>
		{
			constexpr auto expression = []( const auto& action, const Block b ){
				if constexpr ( has_on_owns<decltype(action)> )
				{
					action.on_owns( b );
				}
			};

			const auto b = resource_.owns(blk);

			( expression( static_cast<const Actions&>(*this), blk, b ), ... );
			return b;
		}

		[[nodiscard]] bool can_deallocate( Block blk ) const noexcept
			requires concepts::has_can_deallocate<Resource>
		{
			constexpr auto expression = []( const auto& action, const Block b ){
				if constexpr ( has_on_can_deallocate<decltype(action)> )
				{
					action.on_can_dellocate( b );
				}
			};

			const auto b = resource_.can_deallocate(blk);

			( expression( static_cast<const Actions&>(*this), block_cast<std::byte*>(blk), b ), ... );

			return b;
		}

		[[nodiscard]] Block allocate( std::size_t count, std::size_t alignment ) noexcept
		{
			const auto b = resource_.allocate(count, alignment);

			( static_cast<Actions&>(*this).on_allocate( count, alignment, b ), ... );
			
			return b;
		}

		[[nodiscard]] bool deallocate( Block blk, std::size_t alignment = default_alignment ) noexcept
		{
			const auto b = resource_.deallocate(blk, alignment );

			( static_cast<Actions&>(*this).on_deallocate( block_cast<std::byte*>(blk), b ), ... );

			return b;
		}

		[[nodiscard]] Block resize( Block blk, std::size_t new_count ) noexcept
			requires concepts::has_resize<Resource>
		{
			constexpr auto expression = []( const auto& action, const Block b, std::size_t new_count_, bool success ){
				if constexpr ( has_on_resize<decltype(action)> )
				{
					action.on_resize( b, new_count_, success );
				}
			};

			const auto b = resource_.resize( blk, new_count );

			( expression( static_cast<const Actions&>(*this), block_cast<std::byte*>(blk), b, new_count, b ), ... );

			return b;
		}
	};

	template<typename Resource, typename ... Actions>
	Action_Interface( Resource, Actions ... ) -> Action_Interface<Resource,Actions...>;
}

#endif