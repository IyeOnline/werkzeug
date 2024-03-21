#ifndef WERKZEUG_GUARD_STRING_HPP
#define WERKZEUG_GUARD_STRING_HPP

#include <iosfwd>

#include <concepts>

#include "werkzeug/container/dynamic_array.hpp"



namespace werkzeug
{
	template<typename CharT>
	concept character_type = 
		std::same_as<CharT,char> or 
		std::same_as<CharT,signed char> or
		std::same_as<CharT,unsigned char> or
		std::same_as<CharT,wchar_t> or
		std::same_as<CharT,char16_t> or
		std::same_as<CharT,char32_t> or
		std::same_as<CharT,char8_t>
	;

	template<
		character_type CharT = char,
		memory::concepts::memory_source Resource = memory::resource::fixed::New_Resource
	>
	class basic_string 
		: public dynamic_array_sso<CharT, Resource>
	{

		using base = dynamic_array_sso<CharT, Resource>;
	public :
		constexpr static std::size_t npos = static_cast<std::size_t>(-1);


		explicit basic_string( const char* input )
			: base{ input, input + std::strlen(input)+1 }
		{ }

		explicit basic_string( std::string_view sv )
			: base{ sv.begin(), sv.end() }
		{ }

		explicit basic_string( const base& b )
			: base{ b }
		{ }

		explicit basic_string( base&& b )
			: base{ std::move(b) }
		{ }

		explicit operator const base&() const & noexcept
		{
			return *this;
		}

		explicit operator base() const & noexcept
		{
			return *this;
		}

		explicit operator base() && noexcept
		{
			return std::move(*this);
		}

		std::size_t find( std::size_t pos, CharT c ) const
		{
			if ( pos > this->size() )
			{
				return npos;
			}
			for ( size_t i=pos; i != this->size(); ++i )
			{
				if ( this->operator[](i) == c )
				{
					return i;
				}
			}

			return npos;
		}

		std::size_t find( CharT c ) const
		{
			return find( 0, c );	
		}

		template<typename Traits>
		friend auto& operator<<( std::basic_ostream<CharT, Traits>& os, const basic_string& str )
		{
			os.write(str.data(),static_cast<std::streamsize>(str.size()) );
			return os;
		}

		template<typename Traits>
		friend auto& operator>>( std::basic_istream<CharT, Traits>& is, basic_string& str )
		{
			using stream_t = std::remove_cvref_t<decltype(is)>;
			const auto w_space = is.widen(' ');
			const auto w_tab = is.widen('\t');
			const auto w_nl = is.widen('\n');
			constexpr auto is_separator = [w_space,w_tab,w_nl](const CharT c)
			{
				return c == w_space or c == w_tab or c == w_nl;
			};
			typename stream_t::pos_t old_pos = is.tellg();
			typename stream_t::dist_t dist = 0;
			while ( is.good() and not is_separator(is.peek()) )
			{
				++dist;
				is.seekg(old_pos+dist);
			}
			is.clear();
			is.seekg(old_pos);

			str.clear();
			str.grow_for_overwrite(dist);
			is.getline( str.data(), dist );

			return is;
		}
	};

	using string = basic_string<>;
}

#endif