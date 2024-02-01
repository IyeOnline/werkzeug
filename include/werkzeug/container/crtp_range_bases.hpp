#ifndef WERKZEUG_GUARD_STREAM_HPP
#define WERKZEUG_GUARD_STREAM_HPP

#include <iostream>
#include <format>

#include "werkzeug/iterator.hpp"
#include "werkzeug/concepts.hpp"
#include "werkzeug/algorithm/sorting.hpp"

namespace werkzeug
{
	namespace detail
	{
		template<typename Concrete>
		struct Range_Stream_CRTP_Base
		{ };

		template<typename Concrete>
		std::ostream& operator<<( std::ostream& os, const Range_Stream_CRTP_Base<Concrete>& container )
			requires range_streamable<Concrete>
		{
			os << "{ ";
			for ( const auto& value : static_cast<const Concrete&>(container) )
			{
				os << value << ' ';
			}
			os << '}';
			return os;
		}

		template<typename Concrete, typename Value_Type>
		struct Range_Threeway_CRTP_Base
		{ };

		template<typename Concrete, typename Value_Type>
		[[nodiscard]] auto operator<=>( const Range_Threeway_CRTP_Base<Concrete,Value_Type>& list, const range_of_type<Value_Type> auto& range )
			noexcept ( type_traits<Value_Type>::three_way_compare::nothrow )
			requires ( type_traits<Value_Type>::three_way_compare::possible )
		{
			using result_t = std::compare_three_way_result_t<Value_Type>;

			const auto& concrete_list = static_cast<const Concrete&>(list);
			
			auto i1 = concrete_list.begin();
			const auto e1 = concrete_list.end();
			auto i2 = range.begin();
			const auto e2 = range.end();
			for ( ; i1 != e2 and i2 != e2; ++i1, ++i2 )
			{
				const auto result = *i1 <=> *i2;
				if ( result != result_t::equivalent )
				{
					return result;
				}
			}
			if ( i1 == e1 and i2 != e2 )
			{
				return result_t::less;
			}
			else if ( i1 != e1 and i2 == e2 )
			{
				return result_t::greater;
			}
			else
			{
				return result_t::equivalent;
			}
		}
	}
}

#endif