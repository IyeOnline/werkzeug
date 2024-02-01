#ifndef WERKZEUG_GUARD_EXPECTED_HPP
#define WERKZEUG_GUARD_EXPECTED_HPP

#include <type_traits>
#include <utility>
#include <variant>

#include "werkzeug/parameter_pack.hpp"
#include "werkzeug/traits.hpp"
#include "werkzeug/overload.hpp"


namespace werkzeug
{
	template<typename T, typename ... Errors>
		requires all_unique_v<Errors...>
	class Expected;

	template<typename T>
	struct is_expected
		: std::false_type
	{ };

	template<typename T, typename ... Errors>
	struct is_expected<Expected<T,Errors...>>
		: std::true_type
	{ };

	namespace detail::_expected
	{
		template<typename Value_Type, typename Error_Pack>
		struct make_expected_type;

		template<typename Value_Type, typename ... Error_Types>
		struct make_expected_type<Value_Type, type_pack<Error_Types...>>
			: std::type_identity<Expected<Value_Type,Error_Types...>>
		{ };

		template<typename Value_Type, typename Error_Pack>
		using make_expected_type_t = typename make_expected_type<Value_Type,Error_Pack>::type;

		template<typename ... Out, typename ... In>
		std::variant<Out...> transform_variant( const std::variant<In...>& in )
		{
			constexpr auto v = []( const auto& value )
			{
				return std::variant<Out...>{ std::in_place_type<std::remove_cvref_t<decltype(value)>>, value };
			};
			return std::visit( v, in );
		}

		template<typename ... Out, typename ... In>
		std::variant<Out...> transform_variant( std::variant<In...>&& in )
		{
			constexpr auto v = []( auto&& value )
			{
				return std::variant<Out...>{ std::in_place_type<std::remove_cvref_t<decltype(value)>>, std::move(value) };
			};
			return std::visit( v, in );
		}

		template<type_pack_c Error_Pack, typename ... Args>
		struct first_is_error
			: std::false_type
		{ };

		template<type_pack_c Error_Pack, typename T0>
		struct first_is_error<Error_Pack, T0>
			: std::conditional_t<Error_Pack::template contains<T0>(), std::true_type, std::false_type>
		{ };
	}

	template<typename T, typename ... Errors>
		requires all_unique_v<Errors...>
	class Expected
	{
	public :
		using value_type = T;
		using error_pack = type_pack<Errors...>;
	private :
		using traits = type_traits<T>;
		std::variant<T,Errors...> data_;

	public :
		/**
		 * @brief constructs an expected holding the value from args...
		 * 
		 * @param args... arguments to construct the value from
		 */
		template<typename ... Args>
			requires ( 
				sizeof...(Args) > 0 &&
				not ( is_expected<Args>::value || ... ) &&
				not detail::_expected::first_is_error<error_pack,Args...>::value
			)
		constexpr Expected( Args&& ... args )
			: data_{ std::in_place_type<T>, std::forward<Args>(args) ... }
		{ }

		template<typename Error>
			requires is_type_in_pack_v<Error, error_pack>
		constexpr Expected( Error&& error )
			: data_{ std::in_place_type<std::remove_cvref_t<Error>>, std::forward<Error>(error) }
		{ }

		/**
		 * @brief constructs an expected holding the value from args...
		 * 
		 * @tparam U Type to construct. Can be the value type or an error type
		 * @param args... arguments to construct the value from
		 */
		template<typename U, typename ... Args>
		constexpr Expected( std::in_place_type_t<U> t, Args&& ... args )
			: data_{ t, std::forward<Args>(args) ... }
		{ }

		template<typename, typename ... Other_Errors>
			requires all_unique_v<Other_Errors...>
		friend class Expected;

		/**
		 * @brief Converting constructor, converting from a narrower to a wider error pack
		 * 
		 * @param other input Expected
		 */
		template<typename ... Other_Errors>
			requires pack_includes<error_pack, type_pack<Other_Errors...>>::value
		constexpr Expected( const Expected<T,Other_Errors...>& other )
			: data_{ detail::_expected::transform_variant<T,Errors...>( other.data_ ) }
		{ }

		/** @brief emplaces a T in `* this` */
		template<typename ... Args>
		constexpr T& emplace( Args&& ... args ) noexcept( std::is_nothrow_constructible_v<T,Args...> )
		{
			return data_.template emplace<T>( std::forward<Args>(args) ... );
		}

		
		/** @brief emplaces a `U` in `* this` */
		template<typename U, typename ... Args>
			requires ( std::same_as<U,T> or is_type_in_pack<T, error_pack>::value )
		constexpr T& emplace( std::in_place_type_t<T>, Args&& ... args ) noexcept( std::is_nothrow_constructible_v<T,Args...> )
		{
			return data_.template emplace<U>( std::forward<Args>(args) ... );
		}

		template<typename Error>
		constexpr static Expected make_error( Error&& error_value )
		{
			return Expected{ std::in_place_type<std::remove_cvref_t<Error>>, std::forward<Error>(error_value)  };
		}

		[[nodiscard]] constexpr bool has_value() const noexcept
		{ return data_.index() == 0; }

		constexpr operator bool() const noexcept
		{ return has_value(); }

		constexpr bool is_error() const noexcept
		{ return not has_value(); }

		constexpr auto index() const noexcept
		{ return data_.index(); }

		template<typename E>
			requires is_type_in_pack<E,error_pack>::value
		constexpr const E& get_error() const noexcept
		{ return std::get<E>( data_ ); }

		[[nodiscard]] constexpr T& operator*() & noexcept
		{ return std::get<T>(data_); };
		[[nodiscard]] constexpr const T& operator*() const & noexcept
		{ return std::get<T>(data_); };
		[[nodiscard]] constexpr T&& operator*() && noexcept
		{ return std::get<T>(std::move(data_)); };
		[[nodiscard]] constexpr const T&& operator*() const && noexcept
		{ return std::get<T>(std::move(data_)); };

		[[nodiscard]] constexpr T* operator->() noexcept
		{ return &operator*(); }
		[[nodiscard]] constexpr const T* operator->() const noexcept
		{ return &operator*(); }

		[[nodiscard]] constexpr T& value() & noexcept
		{ return operator*(); }
		[[nodiscard]] constexpr const T& value() const & noexcept
		{ return operator*(); }
		[[nodiscard]] constexpr T&& value() &&noexcept
		{ return operator*(); }
		[[nodiscard]] constexpr const T&& value() const && noexcept
		{ return operator*(); }

		[[nodiscard]] constexpr T& value_or( T& alternative ) & noexcept
		{ return has_value() ? value() : alternative; }
		[[nodiscard]] constexpr const T& value_or( const T& alternative ) const & noexcept
		{ return has_value() ? value() : alternative; }
		[[nodiscard]] constexpr T&& value_or( T&& alternative ) && noexcept
		{ return has_value() ? value() : alternative; }
		[[nodiscard]] constexpr const T&& value_or( const T&& alternative ) const && noexcept
		{ return has_value() ? value() : alternative; }


		/**
		 * @brief obtains a copy of the stored value or constructs a value from args...
		 * 
		 * @param ...args arguments to potentially use for construction
		 */
		template<typename ... Args>
		[[nodiscard]] constexpr T value_or_create( Args&& ... args ) const& 
			noexcept( traits::copy_construct::nothrow and traits::template construct_from<Args...>::nothrow )
			requires ( traits::template construct_from<Args...>::possible )
		{
			if ( has_value() )
			{
				return std::get<T>( data_ );
			}
			else
			{
				return T{ std::forward<Args>(args) ... };
			}
		}

		/**
		 * @brief moves from stored value or constructs a value from args...
		 * 
		 * @param ...args arguments to potentially use for construction
		 */
		template<typename ... Args>
		[[nodiscard]] constexpr T value_or_create( Args&& ... args ) && 
			noexcept( traits::move_construct::nothrow and traits::template construct_from<Args...>::nothrow )
			requires ( traits::template construct_from<Args...>::possible )
		{
			if ( has_value() )
			{
				return T{ std::move( std::get<T>( data_ ) ) };
			}
			else
			{
				return T{ std::forward<Args>(args) ... };
			}
		}

		[[nodiscard]] constexpr auto operator<=>( const Expected& ) const = default;
		[[nodiscard]] constexpr bool operator==( const Expected& ) const = default;


		/** @brief Invokes `f(value())` if `has_value()`, otherwise forwards the error */
		template<typename F>
			requires ( std::invocable<F,T> and not is_expected<std::invoke_result_t<F,T>>::value )
		auto and_then( F&& f ) const
		{
			using other_value_type = std::invoke_result_t<F,T>;
			using return_type = Expected<other_value_type, Errors ...>;
			
			const overload transformation = {
				[&f]( const T& t ){ return return_type{ std::in_place_type<other_value_type>, std::forward<F>( f )( t ) }; },
				[]( auto&& e ){ return return_type{ std::in_place_type<std::remove_cvref_t<decltype(e)>>, std::forward<decltype(e)>(e) }; }
			};
			return std::visit( transformation, data_ );
		}


		/** @brief Invokes `f(value())` if `has_value()`, otherwise forwards the error */
		template<typename F>
			requires ( std::invocable<F,T> and is_expected<std::invoke_result_t<F,T>>::value )
		auto and_then( F&& f ) const
		{
			using invoke_result = std::invoke_result_t<F,T>;
			using other_value_type = typename invoke_result::value_type;
			using other_error_pack = typename invoke_result::error_pack;

			using return_type = detail::_expected::make_expected_type_t< other_value_type, typename append_if_unique<error_pack, other_error_pack>::type>;

			const overload transformation = {
				[&f]( const T& t ){ 
					return return_type{ std::forward<F>( f )( t ) }; 
				}, 
				[]( const auto& t ){ return return_type{ std::in_place_type<std::remove_cvref_t<decltype(t)>>, t }; }
			};
			return std::visit( transformation, data_ );
		}


		/** @brief returns `*this` if `this->has_value()`, `Expected{ f() }` otherwise */
		template<typename F>
			requires std::same_as<T,std::invoke_result_t<F>>
		Expected or_else( F&& f ) const &
		{
			if ( has_value() )
			{
				return *this;
			}
			else 
			{
				return Expected{ std::in_place_type<T>, std::forward<F>(f) };
			}
		}

		/** @brief returns `*this` if `this->has_value()`, `Expected{ f() }` otherwise */
		template<typename F>
			requires std::same_as<T,std::invoke_result_t<F>>
		Expected or_else( F&& f ) &&
		{
			if ( has_value() )
			{
				return std::move(*this);
			}
			else 
			{
				return Expected{ std::in_place_type<T>, std::forward<F>(f) };
			}
		}
	};
}

#endif