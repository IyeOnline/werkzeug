#include <iostream>
#include <vector>

std::ostream& operator<<( std::ostream& out, std::vector<int> v )
{
	for ( const auto& vec : v )
		out << vec << " ";

	return out;
}

int main()
{
	std::vector<int> vec1 {};
	std::vector<int> vec2 {};
	vec1.push_back( 10 );
	vec1.push_back( 20 );
	std::cout << vec1.at( 0 ) << " " << vec1.at( 1 ) << " " << vec1.size() << '\n';
	vec2.push_back( 100 );
	vec2.push_back( 200 );
	std::cout << vec2.at( 0 ) << " " << vec2.at( 1 ) << " " << vec2.size() << '\n';
	std::vector<std::vector<int>> vec2d {};
	vec2d.push_back( vec1 );
	vec2d.push_back( vec2 );
	std::cout << vec2d.at( 0 ) << '\n';
	vec1.at( 0 ) = 7;
	std::cout << vec2d.at( 0 ) << " " << vec2d.at( 1 ) << '\n';
}
