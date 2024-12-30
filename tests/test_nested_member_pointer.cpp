#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#include "testing_utilities.hpp"


#include "werkzeug/nested_member_pointer.hpp"



struct A
{
	int i = 0;
};

struct B
{
	A a;
};

struct C
{
	A a;
	B b;
};


using werkzeug::nested_member_pointer;


TEST_CASE("nested member pointer test")
{
	nested_member_pointer ptr( &C::b, &B::a, &A::i );

	C c;

	auto& i = c->*ptr;

	static_assert( std::same_as<decltype(i),int&> );

	REQUIRE_EQ( i, 0 );
	i = 42;

	REQUIRE_EQ( c.b.a.i, 42 );
}
