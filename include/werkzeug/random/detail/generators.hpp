#ifndef WERKZEUG_GUARD_RANDOM_GENERATORS_HPP
#define WERKZEUG_GUARD_RANDOM_GENERATORS_HPP

#include <array>
#include <cstdint>

namespace werkzeug::detail
{
    /** @brief see https://prng.di.unimi.it/
     */

    constexpr std::uint64_t rotate(const std::uint64_t x, const int k) noexcept
	{
		return (x << k) | (x >> (64 - k));
	}

    class SplitMix64
	{
		std::uint64_t state = 0;

	public:
		explicit constexpr SplitMix64(const std::uint64_t seed ) noexcept
            : state( seed )
        { }

		[[nodiscard]] constexpr std::uint64_t operator()() noexcept
        {
            std::uint64_t result = (state += 0x9E3779B97F4A7C15);
            result = (result ^ (result >> 30)) * 0xBF58476D1CE4E5B9;
            result = (result ^ (result >> 27)) * 0x94D049BB133111EB;
            return result ^ (result >> 31);
        }

		template<std::size_t N>
		[[nodiscard]] constexpr std::array<std::uint64_t, N> generate_seed_sequence() noexcept
		{
			std::array<std::uint64_t, N> res{};
			for (std::size_t i = 0; i < N; ++i)
			{
				res[i] = (*this)();
			}
			return res;
		}
	};


    class Xoshiro256StarStar
    {
        std::array<std::uint64_t, 4> state;
        
        void advance_state() noexcept
        {
            const auto tmp = state[1] << 17;

            state[2] ^= state[0];
            state[3] ^= state[1];
            state[1] ^= state[2];
            state[0] ^= state[3];

            state[2] ^= tmp;

            state[3] = rotate(state[3], 45);
        }

    public:
        using result_type = std::uint64_t;

        explicit Xoshiro256StarStar(const std::uint64_t seed) noexcept
            : state( SplitMix64{seed}.generate_seed_sequence<4>() )
        {}

        result_type operator () () noexcept
        {
            const auto result = rotate(state[1] * 5, 7) * 9;
            advance_state();
            return result;
        }

        [[nodiscard]] static constexpr result_type min() noexcept
        {
            return std::numeric_limits<std::uint64_t>::min();
        }
        [[nodiscard]] static constexpr result_type max() noexcept
        {
            return std::numeric_limits<result_type>::max();
        }
        
        [[nodiscard]] std::array<result_type, 4> get_state() const noexcept
        {
            return state;
        }

        void discard( const std::size_t n ) noexcept
        {
            for ( std::size_t i=0; i < n; ++i )
            {
                advance_state();
            }
        }
        
        void set_state(const std::array<std::uint64_t, 4>& new_state) noexcept
        {
            state = new_state;
        }

        [[nodiscard]] friend bool operator == (const Xoshiro256StarStar& lhs, const Xoshiro256StarStar& rhs) noexcept
        {
            return lhs.state == rhs.state;
        }

        friend std::ostream& operator << (std::ostream& lhs, const Xoshiro256StarStar& rhs)
        {
            lhs << rhs.state[0] << ' ' << rhs.state[1] << ' ' << rhs.state[2] << ' ' << rhs.state[3];
            return lhs;
        }

        friend std::istream& operator >> (std::istream& lhs, Xoshiro256StarStar& rhs)
        {
            lhs >> rhs.state[3] >> rhs.state[2] >> rhs.state[1] >> rhs.state[0];
            return lhs;
        }
    };
}


#endif