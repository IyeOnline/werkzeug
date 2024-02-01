#include "werkzeug/fixed_string.hpp"
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#include "testing_utilities.hpp"
#include "werkzeug/container/stack_dynamic_array.hpp"


using werkzeug::stack_dynamic_array;


TEST_CASE("static_array")
{
	{
		stack_dynamic_array<Lifetime_Informer,5> arr{};

		REQUIRE( Lifetime_Informer::instance_counter == 0 );

		REQUIRE( arr.size() == 0 );
		REQUIRE( arr.capacity() == 5 );

		arr.emplace_back();
		REQUIRE( arr.size() == 1 );
		REQUIRE( arr[0] == 0 );
		REQUIRE( Lifetime_Informer::instance_counter == 1 );
		REQUIRE( Lifetime_Informer::default_ctor_counter == 1 );

		arr.emplace_back( 1 );
		REQUIRE( arr.size() == 2 );
		REQUIRE( arr[1] == 1 );
		REQUIRE( Lifetime_Informer::instance_counter == 2 );
		REQUIRE( Lifetime_Informer::value_ctor_counter == 1 );

		arr.clear();
		REQUIRE( arr.size() == 0);
		REQUIRE( Lifetime_Informer::instance_counter == 0 );
		REQUIRE( Lifetime_Informer::dtor_counter == 2 );
		Lifetime_Informer::reset();
	}
	REQUIRE( Lifetime_Informer::dtor_counter == 0 );
	{
		stack_dynamic_array<Lifetime_Informer,2> arr{ 0, 1 };

		REQUIRE( arr.size() == 2 );
		REQUIRE( arr[0] == 0 );
		REQUIRE( arr[1] == 1 );
		REQUIRE( Lifetime_Informer::value_ctor_counter == 2 );
	}
	REQUIRE( Lifetime_Informer::dtor_counter == 2 );
	REQUIRE( Lifetime_Informer::instance_counter == 0 );
}