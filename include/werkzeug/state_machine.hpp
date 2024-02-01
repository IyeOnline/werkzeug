#ifndef WERKKEUG_GUARD_STATE_MACHINE_HPP
#define WERKKEUG_GUARD_STATE_MACHINE_HPP

#include <utility>
#include <type_traits>
#include <limits>
#include <functional>
#include <array>



namespace werkzeug
{
	/**
	 * @brief A base implementation for a state machine
	 * 
	 * @tparam State_ID_t Enum type used to name the states. All members must be consecutive starting at 0
	 * @tparam N_States number of states
	 * @tparam Concrete_T CRTP parameter for the concrete implementations name
	 * @tparam Args... argument types to pass to the state function
	 */
	template<typename State_ID_t, size_t N_States, typename Concrete_T, typename ... Args>
		requires ( 
			std::is_enum_v<State_ID_t> 
			and std::is_unsigned_v<std::underlying_type_t<State_ID_t>> 
			and N_States <= std::numeric_limits<std::underlying_type_t<State_ID_t>>::max()
		)
	class State_Machine 
	{
		State_ID_t active_state_;

		using state_function = State_ID_t(Concrete_T::*)(Args...);
		using pointer_array_t = std::array<state_function,N_States>;

		template<size_t I>
		consteval static auto pointer_for_index() noexcept
		{
			static_assert( requires ( Concrete_T t, Args ... args )
			{
				{ t.template func_for<static_cast<State_ID_t>(I)>( args ... ) } -> std::same_as<State_ID_t>;
			}, "Concrete derived class must define a function 'func_for<E>(Args ... )' for every enum value E of type 'State_ID_t'");
			return static_cast<state_function>( &Concrete_T::template func_for< static_cast<State_ID_t>(I) > );
		}

		template<size_t ... Is>
		consteval static auto populate( std::index_sequence<Is...> ) noexcept
		{
			return pointer_array_t{ pointer_for_index<Is>() ... };
		}

		static constexpr pointer_array_t states_ = populate( std::make_index_sequence<N_States>() );

	public:
		constexpr State_Machine( State_ID_t initial_state )
			: active_state_{ initial_state }
		{ }

		[[nodiscard]] constexpr State_ID_t get_active() const noexcept
		{
			return active_state_;
		}

		constexpr State_ID_t execute_active( Args ... args )
		{
			const auto next_state = std::invoke( states_[static_cast<size_t>(active_state_)], static_cast<Concrete_T*>(this), std::forward<Args>(args) ... );
			active_state_ = next_state;
			return next_state;
		}
	};
}


#endif