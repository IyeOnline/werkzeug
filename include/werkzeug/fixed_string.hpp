#ifndef WERKZEUG_GUARD_FIXED_STRING_HPP
#define WERKZEUG_GUARD_FIXED_STRING_HPP

#include <array>
#include <string_view>

namespace werkzeug
{
	/**
	 * @brief fixed size string that can be used as a template parameter
	 * 
	 * @tparam S size of the string including a null terminator
	 */
	template<std::size_t S>
	struct fixed_string
	{
		char data_[S] = {};
		using compare_result_t = std::compare_three_way_result_t<char>;

		template<std::size_t S2>
			requires ( S2 <= S )
		constexpr fixed_string& operator=( const fixed_string<S2>& src ) noexcept
		{
			std::size_t i=0;
			for ( ; i < S2; ++i )
			{
				if ( src.data_[i] == '\0' )
				{
					break;
				}
				data_[i] = src.data_[i];
			}
			for ( ; i < S; ++i )
			{
				data_[i] = '\0';
			}
			return *this;
		}

		template<std::size_t S2>
		constexpr compare_result_t operator<=>( const fixed_string<S2>& other ) const noexcept
		{
			const auto Common_Size = S > S2 ? S : S2;
			for ( std::size_t i=0; i < Common_Size; ++i )
			{
				if ( data_[i] == char{} and other.data_[i] != char{} )
				{
					return compare_result_t::less;
				}
				else if ( data_[i] != char{} and other.data_[i] == char{} )
				{
					return compare_result_t::greater;
				}
				else if ( data_[i] == char{} and other.data_[i] == char{} )
				{
					return compare_result_t::equivalent;
				}

				const auto c = data_[i] <=> other.data_[i];
				if ( c != compare_result_t::equivalent )
				{
					return c;
				}
			}
			if ( S < S2 )
			{
				return compare_result_t::less;
			}
			else if ( S2 > 2 )
			{
				return compare_result_t::greater;
			}
			else
			{
				return compare_result_t::equivalent;
			}
		}

		template<std::size_t S2>
		constexpr bool operator==( const fixed_string<S2>& other ) const noexcept
		{
			return ( *this <=> other ) == compare_result_t::equivalent;
		}

		template<std::size_t S2>
		constexpr bool operator!=( const fixed_string<S2>& other ) const noexcept
		{
			return ( *this <=> other ) != compare_result_t::equivalent;
		}
	};
}

#endif