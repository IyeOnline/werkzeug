#pragma once
#include <cassert>
#include <cstddef>
#include <iostream>
#include <type_traits>
#include <utility>
#include <iterator>

namespace custom_STL
{
	namespace detail
	{
		template<typename T>
		struct noexcept_traits
		{
			constexpr static bool cp_ctor = std::is_nothrow_copy_constructible_v<T>;
			constexpr static bool cp_asgn = std::is_nothrow_copy_assignable_v<T>;

			constexpr static bool mv_ctor = std::is_nothrow_move_constructible_v<T>;
			constexpr static bool mv_asgn = std::is_nothrow_move_assignable_v<T>;

			template<typename ... Ts>
			constexpr static bool ctor = std::is_nothrow_constructible_v<T,Ts...>;
			constexpr static bool dtor = std::is_nothrow_destructible_v<T>;

			constexpr static bool swap = std::is_nothrow_swappable_v<T>;
		};

		template<typename Alloc, typename T, typename ... Ts>
		concept has_construct = requires ( Alloc a, T* ptr, Ts ... ts )
		{
			a.construct( ptr, ts ... );
		};

		template<typename Alloc, typename T>
		concept has_destroy = requires ( Alloc a, T* ptr )
		{
			a.destroy( ptr );
		};

		template<typename Alloc, typename T, typename ...Ts>
		concept noexcept_construct = not has_construct<Alloc,T,Ts...> || noexcept( std::declval<Alloc>().construct( std::declval<T*>(), std::declval<Ts>() ... ) );

		template<typename Alloc, typename T>
		concept noexcept_destroy =  not has_destroy<Alloc,T> || noexcept( std::declval<Alloc>().destroy( std::declval<T*>() ) );

		template<typename Alloc, typename T>
		struct allocator_noexcept_traits : noexcept_traits<Alloc>
		{
			constexpr static bool alloc = noexcept( std::declval<Alloc>().allocate( std::size_t{} ) );
			constexpr static bool dealloc = noexcept( std::declval<Alloc>().deallocate( std::declval<T*>(), std::size_t{} ) );

			template<typename ... Ts>
			constexpr static bool construct = noexcept_construct<Alloc,T,Ts...>;
			constexpr static bool destroy = noexcept_destroy<Alloc,T>;
		};
	}

	template<typename T,typename Allocator = std::allocator<T>>
	class vector
	{
		constexpr static std::align_val_t alignment{ alignof(T) };
		constexpr static std::size_t min_capacity = 4;

		using nx_T = detail::noexcept_traits<T>;
		using nx_Alloc = detail::allocator_noexcept_traits<Allocator,T>;

		template<typename ... Ts>
		constexpr static bool nx_construct = nx_T::template ctor<Ts...> && nx_Alloc:: template construct<Ts...>;
		constexpr static bool nx_destroy = nx_T::dtor && nx_Alloc::destroy;

		constexpr static bool nx_reserve = nx_T::mv_ctor && nx_T::dtor && nx_Alloc::alloc && nx_Alloc::dealloc;

		using alloc_traits = std::allocator_traits<Allocator>;

		T* data_ = nullptr;
		std::size_t size_ = 0;
		std::size_t capacity_ = 0;
		[[no_unique_address]] Allocator alloc_;

		static constexpr T* allocate(std::size_t size) noexcept(nx_Alloc::alloc)
		{
			return alloc_traits::allocate( alloc_, size );
		}

		static constexpr void deallocate(T* ptr, std::size_t size ) noexcept(nx_Alloc::dealloc)
		{
			alloc_traits::deallocate( alloc_, ptr, size );
		}

		template<typename ... Ts>
		static constexpr T& construct( T* ptr, Ts&& ... params ) noexcept( nx_construct<Ts...> )
		{
			alloc_traits::construct( alloc_, ptr, std::forward<Ts>(params) ... );
			return *ptr;
		}

		static constexpr void destroy(T* begin_, T* end_) noexcept( nx_destroy )
		{
			if constexpr ( not std::is_trivially_destructible_v<T> )
			{
				for (; begin_ < end_; ++begin_)
				{
					alloc_traits::destroy( alloc_, begin_);
				}
			}
		}

		enum class transfer_type
		{
			move, copy
		};

		enum class transfer_operation
		{
			construct, assign
		};

		template<transfer_type ttype,transfer_operation top>
		constexpr static bool noexcept_transfer = 
			ttype == transfer_type::copy ?
				top == transfer_operation::construct ?
					nx_construct<const T&> :
					nx_T::cp_asgn
					:
				top == transfer_operation::construct ?
					nx_construct<T&&> :
					nx_T::mv_asgn
		;


		template<transfer_type ttype>
		using transfer_ptr_t = std::conditional_t<ttype == transfer_type::copy, const T*, T*>;

		template<transfer_type ttype, transfer_operation top = transfer_operation::construct>
		static constexpr void transfer
		(
			transfer_ptr_t<ttype> src_begin,
			const transfer_ptr_t<ttype> src_end,
			T* dest
		) noexcept(noexcept_transfer<ttype,top>)
		{
			assert(src_begin <= src_end);
			for (; src_begin < src_end; ++src_begin, ++dest)
			{
				if constexpr (top == transfer_operation::construct)
				{
					if constexpr (ttype == transfer_type::copy)
					{
						construct(dest,*src_begin);
					}
					else
					{
						construct(dest,std::move(*src_begin));
					}
				}
				else
				{
					if constexpr (ttype == transfer_type::copy)
					{
						*dest = *src_begin;
					}
					else
					{
						*dest = std::move(*src_begin);
					}
				}
			}
		}

	public:
		using iterator = T*;
		using const_iterator = const T*;

		[[nodiscard]] constexpr vector() noexcept = default;

		[[nodiscard]] constexpr vector( const vector& other, const Allocator& alloc ) noexcept(nx_T::cp_ctor && nx_Alloc::alloc)
			requires std::copy_constructible<Allocator>
			: data_(allocate(other.size_)), size_(other.size_), capacity_(other.size_), alloc_(alloc)
		{
			transfer<transfer_type::copy, transfer_operation::construct>(other.begin(), other.end(), data_);
		}

		[[nodiscard]] constexpr vector(const vector& other) noexcept(nx_T::cp_ctor && nx_Alloc::alloc)
			: vector( other, alloc_traits::select_on_container_copy_construction(other.alloc_) )
		{ }

		[[nodiscard]] constexpr vector& operator = (const vector& other) noexcept(nx_T::cp_asgn && nx_T::cp_ctor && nx_Alloc::cp_asgn && nx_Alloc::alloc)
			requires std::copy_constructible<Allocator>
		{
			if (capacity_ > other.size_)
			{
				//copy assign as much as we can
				transfer<transfer_type::copy, transfer_operation::assign>(other.data_, other.data_ + size_, data_);
				if (size_ <= other.size_)
				{
					//copy construct the rest
					transfer<transfer_type::copy, transfer_operation::construct>(other.data_ + size_, other.data_ + other.size_, data_ + size_);
				}
				else // size_ > other.size_
				{
					//destroy remaining elements
					destroy(data_ + other.size_, data_ + size_);
				}
			}
			else
			{
				destroy(begin(), end());
				deallocate(data_, capacity_);
				data_ = allocate(other.size_);
				transfer<transfer_type::copy, transfer_operation::construct>(other.begin(), other.end(), data_);
			}

			size_ = other.size_;
			capacity_ = other.capacity_;
			if constexpr ( alloc_traits::propagate_on_container_copy_assignment::value )
			{
				alloc_ = other.alloc_
			}
		}

		[[nodiscard]] constexpr vector(vector&& other, const Allocator& alloc ) noexcept(nx_Alloc::cp_ctor)
			requires std::copy_constructible<Allocator>
			: data_{ std::exchange(other.data_, nullptr) },
			size_{ std::exchange(other.size_, 0) },
			capacity_{ std::exchange(other.capacity_, 0) },
			alloc_{ alloc }
		{ }

		[[nodiscard]] constexpr vector(vector&& other) noexcept(nx_Alloc::mv_ctor)
			: data_{ std::exchange(other.data_, nullptr) },
			size_{ std::exchange(other.size_, 0) },
			capacity_{ std::exchange(other.capacity_, 0) },
			alloc_{ std::move(other.alloc_) }
		{ }

		[[nodiscard]] constexpr vector& operator = (vector&& other) noexcept(nx_Alloc::mv_asgn)
		{
			size_ = std::exchange(other.size_, 0);
			if constexpr ( alloc_traits::always_equal )
			{
				destroy(begin(),end());
				swap( data_, other.data_);
				swap( capacity_, other.capacity_ );
			}
			else if constexpr ( alloc_traits::propagate_on_container_move_assignment  )
			{
				destroy(begin(),end());
				swap(data_, other.data_ );
				swap( capacity_, other.capacity_ );
				swap( alloc_, other.alloc_ );
			}
			else
			{
				free();
				data_ = allocate( other.size_ );
				capacity_ = other.size_;

			}

		}

		[[nodiscard]] constexpr vector(const vector&&) = delete;

		[[nodiscard]] constexpr vector( std::input_iterator auto begin, std::input_iterator auto end, Allocator alloc = {} ) noexcept(nx_reserve && nx_Alloc::alloc)
			: alloc_(std::move(alloc))
		{
			static_assert( std::is_constructible_v<T,decltype(*begin)> );
			if constexpr ( std::random_access_iterator<decltype(begin)> )
			{
				reserve( std::distance( begin, end ) );
			}
			for ( ; begin != end; ++begin )
			{
				emplace_back( *begin );
			}
		}

		constexpr ~vector() noexcept(nx_T::dtor && nx_Alloc::dtor && nx_Alloc::dealloc)
		{
			destroy(begin(), end());
			deallocate(data_,capacity_);
		}

		constexpr void reserve(std::size_t new_capacity) noexcept(nx_reserve)
		{
			if ( new_capacity <= capacity_)
			{ return; }

			auto new_data = allocate(new_capacity);
			transfer<transfer_type::move, transfer_operation::construct>(begin(), end(), new_data);
			destroy(begin(), end());
			deallocate(data_,capacity_);
			data_ = new_data;
			capacity_ = new_capacity;
		}

		constexpr void shrink_to_fit() noexcept(nx_reserve)
		{
			if ( capacity_ > size_ )
			{
				auto new_data = allocate( size_ );
				transfer<transfer_type::move,transfer_operation::construct>( begin(), end(), new_data );
				destroy(begin(),end());
				deallocate(data_,capacity_);
				data_ = new_data;
				capacity_ = size_;
			}
		}

		[[nodiscard]] constexpr iterator begin() noexcept { return data_; }
		[[nodiscard]] constexpr iterator end() noexcept { return data_ + size_; }

		[[nodiscard]] constexpr const_iterator begin() const noexcept { return data_; }
		[[nodiscard]] constexpr const_iterator end() const noexcept { return data_ + size_; }

		[[nodiscard]] constexpr std::size_t size() const noexcept
		{ return size_; }

		[[nodiscard]] constexpr std::size_t capacity() const noexcept
		{ return capacity_; }

		[[nodiscard]] constexpr bool empty() const noexcept
		{ return size_ == 0; }

		[[nodiscard]] constexpr T& operator [] (const std::size_t idx) noexcept
		{
			assert(idx < size_);
			return data_[idx];
		}

		[[nodiscard]] constexpr const T& operator [] (const std::size_t idx) const noexcept
		{
			assert(idx < size_);
			return data_[idx];
		}

		template<typename ...Ts>
		constexpr T& emplace_back( Ts&& ... args ) noexcept( nx_reserve && nx_T::template ctor<Ts...> )
			requires std::constructible_from<T,Ts...>
		{
			if (size_ == capacity_)
			{
				const std::size_t new_capacity = std::max( min_capacity, static_cast<std::size_t>(capacity_ * 1.5) );
				reserve(new_capacity);
			}
			++size_;
			return construct( data_+size_-1, std::forward<Ts>(args) ... );
		}

		constexpr void free() noexcept( nx_T::dtor && nx_Alloc::dealloc )
		{
			destroy(begin(),end());
			deallocate(data_);
			data_ = nullptr;
			size_ = 0;
			capacity_ = 0;
		}

		constexpr void erase( iterator begin_, iterator end_ ) noexcept(nx_T::dtor && nx_T::mv_ctor)
		{
			assert( begin_ >= begin() );
			assert( begin_ <= end() );
			assert( end_ >= begin() );
			assert( end_ <= end() );
			assert( begin_ <= end_ );

			const std::size_t delete_count = end_ - begin_;
			transfer<transfer_type::move,transfer_operation::assign>( end_, end(), begin_ );
			destroy(end_,end_+delete_count);

			size_ -= delete_count;
		}

		constexpr void erase( const std::size_t idx ) noexcept(nx_T::dtor && nx_T::swap)
		{
			assert( idx < size_ );
			erase( begin()+idx,begin()+idx+1 );
		}

		friend std::ostream& operator << ( std::ostream& os , const vector& vec )
			requires requires ( std::ostream& os, const T& t ){ os << t; }
		{
			if ( vec.empty() )
			{
				os << "{ }";
			}
			else
			{
				os << "{ ";
				for ( auto it = vec.begin(); it < vec.end()-1; ++it )
				{
					os << *it << ", ";
				}
				os << *(vec.end()-1) << " }";
			}
			return os;
		}

		friend auto operator <=> ( const vector& lhs, const vector& rhs )
			requires std::three_way_comparable<T>
		{
			if ( &lhs == &rhs )
			{ return true; }
			
		}
	};
}