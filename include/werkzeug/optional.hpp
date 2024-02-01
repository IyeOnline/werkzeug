#ifndef WERKZEUG_GUARD_OPTIONA_HPP
#define WERKZEUG_GUARD_OPTIONA_HPP

#include <cassert>
#include <memory>
#include <type_traits>
#include <utility>

#include "werkzeug/traits.hpp"


namespace werkzeug
{
	template<typename T>
	class optional
	{
		using traits = type_traits<T>;

		template<typename ... Params>
		constexpr static bool constructible = std::is_constructible_v<T,Params...>;
		template<typename ... Params>
		constexpr static bool nothrow_constructible = std::is_nothrow_constructible_v<T,Params...>;

		struct empty_struct
		{
			bool has_value_;

			operator bool() const
			{
				return has_value_;
			}
		};
		struct struct_with_T
		{
			bool has_value_;
			T value_;

			operator T&() & noexcept
			{
				return value_;
			}

			operator const T&() const& noexcept
			{
				return value_;
			}

			operator T&&() && noexcept
			{
				return value_;
			}
		};

		union
		{
			empty_struct no = { false };
			struct_with_T yes;
		};
		

	public :
		constexpr optional() noexcept = default;

		template<typename ... Params>
			requires ( constructible<Params...> )
		constexpr optional( std::in_place_t, Params&& ... params ) noexcept( nothrow_constructible<Params...> )
			: yes{ .has_value_=true, .value_=std::forward<Params>( params ) ... }
		{}

		
		template<typename S>
			requires std::is_constructible_v<T,S>
		constexpr optional( S&& s )
			: yes{ .has_value_=true, .value_=std::forward<S>( s ) }
		{}

		template<typename S>
			requires std::is_assignable_v<T,S>
		constexpr optional& operator = ( S&& s )
		{
			asign( std::forward<S>(s) );
			return *this;
		}

		constexpr optional( const optional& other ) noexcept( traits::copy_construct::nothrow )
			requires ( traits::copy_construct::possible )
			: no{ other.has_value() }
		{
			if ( has_value() )
			{
				emplace(other.yes.value_);
			}
		}

		constexpr optional& operator = ( const optional& other ) noexcept( traits::copy_construct::nothrow && traits::copy_assign::nothrow )
			requires ( traits::copy_construct::possible )
		{
			if ( other.has_value_ )
			{
				asign( other.yes.value_ );
			}
			else
			{
				clear();
			}
		}

		constexpr optional( optional&& other ) noexcept( traits::move_construct::nothrow )
			requires ( traits::move_construct::possible )
			: no{ other.has_value() }
		{
			if ( has_value() )
			{
				emplace(std::move(other.yes.value_));
			}
		}

		constexpr optional& operator = ( optional&& other ) noexcept( traits::move_construct::nothrow && traits::move_assign::nothrow )
			requires ( traits::move_assign::possible )
		{
			if (other.has_value_)
			{
				asign( std::move(other.value_) );
			}
			else
			{
				clear();
			}
		}

		constexpr ~optional() noexcept( traits::destroy::nothrow )
		{
			clear();
		}

		template<typename Params>
		constexpr void asign( Params&& params )
		{
			if constexpr ( std::is_assignable_v<T,decltype(params)> )
			{
				if ( has_value() )
				{
					yes.value_ = T{ std::forward<Params>(params) };
					return;
				}
			}
			
			emplace( std::forward<Params>(params) );
		}

		template<typename ... Params>
		constexpr void emplace( Params&& ... params ) noexcept( nothrow_constructible<Params...> && traits::destroy::nothrow )
		{
			clear();
			std::construct_at( &yes.value_, std::forward<Params>(params)... );
			yes.has_value_ = true;
		}

		constexpr bool has_value() const noexcept
		{
			return no.has_value_;
		}

		constexpr auto& value() noexcept
		{
			assert( has_value() );
			return yes.value_;;
		}

		constexpr const auto& value() const noexcept
		{
			assert( has_value() );
			return yes.value_;
		}

		constexpr const auto& value_or( const T& alternative ) const noexcept
		{
			if ( has_value() )
			{
				return yes.value_;
			}
			else
			{
				return alternative;
			}
		}

		constexpr auto value_or( T&& alternative ) && noexcept
		{
			if ( has_value() )
			{
				return std::move(yes.value_);
			}
			else
			{
				return std::move(alternative);
			}
		}

		constexpr void clear() noexcept ( traits::destroy::nothrow )
		{
			if ( has_value() )
			{
				std::destroy_at( &yes.value_ );
			}
			no.has_value_ = false;
		}
	};
}

#endif
