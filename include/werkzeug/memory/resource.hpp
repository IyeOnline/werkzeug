#ifndef WERKZEUG_GUARD_MEMORY_RESOURCE_HPP
#define WERKZEUG_GUARD_MEMORY_RESOURCE_HPP

#include <functional>
#include <cstdint>
#include <iostream>
#include <new>
#include <sys/types.h>
#include <utility>

#include "werkzeug/assertion.hpp"
#include "werkzeug/container/detail/dll_node.hpp"
#include "werkzeug/memory/common.hpp"
#include "werkzeug/memory/concepts.hpp"
#include "werkzeug/memory/operator_new.hpp"

namespace werkzeug::memory::resource
{
	namespace fixed
	{
		class Null_Resource
		{
		public :
			using Block = Block_Type<void*>;
			Block allocate( std::size_t, std::align_val_t = {}) noexcept
			{
				return {};
			}

			bool deallocate( Block blk, std::align_val_t = {} ) noexcept
			{
				if ( blk.ptr != nullptr )
				{
					return false;
				}
				else
				{
					return true;
				}
			}

			bool owns( Block blk ) const noexcept
			{
				return blk.ptr == nullptr;
			}

			bool can_deallocate( Block blk ) const noexcept
			{
				return blk.ptr == nullptr;
			}
		};

		class New_Resource
		{
		public :
			static constexpr inline std::size_t default_alignment = __STDCPP_DEFAULT_NEW_ALIGNMENT__;
			using Block = Block_Type<std::byte*>;

			[[nodiscard]] Block allocate( const std::size_t count, const std::size_t alignment = default_alignment )
			{
				return { reinterpret_cast<std::byte*>( ::operator new[]( count, std::align_val_t{alignment} ) ), count };
			}

			[[nodiscard]] bool deallocate( Block blk, const std::size_t alignment = default_alignment  )
			{
				::operator delete[]( blk.ptr, std::align_val_t{alignment} );
				return true;
			}
		};


		template<typename T>
		class New_Resource_For
		{
		public :
			static constexpr inline std::size_t default_alignment = __STDCPP_DEFAULT_NEW_ALIGNMENT__;
			using Block = Block_Type<std::byte*>;

			[[nodiscard]] Block allocate( const std::size_t count, const std::size_t alignment = default_alignment )
			{
				return { reinterpret_cast<std::byte*>( operator_new_for<T>( count, std::align_val_t{alignment} ) ), count };
			}

			[[nodiscard]] bool deallocate( Block blk, const std::size_t alignment = default_alignment  )
			{
				operator_delete_for<T>( blk.ptr, std::align_val_t{alignment} );
				return true;
			}
		};

		template<std::size_t StorageSize, std::size_t Alignment>
			requires ( StorageSize % Alignment == 0 )
		class Local_Monotonic_Resource
		{
			alignas(Alignment) std::byte storage[StorageSize];
			std::byte* end = &storage;

		public:
			static constexpr inline std::size_t default_alignment = Alignment;
			using Block = Block_Type<std::byte*>;

			Local_Monotonic_Resource() = default;
			Local_Monotonic_Resource( const Local_Monotonic_Resource& ) = delete;
			Local_Monotonic_Resource& operator = ( const Local_Monotonic_Resource& ) = delete;
			Local_Monotonic_Resource( Local_Monotonic_Resource&& ) = delete;
			Local_Monotonic_Resource& operator = ( Local_Monotonic_Resource&& ) = delete;

			[[nodiscard]] std::size_t free_capacity() const noexcept
			{
				return std::end(storage) - end;
			}

			[[nodiscard]] bool owns( Block blk ) const noexcept
			{
				return std::greater_equal<>{}(blk.ptr, std::begin(storage)) && std::less<>(blk.ptr, std::end(storage));
			}

			[[nodiscard]] Block allocate( std::size_t count, std::size_t alignment = default_alignment ) noexcept
			{
				count += count % alignment;
				if ( alignment > default_alignment and reinterpret_cast<std::uintptr_t>(end) % alignment != 0 ) 
				{
					return Block{}; //cannot allocate while leaving a gap in the allocation.
				}
				const auto s = round_to_align<Alignment>( count );
				const auto free_cap = free_capacity();
				if ( s <= free_cap )
				{
					return Block{ std::exchange( end, end+s ), s };
				}
				else
				{
					return Block{};
				}
			}

			[[nodiscard]] bool deallocate( Block blk ) noexcept
			{
				if ( can_deallocate(blk) )
				{
					end -= blk.size;
					return true;
				}
				else
				{
					return false;
				}
			}

			[[nodiscard]] bool can_deallocate( Block blk ) const noexcept
			{
				const auto block_end = blk.ptr + blk.size;
				return block_end == end;
			}

			[[nodiscard]] Block resize( Block blk, std::size_t new_count ) noexcept
			{
				if ( can_deallocate(blk) && (blk.ptr + new_count) <= std::end(storage) )
				{
					if ( new_count > blk.size )
					{
						end += new_count - blk.size;
					}
					else
					{
						end -= blk.size - new_count;
					}
					return Block{ blk.ptr, new_count };
				}
				else
				{
					return {};
				}
			}
		};

		template<std::size_t Storage_Size, std::size_t Alignment>
			requires ( Storage_Size % Alignment == 0 )
		class Local_Resource 
		{
		public :
			static constexpr inline std::size_t default_alignment = Alignment;
			using ptr_t = std::byte*;
			using Block = Block_Type<ptr_t>;

		private :
			static constexpr bool debug = false;
			struct FreeNode
			{
				Block block{};
				FreeNode* prev = nullptr;
				FreeNode* next = nullptr;
			};	

			alignas(Alignment) std::byte storage[Storage_Size];	
			FreeNode head{ Block{ std::begin(storage), Storage_Size }, nullptr };
			FreeNode* tail = &head;

			FreeNode* find_space( std::size_t count, std::size_t alignment ) noexcept
			{
				FreeNode* curr = &head;
				do
				{
					if ( curr->block.size - reinterpret_cast<uintptr_t>(curr->block.ptr)%alignment >= count )
					{
						return curr;
					}

					curr = curr->next;
				} while ( curr != nullptr );
				
				return nullptr;
			}

			FreeNode* find_node_at( ptr_t pos ) noexcept
			{
				FreeNode* curr = &head;
				do
				{
					if ( curr->block.ptr == pos )
					{
						return curr;
					}

					curr = curr->next;
				} while ( curr != nullptr );
				
				return nullptr;
			}

			FreeNode* find_node_infront( ptr_t pos ) noexcept
			{
				FreeNode* curr = &head;
				do
				{
					if ( curr->block.ptr + curr->block.size == pos )
					{
						return curr;
					}

					curr = curr->next;
				} while ( curr != nullptr );
				
				return nullptr;
			}

			ptr_t take_memory( FreeNode* node, const std::size_t size ) noexcept
			{
				assert( size <= node->block.size );
				const auto ptr = node->block.ptr;
				if constexpr ( debug )
				{
					std::cout << " Taking memory from " << node->block;
				}

				if ( size < node->block.size ) //alloc is less than current free section, shrink free section
				{
					if constexpr ( debug )
					{
						std::cout << " reducing size\n";
					}
					node->block.ptr += size;
					node->block.size -= size;
				}
				else //alloc is exactly current free section
				{
					const auto prev = node->prev;
					if constexpr ( debug )
					{
						std::cout << " removing freenode ";
					}
					if ( node->prev != nullptr ) //relink previous to next, rely on implicit destruction of the now lost node
					{
						if constexpr ( debug )
						{
							std::cout << "updating prev\n";
						}
						prev->next = node->next;
						if ( prev->next == nullptr )
						{ tail = prev; }
					}
					else // if ( node == &head )
					{
						if constexpr ( debug )
						{
							std::cout << "freenode is head and its now out of memory\n";
						}
						head.block.ptr = nullptr;
						head.block.size = 0;
					}
				}
				return ptr;
			}

			FreeNode* find_node_storage_space() noexcept
			{
				for ( auto curr = tail; curr != nullptr; curr = curr->prev )
				{
					if ( curr->block.size > sizeof(FreeNode) )
					{
						return curr;
					}
				}
				return nullptr;
			}

			FreeNode* find_node_insertion_point( std::size_t size ) noexcept
			{
				for ( auto curr = &head; curr != nullptr; curr = curr->next )
				{
					if ( curr->block.size >= size )
					{
						return curr;
					}
				}
				return nullptr;
			}

			[[nodiscard]] bool insert_free_node( Block free_block ) noexcept
			{
				if ( head.block.ptr == nullptr )
				{
					head.block = free_block;
					return true;
				}

				const auto space = find_node_storage_space(); 
				if constexpr ( debug )
				{
					std::cout << " free node storage space found at " << space << '\n';
				}
				if ( space == nullptr )
				{
					return false;
				}
				const auto insertion_point = find_node_insertion_point( free_block.size );

				if constexpr ( debug )
				{
					std::cout << " insertion point for freenode found at node " << insertion_point;
					if ( insertion_point == &head )
					{
						std::cout << "(head)";
					}
					std::cout << '\n';
				}

				space->block.size -= sizeof(FreeNode);
				if ( insertion_point == &head )
				{
					const auto new_node = new( space->block.ptr+space->block.size) FreeNode{ head.block, &head, head.next };
					head.block = free_block;
					head.next = new_node;
				}
				else 
				{
					const auto new_node = new( space->block.ptr+space->block.size) FreeNode{ free_block, insertion_point->prev, insertion_point->next };
					insertion_point->prev->next = new_node;
					insertion_point->prev = new_node;
				}

				return true;
			}

		public :
			Local_Resource() = default;
			Local_Resource( const Local_Resource& ) = delete;
			Local_Resource& operator = ( const Local_Resource& ) = delete;
			Local_Resource( Local_Resource&& ) = delete;
			Local_Resource& operator = ( Local_Resource&& ) = delete;

			void print_info(std::ostream& os = std::cout) const
			{
				os << "----------------------  resource info ----------------------\n";
				os << "Whole range: " << std::begin(storage) << " - " << std::end(storage) << " = " << Storage_Size << '\n';
				os << "Free nodes:\n";
				os << "Node             range                             size\n";
				for ( auto curr = &head; curr != nullptr ; curr = curr->next )
				{
					os << ' ' << curr << " : " << curr->block.ptr << " - " << curr->block.ptr + curr->block.size << " = " << curr->block.size << '\n';
				}
				os << "--------------------- end resource info --------------------\n";
				os << std::flush;
			}

			[[nodiscard]] bool owns( Block blk ) const noexcept
			{
				const auto storage_begin = std::begin(storage);
				const auto storage_end = std::end(storage);
				const auto in_lower = blk.ptr >= storage_begin;
				const auto in_upper = blk.ptr + blk.size <= storage_end;
				return in_lower && in_upper;
			}

			[[nodiscard]] bool can_deallocate( Block blk ) const noexcept
			{
				return owns( blk );
			}

			[[nodiscard]] Block allocate( std::size_t count, std::size_t alignment = default_alignment ) noexcept
			{
				WERKZEUG_ASSERT( count % alignment == 0, "Why would you try to allocate memory that isnt aligned with itself...");
				auto size = round_to_align<Alignment>( count );

				if constexpr ( debug )
				{
					std::cout << "allocate request for " << count << ' ' << alignment << '\n';
				}

				const auto node = find_space( size, alignment );

				const auto offset = reinterpret_cast<std::uintptr_t>(node->block.ptr) % alignment;
				if ( node != nullptr ) //if we found a suitable free section
				{
					auto ptr = take_memory( node, size+offset );

					Block res{ ptr+offset, size };
					if constexpr ( debug )
					{
						std::cout << " allocating " << res << " using offset " << offset << '\n';
					}

					return res;
				}
				else //no memory found
				{
					if constexpr ( debug )
					{
						std::cout << "no memory found\n";
					}
					return {};
				}				
			}

			[[nodiscard]] bool deallocate( Block blk ) noexcept
			{
				if constexpr ( debug )
				{
					std::cout << "deallocate request for " << blk << '\n';
				}
				if ( not owns(blk) )
				{ return false; }

				if ( head.block.ptr == nullptr )
				{
					head.block = blk;
					return true;
				}


				const auto node_infront = find_node_infront( blk.ptr );
				if ( node_infront != nullptr )
				{
					if constexpr ( debug )
					{
						std::cout << " found free section infront of block: " << node_infront->block << ". extending.\n";
					}
					node_infront->block.size += blk.size;
					//no early return, we want to find out if there is another free section behind the deallocated section.
				}

				const auto node_behind = find_node_at( blk.ptr+blk.size );
				if ( node_behind != nullptr )
				{
					if constexpr ( debug )
					{
						std::cout << " found free section after block: " << node_behind->block;
					}
					if ( node_infront != nullptr )
					{
						if constexpr ( debug )
						{
							std::cout << " joining up with block infront\n";
						}
						if ( node_behind == &head )
						{
							if constexpr ( debug )
							{
								std::cout << " behind is head, swapping\n";
							}

							std::swap( node_behind->block, node_infront->block );
						}
						node_infront->block.size += node_behind->block.size; 
						werkzeug::detail::delink( node_behind );
						assert( deallocate( { reinterpret_cast<ptr_t>(node_behind), sizeof(FreeNode) } ) );
					}
					else
					{
						if constexpr ( debug )
						{
							std::cout << " extending.\n";
						}
						node_behind->block.ptr -= blk.size;
						node_behind->block.size += blk.size;
					}
				}

				if ( node_infront or node_behind ) // if we could extend an existing free section, we can return here.
				{
					return true;
				}

				if constexpr ( debug )
				{
					std::cout << " No adjecent free section found. Creating new free section:\n";
				}
				return insert_free_node( blk );
			}

			[[nodiscard]] Block resize( Block blk, std::size_t new_count ) noexcept
			{
				if ( not owns(blk) ) 
				{ return {}; }

				if ( blk.size == new_count )
				{ return blk; }

				const auto new_size = round_to_align<Alignment>( new_count );

				const auto [ node, prev ] = find_node_at( blk.ptr+blk.size );

				if ( new_count < blk.size ) //if we are shrinking
				{
					if ( node == nullptr ) // there is no free node after the current allocation
					{
						insert_free_node( blk.ptr, new_count );
					}
					else //there is already free space after the current allocation, we can just shrink it
					{
						node->block.ptr -= blk.size - new_size;
					}
					return { blk.ptr, new_size };
				}
				else if ( node != nullptr ) //we are growing and there is a free memory
				{
					take_memory( node, new_size - blk.size, prev );

					return { blk.ptr, new_size };		
				}
				else // we want to grow, but there is no memory at the location
				{
					return {}; 
				}
			}
		};
	}


	namespace polymorphic
	{
		struct Resource
		{
			using Block = Block_Type<std::byte*>;
			virtual Block allocate( std::size_t count ) = 0;
			virtual bool deallocate( Block blk ) = 0;
			virtual Block resize( Block blk, std::size_t count ) = 0;
			virtual bool is_equal( const Resource& other ) const noexcept = 0;

			virtual ~Resource() = default;
		};


		template<concepts::memory_source R>
		struct Wrapper : Resource
		{
			R r;
			Block allocate( std::size_t count ) override
			{
				return r.allocate(count);
			}

			bool deallocate( Block blk ) override
			{
				return r.deallocate(blk);
			}

			Block resize( Block blk, std::size_t new_size ) override
			{
				if constexpr ( concepts::has_resize<R> )
				{
					return r.resize(blk,new_size);
				}
				else
				{
					return {};
				}
			}

			bool is_equal( const Resource& other ) noexcept 
			{
				return this == &other;
			}
		};
	}

}

#endif