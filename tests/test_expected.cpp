#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#include "testing_utilities.hpp"


#include "werkzeug/expected.hpp"


using werkzeug::Expected;

enum class err_a
{
	a1,
	a2
};

enum class err_b
{
	b1,
	b2
};

enum class err_c
{
	c1, c2
};


using result_type = Expected<Lifetime_Informer, err_a, err_b>;

result_type create( int i )
{
	switch ( i )
	{
		case 0 :
			return result_type{ Lifetime_Informer{} };
		case 1 :
			return result_type{ std::in_place_type<Lifetime_Informer>, i };
		case 2 :
			return result_type::make_error( err_a::a1 );
		case 3 :
			return result_type::make_error( err_b::b2 );
	}

	return result_type::make_error( err_a::a1 );
}

int transform( const Lifetime_Informer& l )
{
	return l.i;
}

Expected<int,err_c> transform_with_error( const Lifetime_Informer& l )
{
	if ( l.i != 0 )
	{
		return l.i;
	}
	else
	{
		return err_c::c1;
	}
}


TEST_CASE( "expected" )
{
	{
		result_type r { 0 };
		REQUIRE( Lifetime_Informer::stats().value_ctor_counter == 1 ); // value constructs in place
		REQUIRE( Lifetime_Informer::stats().copy_ctor_counter == 0 );
		REQUIRE( Lifetime_Informer::stats().move_ctor_counter == 0 );

		REQUIRE( r.has_value() );
		REQUIRE_EQ( r.value(), 0 );
		REQUIRE_EQ( r.value_or( 1 ), 0 );
		REQUIRE( Lifetime_Informer::stats().value_ctor_counter == 2 ); // the fallback is constructed
		REQUIRE( Lifetime_Informer::stats().copy_ctor_counter == 0 ); // returns by reference
		REQUIRE( Lifetime_Informer::stats().move_ctor_counter == 0 );
		REQUIRE( Lifetime_Informer::stats().dtor_counter == 1 ); // the fallback is destroyed

		REQUIRE_EQ( r.value_or_create( 1 ), 0 );
		REQUIRE( Lifetime_Informer::stats().value_ctor_counter == 2 ); // no alternative is constructed
		REQUIRE( Lifetime_Informer::stats().default_ctor_counter == 0 );
		REQUIRE( Lifetime_Informer::stats().copy_ctor_counter == 1 );
		REQUIRE( Lifetime_Informer::stats().move_ctor_counter == 0 );
		REQUIRE( Lifetime_Informer::stats().dtor_counter == 2 ); // the return value is destroyed.
	}
	REQUIRE( Lifetime_Informer::stats().dtor_counter == 3 ); // the held value is destroyed


	Lifetime_Informer::reset();
	{
		auto e = create( 0 );
		REQUIRE( e.has_value() );
		REQUIRE( Lifetime_Informer::stats().default_ctor_counter == 1 );
		REQUIRE( Lifetime_Informer::stats().copy_ctor_counter == 0 );
		REQUIRE( Lifetime_Informer::stats().move_ctor_counter == 1 ); // first option does move construnction
	}

	Lifetime_Informer::reset();
	{
		auto e = create( 1 );
		REQUIRE( e.has_value() );
		REQUIRE( Lifetime_Informer::stats().value_ctor_counter == 1 );
		REQUIRE( Lifetime_Informer::stats().copy_ctor_counter == 0 );
		REQUIRE( Lifetime_Informer::stats().move_ctor_counter == 0 ); // 2nd option does in place construction, so no move should happen
	}

	Lifetime_Informer::reset();
	{
		auto e = create( 2 );
		REQUIRE_FALSE( e.has_value() );
		REQUIRE( Lifetime_Informer::stats().instance_counter == 0 );
		REQUIRE( e.is_error() );
		REQUIRE( e.index() == 1 );
		REQUIRE( e.get_error<err_a>() == err_a::a1 );
	}

	Lifetime_Informer::reset();
	{
		auto e = create( 3 );
		REQUIRE_FALSE( e.has_value() );
		REQUIRE( Lifetime_Informer::stats().instance_counter == 0 );
		REQUIRE( e.is_error() );
		REQUIRE( e.index() == 2 );
		REQUIRE( e.get_error<err_b>() == err_b::b2 );
	}

	Lifetime_Informer::reset();
	{
		auto input = create( 1 );
		REQUIRE ( Lifetime_Informer::stats().value_ctor_counter == 1 );
		auto output = input.and_then( transform );

		REQUIRE( output.has_value() );
		REQUIRE_EQ( output.value(), 1 );
		REQUIRE_EQ( output.value(), 1 );
	}

	Lifetime_Informer::reset();
	{
		auto input = create( 2 );
		REQUIRE ( Lifetime_Informer::stats().instance_counter == 0 );
		auto output = input.and_then( transform );

		REQUIRE_FALSE( output.has_value() );
		REQUIRE_EQ( output.index(), 1 );
		REQUIRE_EQ( output.get_error<err_a>(), err_a::a1 );
	}

	Lifetime_Informer::reset();
	{
		auto input = create( 1 );
		REQUIRE ( Lifetime_Informer::stats().value_ctor_counter == 1 );
		auto output = input.and_then( transform_with_error );
		static_assert( std::same_as<decltype(output), Expected<int,err_a,err_b,err_c>> );
		REQUIRE ( Lifetime_Informer::stats().instance_counter == 1 );

		REQUIRE( output.has_value() );
		REQUIRE_EQ( output.value(), 1 );
	}

	Lifetime_Informer::reset();
	{
		auto input = create( 0 );
		auto output = input.and_then( transform_with_error );
		REQUIRE ( Lifetime_Informer::stats().instance_counter == 1 );

		std::cout << output.index() << '\n';

		REQUIRE_FALSE( output.has_value() );
		REQUIRE_EQ( output.index(), 3 );
		REQUIRE_EQ( output.get_error<err_c>(), err_c::c1 );
	}
}
