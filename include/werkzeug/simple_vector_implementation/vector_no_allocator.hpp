#pragma once
#include <cassert>
#include <cstddef>
#include <type_traits>
#include <new>
#include <utility>
#include <iterator>

#include "werkzeug/concepts.hpp"

namespace custom_STL
{
	template<typename T>
	class vector
	{
		constexpr static std::align_val_t alignment{ alignof(T) };
		constexpr static std::size_t min_capacity = 4;

		constexpr static bool nx_T_cp_ctor = std::is_nothrow_copy_constructible_v<T>;
		constexpr static bool nx_T_cp_asgn = std::is_nothrow_copy_assignable_v<T>;

		constexpr static bool nx_mv_ctor = std::is_nothrow_move_constructible_v<T>;
		constexpr static bool nx_mv_asgn = std::is_nothrow_move_assignable_v<T>;

		constexpr static bool nx_dtor = std::is_nothrow_destructible_v<T>;

		constexpr static bool nx_swap = std::is_nothrow_swappable_v<T>;

		T* data_ = nullptr;
		std::size_t size_ = 0;
		std::size_t capacity_ = 0;

		static T* allocate(std::size_t size)
		{
			return  static_cast<T*>( ::operator new(size * sizeof(T), alignment ) );
		}

		static void deallocate(T* ptr)
		{
			::operator delete((void*)ptr);
		}

		static void destroy(T* begin_, T* end_) noexcept(nx_dtor)
		{
			if constexpr ( not std::is_trivially_destructible_v<T> )
			{
				for (; begin_ < end_; ++begin_)
				{
					begin_->~T();
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
					nx_T_cp_ctor :
					nx_T_cp_asgn
					:
				top == transfer_operation::construct ?
					nx_mv_ctor :
					nx_mv_asgn
		;


		template<transfer_type ttype>
		using transfer_ptr_t = std::conditional_t<ttype == transfer_type::copy, const T*, T*>;

		template<transfer_type ttype, transfer_operation top = transfer_operation::construct>
		static void transfer
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
						new(dest) T(*src_begin);
					}
					else
					{
						new(dest) T(std::move(*src_begin));
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

		vector() noexcept = default;

		vector(const vector& other)
			: data_(allocate(other.size_)), size_(other.size_), capacity_(other.size_)
		{
			transfer<transfer_type::copy, transfer_operation::construct>(other.begin(), other.end(), data_);
		}

		vector& operator = (const vector& other)
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
				deallocate(data_);
				data_ = allocate(other.size_);
				transfer<transfer_type::copy, transfer_operation::construct>(other.begin(), other.end(), data_);
			}

			size_ = other.size_;
			capacity_ = other.capacity_;
		}

		vector(vector&& other) noexcept
			: data_{ std::exchange(other.data_, nullptr) },
			size_{ std::exchange(other.size_, 0) },
			capacity_{ std::exchange(other.capacity_, 0) }
		{ }

		vector& operator = (vector&& other) noexcept
		{
			data_ = std::exchange(other.data_, nullptr);
			size_ = std::exchange(other.size_, 0);
			capacity_ = std::exchange(other.capacity_, 0);
		}

		vector(const vector&&) = delete;

		template<std::size_t S>
		vector( T (&&input)[S] )
		{
			reserve( S );
			for ( auto& v : input )
			{
				emplace_back( std::move(v) );
			}
		}

		vector( forward_iterator<T> auto begin, const end_iterator_for_iterator<decltype(begin)> auto end )
			requires std::constructible_from<T,decltype(*begin)>
		{
			if constexpr ( random_access_iterator<decltype(begin)> )
			{
				reserve( std::distance( begin, end ) );
			}
			for ( ; begin != end; ++begin )
			{
				emplace_back( *begin );
			}
		}

		~vector() noexcept(nx_dtor)
		{
			destroy(begin(), end());
			deallocate(data_);
		}

		void reserve(std::size_t new_capacity)
		{
			if ( new_capacity <= capacity_)
			{ return; }

			auto new_data = allocate(new_capacity);
			transfer<transfer_type::move, transfer_operation::construct>(begin(), end(), new_data);
			destroy(begin(), end());
			deallocate(data_);
			data_ = new_data;
			capacity_ = new_capacity;
		}

		void shrink_to_fit()
		{
			if ( capacity_ > size_ )
			{
				auto new_data = allocate( size_ );
				transfer<transfer_type::move,transfer_operation::construct>( begin(), end(), new_data );
				destroy(begin(),end());
				deallocate(data_);
				data_ = new_data;
				capacity_ = size_;
			}
		}

		[[nodiscard]] iterator begin() noexcept { return data_; }
		[[nodiscard]] iterator end() noexcept { return data_ + size_; }

		[[nodiscard]] const_iterator begin() const noexcept { return data_; }
		[[nodiscard]] const_iterator end() const noexcept { return data_ + size_; }

		[[nodiscard]] std::size_t size() const noexcept
		{ return size_; }

		[[nodiscard]] std::size_t capacity() const noexcept
		{ return capacity_; }

		[[nodiscard]] T& operator [] (const std::size_t idx) noexcept
		{
			assert(idx < size_);
			return data_[idx];
		}

		[[nodiscard]] const T& operator [] (const std::size_t idx) const noexcept
		{
			assert(idx < size_);
			return data_[idx];
		}

		template<typename ...Ts>
		T& emplace_back( Ts&& ... args )
		{
			if (size_ == capacity_)
			{
				const std::size_t new_capacity = std::max( min_capacity, static_cast<std::size_t>(capacity_ * 1.5) );
				reserve(new_capacity);
			}
			new (data_ + size_) T{ std::forward<Ts>(args) ... };
			++size_;
			return data_[size_ - 1];
		}

		void erase( iterator begin_, iterator end_ ) noexcept(nx_dtor && nx_swap)
		{
			assert( begin_ >= begin() );
			assert( begin_ <= end() );
			assert( end_ >= begin() );
			assert( end_ <= end() );
			assert( begin_ <= end_ );

			const std::size_t count = end_ - begin_;
			for ( ; begin_ != end_; ++begin_ )
			{
				swap( *begin_, *(begin_+count) );
			}
			destroy( end_, end() );
			size_ -= count;
		}

		void erase( const std::size_t idx ) noexcept(nx_dtor && nx_swap)
		{
			assert( idx < size_ );
			erase( begin()+idx,begin()+idx+1 );
		}
	};
}