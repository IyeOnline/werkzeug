#ifndef RANDOM_HPP
#define RANDOM_HPP

#include <atomic>
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <limits>
#include <random>
#include <type_traits>

#include "werkzeug/random/detail/defaults.hpp"
#include "werkzeug/random/detail/distributions.hpp"
#include "werkzeug/random/detail/generators.hpp"

namespace werkzeug
{
	class RNG
	{
	public:
		/** @brief controls whether faster (but worse) uniform distributions should be used */
		constexpr static bool use_fast_distributions = true;
		/** @brief constrols whether a faster (self implemented) entropy generator should be used */
		constexpr static bool use_fast_generator = true;

	private:
		template<typename T>
		using uniform_float_dist_t = std::conditional_t<
			use_fast_distributions,
			fast_uniform_distribution<T>,
			std::uniform_real_distribution<T>
		>;

		template<typename T>
		using uniform_int_dist_t = std::conditional_t<
			use_fast_distributions,
			fast_uniform_distribution<T>,
			std::uniform_real_distribution<T>
		>;


		template<typename T>
		using uniform_dist_t = std::conditional_t<
			std::is_integral_v<T>,
			uniform_int_dist_t<T>,
			uniform_float_dist_t<T>
		>;

		using gen_t = std::conditional_t<
			use_fast_generator,
			detail::Xoshiro256StarStar,
			std::mt19937_64
		>;

	public:
		
		using seed_t = gen_t::result_type;
		using result_type = gen_t::result_type;

		constexpr static bool noexcept_invocable = std::is_nothrow_invocable_v<gen_t>;

	private:
		constexpr static bool nx_discard = 
			noexcept( std::declval<gen_t>().discard( seed_t{} ) );
		constexpr static bool nx_seed_ctor = 
			std::is_nothrow_constructible_v<gen_t,seed_t> &&
			nx_discard;
		constexpr static bool nx_move_ctor =
			std::is_nothrow_move_constructible_v<gen_t>;
		constexpr static bool nx_move_assign = 
			std::is_nothrow_move_assignable_v<gen_t>;
		constexpr static bool nx_dtor = 
			std::is_nothrow_destructible_v<gen_t>;
		constexpr static bool nx_compare =
			noexcept( std::declval<gen_t>() == std::declval<gen_t>() );

		static inline seed_t seed_{};
		static inline std::atomic<std::size_t> instance_counter_{};
		
		gen_t gen_;

		// #ifndef NDEBUG
		// std::size_t invocation_counter_{};
		// #endif

	public :
		/** @brief sets the seed for the RNG class.
		 * 
		 * If called with 0 as an argument, randomly selects a seed and "returns" it.
		 * 
		 * Should only be called by the config parser and only be called once.
		 */
		static seed_t initialize( seed_t ) noexcept;
		
		[[nodiscard]] static inline seed_t get_config_seed() noexcept
		{ return seed_; }

		RNG() noexcept( nx_seed_ctor )
			: gen_( seed_ )
		{
			//this will ensure instances of the generator are not indentical
			gen_.discard( instance_counter_++ );
		}
		
		//copy ctor doesnt actually copy. It creates a new instance exactly as if it were default constructed. It only exists because OMP firstprivate relies on it.
		RNG( const RNG& ) noexcept( nx_seed_ctor )
			: RNG() 
		{}

		//copy assignment does not assign anything.
		RNG& operator=( const RNG& ) = delete;
		// {
		// 	return *this;
		// }

		RNG( RNG&& ) noexcept( nx_move_ctor )
			= default;
		RNG& operator=( RNG&& ) noexcept( nx_move_assign )
			= default;

		~RNG() noexcept( nx_dtor ) 
			= default;

		/** @brief gets the smallest possible value in the output range of the rng*/
		[[nodiscard]] constexpr static inline result_type min() noexcept
		{ return decltype(gen_)::min(); }
		
		/** @brief gets the largest possible value in the output range of the rng*/
		[[nodiscard]] constexpr static inline result_type max() noexcept
		{ return decltype(gen_)::max(); }
		
		/** @brief gets a new bit of entropy */
		[[nodiscard]] inline result_type operator()() noexcept(noexcept_invocable)
		{ 
			// #ifndef NDEBUG
			// ++invocation_counter_;
			// #endif
			return gen_(); 
		}
		
		/** @brief discards 'z' results/advances the state by 'z' steps */
		void discard( unsigned long long z ) noexcept( nx_discard )
		{ 
			// #ifndef NDEBUG
			// invocation_counter_ += z;
			// #endif
			gen_.discard( z ); 
		}
		
		friend std::ostream& operator<<( std::ostream& os, const RNG& me )
		{ return os << me.gen_; }
		
		friend std::istream& operator>>( std::istream& is, RNG& me )
		{ return is >> me.gen_; }
		
		friend bool operator==( const RNG& lhs, const RNG& rhs ) noexcept (nx_compare)
		{ return lhs.gen_ == rhs.gen_; }

		// #ifndef NDEBUG
		// [[nodiscard]] inline std::size_t invocation_counter() const noexcept
		// { return invocation_counter_; }
		// #endif
		

		/** @brief returns random a value
		 * 
		 * @tparam T the type you want. Deduction has been disabled for type safety reasons.
		 * 
		 * @param[in] min lower bound of the distribution
		 * @param[in] max upper bound of the distribution
		 * 
		 * @returns a random value of type 'T' in the range [min,max]
		 */
		template<typename T>
		[[nodiscard]] inline T next( const std::common_type_t<T> min, const std::common_type_t<T> max ) noexcept(noexcept_invocable)
		{
			return uniform_dist_t<T>( min, max )( *this );
		}

		/** @brief returns a value
		 * 
		 * @tparam T the type you want. Deduction has been disabled for type safety reasons. Must be floating point
		 * 
		 * @returns a random value in the range of [0,1]
		 */
		template<typename T>
		[[nodiscard]] inline T next() noexcept(noexcept_invocable)
		{
			static_assert( std::is_floating_point_v<T>, "No argument 'next<T>' is only availible for floating point types" );
			return next<T>( detail::defaults<T>::min, detail::defaults<T>::max );
		}

		/** @brief does a coinflip
		 * 
		 * @returns true or false with equal probability
		*/
		[[nodiscard]] inline bool coinflip() noexcept(noexcept_invocable)
		{
			return ( operator()() & 0b1 ) == 0b1; //bit masking to check if lsb of entropy is 1. Better than using uniform dist for this.
		}


		/**
		 * @brief does a biased coinflip
		 * 
		 * @param probability for a true result

		 * @return with the given probability
		 */
		template<typename T>
		[[nodiscard]] inline bool biased_coinflip( T probability ) noexcept(noexcept_invocable)
		{
			static_assert( std::is_floating_point_v<T>, "'chance' must be a floating point value" );

			WERKZEUG_ASSERT( probability < T{1}, "'chance' must be less than one" );

			const auto value = next<T>();

			return value <= probability;
		}

		/** 
		 * @brief given a distribution represented by dist, pick an index
		 */
		template<std::ranges::range Range>
		[[nodiscard]] inline std::size_t pick_index_from_distribution( const Range& dist ) noexcept(noexcept_invocable)
		{
			return std::discrete_distribution<std::size_t>{ std::begin(dist), std::end(dist) }( *this );
		}

		/** 
		 * @brief selects an element from the range [begin, end) with equal probability for every element
		 * 
		 * @tparam It An iterator type
		 * 
		 * @param begin the iterator pointing to the first element in the range
		 * @param end the "past end" iterator for the range
		 * 
		 * @returns an element from the range
		 */
		template<typename It>
		[[nodiscard]] inline const auto& select_uniform_from_range( It begin, It end ) noexcept(noexcept_invocable)
		{
			const auto size = std::distance( begin, end );
			WERKZEUG_ASSERT( size != 0, "Fatal error. given range is empty" );

			const auto idx = next<decltype(size)>( 0, size-1 );
			std::advance(begin,idx);
			return *begin;
		}

		/**
		 * @brief selects an element from the range
		 */
		template<std::ranges::range Range>
		[[nodiscard]] inline const auto& select_uniform_from_range( const Range& r ) noexcept(noexcept_invocable)
		{
			using std::begin, std::end;
			return select_uniform_from_range( begin(r), end(r) );
		}
	};
	
	/** @brief global static rng instance for convienience. usage is NOT THREADSAFE */
	static inline RNG static_rng;

	/** @brief QoL wrapper for static_rng.next() */
	template<typename T>
	[[nodiscard]] inline T next() noexcept(RNG::noexcept_invocable)
	{
		return static_rng.next<T>();
	}


	/** @brief QoL wrapper for static_rng.next() */
	template<typename T>
	[[nodiscard]] inline T next( const std::common_type_t<T> min, const std::common_type_t<T> max ) noexcept(RNG::noexcept_invocable)
	{
		return static_rng.next<T>( min, max );
	}

	/** @brief QoL wrapper for static_rng.coinflip() */
	[[nodiscard]] inline bool coinflip() noexcept(RNG::noexcept_invocable)
	{
		return static_rng.coinflip();
	}
}
#endif