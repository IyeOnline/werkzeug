#ifndef WERKZEUG_ASSERTION_HPP
#define WERKZEUG_ASSERTION_HPP


#include <stdexcept>
#include <iostream>
#include <cassert>

namespace werkzeug
{
	struct exception : public std::logic_error
	{ 
		using std::logic_error::logic_error;
	};

	namespace detail
	{
		inline void print_assert_location( std::ostream& os, const char* const src_loc, const char* const function) noexcept
		{
			os << "Location:\n\t" << src_loc << "\nFunction:\n\t" << function << '\n';
		}

		inline void print_assert_condition( std::ostream& os, const char* const condition ) noexcept
		{
			os << "Condition:\n\t" << condition << '\n';
		}

		template<typename ... Ts>
		inline void print_assert_message( std::ostream& os, Ts&& ... explanation ) noexcept
		{
			os << "Explanation:\n\t";
			( (os << explanation << '\n'), ... );
		}
	}
}


#ifndef NDEBUG
	#if defined __GNUC__
		#define WERKZEUG_GUARD_FUNCTION_NAME __PRETTY_FUNCTION__
	#elif defined _MSC_VER
		#define WERKZEUG_GUARD_FUNCTION_NAME __FUNCSIG__	
	#else
		#define WERKZEUG_GUARD_FUNCTION_NAME __func__
	#endif

	#define WERKZEUG_GUARD_STRINGIFY_IMPL(x) #x
    #define WERKZEUG_GUARD_STRINGIFY( x ) WERKZEUG_GUARD_STRINGIFY_IMPL(x)
	#define WERKZEUG_GUARD_SRC_LOC __FILE__ ":" WERKZEUG_GUARD_STRINGIFY(__LINE__) ":"


	/**
	 * @brief custom assert macro that takes in a (string like) error message.
	 * Please create the error message *inside* of the macro as to not impede release performance
	 * 
	 * @param condition the condition to test for truth
	 * @param message A string like message to be printed if the assertion fails. Please create the error message *inside* the macro to ensure it doesnt impede release performance
	 */
	#define WERKZEUG_ASSERT( condition, message ) \
	if ( not (condition)) \
	{ \
		std::cerr << "ASSERTION FAILED.\n"; \
		werkzeug::detail::print_assert_location( std::cerr, WERKZEUG_GUARD_SRC_LOC, WERKZEUG_GUARD_FUNCTION_NAME ); \
		werkzeug::detail::print_assert_condition( std::cerr, #condition ); \
		werkzeug::detail::print_assert_message( std::cerr, message ); \
		std::terminate(); \
	} \
	assert( condition ) //also use a language level assert to potentially communicate to the optmizer. Currently too lazy to create a custom portable ASSUME macro

	#define WERKZEUG_WARN( condition, message )\
	if ( not (condition) ) \
	{ \
		std::clog << "Warning:\n"; \
		detail::print_assert_location( std::clog, WERKZEUG_GUARD_SRC_LOC, WERKZEUG_GUARD_FUNCTION_NAME ); \
		detail::print_assert_condition( std::clog, #condition ); \
		detail::print_assert_message( std::clog, message ); \
	} (void)0

	#define WERKZEUG_WARN_ALWAYS( message ) \
	{ \
		std::clog << "Warning:\n"; \
		detail::print_assert_location( std::clog, WERKZEUG_GUARD_SRC_LOC, WERKZEUG_GUARD_FUNCTION_NAME ); \
		detail::print_assert_message( std::clog, message ); \
	} (void)0
#else
	/**
	 * @brief custom assert macro that takes in a (string like) error message.
	 * Please create the error message *inside* of the macro as to not impede release performance
	 * 
	 * @param condition the condition to test for truth
	 * @param message A string like message to be printed if the assertion fails. Please create the error message *inside* the macro to ensure it doesnt impede release performance
	 */
	#define WERKZEUG_ASSERT( condition, message ) (void)0
	#define WERKZEUG_WARN( condition, message ) (void)0
	#define WERKZEUG_WARN_ALWAYS( message ) (void)0

#endif



#ifdef _WIN32
#define WERKZEUG_UNREACHABLE() _assume(false)
#else
#define WERKZEUG_UNREACHABLE() __builtin_unreachable()
#endif

#endif