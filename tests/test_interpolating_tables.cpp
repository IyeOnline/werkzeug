#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#include <cmath>


#include "werkzeug/tables/interpolating_tables.hpp"
#include "werkzeug/random/RNG.hpp"
#include "werkzeug/algorithm/analysis_tools.hpp"


using doctest::Approx;

using T = double;

using namespace werkzeug;

T f( const T x0 )
{
	return x0*x0;
}

T f(const T x0, const T x1 )
{
	return 2*x0*x1;
}

T f(const T x0, const T x1, const T x2 )
{
	return x0*x1*x2 + x0*5 - 3*x1*std::cos(x2);
}

T f(const T x0, const T x1, const T x2, const T x3 )
{
	return x0*x1 + x0 - x1 * std::cos(x0) + x3*std::cos(x2);
}

T f(const T x0, const T x1, const T x2, const T x3, const T x4 )
{
	return x0*x1 + x2 + x3*x4*std::cos(x2);
}


using Analysis_Tool = werkzeug::Joined_Tool<T,werkzeug::Continous_Min_Max_Finder<T,10,10>, werkzeug::Running_Average<T>>;

TEST_CASE("2d advanced table interpolation")
{
	werkzeug::RNG rng;
	std::vector<T> x0( 100, 0 );
	std::vector<T> x1( 100, 0 );
	std::iota( x0.begin(), x0.end(), 0. );
	std::iota( x1.begin(), x1.end(), 0. );
	werkzeug::Advanced_Interpolating_Array<2,T> table( {std::move(x0), std::move(x1)} );
	for ( int i=0; i < table.size_Nd()[0]; ++i )
	{
		const auto x = table.ticks()[0][static_cast<std::size_t>(i)];
		for ( int j=0; j<table.size_Nd()[1]; ++j )
		{
			const auto y = table.ticks()[1][static_cast<std::size_t>(j)];
			table.set_value_at_index( {i,j}, f(x,y) );
		}
	}

	Analysis_Tool analysis;
	
	for ( std::size_t i=0; i < 5000; ++i )
	{
		const auto x = rng.next<T>( table.ticks()[0].front(), table.ticks()[0].back() );
		const auto y = rng.next<T>( table.ticks()[1].front(), table.ticks()[1].back() );
		const auto real = f(x,y);
		const auto interpolated = table.interpolate_value_at({x,y}).value;

		const auto diff = real - interpolated;

		if ( diff != 0.0 )
		{
			analysis.add_value(real/interpolated);
		}
	}
	std::cout << std::setprecision(20) << std::fixed;
	std::cout << "2d advanced table interpolation:\n";
	std::cout << analysis << '\n';
	REQUIRE( analysis.average() == Approx(1.0).epsilon(1e-4) );
}

TEST_CASE("3d log-log-lin interpolation")
{
	werkzeug::RNG rng;


	std::vector<T> x0( 100, 0 );
	std::vector<T> x1( 100, 0 );
	std::vector<T> x2( 100, 0 );

	std::generate( x0.begin(), x0.end(), [i=0]() mutable { return std::exp(1 + 0.1*i++); } );
	std::generate( x1.begin(), x1.end(), [i=0]() mutable { return std::exp(1 + 0.2*i++); } );
	std::generate( x2.begin(), x2.end(), [i=0]() mutable { return 1 + 0.5*i++; } );
	werkzeug::Advanced_Interpolating_Array<3,T> table( { std::move(x0), std::move(x1), std::move(x2) } );

	for ( int i=0; i < table.size_Nd()[0]; ++i )
	{
		const auto x = table.ticks()[0][static_cast<std::size_t>(i)];
		for ( int j=0; j < table.size_Nd()[1]; ++j )
		{
			const auto y = table.ticks()[1][static_cast<std::size_t>(j)];
			for ( int k=0; k < table.size_Nd()[2]; ++k )
			{
				const auto z = table.ticks()[2][static_cast<std::size_t>(k)];
				const auto value = f( x, y, z );
				table.set_value_at_index( { i, j, k }, value );
			}
		}
	}

	Analysis_Tool analysis;
	
	for ( std::size_t i=0; i < 10000; ++i )
	{
		const auto x = rng.next<T>( table.ticks()[0].front(), table.ticks()[0].back() );
		const auto y = rng.next<T>( table.ticks()[1].front(), table.ticks()[1].back() );
		const auto z = rng.next<T>( table.ticks()[2].front(), table.ticks()[2].back() );

		const auto real = f(x,y,z);
		const auto interpolated = table.interpolate_value_at( {x,y,z} ).value;

		const auto diff = real - interpolated;

		if ( diff != 0.0 )
		{
			analysis.add_value(real/interpolated);
		}
	}
	std::cout << std::setprecision(20) << std::fixed;
	std::cout << "3d log-log-lin interpolation:\n";
	std::cout << analysis << '\n';
	REQUIRE( analysis.average() == Approx(1.0).epsilon(1e-4) );
}

TEST_CASE("5d log-log-log-log-lin interpolation")
{
	werkzeug::RNG rng;

	std::vector<T> b0( 50, 0 );
	std::vector<T> b1( 50, 0 );
	std::vector<T> b2( 50, 0 );
	std::vector<T> b3( 50, 0 );
	std::vector<T> b4( 20, 0 );

	std::generate( b0.begin(), b0.end(), [i=0]() mutable { return std::exp(1 + 0.1*i++); } );
	std::generate( b1.begin(), b1.end(), [i=0]() mutable { return std::exp(1 + 0.2*i++); } );
	std::generate( b2.begin(), b2.end(), [i=0]() mutable { return std::exp(1 + 0.1*i++); } );
	std::generate( b3.begin(), b3.end(), [i=0]() mutable { return std::exp(1 + 0.01*i++); } );
	std::generate( b4.begin(), b4.end(), [i=0]() mutable { return 1 + 0.5*i++; } );
	werkzeug::Advanced_Interpolating_Array<5,T> table( { std::move(b0), std::move(b1), std::move(b2), std::move(b3), std::move(b4) } );

	for ( int i0=0; i0 < table.size_Nd()[0]; ++i0 )
	{
		const auto x0 = table.ticks()[0][static_cast<std::size_t>(i0)];
		for ( int i1=0; i1 < table.size_Nd()[1]; ++i1 )
		{
			const auto x1 = table.ticks()[1][static_cast<std::size_t>(i1)];
			for ( int i2=0; i2 < table.size_Nd()[2]; ++i2 )
			{
				const auto x2 = table.ticks()[2][static_cast<std::size_t>(i2)];
				for ( int i3=0; i3 < table.size_Nd()[3]; ++i3 )
				{
					const auto x3 = table.ticks()[3][static_cast<std::size_t>(i3)];
					for ( int i4=0; i4 < table.size_Nd()[4]; ++i4 )
					{
						const auto x4 = table.ticks()[4][static_cast<std::size_t>(i4)];
						const auto value = f( x0, x1, x2, x3, x4 );
						table.set_value_at_index( { i0, i1, i2, i3, i4 }, value );
					}
				}
			}
		}
	}

	Analysis_Tool analysis;
	
	for ( std::size_t i=0; i < 10000; ++i )
	{
		const auto x0 = rng.next<T>( table.ticks()[0].front(), table.ticks()[0].back() );
		const auto x1 = rng.next<T>( table.ticks()[1].front(), table.ticks()[1].back() );
		const auto x2 = rng.next<T>( table.ticks()[2].front(), table.ticks()[2].back() );
		const auto x3 = rng.next<T>( table.ticks()[3].front(), table.ticks()[3].back() );
		const auto x4 = rng.next<T>( table.ticks()[4].front(), table.ticks()[4].back() );

		const auto real = f(x0,x1,x2,x3,x4);
		const auto interpolated = table.interpolate_value_at( {x0,x1,x2,x3,x4} ).value;

		const auto diff = real - interpolated;

		if ( diff != 0.0 )
		{
			analysis.add_value(real/interpolated);
		}
	}
	std::cout << std::setprecision(20) << std::fixed;
	std::cout << "5d log-log-log-log-lin interpolation:\n";
	std::cout << analysis << '\n';
	REQUIRE( analysis.average() == Approx(1.0).epsilon(1e-4) );
}