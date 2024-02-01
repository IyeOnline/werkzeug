#ifndef WERKZEUG_GUARD_EXPLICIT_HPP
#define WERKZEUG_GUARD_EXPLICIT_HPP

#include <concepts>
#include <utility>

#include "werkzeug/concepts.hpp"
#include "werkzeug/traits.hpp"

namespace werkzeug
{
	/**
	 * @brief Simple wrapper that does not allow implicit conversions
	 * 
	 * @tparam T 
	 */
	template<typename T>
	class explicit_
	{
		T value_{};

		using traits = type_traits<T>;

	public :
		constexpr explicit_() noexcept = default;

		template<typename U>
			requires lossless_conversion_from<T,U>
		constexpr explicit explicit_( U&& u ) noexcept
			: value_{ std::forward<U>(u) }
		{}

		template<typename U>
			requires lossless_conversion_from<T,U>
		explicit_& operator=( U&& u ) noexcept( noexcept(value_=std::forward<U>(u)) )
		{
			value_ = std::forward<U>(u);
			return *this;
		}

		constexpr operator T& () & noexcept
		{ return value_; }
		constexpr operator const T& () const & noexcept
		{ return value_; }
		constexpr operator T&& () && noexcept
		{ return std::move(value_); }
		constexpr operator const T&& () const && noexcept
		{ return std::move(value_); }

		template<typename U>
			requires ( not std::same_as<T,U> and lossless_conversion_from<U,T> )
		explicit operator U() const noexcept
		{ return value_; }

		constexpr operator auto () = delete;
	};
}

#endif