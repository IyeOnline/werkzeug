#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#include "testing_utilities.hpp"

#include "werkzeug/inheritance_variant.hpp"

enum class type
{
	base, derived_1, derived_2
};

using Base = Polymorphic_Lifetime_Informer_Base;
using Derived_1 = Polymorphic_Lifetime_Informer_Derived<1>;
using Derived_2 = Polymorphic_Lifetime_Informer_Derived<2>;

TEST_CASE("inheritance variant")
{
	using werkzeug::inheritance_variant;

	using variant_t = inheritance_variant<Base, Derived_1, Derived_2>;

	{
		inheritance_variant<Base, Derived_1, Derived_2> var{ std::in_place_type<Base> };
	}
	{
		// Derived_1::noisy = true;
		variant_t var{ Derived_1{} };
		REQUIRE( var.index() == 1 );
		REQUIRE( Derived_1::stats().instance_counter == 1 );
		REQUIRE( Derived_1::stats().default_ctor_counter == 1 );
		REQUIRE( Derived_1::stats().move_ctor_counter == 1 );
		REQUIRE( Derived_1::stats().dtor_counter == 1 ); // temporary for parameter is destroyed
		REQUIRE( var->type_id() == typeid(Derived_1) );

		var.emplace<Derived_2>();
		REQUIRE( var.index() == 2 );
		REQUIRE( Derived_1::stats().dtor_counter == 2 ); //held obj1 is destroyed
		REQUIRE( Derived_1::stats().instance_counter == 0 );
		REQUIRE( Derived_2::stats().default_ctor_counter == 1 );
		REQUIRE( Derived_2::stats().instance_counter == 1 );
		REQUIRE( Derived_2::stats().copy_ctor_counter == 0 );
		REQUIRE( Derived_2::stats().move_ctor_counter == 0 );
		REQUIRE( var->type_id() == typeid(Derived_2) );
	}
	REQUIRE( Derived_2::stats().instance_counter == 0 );
	REQUIRE( Derived_2::stats().dtor_counter == 1 );

	Base::reset();
	Derived_1::reset();
	Derived_2::reset();

	{
		variant_t var;
		REQUIRE( var.index() == var.npos );
		REQUIRE( Base::stats().instance_counter == 0 );
		REQUIRE( Derived_1::stats().instance_counter == 0 );
		REQUIRE( Derived_2::stats().instance_counter == 0 );
	}
}