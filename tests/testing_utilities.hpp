#ifndef WERKZEUG_GUARD_TESTING_UTILITIES_HPP
#define WERKZEUG_GUARD_TESTING_UTILITIES_HPP


#include <cstddef>
#include "werkzeug/memory/actions.hpp"
#include "werkzeug/memory/resource.hpp"
#include "werkzeug/memory/interfaces.hpp"

#define WERKZEUG_TEST_LIFETIME_INFORMER_MAKE_OUTPUT() if ( noisy ) std::cout << __PRETTY_FUNCTION__ << "@ " << static_cast<void*>(this) << '\n' << std::flush



struct stats_t
{
	std::size_t instance_counter = 0;
	std::size_t default_ctor_counter = 0;
	std::size_t value_ctor_counter = 0;
	std::size_t dtor_counter = 0;
	std::size_t copy_ctor_counter = 0;
	std::size_t copy_assign_counter = 0;
	std::size_t move_ctor_counter = 0;
	std::size_t move_assign_counter = 0;
};

template<typename Concrete>
struct Lifetime_Informer_CRTP
{
	static inline stats_t stats_;
	static inline bool noisy = false;

	static const stats_t& stats()
	{
		return stats_;
	}

	static void reset() noexcept
	{
		stats_.instance_counter = 0;
		stats_.default_ctor_counter = 0;
		stats_.value_ctor_counter = 0;
		stats_.dtor_counter = 0;
		stats_.copy_ctor_counter = 0;
		stats_.copy_assign_counter = 0;
		stats_.move_ctor_counter = 0;
		stats_.move_assign_counter = 0;
	}

	struct Silent_Tag
	{};

	Lifetime_Informer_CRTP( Silent_Tag ) noexcept
	{}

	Lifetime_Informer_CRTP() noexcept
	{
		WERKZEUG_TEST_LIFETIME_INFORMER_MAKE_OUTPUT();
		++stats_.default_ctor_counter;
		++stats_.instance_counter;
	}

	Lifetime_Informer_CRTP( const Lifetime_Informer_CRTP& ) noexcept
	{
		WERKZEUG_TEST_LIFETIME_INFORMER_MAKE_OUTPUT();
		++stats_.instance_counter;
		++stats_.copy_ctor_counter;
	}

	Lifetime_Informer_CRTP( Lifetime_Informer_CRTP&& ) noexcept
	{
		WERKZEUG_TEST_LIFETIME_INFORMER_MAKE_OUTPUT();
		++stats_.instance_counter;
		++stats_.move_ctor_counter;
	}

	Lifetime_Informer_CRTP& operator=( const Lifetime_Informer_CRTP& ) noexcept
	{
		WERKZEUG_TEST_LIFETIME_INFORMER_MAKE_OUTPUT();
		++stats_.copy_assign_counter;
		return *this;
	}

	Lifetime_Informer_CRTP& operator=( Lifetime_Informer_CRTP&& ) noexcept
	{
		WERKZEUG_TEST_LIFETIME_INFORMER_MAKE_OUTPUT();
		++stats_.move_assign_counter;
		return *this;
	}

	~Lifetime_Informer_CRTP() noexcept
	{
		WERKZEUG_TEST_LIFETIME_INFORMER_MAKE_OUTPUT();
		++stats_.dtor_counter;
		--stats_.instance_counter;
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
		++stats_.instance_counter;
		++stats_.value_ctor_counter;
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

struct Polymorphic_Lifetime_Informer_Base
	: Lifetime_Informer_CRTP<Polymorphic_Lifetime_Informer_Base>
{
private :
	using LTI_Base = Lifetime_Informer_CRTP<Polymorphic_Lifetime_Informer_Base>;
public :

	using LTI_Base::stats;
	using LTI_Base::reset;

	virtual ~Polymorphic_Lifetime_Informer_Base() = default;


	virtual const std::type_info& type_id() const
	{
		return typeid(*this);
	}

	virtual int number() const
	{
		return -1;
	}

	virtual const stats_t& pstats() const
	{
		return LTI_Base::stats();
	}
};

template<int ID>
struct Polymorphic_Lifetime_Informer_Derived 
	: Polymorphic_Lifetime_Informer_Base
	, Lifetime_Informer_CRTP<Polymorphic_Lifetime_Informer_Derived<ID>>
{
private :
	using LTI_Base = Lifetime_Informer_CRTP<Polymorphic_Lifetime_Informer_Derived<ID>>;
public :

	using LTI_Base::stats;
	using LTI_Base::reset;

	virtual const std::type_info& type_id() const override
	{
		return typeid(*this);
	}

	virtual int number() const override
	{ return ID; }

	virtual const stats_t& pstats() const override
	{
		return LTI_Base::stats();
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