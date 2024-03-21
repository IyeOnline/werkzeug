#ifndef WERKZEUG_GUARD_MANUAL_LIFETIME_HPP
#define WERKZEUG_GUARD_MANUAL_LIFETIME_HPP

#include "werkzeug/assertion.hpp"
#include <type_traits>
#include <utility>
#include <memory>
#include <cassert>

namespace werkzeug
{
	template<typename T>
		requires ( not std::is_array_v<T> or std::is_bounded_array_v<T> )
	union Manual_Lifetime
	{
		constexpr static bool is_array = std::is_array_v<T>;

		char _init{};
		T value;

		constexpr Manual_Lifetime() noexcept
			: _init{}
		{}
		constexpr ~Manual_Lifetime() noexcept
			requires std::is_trivially_destructible_v<T>
		= default;

		constexpr ~Manual_Lifetime() noexcept
			requires ( not std::is_trivially_destructible_v<T> )
		{}

		constexpr Manual_Lifetime( const T& value_ )
			: value{ value_ }
		{ }

		constexpr Manual_Lifetime( T&& value_ )
			: value{ std::move(value_) }
		{ }

		template<typename ... Args>
		constexpr Manual_Lifetime( Args&& ... args )
			: value{ std::forward<Args>(args)... }
		{ }

		template<typename ... Args>
			requires ( not is_array )
		T& emplace( Args&& ... args )
		{
			return *std::construct_at( &value, std::forward<Args>(args) ... );
		}

		template<typename ... Args>
			requires is_array
		auto& emplace_at( const std::size_t index, Args&& ... args )
		{
			WERKZEUG_ASSERT( index < std::size(value), "index must be in range" );

			return *std::construct_at( begin()+index, std::forward<Args>(args) ...  );
		}

		auto begin() noexcept
			requires is_array
		{
			return std::begin(value);
		}

		auto begin() const noexcept
			requires is_array
		{
			return std::begin(value);
		}

		auto end() noexcept
			requires is_array
		{
			return std::end(value);
		}

		auto end() const noexcept
			requires is_array
		{
			return std::end(value);
		}

		auto& operator[]( const std::size_t idx ) noexcept
			requires is_array
		{
			return value[idx];
		}

		const auto& operator[]( const std::size_t idx ) const noexcept
			requires is_array
		{
			return value[idx];
		}


		void destroy()
		{
			std::destroy_at( &value );
		}
	};
}

#endif