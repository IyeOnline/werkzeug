#ifndef WERKZEUG_GUARD_MEMORY_COMMON_HPP
#define WERKZEUG_GUARD_MEMORY_COMMON_HPP

#include <cassert>
#include <concepts>

#include "werkzeug/assertion.hpp"
#include "werkzeug/concepts.hpp"
#include "werkzeug/memory/concepts.hpp"

namespace werkzeug::memory
{
	template<typename PtrT = std::byte*>
		requires std::is_pointer_v<PtrT>
	struct Block_Type
	{
		using pointer = PtrT;
		using value_type = std::remove_pointer_t<PtrT>;
		PtrT ptr = nullptr;
		std::size_t size = 0;

		auto begin() noexcept
		{ return ptr; }
		auto begin() const noexcept
		{ return ptr; }

		auto end() noexcept
		{ return ptr + size; }
		auto end() const noexcept
		{ return ptr + size; }

		operator bool () const noexcept
		{ return ptr != nullptr; }

		[[nodiscard]] decltype(auto) operator [] ( const std::ptrdiff_t index ) noexcept
			requires ( not std::same_as<value_type,void> )
		{ 
			WERKZEUG_ASSERT( index < size, "index must be in range" );
			return ptr[index]; 
		}


		[[nodiscard]] decltype(auto) operator [] ( const std::ptrdiff_t index ) const noexcept
			requires ( not std::same_as<value_type,void> )
		{ 
			WERKZEUG_ASSERT( index < size, "index must be in range" );
			return ptr[index];
		}

		[[nodiscard]] friend bool operator == ( const Block_Type& lhs, const Block_Type& rhs ) noexcept = default;

		friend std::ostream& operator<<( std::ostream& os, const Block_Type& block ) 
		{
			const auto flags = os.flags();

			os << std::showbase;
			os << "{ " << block.ptr << ", " << std::hex << block.size << " }";
			
			os.flags( flags );

			return os;
		}
	};

	template<typename newPtrT,typename oldPtrT>
		requires std::is_pointer_v<newPtrT>
	[[nodiscard]] Block_Type<newPtrT> block_cast( Block_Type<oldPtrT> blk ) noexcept
	{
		constexpr auto old_size = sizeof(std::remove_pointer_t<oldPtrT>);
		constexpr auto new_size = sizeof(std::remove_pointer_t<newPtrT>);
		return { reinterpret_cast<newPtrT>(blk.ptr), blk.size * old_size / new_size };
	}

	
	template<typename newBlockType,typename oldPtrT>
		requires concepts::block<newBlockType>
	[[nodiscard]] newBlockType block_cast( Block_Type<oldPtrT> blk ) noexcept
	{
		return block_cast<decltype(newBlockType::ptr)>( blk );
	}

	template<std::size_t Alignment>
	static std::size_t round_to_align( const std::size_t input )
	{
		return input + input % Alignment;
	}
}

#define WERKZEUG_GUARD_PRINT_MEMORY_BLOCK(blk) std::cout << #blk ": [" << blk.ptr << " - " << blk.ptr+blk.size << "], size= " << blk.size << '\n'

#endif