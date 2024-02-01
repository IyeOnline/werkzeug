#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#include "werkzeug/parameter_pack.hpp"

using namespace werkzeug;


TEST_CASE("parameter_pack")
{
	REQUIRE( true );
}


namespace type_pack_members_test
{
	using p = type_pack<int,double,char>;

	static_assert( std::same_as<p::type_at<0>,int> );
	static_assert( std::same_as<p::type_at<1>,double> );

	static_assert( p::unique_index_of<int>() == 0 );
	static_assert( p::unique_index_of<double>() == 1 );

	static_assert( p::contains<int>() );
	static_assert( not p::contains<bool>() );
}


namespace value_pack_members_test
{
	using p = value_pack<0,1,true>;

	static_assert( p::value_at<0> == 0 );
	static_assert( std::same_as<decltype(p::value_at<0>),const int> );
	static_assert( p::value_at<2> == true );
	static_assert( std::same_as<decltype(p::value_at<2>),const bool> );

	static_assert( p::index_of<0>() == 0 );
	static_assert( p::index_of<1>() == 1 );

	static_assert( p::contains<0>() );
	static_assert( not p::contains<2>() );
}


namespace is_parameter_pack_test
{
	static_assert( type_pack_c<type_pack<>> );
	static_assert( type_pack_c<type_pack<int,double>> );
	static_assert( not type_pack_c<int> );

	static_assert( value_pack_c<value_pack<>> );
	static_assert( value_pack_c<value_pack<0,true>> );
	static_assert( not value_pack_c<int> );
}


namespace same_pack_kind_test
{
	static_assert( same_pack_kind<type_pack<>,type_pack<>> );
	static_assert( same_pack_kind<value_pack<>,value_pack<>> );
	static_assert( not same_pack_kind<type_pack<>,value_pack<>> );
}


namespace all_unique_test
{
	static_assert( all_unique<int,double>::value );
	static_assert( not all_unique_v<int,int> );

	static_assert( all_unique_v<type_pack<int,double>> );
	static_assert( not all_unique_v<type_pack<int,int>> );
}


namespace pack_includes_test
{
	using p1 = type_pack<int,double,char>;
	using p2 = type_pack<int,double>;
	using p3 = type_pack<char>;

	static_assert( pack_includes_v<p1,p2> );
	static_assert( pack_includes_v<p1,p3> );
	static_assert( not pack_includes_v<p2,p3> );
}


namespace append_test
{
	using in = type_pack<int>;

	using out = append<in,double>::type;

	static_assert( std::same_as<out, type_pack<int,double>> );
}


namespace append_if_unique_test
{
	using in = type_pack<int>;

	using out = append_if_unique<in,double>::type;
	static_assert( std::same_as<out, type_pack<int,double>> );

	using out2 = append_if_unique<out,double>::type;
	static_assert( std::same_as<out, type_pack<int,double>> );
}


namespace reduce_to_unique_test
{
	using in = type_pack<int,int,double,int>;
	using out = reduce_to_unique_t<in>;
	using expected = type_pack<int,double>;
	static_assert( std::same_as<out,expected>  );
}

namespace normalize_test
{
	using in1 = type_pack<int, double, char>;
	using in2 = type_pack<double, char, int>;

	static_assert( std::same_as<normalized_t<in1>, normalized_t<in2>> );
}

