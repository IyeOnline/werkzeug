#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#include "testing_utilities.hpp"

#include "werkzeug/type_erased_object.hpp"



TEST_SUITE("Type Erased Object")
{
	TEST_CASE("Simple TE interface")
	{
		werkzeug::Shared_Object obj;
		REQUIRE( obj.is_empty() );
	}
}