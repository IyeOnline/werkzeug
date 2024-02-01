#ifndef WERKZEUG_GUARD_ALGORITHM_RANGE_TRANSFER_HPP
#define WERKZEUG_GUARD_ALGORITHM_RANGE_TRANSFER_HPP

#include <concepts>
#include <iterator>
#include <cstring>

#include "werkzeug/concepts.hpp"


namespace werkzeug
{

	constexpr void destroy_range( 
		forward_iterator auto src_begin,
		const end_iterator_for_iterator<decltype(src_begin)> auto&& src_end
	) noexcept( std::is_nothrow_destructible_v<std::remove_cvref_t<decltype(*src_begin)>> )
	{
		using T = std::remove_cvref_t<decltype(*src_begin)>;
		if constexpr ( not std::is_trivially_destructible_v<T> )
		{
			for ( ; src_begin != src_end; ++src_begin )
			{
				src_begin->~T();
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

	template<typename T_src_, typename T_dest_, transfer_type ttype, transfer_operation top>
	struct transfer_traits
	{
		using T_src = std::remove_cvref_t<T_src_>;
		using T_dest = std::remove_cvref_t<T_dest_>;

		constexpr static std::pair<transfer_type,transfer_operation> _helper_() noexcept
		{
			if constexpr ( ttype == transfer_type::copy )
			{
				if constexpr ( top == transfer_operation::construct )
				{
					if constexpr  ( std::is_constructible_v<T_dest,const T_src&> )
					{
						return { transfer_type::copy, transfer_operation::construct };
					}
					else
					{
						static_assert( false && sizeof(T_src), "requested copy construction is not possible" );
					}
				}
				else if constexpr ( top == transfer_operation::assign )
				{
					if constexpr ( std::is_assignable_v<T_dest,const T_dest&> )
					{
						return { transfer_type::copy, transfer_operation::assign };
					}
					else if constexpr ( std::is_constructible_v<T_dest,const T_src&> )
					{
						return { transfer_type::copy, transfer_operation::construct };
					}
					else
					{
						static_assert( false && sizeof(T_src), "requested could not find suitable copy assign operation" );
					}
				}
			}
			else //move
			{
				if constexpr ( top == transfer_operation::construct )
				{
					if constexpr ( std::is_constructible_v<T_dest,T_src&&> )
					{
						return { transfer_type::move, transfer_operation::construct };
					}
					else if constexpr ( std::is_constructible_v<T_dest,const T_src&> )
					{
						return { transfer_type::copy, transfer_operation::construct };
					}
					else
					{
						static_assert( false && sizeof(T_src), "Cannot find suitable move construct operation" );
					}
				}
				else
				{
					if constexpr ( std::is_assignable_v<T_dest,T_src&&> )
					{
						return { transfer_type::move, transfer_operation::assign };
					}
					else if constexpr ( std::is_constructible_v<T_dest,T_src&&> )
					{
						return { transfer_type::move, transfer_operation::construct };
					}
					else if constexpr ( std::is_assignable_v<T_dest,const T_src&> )
					{
						return { transfer_type::copy, transfer_operation::assign };
					}
					else if constexpr ( std::is_constructible_v<T_dest,const T_src&> )
					{
						return { transfer_type::copy, transfer_operation::construct };
					}
					else
					{
						static_assert( false && sizeof(T_src), "Cannot find suitable move assign operation" );
					}
				}
			}
		}

		constexpr static transfer_type type = _helper_().first;
		constexpr static transfer_operation operation = _helper_().second;

		constexpr static bool trivial = std::is_same_v<T_src,T_dest> and 
			type == transfer_type::copy ?
				operation == transfer_operation::construct ?
					std::is_trivially_copy_constructible_v<T_src> :
					std::is_trivially_copy_assignable_v<T_src>
					:
				operation == transfer_operation::construct ?
					std::is_trivially_move_constructible_v<T_src> :
					std::is_trivially_move_assignable_v<T_src>
		;
		constexpr static bool nothrow = 
			type == transfer_type::copy ?
				operation == transfer_operation::construct ?
					std::is_nothrow_constructible_v<T_dest,const T_src&> :
					std::is_nothrow_assignable_v<T_dest,const T_src&>
					:
				operation == transfer_operation::construct ?
					std::is_nothrow_constructible_v<T_dest,T_src&&> :
					std::is_nothrow_assignable_v<T_dest,T_src&&>
		;
	};

	template<transfer_type type, transfer_operation operation>
	constexpr void transfer_range_with_fallback
	(
		forward_iterator auto src_begin,
		const end_iterator_for_iterator<decltype(src_begin)> auto&& src_end,
		forward_iterator_of_type<std::remove_cvref_t<decltype(*src_begin)>> auto dest_begin
	) noexcept( transfer_traits<std::remove_cvref_t<decltype(*src_begin)>,std::remove_cvref_t<decltype(*dest_begin)>,type,operation>::nothrow )
	{
		using T_src = std::remove_cvref_t<decltype(*src_begin)>;
		using T_dest = std::remove_cvref_t<decltype(*dest_begin)>;
		using traits = transfer_traits<T_src, T_dest,type,operation>;

		if constexpr ( std::contiguous_iterator<decltype(src_begin)> and std::contiguous_iterator<decltype(dest_begin)> and std::is_same_v<T_src,T_dest> and traits::trivial ) //memcpy is possible
		{
			const auto size = static_cast<ptrdiff_t>(sizeof(T_dest)) * (src_end-src_begin);
			if constexpr (  std::same_as<decltype(src_begin),decltype(src_end)> and std::same_as<decltype(src_begin),decltype(dest_begin)> ) // we can check for memcpy safety
			{
				if ( src_end >= src_begin and ( dest_begin >= src_end or dest_begin+size <= src_begin ) ) //check for overlap
				{
					T_dest* const dest = std::addressof(*dest_begin);
					const T_src* const src = std::addressof(*src_begin);
					std::memcpy( dest, src, size );
					return;
				}	
			}
		}

		for ( ; src_begin != src_end; ++src_begin, ++dest_begin )
		{
			if constexpr ( traits::operation == transfer_operation::construct)
			{
				if constexpr ( operation == transfer_operation::assign ) // destroy existing elements if we only fell back to construct,
				{
					std::destroy_at( std::addressof(*dest_begin) );
				}
				if constexpr ( traits::type == transfer_type::copy)
				{
					new(std::addressof(*dest_begin)) T_dest(*src_begin);
				}
				else // move
				{
					new(std::addressof(*dest_begin)) T_dest(std::move(*src_begin));
				}
			}
			else // assign
			{
				if constexpr ( traits::type == transfer_type::copy)
				{
					*dest_begin = *src_begin;
				}
				else // move
				{
					*dest_begin = std::move(*src_begin);
				}
			}
		}
	}
}

#endif