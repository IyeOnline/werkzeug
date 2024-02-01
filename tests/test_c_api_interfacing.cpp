#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#include "testing_utilities.hpp"

#include "werkzeug/C_api_interfacing.hpp"


double c_api_tail( double a, double b, double(*f)(double a, double b, void* ud), void* ud )
{
	return f(a,b,ud);
}

double c_api_mid( double a, double b, double(*f)(double a, void* ud, double b), void* ud )
{
	return f(a,ud,b);
}

double c_api_front( double a, double b, double(*f)(void* ud, double a, double b), void* ud )
{
	return f(ud,a,b);
}



TEST_CASE("C API interfacing")
{
	using werkzeug::make_wrapper;
	using werkzeug::user_data;

	constexpr auto callable  = []( double a, double b )
	{
		return a+b;
	};

	SUBCASE("tail user data")
	{
		auto wrapper = werkzeug::make_wrapper<double(double,double,user_data)>( callable );
		REQUIRE( callable(0,1) == c_api_tail( 0, 1, wrapper, &wrapper ) );

		auto ref_wrapper = werkzeug::make_reference_wrapper<double(double,double,user_data)>( callable );
		REQUIRE( callable(0,1) == c_api_tail( 0, 1, ref_wrapper, &ref_wrapper ) );
	}

	SUBCASE("mid user data")
	{
		auto wrapper = werkzeug::make_wrapper<double(double,user_data,double)>( callable );
		REQUIRE( callable(0,1) == c_api_mid( 0, 1, wrapper, &wrapper ) );

		auto ref_wrapper = werkzeug::make_wrapper<double(double,user_data,double)>( callable );
		REQUIRE( callable(0,1) == c_api_mid( 0, 1, ref_wrapper, &ref_wrapper ) );
	}

	SUBCASE("front user data")
	{
		auto wrapper = werkzeug::make_wrapper<double(user_data,double,double)>( callable );
		REQUIRE( callable(0,1) == c_api_front( 0, 1, wrapper, &wrapper ) );

		auto ref_wrapper = werkzeug::make_wrapper<double(user_data,double,double)>( callable );
		REQUIRE( callable(0,1) == c_api_front( 0, 1, ref_wrapper, &ref_wrapper ) );
	}
}