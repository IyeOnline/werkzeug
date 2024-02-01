#ifndef WERKZEUG_GUARD_ALGORITHM_SORTING_HPP
#define WERKZEUG_GUARD_ALGORITHM_SORTING_HPP

#include <compare>
#include <tuple>
#include <type_traits>
#include <utility>

#include "werkzeug/algorithm/invoke.hpp"

namespace werkzeug
{
	template<typename T>
	T negate_ordering( T t )
	{
		if ( t == T::greater )
		{
			return T::less;
		}
		else if ( t == T::less )
		{
			return T::greater;
		}
		else
		{
			return t;
		}
	}

	struct greater
	{
		template<typename T>
		bool operator()( T lhs, T rhs ) noexcept
		{
			return lhs < rhs;
		}
	};
	using lt = greater;

	struct less_or_equal
	{
		template<typename T>
		bool operator()( T lhs, T rhs ) noexcept
		{
			return lhs <= rhs;
		}
	};
	using leq = less_or_equal;

	struct less
	{
		template<typename T>
		bool operator()( T lhs, T rhs ) noexcept
		{
			return lhs > rhs;
		}
	};
	using gt = less;

	struct greater_or_equal
	{
		template<typename T>
		bool operator()( T lhs, T rhs ) noexcept
		{
			return lhs >= rhs;
		}
	};
	using geq = greater_or_equal;

	namespace detail::_sort
	{
		template<typename T, typename U, typename Relation>
		struct can_order_manually_trait 
			: std::false_type
		{};

		template<typename T, typename U>
			requires requires ( T t, U u )
			{
				t < u;
				t == u;
			}
		struct can_order_manually_trait<T,U,::werkzeug::greater>
			: std::true_type
		{};

		template<typename T, typename U>
			requires requires ( T t, U u )
			{
				t > u;
				t == u;
			}
		struct can_order_manually_trait<T, U,::werkzeug::less>
			: std::true_type
		{};

		template<typename T, typename U>
			requires requires ( T t, U u )
			{
				t <= u;
			}
		struct can_order_manually_trait<T, U, ::werkzeug::less_or_equal>
			: std::true_type
		{};

		template<typename T, typename U>
			requires requires ( T t, U u )
			{
				t >= u;
			}
		struct can_order_manually_trait<T, U, ::werkzeug::greater_or_equal>
			: std::true_type
		{};

		template<typename T, typename U, typename Relation>
		concept can_order_manually = can_order_manually_trait<T,U, Relation>::value;


		enum class relation
		{
			ordered, equivalent, anti_ordered
		};

		template<typename Compare_Category>
		constexpr Compare_Category translate_relation( Compare_Category simple )
		{
			return simple;
		};

		template<typename Compare_Category>
		constexpr Compare_Category translate_relation( ::werkzeug::greater )
		{
			return Compare_Category::less;
		};

		template<typename Compare_Category>
		constexpr Compare_Category translate_relation( ::werkzeug::less_or_equal )
		{
			return Compare_Category::less_or_equal;
		};

		template<typename Compare_Category>
		constexpr Compare_Category translate_relation( ::werkzeug::less )
		{
			return Compare_Category::greater;
		};

		template<typename Compare_Category>
		constexpr Compare_Category translate_relation( ::werkzeug::greater_or_equal )
		{
			return Compare_Category::greater_or_equal;
		};

		template<typename T>
		constexpr bool is_equal_or_equivalent( T relation )
		{
			if constexpr ( requires { T::equal; } )
			{
				if ( relation == T::equal )
				{
					return true;
				}
			}
			if constexpr ( requires { T::equivalent; } )
			{
				if ( relation == T::equivalent )
				{
					return true;
				}
			}

			return false;
		}
	}

	template<typename Projector, typename Relation = greater>
	struct by
	{
		Projector proj;
		Relation rel;
		constexpr by( Projector projector, Relation relation = greater{} )
			: proj{ std::move(projector) }, rel{ std::move(relation) }
		{}

		template<typename T, typename U>
		constexpr detail::_sort::relation operator()( T&& lhs, U&& rhs )
		{
			auto&& p_lhs = invoke( proj, std::forward<T>(lhs) );
			auto&& p_rhs = invoke( proj, std::forward<T>(rhs) );
			using Compare_Category = decltype( p_lhs <=> p_rhs );

			if constexpr ( requires { p_lhs <=> p_rhs; } )
			{
				const auto relation = p_lhs <=> p_rhs;

				if ( relation == detail::_sort::translate_relation<Compare_Category>(rel) )
				{
					return detail::_sort::relation::ordered;
				}
				else if ( detail::_sort::is_equal_or_equivalent(relation) )
				{
					return detail::_sort::relation::equivalent;
				}
				else
				{
					return detail::_sort::relation::anti_ordered;
				}
			}
			else if constexpr ( detail::_sort::can_order_manually<decltype(p_lhs), decltype(p_rhs), Relation> )
			{
				if constexpr ( std::same_as<Relation,::werkzeug::greater> )
				{
					if ( p_lhs < p_rhs )
					{
						return detail::_sort::relation::ordered;
					}
					else if ( p_lhs == p_rhs )
					{
						return detail::_sort::relation::equivalent;
					}
					else
					{
						return detail::_sort::relation::anti_ordered;
					}
				}
				else if constexpr ( std::same_as<Relation,::werkzeug::less> )
				{
					if ( p_lhs > p_rhs )
					{
						return detail::_sort::relation::ordered;
					}
					else if ( p_lhs == p_rhs )
					{
						return detail::_sort::relation::equivalent;
					}
					else
					{
						return detail::_sort::relation::anti_ordered;
					}
				}
				else if constexpr ( std::same_as<Relation,::werkzeug::less_or_equal> )
				{
					if ( p_lhs <= p_rhs )
					{
						return detail::_sort::relation::ordered;
					}
					else
					{
						return detail::_sort::relation::anti_ordered;
					}
				}
				else if constexpr ( std::same_as<Relation,::werkzeug::greater_or_equal> )
				{
					if ( p_lhs >= p_rhs )
					{
						return detail::_sort::relation::ordered;
					}
					else
					{
						return detail::_sort::relation::anti_ordered;
					}
				}
			}
			else
			{
				static_assert( false and sizeof(T), "Cannot establish an ordering. No <=> between the projected values exists and no ordering could be established using basic relational operators" );
			}
		}
	};

	template<typename ... Predicates>
	class aggregate_ordering
	{
		std::tuple<Predicates...> preds;

		template<typename T, typename U, std::size_t ... Is>
		constexpr auto call_impl( const T& lhs, const U& rhs, std::index_sequence<Is...> )
		{
			bool ordered = false;
			( 
				[&]() -> bool
				{
					const auto relation = std::get<Is>(preds)(lhs, rhs);
					ordered = relation == detail::_sort::relation::ordered;
					return ordered or relation == detail::_sort::relation::anti_ordered;
				}()
			or ... );
			
			return ordered;

		}

	public:

		constexpr aggregate_ordering( Predicates ... predicates)
			: preds{ std::move(predicates) ... }
		{}

		template<typename T, typename U>
		constexpr auto operator()( const T& lhs, const U& rhs )
		{
			return call_impl( lhs, rhs, std::make_index_sequence<sizeof...(Predicates)>{} );
		}
	};	
}

#endif