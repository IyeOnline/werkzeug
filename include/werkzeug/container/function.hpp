#ifndef WERKZEUG_GUARD_FUNCTION_HPP
#define WERKZEUG_GUARD_FUNCTION_HPP

#include "werkzeug/macros.hpp"
#include "werkzeug/concepts.hpp"
#include "werkzeug/memory/resource.hpp"
#include "werkzeug/memory/allocator.hpp"



namespace werkzeug
{
	template<typename T>
	class function
	{
		static_assert(false && sizeof(T), "Please use the correct syntax");
	};


	/**
	 * @brief type erasure function wrapper with SSO
	 * 
	 * @tparam Ret return type
	 * @tparam Args... argument types
	 */
	template<typename Ret, typename ... Args>
	class function<Ret(Args...)>
	{
		enum class held_type_t
		{
			empty, f_ptr, callable, static_callable
		};

		struct Holder_Base
		{
			virtual Ret invoke( Args... ) = 0;
			virtual ~Holder_Base() = default;
		};

		template<typename Callable>
		struct Holder : Holder_Base
		{
			[[no_unique_address]] Callable c_;

			Holder( Callable c )
				: c_{ std::move(c) }
			{}

			~Holder() override = default;

			Ret invoke( Args... args ) override
			{
				return c_( std::forward<Args>(args) ... );
			}
		};

		template<typename Callable,typename Resource>
		struct Holder_With_Allocator : Holder_Base
		{
			using allocator_t = memory::allocator<Holder_With_Allocator,Resource>;

			union //disable automatic destruction
			{
				[[no_unique_address]] Callable c_;
			};
			union
			{
				[[no_unique_address]] allocator_t alloc_;
			};
			

			Holder_With_Allocator( Callable c, allocator_t alloc )
				: c_(std::move(c)), alloc_( std::move(alloc) )
			{ }

			Ret invoke( Args... args ) override
			{
				return c_( std::forward<Args>(args) ... );
			}

			~Holder_With_Allocator() override
			{
				c_.~Callable();
				auto alloc_copy = alloc_;
				alloc_.~allocator_t();
				alloc_copy.deallocate_single(this);
			}
		};


		template<typename Callable>
		constexpr static bool can_intern = sizeof(Holder<Callable>) == sizeof(void*);

		using f_ptr_t = Ret(*)(Args...);

		held_type_t held_type = held_type_t::empty;
		union
		{
			f_ptr_t f_ptr;
			Holder_Base* callable_ptr;
			alignas(alignof(Holder_Base)) std::byte storage[sizeof(Holder_Base)];
		};

		Holder_Base* storage_pointer()
		{
			return reinterpret_cast<Holder_Base*>(&storage);
		}

	public :
		function() = default;

		function( f_ptr_t func )
			: held_type{held_type_t::f_ptr}, f_ptr{func}
		{}

		template<typename Callable>
		using default_resource_t = memory::resource::fixed::New_Resource<alignof(Callable)>;

		template<typename Callable, typename Resource = default_resource_t<Callable> >
			requires ( not can_intern<Callable> or not std::same_as<Resource,default_resource_t<Callable>> )
		function( Callable&& callable, Resource&& resource = Resource{} )
			: held_type{held_type_t::callable}
		{
			using Resource_t = std::conditional_t< std::is_lvalue_reference_v<Resource>, 
				Resource&,
				Resource
			>;
			using holder_t = Holder_With_Allocator<Callable,Resource_t>;
			typename holder_t::allocator_t alloc( std::forward<Resource>(resource) );
			callable_ptr = alloc.allocate_single();
			new(callable_ptr) holder_t{ std::forward<Callable>(callable), std::move(alloc) };
		}

		template<typename Callable>
			requires can_intern<Callable>
		function( Callable&& callable )
			: held_type{ held_type_t::static_callable }
		{
			new( storage_pointer() ) Holder<Callable>{ std::forward<Callable>( callable ) };
		}

		template<typename Callable, typename ... Args>
		Callable& emplace( Args&& ... args )
		{

		}

		template<typename Callable, memory::concepts::memory_source Resource, typename ... Args>
		Callable& emplace_with_resource ( Resource&& r, Args&& ... args )
		{
			
		}

		Ret operator()(Args ... args )
		{
			assert( held_type != held_type_t::empty );
			switch ( held_type )
			{
				case held_type_t::f_ptr :
				{
					return f_ptr(std::forward<Args>(args) ... );
				}
				case held_type_t::callable :
				{
					return callable_ptr->invoke(std::forward<Args>(args) ... );
				}
				case held_type_t::static_callable :
				{
					return storage_pointer()->invoke( std::forward<Args>(args) ... );
				}
			}
			WERKZEUG_UNREACHABLE();
		}

		~function()
		{
			switch ( held_type )
			{
				case held_type_t::callable :
				{
					callable_ptr->~Holder_Base();
					break;
				}
				case held_type_t::static_callable :
				{
					storage_pointer()->~Holder_Base();
					break;
				}
				case held_type_t::empty :
				case held_type_t::f_ptr :
				{
					break;
				}
			}
		}
	};

	namespace detail::_function
	{
		template<typename Callable>
		concept can_deduce = requires 
		{
			&Callable::operator();
		};

		template<typename>
		struct signature;

		template<typename Ret, typename Args>
		struct signature<Ret(*)(Args...)>
		{
			using type = Ret(Args...);
		};

		template<typename Class, typename Ret, typename ... Args>
		struct signature<Ret(Class::*)(Args...)>
		{
			using type = Ret(Args...);
		};

		template<typename Class, typename Ret, typename ... Args>
		struct signature<Ret(Class::*)(Args...) &>
		{
			using type = Ret(Args...);
		};

		template<typename Class, typename Ret, typename ... Args>
		struct signature<Ret(Class::*)(Args...) const>
		{
			using type = Ret(Args...);
		};

		template<typename Class, typename Ret, typename ... Args>
		struct signature<Ret(Class::*)(Args...) const &>
		{
			using type = Ret(Args...);
		};

		template<typename T>
		struct determine_signature
		{
			static_assert(false and sizeof(T), "This callable cannot have its signature deduced." );
		};

		template<typename T>
			requires can_deduce<T>
		struct determine_signature<T>
		{
			using type = typename signature<decltype(&T::operator())>::type;
		};

		template<typename T>
			requires std::is_function_v<T>
		struct determine_signature<T>
		{
			using type = typename signature<T>::type;
		};
	}

	template<typename Callable>
	function( Callable ) -> function<typename detail::_function::determine_signature<Callable>::type>;

	template<typename Callable, memory::concepts::memory_source Resource>
	function( Callable, Resource ) -> function<typename detail::_function::determine_signature<Callable>::type>;
}

#endif