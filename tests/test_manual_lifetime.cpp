#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#include "testing_utilities.hpp"


#include "werkzeug/manual_lifetime.hpp"


using werkzeug::Manual_Lifetime;

// struct Informer_A

TEST_CASE("expected")
{
	Lifetime_Informer::noisy = true;

	{
		Manual_Lifetime<Lifetime_Informer> inf;
		REQUIRE ( Lifetime_Informer::instance_counter == 0 );
	}
	REQUIRE( Lifetime_Informer::dtor_counter == 0 );

	{
		Manual_Lifetime<Lifetime_Informer> inf{ 5 };
		Manual_Lifetime<Lifetime_Informer> inf2{ 0 };
		inf.destroy();
		inf2.destroy();
	}
	REQUIRE ( Lifetime_Informer::value_ctor_counter == 2 );
	REQUIRE ( Lifetime_Informer::dtor_counter == 2 );
}