#ifndef WERKKEUG_GUARD_INHERITANCE_VARIANT_HPP
#define WERKKEUG_GUARD_INHERITANCE_VARIANT_HPP

#include <algorithm>
#include <type_traits>
#include <iostream>
#include <cstddef>
#include <new>
#include <cassert>
#include <concepts>
#include <limits>


#include "werkzeug/assertion.hpp"
#include "werkzeug/traits.hpp"
#include "werkzeug/parameter_pack.hpp"


namespace werkzeug
{
	namespace detail
	{
		template<template <typename> class trait, typename Base, typename ... Children>
		struct joined_traits_base
		{
			constexpr static bool possible = ( ( std::is_abstract_v<Base> && sizeof...(Children) > 0 ) || trait<Base>::possible ) && ( trait<Children>::possible && ... );
			constexpr static bool trivial = ( ( std::is_abstract_v<Base> && sizeof...(Children) > 0 ) || trait<Base>::trivial ) && ( trait<Children>::trivial && ... );
			constexpr static bool nothrow = not possible || ( ( ( std::is_abstract_v<Base> && sizeof...(Children) > 0 ) || trait<Base>::nothrow ) && ( trait<Children>::nothrow && ... ) );
		};

		template<typename Base, typename ... Children>
		struct joined_traits
		{
			using default_construct = joined_traits_base<default_construct_trait, Base, Children...>;
			using copy_construct = joined_traits_base<copy_construct_trait, Base, Children...>;
			using move_construct = joined_traits_base<move_construct_trait, Base, Children...>;
			using copy_assign = joined_traits_base<copy_assign_trait, Base, Children...>;
			using move_assign = joined_traits_base<move_assign_trait, Base, Children...>;
			using destroy = joined_traits_base<destroy_trait, Base, Children...>;
			using equality_compare = joined_traits_base<equality_compare_trait, Base, Children...>;
			using three_way_compare = joined_traits_base<three_way_compare_trait, Base, Children...>;

			using swap = joined_traits_base<swap_trait,Base,Children...>;
			using hash = joined_traits_base<hash_trait,Base,Children...>;
		};

		//primary template, also serves as the empty type to end the recursive one
		template<typename Base, std::size_t my_index, typename ... Stored_Types>
		union based_storage_impl
		{
			template<typename F, typename ... Params>
			static constexpr bool nothrow_apply = true; //for the recursion
		};

		//"specialization" that actually does all of the work
		template<typename Base, std::size_t my_index, typename T1, typename ... Ts>
		union based_storage_impl<Base, my_index, T1,Ts...>
		{
		private:
			constexpr static bool is_last = sizeof...(Ts) == 0;

			/** @brief the proper union member */
			T1 t1;
			/** @brief the storage for the remaining types in the pack */
			based_storage_impl<Base,my_index+1, Ts...> rest = {}; //by giving this union member a default initializers, the entire type because default constructible, since the empty base union is default constructible

			/**
			 * @brief implementation function for the get function. This allows to merge the const and non const overloads
			 * 
			 * @param storage pointer to the union to get from
			 * @param index index of the currently active member
			 * @return a `Base*` to the contained object
			 */
			static constexpr auto get_impl( auto* storage, std::size_t index ) noexcept 
				-> std::conditional_t<std::is_const_v<std::remove_pointer_t<decltype(storage)>>,const Base*, Base*>
			{
				if ( index == my_index )
				{ return &(storage->t1); }
				else if constexpr ( not is_last )
				{ return storage->rest.get(index); }
				WERKZEUG_UNREACHABLE();
			}

			/**
			 * @brief implementation function for the get function. This allows to merge the const and non const overloads
			 * 
			 * @tparam index index to get
			 * @param storage pointer to the union to get from
			 * @return properly typed pointer to the contained object
			 */
			template<std::size_t index>
			static constexpr auto* get_impl( auto* storage ) noexcept
			{
				if constexpr ( index == my_index )
				{ return &(storage->t1); }
				else if constexpr ( not is_last )
				{ return storage->rest.template get<index>(); }
				WERKZEUG_UNREACHABLE();
			}

			/**
			 * @brief implementation function for the get function. This allows to merge the const and non const overloads
			 * 
			 * @tparam T type to get
			 * @param storage pointer to the union to get from
			 * @return properly typed pointer to the contained object
			 */
			template<typename T>
			static constexpr auto* get_impl( auto* storage ) noexcept
			{
				if constexpr ( std::same_as<T,T1> )
				{ return &(storage->t1); }
				else if constexpr ( not is_last )
				{ return storage->rest.template get<T>(); }
				WERKZEUG_UNREACHABLE();
			}

		public :
			template<typename F, typename ... Params>
			constexpr static bool nothrow_apply = requires ( F f, T1* t_ptr, Params ... params )
			{
				{ f(t_ptr, params...) } noexcept;
			} and decltype(rest)::template nothrow_apply<F,Params...>;
		private :
			/**
			 * @brief implementation function for the apply function. This allows to merge the const and non const overloads
			 * 
			 * @param storage pointer to the union to apply to
			 * @param index index of the active union member
			 * @param f invocable to be applied to the active member
			 * @param params parameters to pass to the unvocable
			 * @return decltype(auto) to the result of the invocation
			 */
			template<typename R = void, typename F = void, typename ... Params>
			static constexpr decltype(auto) apply_impl( auto* storage, std::size_t index ,F&& f, Params&& ... params ) noexcept( nothrow_apply<F,Params...> )
			{
				if ( index == my_index )
				{
					if constexpr ( std::is_same_v<R,void> )
					{
						(void)f( std::addressof(storage->t1), std::forward<Params>( params ) ... );
						return;
					}
					else
					{
						return static_cast<R>( f( std::addressof(storage->t1), std::forward<Params>( params ) ... ) );
					}
				}
				else if constexpr ( not is_last )
				{
					return storage->rest.template apply<R>( index, std::forward<F>(f), std::forward<Params>(params)... );
				}
				WERKZEUG_UNREACHABLE();
			}
		public:
			constexpr based_storage_impl() noexcept
				: rest{}
			{}

			constexpr ~based_storage_impl() 
				requires std::is_trivially_destructible_v<T1>
			= default;

			constexpr ~based_storage_impl() //this is required if T1 is not trivially destructible.
			{ }

			/**
			 * @brief constructs the given type in place
			 * 
			 * @param params parameters to construct from
			 */
			template<std::size_t I, typename ... Params>
			constexpr auto& construct( Params&& ... params ) noexcept( std::is_nothrow_constructible_v<T1,Params...> )
			{
				if constexpr ( I == my_index )
				{
					return *std::construct_at( &t1, std::forward<Params>(params) ... );
				}
				else if constexpr ( not is_last )
				{
					std::construct_at( &rest );
					return rest.template construct<I>( std::forward<Params>(params)...);
				}
				WERKZEUG_UNREACHABLE();
			}

			constexpr auto* get( std::size_t index ) noexcept
			{ return get_impl( this, index ); }

			constexpr auto* get( std::size_t index ) const noexcept
			{ return get_impl( this, index ); }

			template<std::size_t index>
			constexpr auto* get() noexcept
			{ return get_impl<index>(this); }

			template<std::size_t index>
			constexpr auto* get() const noexcept
			{ return get_impl<index>(this); }

			template<typename T>
			constexpr T* get() noexcept
			{ return get_impl<T>(this); }

			template<typename T>
			constexpr T* get() const noexcept
			{ return get_impl<T>(this); }
			
			template<typename R=void, typename F=void, typename ... Params>
			constexpr decltype(auto) apply( std::size_t index ,F&& f, Params&& ... params )
			{ return apply_impl<R>( this, index, std::forward<F>(f), std::forward<Params>(params)... ); }

			template<typename R=void, typename F=void, typename ... Params>
			constexpr decltype(auto) apply( std::size_t index ,F&& f, Params&& ... params ) const
			{ return apply_impl<R>( this, index, std::forward<F>(f), std::forward<Params>(params)... ); }
		};

		/**
		 * @brief storage type that will ignore an abstract base and correct the indices
		 */
		template<typename Base, typename ... Children>
		using based_storage = 
			std::conditional_t<std::is_abstract_v<Base>,
				detail::based_storage_impl<Base, 1, Children...>,
				detail::based_storage_impl<Base, 0, Base, Children...>
			>;
	}


	/**
	 * @brief A variant type that allows to get a pointer to the base class
	 * 
	 * @tparam Base The base class for all types held. If not abstract, this can also be held
	 * @tparam Children... a list of derived types
	 */
	template<typename Base, typename ... Children>
	class inheritance_variant
	{
		static_assert( sizeof...(Children) > 0, "This type make little sense only one type. Use an optional instead" );
		static_assert( ( std::is_base_of_v<Base,Children> && ... ), "This type is intended to be used as a polymorphic variant, allowing access to the held object via a base pointer" );

		using alternative_pack = type_pack<Base,Children...>;
		
	public :
		constexpr static bool abstract_base = std::is_abstract_v<Base>;
		constexpr static std::size_t alternative_count = 1 + sizeof...(Children);
		constexpr static std::size_t npos = std::numeric_limits<std::size_t>::max();

		template<typename T>
		constexpr static bool is_allowed = std::is_same_v<T, Base> || (std::is_same_v<T, Children> || ...);

		/** @brief gets the index of type `T` */
		template<typename T>
			requires ( is_allowed<T> )
		constexpr static std::size_t index_of = alternative_pack::template unique_index_of<T>();

		/** @ brief gets the type at index `I` */
		template<std::size_t I>
			requires ( I < alternative_count )
		using type = typename alternative_pack::template type_at<I>;

	private :
		template<typename T>
		using traits = type_traits<T>;
		using joined_traits = detail::joined_traits<Base,Children...>;
		
		constexpr static auto destroy_expression = []( auto* location )
		{
			std::destroy_at(location); 
		};
		constexpr static auto copy_construct_expression = []( auto* dest, const Base* source )
		{
			using T = std::remove_pointer_t<decltype(dest)>;
			std::construct_at( dest, static_cast<const T&>(*source) );
		};
		constexpr static auto move_construct_expression = []( auto* dest, Base* source )
		{
			using T = std::remove_pointer_t<decltype(dest)>;
			std::construct_at( dest, static_cast<T&&>(*source) );
		};
		constexpr static auto copy_assign_expression = []( auto* dest, const Base* source )
		{
			using T = std::remove_pointer_t<decltype(dest)>;
			if constexpr ( traits<T>::copy_assign::possible )
			{
				*dest = static_cast<const T&>( *source );
				return true;
			}
			return false;
		};
		constexpr static auto move_assign_expression = []( auto* dest, Base* source )
		{
			using T = std::remove_pointer_t<decltype(dest)>;
			if constexpr ( traits<T>::move_assign::possible )
			{
				*dest = static_cast<T&&>( *source );
				return true;
			}
			return false;
		};
		// constexpr static auto three_way_compare_expression = []( const auto* lhs, const Base* rhs )
		// {
		// 	using T = std::remove_pointer_t<decltype(lhs)>;
		// 	return *lhs <=> static_cast<const T&>( *rhs );
		// };
		// constexpr static auto equality_compare_expression = []( const auto* lhs, const Base* rhs )
		// {
		// 	using T = std::remove_pointer_t<decltype(lhs)>;
		// 	return *lhs == static_cast<const T&>( *rhs );
		// };

		detail::based_storage<Base,Children...> storage{};

		std::size_t index_ = npos;

		constexpr auto* that() noexcept
		{
			WERKZEUG_ASSERT( has_value(), "Accessing the member requires it to have a value" );
			return storage.get(index_);
		}

		constexpr auto* that() const noexcept
		{
			WERKZEUG_ASSERT( has_value(), "Accessing the member requires it to have a value" );
			return storage.get(index_);
		}

		template<typename T>
		static constexpr decltype(auto) as_impl( auto* variant ) noexcept
		{
			constexpr bool is_pointer = std::is_pointer_v<T>;

			using bare_T = std::remove_const_t<std::remove_pointer_t<std::remove_reference_t<T>>>;
			using c_qual_T = std::conditional_t<std::is_const_v<decltype(variant)>,const bare_T, bare_T>;

			if constexpr ( std::same_as<bare_T,Base> )
			{
				if constexpr ( is_pointer )
				{ return variant->that(); }
				else
				{ return *(variant->that()); }
			}
			else
			{
				WERKZEUG_ASSERT( variant->index_ == index_of<bare_T>, "Accessing for a specific type requires it to hold that type" );
				if constexpr ( is_pointer )
				{ return static_cast<c_qual_T*>( variant->that() ); }
				else
				{ return *static_cast<c_qual_T*>( variant->that() ); }
			}
		}

		template<std::size_t I> 
		static constexpr decltype(auto) as_impl( auto* variant ) noexcept
		{
			using c_qual_T = std::conditional_t<std::is_const_v<decltype(variant)>,const type<I>, type<I>>;

			if constexpr ( I == 0 )
			{
				return *variant->that();
			}
			else
			{
				WERKZEUG_ASSERT( variant->index_ == I, "Accessing for a specific type requires it to hold that type" );
				return *static_cast<c_qual_T*>( variant->that() );
			}
		}

	public :
		constexpr inheritance_variant() noexcept = default;

		constexpr ~inheritance_variant() noexcept
			requires joined_traits::destroy::trivial
			= default;

		constexpr ~inheritance_variant() noexcept 
		( joined_traits::destroy::nothrow )
			requires ( not joined_traits::destroy::trivial )
		{
			clear();
		}

		template<typename T>
		constexpr inheritance_variant( T&& obj ) 
			noexcept ( std::is_nothrow_constructible_v<T,decltype(obj)> )
			requires is_allowed<T>
		{
			emplace<T>( std::forward<T>(obj) );
		}

		template<typename T, typename ... Params>
		constexpr inheritance_variant( std::in_place_type_t<T>, Params&& ... params ) 
			noexcept( std::is_nothrow_constructible_v<T,Params...> )
			requires ( is_allowed<T> and std::is_constructible_v<T,Params...> )
		{
			emplace<T>( std::forward<T>(params) ... );
		}

		template<typename T>
		constexpr inheritance_variant& operator = ( const T& object ) 
			noexcept ( traits<T>::copy_assign::nothrow && traits<T>::copy_construct::nothrow )
			requires ( is_allowed<T> && traits<T>::copy_construct::possible )
		{
			if constexpr ( traits<T>::copy_assign::possible )
			{
				if ( index_ == index_of<T> )
				{
					if ( storage.apply( index_, copy_assign_expression, &object ) )
					{ return *this; }
				}
			}
			clear();
			index_ = index_of<T>;
			storage.apply( index_, copy_construct_expression,& object );
			return *this;
		}

		constexpr inheritance_variant( const inheritance_variant& other ) 
			noexcept ( joined_traits::copy_construct::nothrow )
			: index_(other.index_)
		{
			if ( other.has_value() )
			{
				storage.apply( index_, copy_construct_expression, other.that() );
			}
		}
		constexpr inheritance_variant( inheritance_variant&& other ) 
			noexcept ( joined_traits::move_construct::nothrow )
			: index_(other.index_)
		{
			if ( other.has_value() )
			{
				storage.apply( index_, move_construct_expression, other.that() );
			}
		}
		inheritance_variant( const inheritance_variant&& ) = delete;

		constexpr inheritance_variant& operator = ( const inheritance_variant& other ) 
			noexcept ( joined_traits::copy_assign::nothrow && joined_traits::copy_construct::nothrow )
		{
			if ( not other.has_value() )
			{
				clear();
				return *this;
			}

			if ( other.index_ == index_ )
			{
				if ( storage.template apply<bool>( index_, copy_assign_expression, other.that() ) )
				{
					return *this;
				}
			}
			clear();
			index_ = other.index_;
			storage.apply( index_, copy_construct_expression, other.that() );

			return *this;
		}

		constexpr inheritance_variant& operator = ( inheritance_variant&& other ) 
			noexcept ( joined_traits::move_assign::nothrow && joined_traits::move_construct::nothrow )
		{
			if ( not other.has_value() )
			{
				clear();
				return *this;
			}

			if ( other.index_ == index_ )
			{
				if ( storage.template apply<bool>( index_, move_assign_expression, other.that() ) )
				{
					return *this;
				}
			}
			clear();
			index_ = other.index_;
			storage.apply( index_, move_construct_expression, other.that() );

			return *this;
		}

		/**
		 * @brief constructs an object of type 'T' inside the variant, potentially destroying the held object first.
		 * 
		 * @tparam T type to construct
		 * @param params... parameters to construct the object from
		 * @returns reference to the emplaced object
		 */
		template<typename T, typename ... Params>
		constexpr T& emplace( Params&& ... params ) 
			noexcept( std::is_nothrow_constructible_v<T,Params...> )
			requires is_allowed<T>
		{
			clear();
			index_ = index_of<T>;
			return storage.template construct<index_of<T>>( std::forward<Params>(params)... );
		}

		template<std::size_t I, typename ... Params>
			requires ( I < alternative_count )
		constexpr auto& emplace( Params&& ... params ) 
			noexcept( std::is_nothrow_constructible_v< type<I>,Params...> )
		{
			clear();
			index_ = I;
			return storage.template construct<I>( std::forward<Params>(params)... );
		}

		constexpr void clear() noexcept( joined_traits::destroy::nothrow )
		{
			if ( has_value() )
			{
				storage.apply( index_, destroy_expression );
				index_ = npos;
			}
		}

		/** @brief querries whether the variant is currently holding any value*/
		[[nodiscard]] constexpr bool has_value() const noexcept
		{ return index_ != npos; }

		/** @brief gets the current index of the held type from the list of types the variant was instantiated with*/
		[[nodiscard]] constexpr auto index() const noexcept
		{ return index_; }

		/** @brief checks whether the variant currently holds the alternative with index I*/
		template<std::size_t I>
		[[nodiscard]] constexpr bool holds_alternative() const noexcept
		{ return I == index_; }

		/** @brief checks whether the variant currently holds the alternative type T*/
		template<typename T>
			requires ( is_allowed<std::decay_t<T>> and all_unique_v<Base,Children...> )
		[[nodiscard]] constexpr bool holds_alternative() const noexcept
		{ return index_of<T> == index_; }


		/** @brief returns the variant as the desired type. Can be used to obtain pointers or references to the held object. The variant must actually hold the specified type.*/
		template<typename T>
			requires ( is_allowed<std::decay_t<T>> and all_unique_v<Base,Children...> )
		[[nodiscard]] constexpr decltype(auto) as() noexcept
		{
			return as_impl<T>(this);
		}

		/** @brief returns the variant as the desired type. Can be used to obtain pointers or references to the held object. The variant must actually hold the specified type.*/
		template<typename T>
			requires ( is_allowed<std::decay_t<T>> and all_unique_v<Base,Children...> )
		[[nodiscard]] constexpr decltype(auto) as() const noexcept
		{
			return as_impl<T>(this);
		}

		template<std::size_t I>
			requires ( I <= sizeof...(Children))
		[[nodiscard]] constexpr decltype(auto) as() noexcept
		{
			return as_impl<I>(this);
		}

		template<std::size_t I>
			requires ( I <= sizeof...(Children))
		[[nodiscard]] constexpr decltype(auto) as() const noexcept
		{
			return as_impl<I>(this);
		}

		[[nodiscard]] constexpr auto operator -> () noexcept
		{
			return that();
		}

		[[nodiscard]] constexpr auto operator -> () const noexcept
		{
			return that();
		}


		[[nodiscard]] constexpr friend auto operator <=> ( const inheritance_variant& lhs, const inheritance_variant& rhs ) 
			noexcept( joined_traits::three_way_compare::nothrow )
			requires std::three_way_comparable<Base> || joined_traits::three_way_compare::possible
		{
			using three_way_result_t = std::common_comparison_category_t<
				std::compare_three_way_result_t<Base>,
				std::compare_three_way_result_t<Children>...
			>;

			if ( not lhs.has_value() && not rhs.has_value() )
			{
				return three_way_result_t::equivalent;
			}
			else if ( not lhs.has_value() )
			{
				return three_way_result_t::less;
			}
			else if ( not rhs.has_value() )
			{
				return three_way_result_t::greater;
			}
			else if ( lhs.index_ != rhs.index_ )
			{
				return three_way_result_t{ lhs.index_ <=> rhs.index_ };
			}
			else
			{
				// if constexpr ( ( std::three_way_comparable<Children> && ... ) )
				// {
				// 	return three_way_result_t{ lhs.storage.apply( lhs.index_, three_way_compare_expression, rhs.that() ) };
				// }
				// else
				{	
					return three_way_result_t{ *(lhs.that()) <=> *(rhs.that()) };
				}
			}
		}

		[[nodiscard]] friend constexpr bool operator == ( const inheritance_variant& lhs, const inheritance_variant& rhs ) 
			noexcept ( joined_traits::three_way_compare::nothrow )
			requires ( traits<Base>::equality_compare::possible ) 
		{
			if ( not lhs.has_value() or not rhs.has_value() or lhs.index_ != rhs.index_ )
			{
				return false;
			}
			else
			{
				// if constexpr ( (traits<Children>::equality_compare::possible && ... ))
				// {
				// 	return lhs.storage.apply( lhs.index_, equality_compare_expression, rhs.that() );
				// }
				// else
				{
					return *(lhs.that()) == *(rhs.that());
				}
			}
		}

		friend std::ostream& operator << ( std::ostream& os, const inheritance_variant& variant )
			requires ( streamable<Base> || (( streamable<Children> && ... ) && abstract_base ) )
		{
			if ( not variant.has_value() )
			{
				os << "{}";
			}
			else
			{
				if constexpr ( ( streamable<Children> && ... ) )
				{
					constexpr static auto print_expression = []( const auto* object, std::ostream& os_ )
					{
						using T = std::remove_pointer_t<decltype(object)>;
						os_ << static_cast<const T&>(*object);
					};

					variant.storage.apply( variant.index_, print_expression, os );
				}
				else
				{
					os << *( variant.that() );
				}
			}
			return os;
		}

		/** @brief gets the hash of the held object*/
		std::size_t hash() const 
			noexcept( joined_traits::hash::nothrow )
			requires( joined_traits::hash::possible )
		{
			constexpr static auto hash_expression = []( const auto* object )
			{
				using T = std::remove_pointer_t<decltype(object)>;
				return std::hash<T>{}( static_cast<const T&>(*object));
			};

			return storage.template apply<std::size_t>( index_, hash_expression );
		}


		/** @brief swaps `*this` and `other`. If both hold the same alternative the members are swapped directly.*/
		void swap( inheritance_variant& other )
			noexcept( joined_traits::swap::nothrow )
			requires( joined_traits::swap::possible )
		{
			if ( index_ == other.index_ and index_ != npos )
			{
				constexpr auto swap_expression = []( auto* self_obj, inheritance_variant& other_variant )
				{
					using T = std::remove_pointer_t<decltype(self_obj)>;
					T& a = static_cast<T&>( * self_obj );
					T& b = other_variant.template as<T&>();
					using std::swap;
					swap( a, b );
				};
				storage.template apply( index_, swap_expression, other );
			}
			else
			{
				inheritance_variant temp = std::move(other);
				other = std::move(*this);
				*this = std::move(temp);
			}
		}

		template<typename Fct, typename ... Params>
		static constexpr bool nothrow_apply = decltype(storage)::template nothrow_apply<Fct,Params...>;

	private:
		template<typename Fct, typename ... Params>
		static constexpr decltype(auto) visit_impl ( auto&& variant, Fct&& visitor, Params&& ... params ) 
			noexcept ( nothrow_apply<Fct,Params...> )
		{
			constexpr bool is_rvalue = std::is_rvalue_reference_v<decltype(variant)>;
			
			using result_type = typename common_result_type_or_void<Fct,Base,Children...>::type;

			constexpr auto invoke_expression = []( auto* object, auto&& visitor_, auto&&... params_ ) -> decltype(auto)
			{
				using T = std::remove_const_t<std::remove_pointer_t<decltype(object)>>;
				using cast_type = std::conditional_t<is_rvalue, T&&,const T&>;
				
				return std::forward<decltype(visitor_)>(visitor_)( static_cast<cast_type>(*object), std::forward<decltype(params_)>(params_) ... );
			};

			return variant.storage.template apply<result_type>( variant.index_, invoke_expression, std::forward<Fct>(visitor), std::forward<Params>(params)... );
		}

	public:
		/**
		 * @brief invokes 'visitor' on the currently held alternative
		 * 
		 * @param visitor a visitor that must be callable with all possible alternative types
		 * @param params... additional arguments that may be passed to the visitor
		 * @return the return value of the visitor or void if no common return type exists
		 */
		template<typename Fct, typename ... Params>
		constexpr decltype(auto) visit( Fct&& visitor, Params&& ... params ) &
			noexcept( nothrow_apply<Fct,Params...> )
		{
			return visit_impl( *this, std::forward<Fct>(visitor), std::forward<Params>(params) ... );
		}

		/**
		 * @brief invokes 'visitor' on the currently held alternative
		 * 
		 * @param visitor a visitor that must be callable with all possible alternative types
		 * @param params... additional arguments that may be passed to the visitor
		 * @return the return value of the visitor or void if no common return type exists
		 */
		template<typename Fct, typename ... Params>
		constexpr decltype(auto) visit( Fct&& visitor, Params&& ... params ) const &
			noexcept( nothrow_apply<Fct,Params...> )
		{
			return visit_impl( *this, std::forward<Fct>(visitor), std::forward<Params>(params) ... );
		}

		/**
		 * @brief invokes 'visitor' on the currently held alternative
		 * 
		 * @param visitor a visitor that must be callable with all possible alternative types
		 * @param params... additional arguments that may be passed to the visitor
		 * @return the return value of the visitor or void if no common return type exists
		 */
		template<typename Fct, typename ... Params>
		constexpr decltype(auto) visit( Fct&& visitor, Params&& ... params ) &&
			noexcept( nothrow_apply<Fct,Params...> )
		{
			return visit_impl( std::move(*this), std::forward<Fct>(visitor), std::forward<Params>(params) ... );
		}
	};

	template<typename Fct, typename Base, typename ... Children, typename ... Args>
	constexpr decltype(auto) visit( Fct&& visitor, inheritance_variant<Base,Children...>& variant, Args&& ... args )
		noexcept( decltype(variant)::template nothrow_apply<Fct,Args...> )
	{
		return	variant.visit( std::forward<Fct>(visitor), std::forward<Args>(args) ...  );
	}

	template<typename Fct, typename Base, typename ... Children, typename ... Args>
	constexpr decltype(auto) visit( Fct&& visitor, const inheritance_variant<Base,Children...>& variant, Args&& ... args )
		noexcept( inheritance_variant<Base,Children...>::template nothrow_apply<Fct,Args...> )
	{
		return variant.visit( std::forward<Fct>(visitor), std::forward<Args>(args) ...  );
	}

	template<typename Fct, typename Base, typename ... Children, typename ... Args>
	constexpr decltype(auto) visit( Fct&& visitor, inheritance_variant<Base,Children...>&& variant, Args&& ... args )
		noexcept( inheritance_variant<Base,Children...>::template nothrow_apply<Fct,Args...> )
	{
		return std::move(variant).visit( std::forward<Fct>(visitor), std::forward<Args>(args) ...  );
	}

	template<typename Base, typename ... Children>
	void swap( inheritance_variant<Base,Children...>& a, inheritance_variant<Base,Children...>& b )
		noexcept ( detail::joined_traits<Base,Children...>::swap::nothrow )
	{
		return a.swap(b);
	}
}

template<typename Base, typename ... Children>
	requires ( ::werkzeug::detail::joined_traits<Base,Children...>::hash::possible )
struct std::hash<werkzeug::inheritance_variant<Base,Children...>>
{
	std::size_t operator()( const werkzeug::inheritance_variant<Base,Children...>& variant ) const
		noexcept( ::werkzeug::detail::joined_traits<Base,Children...>::hash::nothrow )
	{
		return variant.hash();
	}
};

#endif