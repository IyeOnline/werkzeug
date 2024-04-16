#ifndef WERKZEUG_GUARD_CONTAINER_DYNAMIC_ARRAY_HPP
#define WERKZEUG_GUARD_CONTAINER_DYNAMIC_ARRAY_HPP


#include <compare>
#include <concepts>
#include <cstddef>
#include <type_traits>
#include <limits>
#include <new>
#include <utility>
#include <iterator>
#include <cstring>
#include <span>

#include "werkzeug/assertion.hpp"
#include "werkzeug/concepts.hpp"

#include "werkzeug/iterator.hpp"
#include "werkzeug/container/crtp_range_bases.hpp"
#include "werkzeug/traits.hpp"
#include "werkzeug/algorithm/range_transfer.hpp"
#include "werkzeug/memory/concepts.hpp"
#include "werkzeug/memory/allocator.hpp"
#include "werkzeug/memory/growth_strategies.hpp"


namespace werkzeug
{
	namespace detail
	{
		template<bool has_buff>
		struct size_and_state_t
		{
			constexpr static size_t high_mask = 1ull << (sizeof(size_t)*7+7);
			constexpr static size_t body_mask = ~high_mask;

			std::size_t size_ : 63 = 0;
			bool in_buffer_ : 1 = true;

			constexpr bool in_buffer() const noexcept
			{
				return in_buffer_;
			}
			std::size_t size() const noexcept
			{
				return size_;
			}
			void set_in_buffer( const bool b ) noexcept
			{
				in_buffer_ = b;
			}
			void set_size( std::size_t new_size ) noexcept
			{
				WERKZEUG_ASSERT( new_size < std::numeric_limits<std::size_t>::max()/2, "A container in SSO has one less bit for its size" );
				
				size_ = new_size & body_mask;
			}
		};


		template<>
		struct size_and_state_t<false>
		{
			std::size_t size_ = 0;
			constexpr static bool in_buffer_ = false;

			constexpr bool in_buffer() const noexcept
			{
				return false;
			}
			std::size_t size() const noexcept
			{
				return size_;
			}
			consteval static void set_in_buffer( const bool ) noexcept
			{ 
				// intentionally empty
			}
			void set_size( const std::size_t new_size ) noexcept
			{
				size_ = new_size;
			}
		};

		static_assert( sizeof(size_and_state_t<true>) == sizeof(size_and_state_t<false>) );
	}

	/**
	 * @brief dynamic array that can grow when new elements are inserted
	 * 
	 * @details The buffer size can be chosen as zero. The buffer shares memory with the allocation information. If the buffer size is not zero, move construction and assignment will have linear complexity
	 * 
	 * @tparam T type to store
	 * @tparam Buffer_Size size of the internal buffer
	 * @tparam Resource memory resource to request memory from
	 * @tparam Strategy resize strategy to use when the array grows
	 */
	template<
		typename T, 
		std::size_t Buffer_Size = 0,
		memory::concepts::memory_source Resource = memory::resource::fixed::New_Resource_For<T[]>, 
		memory::growth_strategy::strategy Strategy =  memory::growth_strategy::default_strategy 
	>
	class basic_dynamic_array_small_buffer
		: public detail::Range_Stream_CRTP_Base<basic_dynamic_array_small_buffer<T,Buffer_Size,Resource,Strategy>>
		, public detail::Range_Threeway_CRTP_Base<basic_dynamic_array_small_buffer<T,Buffer_Size,Resource,Strategy>,T>
	{
		template<typename U, std::size_t Other_Buffer_Size,memory::concepts::memory_source Other_Resource, memory::growth_strategy::strategy Other_Strategy>
		friend class basic_dynamic_array_small_buffer;

	protected :
		constexpr static bool has_buffer = Buffer_Size > 0;

		detail::size_and_state_t<has_buffer> ss; // alignof(size_t)

		using Allocator =  memory::Allocator<T,Resource>;
		union
		{
			typename Allocator::Block block_{}; //alignof(void*/size_t)
			std::array<T,Buffer_Size> buffer_; //alignof(T)
		};
		[[no_unique_address]] Allocator alloc; // unknown alignment

		constexpr static bool nx_cp_ctor = transfer_traits<T,T,transfer_type::copy, transfer_operation::construct>::nothrow;
		constexpr static bool nx_cp_asgn = transfer_traits<T,T,transfer_type::copy, transfer_operation::assign>::nothrow;
		constexpr static bool nx_mv_ctor = transfer_traits<T,T,transfer_type::move, transfer_operation::construct>::nothrow;
		constexpr static bool nx_mv_asgn = transfer_traits<T,T,transfer_type::move, transfer_operation::assign>::nothrow;
		constexpr static bool nx_dtor = std::is_nothrow_destructible_v<T>;
		constexpr static bool nx_swap = std::is_nothrow_swappable_v<T>;

		using traits = type_traits<T>;

		template<typename T_src,transfer_type ttype, transfer_operation top>
		constexpr static bool nx_convert = transfer_traits<T,T_src,ttype, top>::nothrow;

		constexpr static bool nx_reserve = Allocator::is_nothrow and ( ( std::is_nothrow_move_constructible_v<T> == std::is_move_constructible_v<T> ) or ( std::is_nothrow_copy_constructible_v<T> == std::is_copy_constructible_v<T> ) );

	public:
		using size_type = size_t;
		using difference_type = std::make_signed_t<size_type>;
		using iterator = Tagged_Iterator_Wrapper<T*,basic_dynamic_array_small_buffer>;
		using const_iterator = Tagged_Iterator_Wrapper<const T*,basic_dynamic_array_small_buffer>;
		using reverse_iterator = Reverse_Iterator_Wrapper<iterator>;
		using const_reverse_iterator = Reverse_Iterator_Wrapper<const_iterator>;
		using stable_iterator = Stable_Contigous_Iterator<basic_dynamic_array_small_buffer>;
		using const_stable_iterator = Stable_Contigous_Iterator<const basic_dynamic_array_small_buffer>;

		static_assert( std::input_or_output_iterator<iterator> );
		
		basic_dynamic_array_small_buffer() noexcept
			requires std::is_default_constructible_v<Resource>
			: block_{}
		{ }

		basic_dynamic_array_small_buffer( Resource res )
			: alloc{ res }
		{ }

		basic_dynamic_array_small_buffer( size_t size )
			noexcept ( traits::default_construct::nothrow and Allocator::is_nothrow )
			requires ( std::is_default_constructible_v<Resource> and std::is_default_constructible_v<T> )
		{
			resize( size );
		}

		basic_dynamic_array_small_buffer( size_t size, const T& value )
			noexcept ( traits::copy_construct::nothrow and Allocator::is_nothrow )
			requires ( std::is_default_constructible_v<Resource> )
		{
			resize( size, value );
		}

	private:
		struct copy_ctor_tag
		{};

		/**
		 * @brief Copy constructs a new array from `other`
		 */
		template<std::size_t other_buffer_size, typename Other_Strategy>
		basic_dynamic_array_small_buffer( const basic_dynamic_array_small_buffer<T,other_buffer_size,Resource,Other_Strategy>& other, copy_ctor_tag ) 
			noexcept( Allocator::is_nothrow_alloc and nx_cp_ctor )
			: alloc( other.alloc )
		{
			ss.set_size(other.size());
			ss.set_in_buffer(other.size() <= Buffer_Size );
			if ( not is_in_buffer() )
			{
				block_ = alloc.allocate(other.size());
			}
			transfer_range_with_fallback<transfer_type::copy, transfer_operation::construct>( other.begin(), other.end(), begin() );
		}

		template<std::size_t other_buffer_size, typename Other_Strategy>
		basic_dynamic_array_small_buffer& assign_operator_impl( const basic_dynamic_array_small_buffer<T,other_buffer_size,Resource,Other_Strategy>& other ) 
			noexcept(Allocator::is_nothrow_alloc and nx_cp_asgn)
		{
			if ( this->capacity() >= other.size() )
			{
				//copy assign as much as we can
				transfer_range_with_fallback<transfer_type::copy, transfer_operation::assign>( other.begin(), other.begin()+this->size(), this->begin() );
				if ( size() <= other.size() )
				{
					//copy construct the rest
					transfer_range_with_fallback<transfer_type::copy, transfer_operation::construct>( other.begin()+this->size(), other.end(), this->begin() + this->size());
				}
				else // size() > other.size()
				{
					//destroy remaining elements
					destroy_range( begin() + other.size(), end() );
				}

				ss.set_size(other.size());
				return *this;
			}
			else // other.size() > this->capacity()
			{
				if ( this->is_in_buffer() ) // if its in buffer and doesnt fit, we need a new allocation anyways.
				{
					destroy_range( begin(),end() );
					block_ = alloc.allocate( other.size() );
					ss.set_in_buffer(false);
				}
				else
				{
					if constexpr ( memory::concepts::has_resize<Allocator> )
					{
						const auto new_block = alloc.resize( block_, other.size() );
						if ( new_block.ptr != nullptr )
						{
							block_ = new_block;
							transfer_range_with_fallback<transfer_type::copy, transfer_operation::assign>( other.begin(), other.begin()+this->size(), this->begin() );
							transfer_range_with_fallback<transfer_type::copy, transfer_operation::construct>( other.begin()+this->size(), other.end(), this->begin() + this->size());

							ss.set_size(other.size());
							return *this;
						}
					}
					// either no resize or it wasnt successfull, so we destroy and free the old allocation.
					destroy_range( begin(), end() );
					WERKZEUG_ASSERT( alloc.deallocate( block_ ), "deallocation must succeed." );
					block_ = alloc.allocate(other.size() );
					WERKZEUG_ASSERT( block_.ptr != nullptr, "new allocation must succeed" );
				}
				
				transfer_range_with_fallback<transfer_type::copy, transfer_operation::construct>( other.begin(), other.end(), block_.begin() );
				ss.set_size(other.size());


				return *this;
			}
		}
		
	public :
		basic_dynamic_array_small_buffer( const basic_dynamic_array_small_buffer& other )
			: basic_dynamic_array_small_buffer( other, copy_ctor_tag{} )
		{}

		template<std::size_t other_buffer_size, typename Other_Strategy>
		basic_dynamic_array_small_buffer( const basic_dynamic_array_small_buffer<T,other_buffer_size,Resource,Other_Strategy>& other ) 
			noexcept( Allocator::is_nothrow_alloc and nx_cp_ctor )
			: basic_dynamic_array_small_buffer( other, copy_ctor_tag{} )
		{}

		/**
		 * @brief Copy assigns to `*this` from `other`
		 * 
		 * @details Will try to perform assignment into existing elements if no reallocation is required.
		 */
		template<std::size_t other_buffer_size, typename Other_Strategy>
		basic_dynamic_array_small_buffer& operator=( const basic_dynamic_array_small_buffer<T,other_buffer_size,Resource,Other_Strategy>& other ) 
			noexcept(Allocator::is_nothrow_alloc and nx_cp_asgn)
		{
			return assign_operator_impl( other );
		}

		/**
		 * @brief Copy assigns to `*this` from `other`
		 * 
		 * @details Will try to perform assignment into existing elements if no reallocation is required.
		 */
		basic_dynamic_array_small_buffer& operator=( const basic_dynamic_array_small_buffer& other ) 
			noexcept(Allocator::is_nothrow_alloc and nx_cp_asgn)
		{
			return assign_operator_impl( other );
		}

		/**
		 * @brief Move constructs a new object from `other`
		 * 
		 * @details The complexity is constant when the source is not in the buffer and linear otherwise. If the source is in the buffer, its elements will get moved from and destroyed.
		 */
		template<std::size_t other_buffer_size, typename Other_Strategy>
		basic_dynamic_array_small_buffer( basic_dynamic_array_small_buffer<T,other_buffer_size,Resource, Other_Strategy>&& other ) 
			noexcept(nx_mv_ctor and (Allocator::is_nothrow_alloc or not has_buffer) )
			: alloc{ std::move(other.alloc) }
		{ 
			if ( other.is_in_buffer() and other.size() < Buffer_Size )
			{
				transfer_range_with_fallback<transfer_type::move,transfer_operation::construct>( other.begin(), other.end(), data() );
				destroy_range( other.begin(), other.end() );
				other.ss = {};
				ss.set_size( other.size() );
				ss.set_in_buffer( true );
			}
			else if ( other.is_in_buffer() )
			{
				block_ = alloc.allocate( other.size() );
				transfer_range_with_fallback<transfer_type::move,transfer_operation::construct>( other.begin(), other.end(), block_.ptr );
				destroy_range( other.begin(), other.end() );
				other.ss = {};
				ss.set_size(other.size());
				ss.set_in_buffer(false);
			}
			else
			{
				block_ = std::exchange( other.block_,{} );
				ss = std::exchange( other.ss, {} );
			}
		}

		template<std::size_t other_buffer_size, typename Other_Resource, typename Other_Strategy>
		basic_dynamic_array_small_buffer( basic_dynamic_array_small_buffer<T,other_buffer_size,Other_Resource, Other_Strategy>&& ) = delete;

		/**
		 * @brief Move assign from a source to this.
		 * 
		 * @details Constant complexity when the source is in buffer, otherwise linear. If the source is in buffer, its elements will get moved from and then destroyed
		 */
		template<std::size_t other_buffer_size, typename Other_Strategy>
		basic_dynamic_array_small_buffer& operator=( basic_dynamic_array_small_buffer<T,other_buffer_size,Resource,Other_Strategy>&& other ) 
			noexcept(nx_mv_asgn and (Allocator::is_nothrow_alloc or not has_buffer) )
		{
			if ( other.is_in_buffer() and this->is_in_buffer() and other.size() <= Buffer_Size )
			{
				transfer_range_with_fallback<transfer_type::move,transfer_operation::assign>( other.begin(), other.begin()+this->size(), this->data() );
				transfer_range_with_fallback<transfer_type::move,transfer_operation::construct>( other.begin()+this->size(), other.end(), this->data() + this->size() );
				destroy_range(other.begin(),other.end());
				other.ss = {};
				ss.set_in_buffer(true);
				ss.set_size(other.size());

				return *this;
			}
			else if ( other.is_in_buffer() and this->is_in_buffer() )
			{
				destroy_rage(begin(),end());
				block_ = alloc.allocate(other.size());
				WERKZEUG_ASSERT( block_.ptr != nullptr, "allocation must succeed" );
				transfer_range_with_fallback<transfer_type::move,transfer_operation::construct>( other.begin(), other.end(), block_.ptr );
				ss.set_size( other.size() );
				ss.set_in_buffer( false );
				return *this;
			}
			else if ( other.is_in_buffer() and this->capacity() <= other.size() )
			{
				if constexpr ( Allocator::has_resize )
				{
					const auto realloc_block = alloc.resize( block_, other.size() );
					if ( realloc_block.ptr != nullptr )
					{
						block_ = realloc_block;
						transfer_range_with_fallback<transfer_type::move,transfer_operation::assign>( other.begin(), other.begin()+this->size(), this->begin() );
						transfer_range_with_fallback<transfer_type::move,transfer_operation::construct>( other.begin()+this->size(), other.end(), this->begin()+this->size() );
						destroy_range( other.begin(), other.end() );
						ss.set_size(other.size());
						ss.set_in_buffer(false);
						other.ss = {};
						return *this;
					}
				}
				const auto new_block = alloc.allocate( other.size() );
				transfer_range_with_fallback<transfer_type::move, transfer_operation::construct>( other.begin(), other.end(), begin() );
				destroy_range( other.begin(), other.end() );
				ss.set_size(other.size());
				ss.set_in_buffer(false);
				other.ss = {};
				return *this;
			}
			else if ( other.is_in_buffer() )
			{
				const auto assign_size = std::min( other.size(), this->size() );
				transfer_range_with_fallback<transfer_type::move, transfer_operation::assign>( other.begin(), other.begin()+assign_size, this->begin() );
				transfer_range_with_fallback<transfer_type::move, transfer_operation::construct>( other.begin()+assign_size, other.end(), this->begin()+assign_size );
				destroy_range( other.begin(), other.end() );
				ss.set_size( other.size() );
				ss.set_in_buffer( false );
				other.ss = {};
				return *this;
			}
			else
			{
				block_ = std::exchange( other.block_,{} );
				ss = std::exchange( other.ss, {} );
				return *this;
			}
		}

		template<std::size_t other_buffer_size, typename Other_Resource, typename Other_Strategy>
		basic_dynamic_array_small_buffer& operator=( basic_dynamic_array_small_buffer<T,other_buffer_size,Other_Resource, Other_Strategy>&& ) = delete;

		basic_dynamic_array_small_buffer( const basic_dynamic_array_small_buffer&& ) = delete;

		basic_dynamic_array_small_buffer( range auto&& input_range, Resource memory ) 
			noexcept( nx_convert<decltype(*std::ranges::begin(input_range)),std::is_lvalue_reference_v<decltype(input_range)> ? transfer_type::copy : transfer_type::move, transfer_operation::construct> )
			: alloc( memory )
		{
			append( std::ranges::begin(input_range), std::ranges::end(input_range) );
		}

		basic_dynamic_array_small_buffer( range auto&& input_range ) 
			noexcept( nx_convert<decltype(*std::ranges::begin(input_range)), std::is_lvalue_reference_v<decltype(input_range)> ? transfer_type::copy : transfer_type::move, transfer_operation::construct> )
			requires std::is_default_constructible_v<Allocator>
			: basic_dynamic_array_small_buffer( std::forward<decltype(input_range)>(input_range), {} )
		{ }

		template<typename U,std::size_t N>
		explicit(not std::same_as<T,U>) basic_dynamic_array_small_buffer( U(&&arr)[N] ) 
			noexcept( nx_convert<U,transfer_type::move,transfer_operation::construct> )
		{
			reserve( N );
			for ( auto&& value : arr )
			{
				emplace_back( std::move(value) );
			}
		}

		~basic_dynamic_array_small_buffer() 
			noexcept( Allocator::is_nothrow_dealloc and nx_dtor)
		{
			destroy_range( begin(), end() );
			if ( not is_in_buffer() )
			{
				if ( block_.ptr != nullptr )
				{
					WERKZEUG_ASSERT( alloc.deallocate(block_), "deallocation must succeed" );
				}
			}
		}

		static basic_dynamic_array_small_buffer make_with_capacity( std::size_t capacity )
			requires std::is_default_constructible_v<Allocator>
		{
			basic_dynamic_array_small_buffer arr{};
			arr.reserve( capacity );
			return arr;
		}

		static basic_dynamic_array_small_buffer make_with_capacity( std::size_t capacity, Resource r )
		{
			basic_dynamic_array_small_buffer arr{ r };
			arr.reserve( capacity );
			return arr;
		}

		operator std::span<T>() noexcept
		{
			return { data(), size() };
		}

		operator std::span<const T>() const noexcept
		{
			return { data(), size() };
		}


		/**
		 * @brief moves the array into dynamic storage, even if it is currently in the SSO buffer
		 */
		void externalize( size_t allocation_size=0 ) 
			noexcept(Allocator::is_nothrow and nx_mv_ctor)
		{
			if constexpr ( has_buffer )
			{
				if ( not is_in_buffer() )
				{
					return;
				}

				allocation_size = std::max( allocation_size, size() );
				auto new_block = alloc.allocate( allocation_size );
				WERKZEUG_ASSERT( new_block, "Allocation failure" );

				transfer_range_with_fallback<transfer_type::move, transfer_operation::construct>( begin(), end(), new_block.begin() );

				destroy_range( begin(), end() );

				block_ = new_block;
				ss.set_in_buffer(false);
			}
		}

		/**
		 * @brief Reserves enouch memory for `new_capacity` elements. If `new_capacity` is less than `capacity()`, nothing happens
		 * 
		 * @param new_capacity capacity request
		 * @return the actually allocated capacity
		 */
		size_t reserve( const std::size_t new_capacity ) 
			noexcept(nx_reserve)
		{
			if ( new_capacity <= capacity() )
			{ return capacity(); }

			if constexpr ( has_buffer )
			{
				if ( is_in_buffer() and new_capacity < Buffer_Size )
				{
					return capacity();
				}
				else if ( is_in_buffer() )
				{
					auto new_block = alloc.allocate( new_capacity );
					WERKZEUG_ASSERT( new_block.ptr != nullptr, "allocation must succeed" );
					transfer_range_with_fallback<transfer_type::move, transfer_operation::construct>( begin(), end(), new_block.begin() );
					ss.set_in_buffer(false);
					block_ = new_block;
					return capacity();
				}
				else
				{
					// fallthrough
				}
			}

			if ( block_.ptr == nullptr )
			{ 
				block_ = alloc.allocate( new_capacity ); 
				WERKZEUG_ASSERT( block_.ptr != nullptr, "allocation must succeed" );
				return capacity();
			}

			if constexpr ( memory::concepts::has_resize<Allocator> )
			{
				const auto realloc_block = alloc.resize( block_, new_capacity );
				if ( realloc_block.ptr != nullptr )
				{
					block_ = realloc_block;
					return capacity();
				}
			}
			const auto new_block = alloc.allocate( new_capacity );
			WERKZEUG_ASSERT( new_block.ptr != nullptr, "allocation must succeed" );
			transfer_range_with_fallback<transfer_type::move, transfer_operation::construct>( block_.begin(), block_.end(), new_block.begin() );
			destroy_range(begin(), end());
			WERKZEUG_ASSERT( alloc.deallocate( block_ ), "deallocation must succeed" );
			block_ = new_block;

			return capacity();
		}

		/**
		 * @brief resizes the vector to the given `new_size`. Potential new elements are constructed from `args...`
		 * 
		 * @param new_size new size the array shall have after the call
		 * @param args... arguments to construct potential new elements from
		 * @return `capacity()` 
		 */
		template<typename ... Args>
		size_t resize( const std::size_t new_size, const Args& ... args ) 
			noexcept(nx_reserve and std::is_nothrow_constructible_v<T,Args...> )
			requires std::constructible_from<T,Args...>
		{
			if ( new_size < size() )
			{
				const auto destroy_count = static_cast<difference_type>(size() - new_size);
				destroy_range( begin() + destroy_count, end() );
				ss.set_size(new_size);
			}
			else if ( new_size > size() )
			{
				const auto new_element_count = new_size - size();
				reserve( new_size );
				for ( size_t i=0; i<new_element_count; ++i )
				{
					emplace_back( args ... );
				}
			}

			return capacity();
		}

		/**
		 * @brief grows the allocation to the new size updates size()
		 * 
		 * The behaviour if undefined if no elements are placed into the allocation by the user afterwards
		 */
		void grow_for_overwrite( const std::size_t new_size ) noexcept(nx_reserve)
		{
			reserve( new_size );

			ss.set_size( new_size );
		}

		// void force_size_after_overwrite( const std::size_t new_size ) noexcept
		// {
		// 	ss.set_size(new_size);
		// }

		/** @brief shrinks the allocation such that `size() == capacity()` afterwards. */
		void shrink_to_fit() 
			noexcept(Allocator::is_nothrow and nx_mv_ctor)
		{
			if ( is_in_buffer() or block_.size == size() )
			{
				return;
			}

			if constexpr ( memory::concepts::has_resize<Allocator> )
			{
				const auto new_block = alloc.resize(block_, size());
				WERKZEUG_ASSERT( new_block.ptr == block_.ptr, "shrinking resize should always succeed" );
				return;
			}
			
			if ( size() <= Buffer_Size )
			{
				const auto old_block = block_;
				ss.set_in_buffer(true);
				transfer_range_with_fallback<transfer_type::move,transfer_operation::construct>( old_block.begin(), old_block.begin() + this->size(), data() );
				destroy_range( old_block.begin(), old_block.begin()+this->size() );
				WERKZEUG_ASSERT( alloc.deallocate(old_block), "deallocation must succeed" );
				return;
			}
			else
			{
				auto new_block = alloc.allocate( size() );
				WERKZEUG_ASSERT( new_block.ptr != nullptr, "allocation must succeed" );
				transfer_range_with_fallback<transfer_type::move,transfer_operation::construct>( begin(), end(), new_block.begin() );
				destroy_rage( begin(),end() );
				WERKZEUG_ASSERT( alloc.deallocate(block_), "deallocation must succeed" );
				block_ = new_block;
				return;
			}
		}

		/** @brief destroys all contained elements and sets the size to 0. Does not touch capacity */
		void clear() 
			noexcept(nx_dtor)
		{
			destroy_range( begin(), end() );
			ss.set_size(0);
		}

		/** @brief returns a pointer to the underlying array */
		[[nodiscard]] T* data() noexcept
		{
			if constexpr ( has_buffer )
			{
				if ( is_in_buffer() )
				{
					return buffer_.data();
				}
			}
			return block_.ptr;
		}
		
		/** @brief returns a pointer to the underlying array */
		[[nodiscard]] const T* data() const noexcept
		{
			if constexpr ( has_buffer )
			{
				if ( is_in_buffer() )
				{
					return buffer_.data();
				}
			}
			return block_.ptr;
		}

		[[nodiscard]] iterator begin() noexcept { return data(); }
		[[nodiscard]] iterator end() noexcept { return data() + size(); }

		[[nodiscard]] const_iterator begin() const noexcept { return data(); }
		[[nodiscard]] const_iterator end() const noexcept { return data() + size(); }

		[[nodiscard]] reverse_iterator rbegin() noexcept { return end()-1; }
		[[nodiscard]] reverse_iterator rend() noexcept { return begin()-1; }

		[[nodiscard]] const_reverse_iterator rbegin() const noexcept { return end()-1; }
		[[nodiscard]] const_reverse_iterator rend() const noexcept { return begin()-1; }

		[[nodiscard]] stable_iterator stable_begin() noexcept { return stable_begin(*this); }
		[[nodiscard]] stable_iterator stable_end() noexcept { return stable_end(*this); }

		[[nodiscard]] const_stable_iterator stable_begin() const noexcept { return stable_begin(*this); }
		[[nodiscard]] const_stable_iterator stable_end() const noexcept { return stable_end(*this); }

		[[nodiscard]] std::size_t size() const noexcept
		{ return ss.size(); }

		[[nodiscard]] bool is_empty() const noexcept
		{ return size() == 0; }

		[[nodiscard]] T& front() noexcept
		{
			WERKZEUG_ASSERT( not is_empty(), "accessing requires the container to not be empty" );
			return *begin();
		}

		[[nodiscard]] const T& front() const noexcept
		{
			WERKZEUG_ASSERT( not is_empty(), "accessing requires the container to not be empty" );
			return *begin();
		}

		[[nodiscard]] T& back() noexcept
		{
			WERKZEUG_ASSERT( not is_empty(), "accessing requires the container to not be empty" );
			return *(end()-1);
		}

		[[nodiscard]] const T& back() const noexcept
		{
			WERKZEUG_ASSERT( not is_empty(), "accessing requires the container to not be empty" );
			return *(end()-1);
		}

		/** @brief whether the storage is currently in the buffer */
		[[nodiscard]] bool is_in_buffer() const noexcept
		{
			return ss.in_buffer();
		}

		[[nodiscard]] std::size_t capacity() const noexcept
		{ 
			if constexpr ( has_buffer )
			{
				if ( is_in_buffer() )
				{
					return Buffer_Size;
				}
			}
			return block_.size;
		}

		[[nodiscard]] static consteval std::size_t buffer_size() noexcept
		{
			return Buffer_Size;
		}

		[[nodiscard]] T& operator[]( const std::size_t idx ) noexcept
		{
			WERKZEUG_ASSERT( not is_empty(), "accessing requires the container to not be empty" );
			return data()[idx];
		}

		[[nodiscard]] const T& operator[]( const std::size_t idx ) const noexcept
		{
			WERKZEUG_ASSERT( not is_empty(), "accessing requires the container to not be empty" );
			return data()[idx];
		}


		/**
		 * @brief appends the range [source_begin,source_end) to the vector
		 */
		template<typename TBegin, end_iterator_for_iterator<TBegin> TEnd>
			requires std::is_constructible_v<T,typename std::iterator_traits<TBegin>::value_type>
		iterator append( TBegin source_begin, const TEnd source_end ) noexcept( nx_reserve and std::is_nothrow_constructible_v<T,typename std::iterator_traits<TBegin>::value_type> )
		{
			if constexpr ( random_access_iterator<decltype(source_begin)> )
			{
				const auto source_size = std::distance( source_begin, source_end );
				const auto reserve_size = source_size + size();
				
				reserve( reserve_size );

				//TODO: consider moving when the iterator can be moved from.
				transfer_range_with_fallback<transfer_type::copy,transfer_operation::construct>( std::forward<decltype(source_begin)>(source_begin), std::forward<decltype(source_end)>(source_end), end() );
				ss.set_size( reserve_size );

				return end();
			}

			for ( ; source_begin != source_end; ++source_begin )
			{
				emplace_back( *source_begin );
			}

			return end();
		}

		/**
		 * @brief appends the range r onto the vector. moves if the range is an rvalue
		 */
		template<range R>
			requires std::is_constructible_v<T,typename range_traits<R>::value_type>
		void append( R&& r) noexcept( nx_reserve and std::is_nothrow_constructible_v<T,typename range_traits<R>::value_type> )
		{
			return append( std::ranges::begin(r), std::ranges::end(r) );
		}


		/**
		 * @brief creates a new element in place from `args...` at `it`
		 * 
		 * @param it position to construct the element at
		 * @param args... arguments to construct a T from
		 * @returns reference to the newly created object
		 */
		template<typename ... Ts>
		T& emplace_at( iterator it, Ts&& ... args ) noexcept( nx_reserve and std::is_nothrow_constructible_v<T,Ts...> )
		{
			WERKZEUG_ASSERT( it >= begin(), "target iterator must be in range" );
			WERKZEUG_ASSERT( it <= end(), "target iterator must be in range" );
			if ( size() == 0 or it == end() )
			{
				return emplace_back( std::forward<Ts>(args)... );
			}

			const auto idx = it - begin();

			if ( size() == capacity() )
			{
				const std::size_t new_capacity = Strategy::grow(capacity());

				bool has_resized = false;
				typename Allocator::Block new_block;
				if constexpr ( memory::concepts::has_resize<Allocator> )
				{
					if ( block_.ptr != nullptr )
					{
						new_block = alloc.resize( block_, new_capacity );
						has_resized = new_block.ptr != nullptr;
					}
				}
				if ( not has_resized )
				{
					new_block = alloc.allocate( new_capacity );
					WERKZEUG_ASSERT( new_block.ptr != nullptr, "allocation must succeed" );
					transfer_range_with_fallback<transfer_type::move,transfer_operation::construct>( begin(), begin()+idx, new_block.begin() ); //move elemenbts before insertion to new block
				}
				new(std::addressof(*end())) T{ std::move(*(end()-1)) }; //move construct the last element in new block
				transfer_range_with_fallback<transfer_type::move,transfer_operation::assign>( rbegin()+1, reverse_iterator{ std::addressof(*(it-1)) }, rbegin() ); //move the rest backwards/to new block
				auto& ref = *new( new_block.begin()+idx) T( std::forward<Ts>(args) ... );

				ss.set_size(size()+1);
				ss.set_in_buffer(false);

				if ( not has_resized )
				{
					destroy_range( begin(), end() );
					WERKZEUG_ASSERT( alloc.deallocate(block_), "deallocation must succeed" );
				}
				block_ = new_block;
				return ref;
			}
			else
			{
				new(std::addressof(*end())) T{ std::move(*(end()-1)) }; //move construct the last element
				transfer_range_with_fallback<transfer_type::move,transfer_operation::assign>( rbegin()+1, reverse_iterator{ std::addressof(*(it-1)) }, rbegin() ); //move the rest of the range
				auto& ref = *new(std::addressof(*it)) T( std::forward<Ts>(args) ... ); //create new element
				ss.set_size(size()+1);
				return ref;
			}
		}

		/**
		 * @brief creates a new element in place from `args...` at the back of the vector
		 * 
		 * @param args... arguments to construct a T from
		 * @returns reference to the newly created object
		 */
		template<typename ... Ts>
		T& emplace_back( Ts&& ... args ) noexcept( nx_reserve and std::is_nothrow_constructible_v<T,Ts...> )
		{
			if ( size() == capacity() )
			{
				const std::size_t new_capacity = Strategy::grow(capacity());
				reserve(new_capacity);
			}
			auto& ref = *new (std::addressof(*end())) T( std::forward<Ts>(args) ... );
			ss.set_size(size()+1);
			return ref;
		}

		/**
		 * @brief copies the given 'value' to the end of the range
		 * 
		 * @returns reference to the newly created object in the container
		 */
		T& push_back( const T& value ) noexcept( nx_reserve and std::is_nothrow_copy_constructible_v<T> )
		{
			return emplace_back( value );
		}

		/**
		 * @brief moves the given 'value' to the end of the range
		 * 
		 * @returns reference to the newly created object in the container
		 */
		T& push_back( T&& value ) noexcept( nx_reserve and std::is_nothrow_move_constructible_v<T> )
		{
			return emplace_back( std::move(value) );
		}

		/**
		 * @brief pops the last item from the vector and returns it
		 */
		[[nodiscard]] T pop_back() noexcept
		{
			WERKZEUG_ASSERT( not is_empty(), "container may not be empty" );
			T last_ = std::move( back() );
			std::destroy_at( &(back()) );
			ss.set_size(size()-1);

			return last_;
		}

		iterator insert( iterator it, const T& value )
		{
			auto idx = it - begin();
			emplace_at( it, value );

			return begin() + idx;
		}

		iterator insert( iterator it, T&& value )
		{
			auto idx = it - begin();
			emplace_at( it, std::move(value) );

			return begin() + idx;
		}

		/**
		 * @brief removes all elements in the range [begin_, end_) from the vector
		 * 
		 * @param begin_ inclusive begin of the range
		 * @param end_ exclusive end of the range
		 */
		void erase( iterator begin_, const iterator end_ ) noexcept( nx_dtor && nx_swap )
		{
			WERKZEUG_ASSERT( begin_ >= begin(), "range must be valid" );
			WERKZEUG_ASSERT( begin_ <= end(), "range must be valid" );
			WERKZEUG_ASSERT( end_ >= begin(), "range must be valid" );
			WERKZEUG_ASSERT( end_ <= end(), "range must be valid" );
			WERKZEUG_ASSERT( begin_ <= end_, "range must be valid" );

			const std::size_t erase_count = end_ - begin_;
			const std::size_t count_after_eraseure = end_ - end();
			transfer_range_with_fallback<transfer_type::move,transfer_operation::assign>( end_, end(), begin_ );

			destroy_range( begin_+count_after_eraseure, end() );

			ss.set_size(size()-erase_count);
		}
		
		/**
		 * @brief removes the element referenced by `it`
		 */
		void erase( const iterator it ) noexcept(nx_dtor && nx_swap )
		{
			erase( it, it+1 );
		}

		/**
		 * @brief removes the element at index `idx`
		 */
		void erase( const std::size_t idx ) noexcept(nx_dtor && nx_swap)
		{
			WERKZEUG_ASSERT( idx < size(), "index must be valid for the container" );
			erase( begin()+idx, begin()+idx+1 );
		}

		template<std::size_t s1, typename R1, typename S1>
		bool operator==( const basic_dynamic_array_small_buffer<T,s1,R1,S1>& other ) noexcept
		{
			if ( size() != other.size() )
			{
				return false;
			}
			if ( size() == 0 )
			{
				return true;
			}

			auto i1 = this->begin();
			const auto e1 = this->end();
			auto i2 = other.begin();
			for ( ; i1 != e1; ++i1, ++i2 )
			{
				if ( *i1 != i2 )
				{
					return false;
				}
			}
			return true;
		}
	};

	template<typename T, std::size_t Buffer_Size, typename Resource, typename Strategy>
	auto operator<=>( const basic_dynamic_array_small_buffer<T,Buffer_Size,Resource,Strategy>& vec, const std::span<const T> span ) noexcept
		requires std::three_way_comparable<T>
	{
		using result_t = std::compare_three_way_result_t<T>;
		for ( std::size_t i = 0; i < std::min( vec.size(), span.size() ); ++i )
		{
			const auto result = vec[i] <=> span[i];
			if ( result != result_t::equivalent )
			{
				return result;
			}
		}
		if ( vec.size() < span.size() )
		{
			return result_t::less;
		}
		else if ( vec.size() > span.size() )
		{
			return result_t::greater;
		}
		else 
		{
			return result_t::equivalent;
		}
	}

	template<typename T, std::size_t Buffer_Size, typename Resource, typename Strategy>
	auto operator<=>( const std::span<const T> span, const basic_dynamic_array_small_buffer<T,Buffer_Size,Resource,Strategy>& vec ) noexcept
		requires std::three_way_comparable<T>
	{
		using result_t = std::compare_three_way_result_t<T>;
		for ( std::size_t i = 0; i < std::min( vec.size(), span.size() ); ++i )
		{
			const auto result = span[i] <=> vec[i];
			if ( result != result_t::equivalent )
			{
				return result;
			}
		}
		if ( vec.size() < span.size() )
		{
			return result_t::greater;
		}
		else if ( vec.size() > span.size() )
		{
			return result_t::less;
		}
		else 
		{
			return result_t::equivalent;
		}
	}

	template<typename T, std::size_t Buffer_Size, typename Resource, typename Strategy>
	auto operator<=>(  const basic_dynamic_array_small_buffer<T,Buffer_Size,Resource,Strategy>& lhs, const basic_dynamic_array_small_buffer<T,Buffer_Size,Resource,Strategy>& rhs ) noexcept
		requires std::three_way_comparable<T>
	{
		return lhs <=> std::span{ rhs };
	}

	template<typename T, std::size_t Buffer_Size, typename Resource, typename Strategy>
		requires std::equality_comparable<T>
	auto operator==( const basic_dynamic_array_small_buffer<T,Buffer_Size,Resource,Strategy>& vec, const std::span<const T> span ) noexcept
	{
		const auto result = ( vec <=> span );
		return result == decltype(result)::equivalent;
	}

	template<typename T, std::size_t Buffer_Size, typename Resource, typename Strategy>
		requires std::equality_comparable<T>
	auto operator==( const std::span<const T> span, const basic_dynamic_array_small_buffer<T,Buffer_Size,Resource,Strategy>& vec ) noexcept
	{
		const auto result = ( vec <=> span );
		return result == decltype(result)::equivalent;
	}

	template<typename T, std::size_t Buffer_Size, typename Resource, typename Strategy>
		requires std::equality_comparable<T>
	auto operator==(  const basic_dynamic_array_small_buffer<T,Buffer_Size,Resource,Strategy>& lhs, const basic_dynamic_array_small_buffer<T,Buffer_Size,Resource,Strategy>& rhs ) noexcept
	{
		const auto result = ( lhs <=> rhs );
		return result == decltype(result)::equivalent;
	}


	/**
	 * @brief dynamic array (i.e. std::vector)
	 * 
	 * @tparam T type to store
	 * @tparam Resource memory resource
	 */
	template<typename T, memory::concepts::memory_source Resource = memory::resource::fixed::New_Resource_For<T[]>>
	using dynamic_array = basic_dynamic_array_small_buffer<T,0,Resource>;


	/**
	 * @brief dynamic array (i.e. std::vector) with SSO
	 * 
	 * @tparam T type to store
	 * @tparam Resource memory resource
	 */
	template<typename T, memory::concepts::memory_source Resource = memory::resource::fixed::New_Resource_For<T[]>>
	using dynamic_array_sso = basic_dynamic_array_small_buffer<T,2*sizeof(void*)/sizeof(T),Resource>;

	static_assert( sizeof(dynamic_array<char>) == sizeof(dynamic_array_sso<char>) );
}

#endif