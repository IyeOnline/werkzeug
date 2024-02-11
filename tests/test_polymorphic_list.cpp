#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#include "testing_utilities.hpp"

#include "werkzeug/container/polymorphic_list.hpp"


using werkzeug::polymorphic_list;

using Base = Polymorphic_Lifetime_Informer_Base;
using Derived_1 = Polymorphic_Lifetime_Informer_Derived<1>;
using Derived_2 = Polymorphic_Lifetime_Informer_Derived<2>;

TEST_CASE("polymorphic_list")
{
	test_resource resource;

	{
		polymorphic_list<Base,test_resource&> list{resource};

		REQUIRE( Base::stats().instance_counter == 0 );
		REQUIRE( resource.stats().alloc_calls == 0 );

		REQUIRE( list.size() == 0 );
		REQUIRE( list.begin() == list.end() );

		list.emplace_back();
		REQUIRE( list.size() == 1 );
		REQUIRE( ++list.begin() == list.end() );
		REQUIRE( Base::stats().instance_counter == 1 );
		REQUIRE( Base::stats().default_ctor_counter == 1 );
		REQUIRE( std::begin(list)->type_id() == typeid(Base) );
		REQUIRE( resource.stats().alloc_calls == 1 );

		list.emplace_back<Derived_1>();
		REQUIRE( list.size() == 2 );
		REQUIRE( std::begin(list)->type_id() == typeid(Base) );
		REQUIRE( (++std::begin(list))->type_id() == typeid(Derived_1) );
		REQUIRE( Derived_1::stats().instance_counter == 1 );
		REQUIRE( Derived_1::stats().default_ctor_counter == 1 );
		REQUIRE( resource.stats().alloc_calls == 2 );

		list.clear();
		REQUIRE( resource.stats().dealloc_calls == 2 );
		REQUIRE( list.size() == 0);
		REQUIRE( Base::stats().instance_counter == 0 );
		REQUIRE( Base::stats().dtor_counter == 2 );
		REQUIRE( Derived_1::stats().dtor_counter == 1 );
		Base::reset();
		Derived_1::reset();
		Derived_2::reset();

		list.emplace_at<Derived_2>( list.begin() );
		REQUIRE( list.size() == 1 );
		REQUIRE( Derived_2::stats().instance_counter == 1 );
		REQUIRE( Derived_2::stats().default_ctor_counter == 1 );
		REQUIRE( std::begin(list)->type_id() == typeid(Derived_2) );

		list.emplace_at<Derived_1>( list.begin() );
		REQUIRE( list.size() == 2 );
		REQUIRE( Derived_1::stats().instance_counter == 1 );
		REQUIRE( Derived_1::stats().default_ctor_counter == 1 );
		REQUIRE( std::begin(list)->type_id() == typeid(Derived_1) );

		list.emplace_front();
		REQUIRE( list.size() == 3 );
		REQUIRE( Base::stats().instance_counter == 3 );
		{
			auto it = list.begin();
			REQUIRE( it->type_id() == typeid(Base) );
			REQUIRE( (++it)->type_id() == typeid(Derived_1) );
			REQUIRE( (++it)->type_id() == typeid(Derived_2) );
		}
	}
	REQUIRE( Base::stats().dtor_counter == 3 );
	REQUIRE( resource.stats().delta() == 0 );
	Base::reset();
	Derived_1::reset();
	Derived_2::reset();

	{
		polymorphic_list<Base> l1;
		l1.emplace_back<Base>();
		l1.emplace_back<Derived_1>();
		l1.emplace_back<Derived_2>();
		REQUIRE( Base::stats().instance_counter == 3 );
		REQUIRE( Base::stats().dtor_counter == 0 );

		polymorphic_list l2{ l1 };
		REQUIRE( l2.size() == 3 );
		REQUIRE( Base::stats().copy_ctor_counter == 3 );
		REQUIRE( Base::stats().instance_counter == 6 );

		{
			auto i1 = l1.begin();
			auto i2 = l2.begin();
			REQUIRE_EQ( i1->type_id(), i2->type_id() );
			++i1, ++i2;
			REQUIRE_EQ( i1->type_id(), i2->type_id() );
			++i1, ++i2;
			REQUIRE_EQ( i1->type_id(), i2->type_id() );
		}
		REQUIRE( Base::stats().instance_counter == 6 );
		REQUIRE( Base::stats().dtor_counter == 0 );

		polymorphic_list l3{ std::move(l1) };
		REQUIRE( Base::stats().instance_counter == 6 );
		REQUIRE( Base::stats().dtor_counter == 0 );
		REQUIRE( l3.size() == 3 );
		REQUIRE( l1.size() == 0 );

		l3.splice_at( ++(l3.begin()), std::move(l2) );
		REQUIRE( Base::stats().instance_counter == 6 );
		REQUIRE( Base::stats().dtor_counter == 0 );
		REQUIRE( l3.size() == 6 );
		REQUIRE( l2.size() == 0 );
		REQUIRE( l2.begin() == l2.end() );
		{
			auto i1 = l3.begin();
			REQUIRE_EQ( i1->type_id(), typeid(Base) );
			++i1;
			REQUIRE_EQ( i1->type_id(), typeid(Base) );
			++i1;
			REQUIRE_EQ( i1->type_id(), typeid(Derived_1) );
			++i1;
			REQUIRE_EQ( i1->type_id(), typeid(Derived_2) );
			++i1;
			REQUIRE_EQ( i1->type_id(), typeid(Derived_1) );
			++i1;
			REQUIRE_EQ( i1->type_id(), typeid(Derived_2) );
		}

		REQUIRE( Base::stats().instance_counter == 6 );
	}
	
	REQUIRE( Base::stats().dtor_counter == 6 );
	REQUIRE( resource.stats().delta() == 0 );
}