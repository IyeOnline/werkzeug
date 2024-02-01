#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"


#include <algorithm>

#include "werkzeug/algorithm/sorting.hpp"





struct S
{
	int i;
	int j;
	double d;

	static std::ostream& header( std::ostream& os )
	{
		os << "-----------------\n";
		os << "i\tj\td\n";
		os << "-----------------\n";
		return os;
	}

	friend std::ostream& operator<<( std::ostream& os, const S& s )
	{
		return os << s.i << '\t' << s.j << '\t' << s.d;
	}

	auto operator<=>( const S& other ) const = default;
};



using werkzeug::aggregate_ordering, werkzeug::by, werkzeug::less, werkzeug::greater;


TEST_CASE("ordering/by")
{
	constexpr auto ordering = aggregate_ordering{ 
		by{ &S::i, less{} }, 
		by{ &S::d, less{} },
		by{ &S::j, greater{} } 
	};

	S arr[] = {
		{ .i = 0, .j= 1, .d=2.0 },
		{ .i = 2, .j= 2, .d=3.0 },
		{ .i = 2, .j= 2, .d=2.0 },
		{ .i = 0, .j= 3, .d=2.0 },
		{ .i = 3, .j= 4, .d=2.0 }
	};

	std::ranges::stable_sort( arr, ordering );

	REQUIRE( arr[0].i == 3 );

	REQUIRE( arr[1].i == 2 );
	REQUIRE( arr[1].d == 3.0 );

	REQUIRE( arr[2].i == 2 );
	REQUIRE( arr[2].d == 2.0 );

	REQUIRE( arr[3].j == 1 );
}