#include "werkzeug/iterator.hpp"
#include <cstddef>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#include "testing_utilities.hpp"

#include "werkzeug/container/dynamic_array.hpp"

using werkzeug::basic_dynamic_array_small_buffer;


TEST_CASE("dynamic_array no buffer")
{
	Lifetime_Informer::noisy = true;
	{
		basic_dynamic_array_small_buffer<Lifetime_Informer,0,test_resource> arr;
		REQUIRE ( Lifetime_Informer::stats().instance_counter == 0 );
		REQUIRE( arr.size() == 0 );
		REQUIRE( arr.capacity() == 0 );

		arr.emplace_back();
		REQUIRE( arr.size() == 1 );
		REQUIRE( arr.capacity() == 1 );
		REQUIRE ( Lifetime_Informer::stats().instance_counter == 1 );
		REQUIRE ( Lifetime_Informer::stats().default_ctor_counter == 1 );
		REQUIRE( arr[0] == 0 );
	}
}

TEST_CASE("dynamic_array no buffer")
{
	{
		basic_dynamic_array_small_buffer<Lifetime_Informer,4,test_resource> arr;
		REQUIRE ( Lifetime_Informer::stats().instance_counter == 0 );
		REQUIRE( arr.size() == 0 );
		REQUIRE( arr.capacity() == 4 );
		REQUIRE ( arr.is_in_buffer() );

		arr.emplace_back();
		REQUIRE( arr.size() == 1 );
		REQUIRE( arr.capacity() == 4 );
		REQUIRE( arr[0] == 0 );
		REQUIRE ( arr.is_in_buffer() );

		arr.reserve( 42 );
		REQUIRE( arr.size() == 1 );
		REQUIRE( arr.capacity() == 42 );
		REQUIRE( arr[0] == 0 );
		REQUIRE_FALSE( arr.is_in_buffer() );
	}
}