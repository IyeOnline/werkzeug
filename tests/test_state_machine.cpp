#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#include "testing_utilities.hpp"


#include "werkzeug/state_machine.hpp"


using werkzeug::State_Machine;


enum class states : unsigned char
{
    A, B
};

struct Flip_Flop_Machine
    : werkzeug::State_Machine<states,2,Flip_Flop_Machine>
{
    int i = 0;

    using Base = werkzeug::State_Machine<states,2,Flip_Flop_Machine>;

    constexpr Flip_Flop_Machine()
        : Base{ states::A }
    { };

    template<states>
    states func_for() = delete;

    using Base::execute_active;
};

template<>
states Flip_Flop_Machine::func_for<states::A>()
{
    std::cout << "FLIP\n";
    ++i;
    return states::B;
}

template<>
states Flip_Flop_Machine::func_for<states::B>()
{
    std::cout << "flop\n";
    ++i;
    return states::A;
}




TEST_CASE("state machine flip flop")
{
	Flip_Flop_Machine state_machine{};
	for ( int i=0; i < 10; ++i )
    {
	    const auto new_state = state_machine.execute_active();
		REQUIRE ( new_state == ( i%2==0 ? states::B : states::A ) );
    }
	REQUIRE( state_machine.i == 10 );
}





struct Adding_Machine
    : werkzeug::State_Machine<states,2,Adding_Machine, int >
{
    int V = 0;

    using Base = werkzeug::State_Machine<states,2,Adding_Machine,int>;

    constexpr Adding_Machine()
        : Base{ states::A }
    { };

    template<states>
    states func_for( int ) = delete;

    using Base::execute_active;
};

template<>
states Adding_Machine::func_for<states::A>( int i )
{
    
    V += 2*i;
    return states::B;
}

template<>
states Adding_Machine::func_for<states::B>( int i )
{
    V += i/2;
    return states::A;
}


TEST_CASE("state machine adder")
{
	Adding_Machine state_machine{};
	int V = 0;
	for ( int i=0; i < 10; ++i )
    {
	    const auto new_state = state_machine.execute_active(i);
		REQUIRE ( new_state == ( i%2==0 ? states::B : states::A ) );
		if ( i % 2 == 0 )
		{
			V += 2*i;
		}
		else
		{
			V += i/2;
		}
		REQUIRE ( state_machine.V == V );
    }
}
