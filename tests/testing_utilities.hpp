#ifndef WERKZEUG_GUARD_TESTING_UTILITIES_HPP
#define WERKZEUG_GUARD_TESTING_UTILITIES_HPP


#include <cstddef>
#include "werkzeug/memory/actions.hpp"
#include "werkzeug/memory/resource.hpp"
#include "werkzeug/memory/interfaces.hpp"

#define WERKZEUG_TEST_LIFETIME_INFORMER_MAKE_OUTPUT() if ( noisy ) std::cout << __PRETTY_FUNCTION__ << "@ " << static_cast<void*>(this) << '\n' << std::flush


template<typename Concrete>
struct Lifetime_Informer_CRTP
{
	static inline std::size_t instance_counter = 0;
	static inline std::size_t default_ctor_counter = 0;
	static inline std::size_t value_ctor_counter = 0;
	static inline std::size_t dtor_counter = 0;
	static inline std::size_t copy_ctor_counter = 0;
	static inline std::size_t copy_assign_counter = 0;
	static inline std::size_t move_ctor_counter = 0;
	static inline std::size_t move_assign_counter = 0;
	static inline bool noisy = false;

	static void reset() noexcept
	{
		instance_counter = 0;
		default_ctor_counter = 0;
		value_ctor_counter = 0;
		dtor_counter = 0;
		copy_ctor_counter = 0;
		copy_assign_counter = 0;
		move_ctor_counter = 0;
		move_assign_counter = 0;
	}

	struct Silent_Tag
	{};

	Lifetime_Informer_CRTP( Silent_Tag ) noexcept
	{}

	Lifetime_Informer_CRTP() noexcept
	{
		WERKZEUG_TEST_LIFETIME_INFORMER_MAKE_OUTPUT();
		++default_ctor_counter;
		++instance_counter;
	}

	Lifetime_Informer_CRTP( const Lifetime_Informer_CRTP& ) noexcept
	{
		WERKZEUG_TEST_LIFETIME_INFORMER_MAKE_OUTPUT();
		++instance_counter;
		++copy_ctor_counter;
	}

	Lifetime_Informer_CRTP( Lifetime_Informer_CRTP&& ) noexcept
	{
		WERKZEUG_TEST_LIFETIME_INFORMER_MAKE_OUTPUT();
		++instance_counter;
		++move_ctor_counter;
	}

	Lifetime_Informer_CRTP& operator=( const Lifetime_Informer_CRTP& ) noexcept
	{
		WERKZEUG_TEST_LIFETIME_INFORMER_MAKE_OUTPUT();
		++copy_assign_counter;
		return *this;
	}

	Lifetime_Informer_CRTP& operator=( Lifetime_Informer_CRTP&& ) noexcept
	{
		WERKZEUG_TEST_LIFETIME_INFORMER_MAKE_OUTPUT();
		++move_assign_counter;
		return *this;
	}

	~Lifetime_Informer_CRTP() noexcept
	{
		WERKZEUG_TEST_LIFETIME_INFORMER_MAKE_OUTPUT();
		++dtor_counter;
		--instance_counter;
	}
};

struct Lifetime_Informer : Lifetime_Informer_CRTP<Lifetime_Informer>
{
	using base = Lifetime_Informer_CRTP<Lifetime_Informer>;


	int i = 0;

	Lifetime_Informer() = default;
	Lifetime_Informer( const Lifetime_Informer& ) = default;
	Lifetime_Informer( Lifetime_Informer&& ) = default;
	Lifetime_Informer& operator=( const Lifetime_Informer& ) = default;
	Lifetime_Informer& operator=( Lifetime_Informer&& ) = default;
	
	Lifetime_Informer( const int i_ ) noexcept
		: base{ typename base::Silent_Tag{} }, i{i_}
	{
		WERKZEUG_TEST_LIFETIME_INFORMER_MAKE_OUTPUT();
		++instance_counter;
		++value_ctor_counter;
	}

	~Lifetime_Informer()
	{
		if ( noisy )
		{
			std::cout << __PRETTY_FUNCTION__ << " with i = " << i << '\n' << std::flush;
		}
	}

	operator int() const noexcept
	{
		return i;
	}
};

#undef WERKZEUG_TEST_LIFETIME_INFORMER_MAKE_OUTPUT


struct test_resource 
	: 
	werkzeug::memory::actions::Action_Interface<
		werkzeug::memory::resource::fixed::New_Resource,
		werkzeug::memory::actions::Logging,
		werkzeug::memory::actions::Statistics
	>
{};


#endif