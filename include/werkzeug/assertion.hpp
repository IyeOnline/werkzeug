#ifndef WERKZEUG_ASSERTION_HPP
#define WERKZEUG_ASSERTION_HPP


#include <stdexcept>
#include <iostream>
#include <source_location>
#include <format>

namespace werkzeug
{
	struct exception : public std::logic_error
	{ 
		using std::logic_error::logic_error;
	};

	namespace detail
	{
		template<typename ... Ts>
		void print_assert_message( std::ostream& os, const char* condition, std::source_location loc, std::format_string<Ts...> fmt, Ts&& ... args )
		{
			os << "Location:\n\t" << loc.file_name() << ":" << loc.function_name() << ":" << loc.line() << '\n';
			os << "Condition:\n\t" << condition << '\n';
			os << "Details:\n\t" << std::format( fmt, std::forward<Ts>(args) ... ) << '\n';
		}
	}
}

#define WERKZEUG_ASSERT_ALWAYS( condition, ... ) \
if ( not static_cast<bool>( condition ) ) \
{ \
	std::cerr << "ASSERTION FAILED:\n"; \
	::werkzeug::detail::print_assert_message( std::cerr, #condition, std::source_location::current() __VA_OPT__(,) __VA_ARGS__ ); \
	std::terminate(); \
} \

#define WERKZEUG_WARN_ALWAYS( condition, ... ) \
if ( not static_cast<bool>( condition ) ) \
{ \
	std::cerr << "Warning:\n"; \
	::werkzeug::detail::print_assert_message( std::cerr, #condition, std::source_location::current() __VA_OPT__(,) __VA_ARGS__ ); \
} \


#ifndef NDEBUG
	/**
	 * @brief custom assert macro that takes in a (string like) error message.
	 * Please create the error message *inside* of the macro as to not impede release performance
	 * 
	 * @param condition the condition to test for truth
	 * @param message A string like message to be printed if the assertion fails. Please create the error message *inside* the macro to ensure it doesnt impede release performance
	 */
	#define WERKZEUG_ASSERT( condition, ... ) WERKZEUG_ASSERT_ALWAYS( condition __VA_OPT__(,) __VA_ARGS__ )
	#define WERKZEUG_WARN( condition, ... ) WERKZEUG_WARN_ALWAYS( condition __VA_OPT__(,) __VA_ARGS__ )
#else
	/**
	 * @brief custom assert macro that takes in a (string like) error message.
	 * Please create the error message *inside* of the macro as to not impede release performance
	 * 
	 * @param condition the condition to test for truth
	 * @param message A string like message to be printed if the assertion fails. Please create the error message *inside* the macro to ensure it doesnt impede release performance
	 */
	#define WERKZEUG_ASSERT( condition, ... ) (void)0
	#define WERKZEUG_WARN( condition, ... ) (void)0
#endif



#ifdef _WIN32
#define WERKZEUG_UNREACHABLE() _assume(false)
#else
#define WERKZEUG_UNREACHABLE() __builtin_unreachable()
#endif

#endif