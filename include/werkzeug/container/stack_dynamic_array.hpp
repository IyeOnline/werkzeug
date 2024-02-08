#ifndef WERKZEUG_GUARD_CONTAINER_STATIC_ARRAY_HPP
#define WERKZEUG_GUARD_CONTAINER_STATIC_ARRAY_HPP

#include <cstddef>
#include <type_traits>

#include "werkzeug/container/crtp_range_bases.hpp"
#include "werkzeug/assertion.hpp"
#include "werkzeug/manual_lifetime.hpp"
#include "werkzeug/traits.hpp"
#include "werkzeug/iterator.hpp"
#include "werkzeug/algorithm/range_transfer.hpp"

namespace werkzeug
{
	template<typename T, std::size_t Capacity>
	class stack_dynamic_array
		: public detail::Range_Stream_CRTP_Base<stack_dynamic_array<T,Capacity>>
		, public detail::Range_Threeway_CRTP_Base<stack_dynamic_array<T, Capacity>, T>
	{
		Manual_Lifetime<T[Capacity]> storage_;
		T* end_ = std::begin(storage_);

		using traits = type_traits<T>;

		enum class tag{};

	public :
		using value_type = T;
		using iterator = Tagged_Iterator_Wrapper<T*,tag>;
		using const_iterator = Tagged_Iterator_Wrapper<const T*, tag>;
		using reverse_iterator = Reverse_Iterator_Wrapper<iterator>;
		using const_reverse_iterator = Reverse_Iterator_Wrapper<const_iterator>;

		constexpr stack_dynamic_array() = default;

		constexpr stack_dynamic_array( const stack_dynamic_array& )
			requires traits::copy_construct::trivial
		= default;

		template<std::size_t Other_Capacity>
		constexpr stack_dynamic_array( const stack_dynamic_array<T,Other_Capacity>& src )
			noexcept ( traits::copy_construct::nothrow )
			requires ( not traits::copy_construct::trivial or Other_Capacity <= Capacity )
		{
			transfer_range_with_fallback<transfer_type::copy, transfer_operation::construct>(src.begin(), src.end(), this->begin() );
			end_ = std::begin(storage_) + src.size();
		}

		constexpr stack_dynamic_array( stack_dynamic_array&& )
			requires traits::move_construct::trivial
		= default;

		template<std::size_t Other_Capacity>
		constexpr stack_dynamic_array( stack_dynamic_array<T,Other_Capacity>&& src )
			noexcept ( traits::move_construct::nothrow )
			requires ( not traits::move_construct::trivial or Other_Capacity <= Capacity )
		{
			transfer_range_with_fallback<transfer_type::move, transfer_operation::construct>(src.begin(), src.end(), this->begin() );
			end_ = std::begin(storage_) + src.size();
		}


		constexpr stack_dynamic_array& operator=( const stack_dynamic_array& )
			requires traits::copy_assign::trivial
		= default;
		constexpr stack_dynamic_array& operator=( stack_dynamic_array&& )
			requires traits::move_assign::trivial
		= default;

		template<std::size_t Other_Capacity>
		constexpr stack_dynamic_array& operator=( const stack_dynamic_array<T,Other_Capacity>& src )
			noexcept( traits::copy_assign::nothrow )
			requires ( not traits::copy_assign::trivial or Other_Capacity <= Capacity )
		{
			transfer_range_with_fallback<transfer_type::copy, transfer_operation::assign>(src.begin(), src.begin()+this->size(), this->begin() );
			transfer_range_with_fallback<transfer_type::copy, transfer_operation::construct>( src.begin() + this->size(), src.end(), this->begin() + this->size()+1 );
			destroy_range( this->begin()+src.size(), this->end() );
			end_ = std::begin(storage_) + src.size();
		}

		template<std::size_t Other_Capacity>
		constexpr stack_dynamic_array& operator=( stack_dynamic_array<T,Other_Capacity>&& src )
			noexcept( traits::copy_assign::nothrow )
			requires ( not traits::copy_assign::trivial or Other_Capacity <= Capacity )
		{
			transfer_range_with_fallback<transfer_type::move, transfer_operation::assign>(src.begin(), src.begin()+this->size(), this->begin() );
			transfer_range_with_fallback<transfer_type::move, transfer_operation::construct>( src.begin() + this->size(), src.end(), this->begin() + this->size()+1 );
			destroy_range( this->begin()+src.size(), this->end() );
			end_ = std::begin(storage_) + src.size();
		}


		constexpr ~stack_dynamic_array()
			requires traits::destroy::trivial
		= default;

		constexpr ~stack_dynamic_array()
			noexcept( traits::destroy::nothrow )
			requires( not traits::destroy::trivial )
		{
			destroy_range( begin(), end() );
		}

		template<std::constructible_from<T> ... Elements>
			requires ( sizeof...(Elements) == Capacity )
		constexpr stack_dynamic_array( Elements&& ... elements )
			: storage_{ std::forward<Elements>(elements)... }, end_{ std::end(storage_) }
		{ }


		constexpr void clear() noexcept( traits::destroy::nothrow )
		{
			destroy_range( begin(), end() );
			end_ = std::begin(storage_);
		}

		constexpr T* data() noexcept { return std::begin(storage_); }
		constexpr const T* data() const noexcept { return std::begin(storage_); }

		[[nodiscard]] constexpr iterator begin() noexcept { return data(); }
		[[nodiscard]] constexpr iterator end() noexcept { return end_; }

		[[nodiscard]] constexpr const_iterator begin() const noexcept { return data(); }
		[[nodiscard]] constexpr const_iterator end() const noexcept { return end_; }

		[[nodiscard]] constexpr reverse_iterator rbegin() noexcept { return end()-1; }
		[[nodiscard]] constexpr reverse_iterator rend() noexcept { return begin()-1; }

		[[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept { return end()-1; }
		[[nodiscard]] constexpr const_reverse_iterator rend() const noexcept { return begin()-1; }

		[[nodiscard]] constexpr std::ptrdiff_t size() const noexcept { return end_ - std::begin(storage_); }
		[[nodiscard]] consteval static std::ptrdiff_t capacity() noexcept { return Capacity; }

		[[nodiscard]] constexpr T& operator[]( const std::ptrdiff_t i ) noexcept
		{
			WERKZEUG_ASSERT( i >= 0 and i < size(), "Index must be valid" );

			return storage_[static_cast<std::size_t>(i)];
		}
		[[nodiscard]] constexpr const T& operator[]( const std::ptrdiff_t i ) const noexcept
		{
			WERKZEUG_ASSERT( i >= 0 and i < size(), "Index must be valid" );

			return storage_[static_cast<std::size_t>(i)];
		}

		constexpr void grow_for_overwrite( std::ptrdiff_t new_size ) noexcept
		{
			WERKZEUG_ASSERT(  new_size < Capacity, "Requested size must be less than capacity" );

			if ( new_size > size() )
			{
				end_ = std::begin(storage_) + new_size;
			}
		}

		template<typename ... Args>
		constexpr T& emplace_back( Args&& ... args )
			noexcept ( std::is_nothrow_constructible_v<T, Args...> )
			requires ( std::is_constructible_v<T,Args...> )
		{
			WERKZEUG_ASSERT( size() < capacity(), "Current size must be less than capacity" );
			T& ref = *std::construct_at( end_, std::forward<Args>(args) ... );
			++end_;
			return ref;
		}

		template<typename ... Args>
		constexpr T& emplace_at( iterator it, Args&& ... args )
			noexcept ( std::is_nothrow_constructible_v<T, Args...> )
			requires ( std::is_constructible_v<T,Args...> )
		{
			if ( it - begin() == size() )
			{
				return emplace_back( std::forward<Args>(args) ... );
			}
			WERKZEUG_ASSERT( size() < capacity(), "Current size must be less than capacity" );

			std::construct_at(end_+1, std::move(*end_) ); //move construct last element
			transfer_range_with_fallback<transfer_type::move,transfer_operation::assign>( rbegin()+1, reverse_iterator{ std::addressof(*(it-1)) }, rbegin() ); //move the rest backwards/to new block
			std::destroy_at( std::addressof(*it) );

			T& ref = *std::construct_at( std::addressof(*it), std::forward<Args>(args) ... );
			++end_;
			return ref;
		}
	};

}

#endif