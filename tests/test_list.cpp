#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#include "testing_utilities.hpp"


#include "werkzeug/container/list.hpp"


using werkzeug::list;



TEST_CASE("list")
{
	test_resource resource;
	{
		list<Lifetime_Informer,test_resource&> list{resource};

		REQUIRE( Lifetime_Informer::instance_counter == 0 );
		REQUIRE( resource.stats().alloc_calls == 0 );

		REQUIRE( list.size() == 0 );
		REQUIRE( list.begin() == list.end() );

		list.emplace_back();
		REQUIRE( list.size() == 1 );
		REQUIRE( ++list.begin() == list.end() );
		REQUIRE( Lifetime_Informer::instance_counter == 1 );
		REQUIRE( Lifetime_Informer::default_ctor_counter == 1 );
		REQUIRE( *std::begin(list) == 0 );
		REQUIRE( resource.stats().alloc_calls == 1 );

		list.emplace_back( 1 );
		REQUIRE( list.size() == 2 );
		REQUIRE( *std::begin(list) == 0 );
		REQUIRE( *(++std::begin(list)) == 1 );
		REQUIRE( Lifetime_Informer::instance_counter == 2 );
		REQUIRE( Lifetime_Informer::value_ctor_counter == 1 );
		REQUIRE( resource.stats().alloc_calls == 2 );

		list.clear();
		REQUIRE( resource.stats().dealloc_calls == 2 );
		REQUIRE( list.size() == 0);
		REQUIRE( Lifetime_Informer::instance_counter == 0 );
		REQUIRE( Lifetime_Informer::dtor_counter == 2 );
		Lifetime_Informer::reset();

		list.emplace_at( list.begin(), 1 );
		REQUIRE( list.size() == 1 );
		REQUIRE( Lifetime_Informer::instance_counter == 1 );
		REQUIRE( Lifetime_Informer::value_ctor_counter == 1 );
		REQUIRE( *std::begin(list) == 1 );

		list.emplace_at( list.begin() );
		REQUIRE( list.size() == 2 );
		REQUIRE( Lifetime_Informer::instance_counter == 2 );
		REQUIRE( Lifetime_Informer::default_ctor_counter == 1 );
		REQUIRE( *std::begin(list) == 0 );

		list.emplace_front( -1 );
		{
			auto it = list.begin();
			REQUIRE( *it == -1 );
			REQUIRE( *(++it) == 0 );
			REQUIRE( *(++it) == 1 );
		}
	}
	REQUIRE( Lifetime_Informer::dtor_counter == 3 );
	REQUIRE( resource.stats().delta() == 0 );
	Lifetime_Informer::reset();

	{
		list<Lifetime_Informer> l1;
		l1.emplace_back();
		l1.emplace_back( 1 );
		l1.emplace_back( 2 );
		REQUIRE( Lifetime_Informer::instance_counter == 3 );
		REQUIRE( Lifetime_Informer::dtor_counter == 0 );

		list l2{ l1 };
		REQUIRE( l2.size() == 3 );
		REQUIRE( Lifetime_Informer::copy_ctor_counter == 3 );
		REQUIRE( Lifetime_Informer::instance_counter == 6 );

		REQUIRE_EQ( ( l1 <=> l2 ), std::strong_ordering::equal );

		{
			auto i1 = l1.begin();
			auto i2 = l2.begin();
			REQUIRE( *i1 == *i2 );
			++i1, ++i2;
			REQUIRE( *i1 == *i2 );
			++i1, ++i2;
			REQUIRE( *i1 == *i2 );
		}
		REQUIRE( Lifetime_Informer::instance_counter == 6 );
		REQUIRE( Lifetime_Informer::dtor_counter == 0 );

		list l3{ std::move(l1) };
		REQUIRE( Lifetime_Informer::instance_counter == 6 );
		REQUIRE( Lifetime_Informer::dtor_counter == 0 );
		REQUIRE( l3.size() == 3 );
		REQUIRE( l1.size() == 0 );

		l3.splice_at( ++(l3.begin()), std::move(l2) );
		REQUIRE( Lifetime_Informer::instance_counter == 6 );
		REQUIRE( Lifetime_Informer::dtor_counter == 0 );
		REQUIRE( l3.size() == 6 );
		REQUIRE( l2.size() == 0 );
		REQUIRE( l2.begin() == l2.end() );
		{
			auto i1 = l3.begin();
			REQUIRE( *i1 == 0 );
			++i1;
			REQUIRE( *i1 == 0 );
			++i1;
			REQUIRE( *i1 == 1 );
			++i1;
			REQUIRE( *i1 == 2 );
			++i1;
			REQUIRE( *i1 == 1 );
			++i1;
			REQUIRE( *i1 == 2 );
		}

		REQUIRE( Lifetime_Informer::instance_counter == 6 );
	}
	
	REQUIRE( Lifetime_Informer::dtor_counter == 6 );
	REQUIRE( resource.stats().delta() == 0 );
}