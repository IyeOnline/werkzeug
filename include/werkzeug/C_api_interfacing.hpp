#ifndef WERKKEUG_GUARD_C_API_INTERFACE_HPP
#define WERKKEUG_GUARD_C_API_INTERFACE_HPP

#include "werkzeug/concepts.hpp"
#include "werkzeug/parameter_pack.hpp"

namespace werkzeug
{
	/** @brief tag type to specify which element of a C function is the `void*` for user data */
	struct user_data
	{ };


	/**
	 * @brief wrapper type that can be used with a C API expecting a function pointer and a user-data void*
	 * 
	 * @tparam Signature the Signature "R(Args..)" of the C API expects. Must contain at least one 'user_data' placeholder.
	 * @tparam Callable type of the callable. May be a reference
	 */
	template<signature Signature,typename Callable>
	class Callable_Wrapper;

	template<typename Callable, typename R, typename ... Args>
	class Callable_Wrapper<R(Args...), Callable>
	{
		Callable c; //we store the callable

		using argument_pack = type_pack<Args...>;
		constexpr static std::size_t user_data_index = argument_pack::template unique_index_of<::werkzeug::user_data>();

		static_assert( user_data_index < argument_pack::size, "The argument list must contain exactly one user data tag" );

		template<typename T>
		struct translate_type 
			: std::conditional<std::same_as<T, user_data>
				, void*
				, T
			>
		{ };

		template<typename T>
		using translate_type_t = typename translate_type<T>::type;

		
		using plain_callable_t = std::remove_reference_t<Callable>;
		using f_ptr_t = R(*)(translate_type_t<Args> ... );

		static R wrap( translate_type_t<Args> ... args ) // the actual wrapper function that the C API will use
		{
			const std::tuple< translate_type_t<Args>& ... > arg_tup( args... ); 

			auto& that = *static_cast<plain_callable_t*>( std::get<user_data_index>(arg_tup) );

			constexpr auto selectiv_invoke = []<std::size_t ... Is>( auto& that_, auto& arg_tup_, std::index_sequence<Is...> )
			{
				return that_( std::get< (Is >= user_data_index ? Is+1 : Is ) >(arg_tup_) ... );
			};


			return selectiv_invoke( that, arg_tup, std::make_index_sequence<sizeof...(Args)-1>{} );
		}
	public :
		Callable_Wrapper( Callable c_ )
			: c{ std::forward<Callable>(c_) }
		{};


		/** @brief converts the callable to a function pointer that can be passed to the C API */
		operator f_ptr_t () const noexcept
		{ return &Callable_Wrapper::wrap; }

		/** @brief gets a void* to this */
		void* user_data()
		{ return this; }

		/** @brief gets a void* to this */
		const void* user_data() const
		{ return this; }
	};

	/**
	 * @brief Creats a wrapper with the given signature, wrapping 'callable'
	 * 
	 * @tparam Signature signature the C API expects. Must contain exactly one 'user_data' placeholder
	 * @tparam Callable Callable type
	 * @param callable the callable to wrap
	 */
	template<signature Signature, typename Callable>
	Callable_Wrapper<Signature,Callable> make_wrapper( Callable&& callable )
	{
		return { std::forward<Callable>(callable) };
	}

	/**
	 * @brief Creats a wrapper with the given signature, wrapping the reference 'callable'
	 * 
	 * @tparam Signature signature the C API expects. Must contain exactly one 'user_data' placeholder
	 * @tparam Callable Callable type
	 * @param callable the callable to wrap
	 */
	template<signature Signature, typename Callable>
	Callable_Wrapper<Signature,Callable&> make_reference_wrapper( Callable& callable )
	{
		return { callable };
	}
}

#endif