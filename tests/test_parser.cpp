#include "werkzeug/parsing.hpp"
#include <iostream>

using namespace werkzeug::parsing;

struct S
{
	int i;
	int j;

	static S make( int i, std::monostate, int j ) { return { i, j }; }
};

std::ostream& operator<<( std::ostream& os, const S& s ) { return os << "{ " << s.i << ", " << s.j << " }"; }

int main()
{
	{
		std::string_view txt = "1234 5678";

		auto p = compound_parser {
			S::make,
			integral_parser<int> {},
			swallow_whitespace {},
			integral_parser<int> {},
		};

		auto [it, value] = p( txt );

		std::cout << ( it == txt.begin() ) << '\n';
		std::cout << ( it == txt.end() ) << '\n';
		std::cout << value << '\n';
	}

	{
		auto p = compound_parser{ compound_parser { integral_parser<int> {} } };

		std::string_view txt = "1234";

		auto [it, value] = p( txt );

		std::cout << ( it == txt.begin() ) << '\n';
		std::cout << ( it == txt.end() ) << '\n';
		std::cout << value << '\n';
	}
}
