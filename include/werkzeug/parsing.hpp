#include <charconv>
#include <cstdint>
#include <variant>

#include "werkzeug/traits.hpp"

namespace werkzeug::parsing
{
	enum class result_type
	{
		valid,
		skipped,
		invalid
	};

	template <random_access_iterator It, typename T>
	struct result
	{
		It it = {};
		T value = {};

		constexpr auto is_success( It begin ) const noexcept { return it == begin; }
	};

	namespace detail
	{
		template <typename T, typename It>
		struct is_result_trait : std::false_type
		{
		};

		template <typename T, typename It>
		struct is_result_trait<result<It, T>, It> : std::true_type
		{
		};

		template <typename T, typename It>
		concept is_result = is_result_trait<T, It>::value;

		template <char Low, char High>
		constexpr auto in_range( const char c ) noexcept
		{
			return c >= Low and c <= High;
		}
	} // namespace detail

	template <typename T>
	concept parser = requires( T t, const char* it ) {
		{ t( it, it ) } -> detail::is_result<const char*>;
		typename T::template result_type<const char*>;
		typename T::value_type;
	};

	template <typename T, auto Arg>
		requires std::is_fundamental_v<T> and requires( T& t, const char* ptr ) { std::from_chars( ptr, ptr, t, Arg ); }
	struct from_chars_parser
	{
		template <typename It>
		using result_type = result<It, T>;
		using value_type = T;

		template <random_access_iterator It>
		constexpr result_type<It> operator()( It begin, It end ) const noexcept
		{
			T out;
			const auto [ptr, ec] = std::from_chars( begin, end, out, Arg );

			if ( ec == std::errc {} )
			{
				return { ptr, out };
			}
			else
			{
				return { begin };
			}
		}

		constexpr auto operator()( std::string_view s ) const noexcept { return operator()( s.begin(), s.end() ); }
	};

	template <std::integral T, int Radix = 10>
		requires( Radix >= 2 and Radix <= 36 )
	struct integral_parser : from_chars_parser<T, Radix>
	{
		using value_type = T;
		template <typename It>
		using result_type = typename from_chars_parser<T, Radix>::template result_type<It>;
	};

	using signed_parser = integral_parser<int64_t>;
	using unsigned_parser = integral_parser<uint64_t>;

	template <std::floating_point T, std::chars_format fmt = std::chars_format::general>
	struct floating_point_parser : from_chars_parser<T, fmt>
	{
		using value_type = T;
		using result_type = typename from_chars_parser<T, fmt>::result_type;
	};

	using double_parser = floating_point_parser<double>;

	template <size_t Min_Count, size_t Max_Count, typename Cond>
	struct swallow_parser
	{
		[[no_unique_address]] Cond cond;
		using value_type = std::monostate;
		template <typename It>
		using result_type = result<It, value_type>;

		template <forward_iterator_of_type<char> It>
		constexpr auto operator()( It begin, It end ) const noexcept -> result_type<It>
		{
			size_t n = 0;
			auto it = begin;
			for ( ; it != end and n < Max_Count; ++it, ++n )
			{
				const auto c = *it;
				if ( not cond( c ) )
				{
					break;
				}
			}
			if ( n < Min_Count )
			{
				return { begin };
			}
			return { it };
		}
	};

	using swallow_whitespace = swallow_parser<0, std::numeric_limits<size_t>::max(), decltype( []( const char c ) { return c == ' ' or c == '\t'; } )>;


	template <typename C, typename T>
	concept constraint = requires( C c, T t ) {
		{ c( t ) } -> std::same_as<bool>;
	};

	template <auto Min, auto Max = std::numeric_limits<decltype( Min )>::max()>
		requires std::same_as<decltype( Min ), decltype( Max )>
	struct range_constraint
	{
		using value_type = decltype( Min );

		constexpr auto operator()( value_type v ) const noexcept { return v >= Min and v <= Max; }
	};

	static_assert( constraint<range_constraint<0, 1>, int> );

	template <parser Underlying, constraint<typename Underlying::value_type> Constraint>
	struct constrained_parser
	{
		[[no_unique_address]] Underlying u;
		[[no_unique_address]] Constraint c;

		template <typename It>
		constexpr auto operator()( It begin, It end ) const
		{
			auto r = u( begin, end );

			if ( r.is_success( begin ) and not c( r.value ) )
			{
				r.it = begin;
			}

			return r;
		}

		constexpr auto operator()( std::string_view s ) const noexcept { return operator()( s.begin(), s.end() ); }
	};

	namespace detail
	{
		template<parser L, parser R>
		struct is_same_parser
			: std::false_type
		{ };

		template<parser P>
		struct is_same_parser<P,P>
			: std::true_type
		{};

		template<parser P, constraint<typename P::value_type> CL, constraint<typename P::value_type> CR>
		struct is_same_parser<constrained_parser<P, CL>,constrained_parser<P, CR>>
			: std::true_type
		{};

		template<parser P, constraint<typename P::value_type> C>
		struct is_same_parser<constrained_parser<P, C>,P>
			: std::true_type
		{};

		template<parser P, constraint<typename P::value_type> C>
		struct is_same_parser<P,constrained_parser<P, C>>
			: std::true_type
		{};
	}

	template<typename L, typename R>
	concept same_parser = detail::is_same_parser<L, R>::value;

	struct identity_combine
	{
		constexpr auto operator()( auto v ) const noexcept { return v; }
	};

	template <typename Combiner, parser... Parts>
		requires( sizeof...( Parts ) >= 1 )
	struct compound_parser
	{
		[[no_unique_address]] Combiner comb;
		[[no_unique_address]] std::tuple<Parts...> parts;

		compound_parser( Combiner comb_, Parts... parts_ ) : comb { comb_ }, parts { parts_... } { }

		compound_parser( Parts ... p ) 
			requires ( std::same_as<Combiner,identity_combine> and sizeof...(Parts) == 1 )
			: comb{ identity_combine{} }, parts{ p... }
		{}

		using value_type = decltype( std::declval<Combiner>()( std::declval<typename Parts::value_type>()... ) );

		template <typename It>
		using result_type = result<It, value_type>;

		template <typename It>
		constexpr auto parse_single( It& begin, It end, auto& part, auto& out )
		{
			auto r = part( begin, end );
			if ( r.it != begin )
			{
				begin = r.it;
				out = r.value;
				return true;
			}
			return false;
		}

		template <typename It, size_t... Is>
		constexpr auto operator_impl( It begin, It end, std::index_sequence<Is...> ) -> result_type<It>
		{
			auto orig = begin;
			std::tuple<typename Parts::value_type... > values;

			auto success = ( parse_single( begin, end, std::get<Is>( parts ), std::get<Is>( values ) ) && ... );

			if ( success )
			{
				auto value = std::apply( comb, values );
				return { begin, value };
			}
			return { orig };
		}

		template <typename It>
		constexpr auto operator()( It begin, It end )
		{
			return operator_impl( begin, end, std::make_index_sequence<sizeof...( Parts )> {} );
		}

		constexpr auto operator()( std::string_view s ) { return operator()( s.begin(), s.end() ); }
	};

	template<parser P>
		requires ( not instantiation_of<P,compound_parser> )
	compound_parser( P ) -> compound_parser<identity_combine, P>;

	template <parser... Alternatives>
	struct best_match_parser
	{
		std::tuple<Alternatives... > alternatives;
	};
} // namespace werkzeug::parsing