#ifndef WERKZEUG_GUARD_RANDOM_DISTRIBUTIONS_HPP
#define WERKZEUG_GUARD_RANDOM_DISTRIBUTIONS_HPP

#include <cstdint>
#include <type_traits>
#include <iostream>

#include "werkzeug/random/detail/defaults.hpp"

namespace werkzeug
{
	namespace detail
	{
		/** @brief fast uniform int implementation
		 * 
		 * See https://www.pcg-random.org/posts/bounded-rands.html
		 * 
		 * and the original paper https://arxiv.org/pdf/1805.10941.pdf
		 */
		template<typename Generator>
		std::uint32_t bounded_rand(Generator& rng, std::uint32_t range) 
		{
			using uint32 = std::uint32_t;
			using uint64 = std::uint64_t;

			uint32 x = rng();
			uint64 m = uint64(x) * uint64(range);
			uint32 l = uint32(m);
			if (l < range) 
			{
				uint32 t = -range;
				if (t >= range) 
				{
					t -= range;
					if (t >= range) 
						t %= range;
				}
				while (l < t) 
				{
					x = rng();
					m = uint64(x) * uint64(range);
					l = uint32(m);
				}
			}
			return m >> 32;
		}
	}
	/**
	 * @brief Faster and more importantly implementation fixed uniform distribution. The standard distributions are unspecified so they may and *do* change between compilers and compiler versions.
	 * 
	 * @tparam T 
	 */
	template<typename T>
	struct fast_uniform_distribution
	{
		using result_type = T;
		struct param_type
		{
			using distribution_type = fast_uniform_distribution;
			T min_;
			T max_;

			constexpr T min() const noexcept
			{ return min_; }

			constexpr T max() const noexcept
			{ return max_; }
		};

	protected :
		param_type param_; 

		template<typename Generator>
		static std::uint64_t create( std::uint64_t s, Generator& g ) noexcept ( noexcept(g()) )
		{
			static_assert( std::is_integral_v<T> );
			std::uint64_t t = (-s ) % s ;
			std::uint64_t x;
			do {
				x = g();
			} while ( x < t );

			return x % s ;
		}

	public :
		constexpr fast_uniform_distribution( const param_type param = param_type() ) noexcept
			: param_( param )
		{}

		constexpr fast_uniform_distribution( const T min, const T max ) noexcept
			: param_{min, max}
		{}

		static constexpr void reset() noexcept
		{}

		constexpr param_type param() const noexcept
		{ return param_; }

		constexpr void param ( const param_type p ) noexcept
		{ param_ = p; }

		constexpr T min() const noexcept
		{ return param_.min(); }

		constexpr T max() const noexcept
		{ return param_.max(); }

		constexpr bool operator == ( const fast_uniform_distribution& other ) const noexcept
		{ return param_ == other.param_; }
		constexpr bool operator != ( const fast_uniform_distribution& other ) const noexcept
		{ return param_ != other.param_; }

		friend std::ostream& operator << ( std::ostream& os, const fast_uniform_distribution& dist ) noexcept
		{
			os << dist.min() << ' ' << dist.max();
			return os;
		}
		friend std::istream& operator >> ( std::istream& is, fast_uniform_distribution& dist ) noexcept
		{
			is >> dist.param_.min_ >> dist.param_.max_;
			return is;
		}

		template<typename Generator>
		T operator () ( Generator& g ) const noexcept( noexcept( g() ) )
		{
			return operator()( g, param_ );
		}

		template<typename Generator>
		T operator () ( Generator& g, const param_type p ) const noexcept( noexcept( g() ) )
		{
			if constexpr ( std::is_floating_point_v<T> )
			{
				const T value_range = p.max() - p.min();

				const auto entropy = g();

				T r;
				// https://prng.di.unimi.it/#remarks for the specialized float and double case
				constexpr bool gen_has_uint64 = std::is_same_v<decltype(entropy),std::uint64_t>;
				constexpr bool can_float = gen_has_uint64 && sizeof(float) == 4 && std::is_same_v<T,float> && std::numeric_limits<float>::is_iec559;
				constexpr bool can_double = gen_has_uint64 && sizeof(double) == 8 && std::is_same_v<T,double> && std::numeric_limits<double>::is_iec559;
				if constexpr ( can_float )
				{
					r = static_cast<float>( entropy >> 8 ) * 0x1.0p-24;
				}
				else if constexpr ( can_double )
				{
					r = static_cast<double>(entropy >> 11) * 0x1.0p-53;
				}
				else
				{
					constexpr T factor = static_cast<T>(1) / static_cast<T>( Generator::max() - Generator::min() );
					r = static_cast<T>( entropy ) * factor;
				}
				return p.min() + r * value_range;
			}
			else if constexpr ( std::is_integral_v<T> )
			{
				const auto value_range = static_cast<std::uint64_t>( p.max() ) + static_cast<std::uint64_t>( std::abs( p.min() ) );

				auto v = create( value_range, g );
				if constexpr ( std::is_unsigned_v<T> ) //if its unsigned, then this is always fine
				{
					return p.min() + static_cast<T>( v );
				}
				else
				{
					constexpr auto max_T = static_cast<decltype(v)>( std::numeric_limits<T>::max() );
					if ( v > max_T ) //overflow check
					{
						v -= max_T;
						return p.min() + static_cast<T>( v ) + std::numeric_limits<T>::max();
					}
					else
					{
						return p.min() + static_cast<T>( v );
					}
				}
			}
			else
			{
				static_assert( false && sizeof(T) );
			}
		}
	};
	
}
#endif