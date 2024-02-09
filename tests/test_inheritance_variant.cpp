#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#include "testing_utilities.hpp"

#include "werkzeug/inheritance_variant.hpp"

enum class type
{
	base, derived_1, derived_2
};


struct Base : Lifetime_Informer_CRTP<Base>
{
	virtual type type_id()
	{
		return type::base;
	}

	virtual ~Base() = default;
};

struct Derived_1 : Base, Lifetime_Informer_CRTP<Derived_1>
{
	using Lifetime_Informer_CRTP<Derived_1>::stats;
	using Lifetime_Informer_CRTP<Derived_1>::noisy;
	using Lifetime_Informer_CRTP<Derived_1>::reset;

	virtual type type_id()
	{
		return type::derived_1;
	}
};

struct Derived_2 : Base, Lifetime_Informer_CRTP<Derived_2>
{
	using Lifetime_Informer_CRTP<Derived_2>::stats;
	using Lifetime_Informer_CRTP<Derived_2>::noisy;
	using Lifetime_Informer_CRTP<Derived_2>::reset;

	virtual type type_id()
	{
		return type::derived_2;
	}
};



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
		REQUIRE( var->type_id() == type::derived_1 );

		var.emplace<Derived_2>();
		REQUIRE( var.index() == 2 );
		REQUIRE( Derived_1::stats().dtor_counter == 2 ); //held obj1 is destroyed
		REQUIRE( Derived_1::stats().instance_counter == 0 );
		REQUIRE( Derived_2::stats().default_ctor_counter == 1 );
		REQUIRE( Derived_2::stats().instance_counter == 1 );
		REQUIRE( Derived_2::stats().copy_ctor_counter == 0 );
		REQUIRE( Derived_2::stats().move_ctor_counter == 0 );
		REQUIRE( var->type_id() == type::derived_2 );
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