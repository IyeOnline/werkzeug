#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#include "testing_utilities.hpp"


#include "werkzeug/memo.hpp"

using werkzeug::Memoizing_Function;




struct Test_Function
{
	static inline bool invoked = false;

	static bool querry_reset()
	{
		return std::exchange( invoked, false );
	}

	static inline int function( int a, int b )
	{
		invoked = true;
		return a + b;
	}
};

#define MAKE_TEST( ARGUMENTS , RESULT ) \
	REQUIRE_FALSE( f.known( ARGUMENTS ) ); \
	REQUIRE_FALSE( Test_Function::querry_reset() ); \
	\
	REQUIRE_EQ( f( ARGUMENTS ), RESULT ); \
	REQUIRE( Test_Function::querry_reset() ); \
	\
	REQUIRE( f.known( ARGUMENTS ) ); \
	REQUIRE_FALSE( Test_Function::querry_reset() ); \
	\
	REQUIRE_EQ( f( ARGUMENTS ), RESULT ); \
	REQUIRE_FALSE( Test_Function::querry_reset() )

#define COMMA ,


TEST_CASE("Memo")
{
	Memoizing_Function f{ Test_Function::function };

	MAKE_TEST( 0 COMMA 0, 0 );
	MAKE_TEST( 1 COMMA 1, 2 );
}