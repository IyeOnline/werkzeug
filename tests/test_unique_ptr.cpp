#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#include "testing_utilities.hpp"


#include "werkzeug/memory/unique_ptr.hpp"


using werkzeug::unique_ptr;


TEST_CASE("unique_ptr")
{
	test_resource resource;

	{
		unique_ptr<Lifetime_Informer> ptr;
		ptr = werkzeug::make_unique<Lifetime_Informer>();
		REQUIRE( Lifetime_Informer::default_ctor_counter == 1 );
	}
	REQUIRE( Lifetime_Informer::dtor_counter == 1 );
	Lifetime_Informer::reset();

	{
		auto ptr1 = werkzeug::make_unique_with_resource_reference<Lifetime_Informer>( resource, 1 );
		REQUIRE( Lifetime_Informer::value_ctor_counter == 1 );
		REQUIRE( resource.stats().alloc_calls == 1 );
		REQUIRE( *ptr1 == 1 );
		auto ptr2 = std::move(ptr1);
		REQUIRE( not ptr1 );
		REQUIRE( *ptr2 == 1 );
		REQUIRE( resource.stats().alloc_calls == 1 );
	}
	REQUIRE( resource.stats().delta() == 0 );
	resource.reset_stats();
	Lifetime_Informer::reset();

	{
		auto arr = werkzeug::make_unique_with_resource_reference<Lifetime_Informer[]>( resource, 6 );
		REQUIRE( arr.size() == 6 );
		REQUIRE( resource.stats().alloc_size == arr.size() * sizeof(Lifetime_Informer) );
		REQUIRE( Lifetime_Informer::default_ctor_counter == 6 );
		REQUIRE( arr[0] == 0 );
	}
	REQUIRE( Lifetime_Informer::dtor_counter == 6 );
	REQUIRE( resource.stats().delta() == 0 );
	resource.reset_stats();
	Lifetime_Informer::reset();

	{
		auto arr = werkzeug::make_unique_with_resource_reference_for_overwrite<Lifetime_Informer[6]>( resource );
		REQUIRE( arr.size() == 6 );
		REQUIRE( resource.stats().alloc_size == arr.size() * sizeof(Lifetime_Informer) );
		REQUIRE( Lifetime_Informer::instance_counter == 0 );
		for ( std::size_t i=0; i < arr.size(); ++i )
		{
			new( arr.get()+i) Lifetime_Informer{};
		}
	}
	REQUIRE( resource.stats().delta() == 0 );
	resource.reset_stats();
	Lifetime_Informer::reset();
}