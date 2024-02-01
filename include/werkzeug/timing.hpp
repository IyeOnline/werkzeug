#ifndef WERKZEUG_GUARD_TIMING_HPP
#define WERKZEUG_GUARD_TIMING_HPP

#include <array>
#include <atomic>
#include <chrono>
#include <string>
#include <type_traits>
#include <utility>
#include <concepts>
#include <functional>

namespace werkzeug
{
	template<typename T, typename Duration>
	struct measurement_result
	{
		T result;
		Duration duration;
	};

	template<typename Duration>
	struct measurement_result<void,Duration>
	{
		Duration duration;
	};

	/**
	* @brief measures the time `f` takes to run with `args...`
	* 
	* @tparam Clock clock type to use, defaults to steady clock
	* @param f function to measure
	* @param args... arguments to pass to `f`
	* @returns a struct that has a member `duration` and, if the function returns non-void, also a member `result`
	*/
	template<typename Clock = std::chrono::steady_clock, typename Fct = void, typename ... Args>
	auto measure( Fct&& f, Args&& ... args )
	{
		const auto start = Clock::now();

		if constexpr ( not std::is_same_v<std::invoke_result_t<Fct,Args...>,void> )
		{
			auto result_ = std::invoke( std::forward<Fct>(f), std::forward<Args>(args) ... );
			auto duration_ = Clock::now() - start;


			return measurement_result{ std::move(result_), std::move(duration_) };
		}
		else
		{
			std::invoke( std::forward<Fct>(f), std::forward<Args>(args) ... );
			auto duration_ = Clock::now() - start;

			return measurement_result<void,decltype(duration_)>{ std::move(duration_) };
		}
	}

	template<typename Clock = std::chrono::steady_clock>
	class Timer
	{
		using time_point = typename Clock::time_point;

		time_point last = now();

		const auto now() const noexcept
		{
			return Clock::now();
		}
	public :
		[[nodiscard]] auto time_elapsed() const noexcept
		{
			return now() - last;
		}

		auto reset() noexcept
		{
			return now() - std::exchange( last, now() );
		}
	};

	class Progress_Meter
	{
		std::size_t max{};
		std::atomic<std::size_t> counter{};
		double interval{};
		double last_mark{};
	public :
		Progress_Meter( std::size_t max_, const double interval_percent_ = .1 ) noexcept
			: max(max_), interval(interval_percent_)
		{ }

		std::size_t increment() noexcept
		{
			return ++counter;
		}

		double get_completion_ratio() const noexcept
		{
			return static_cast<double>(counter)/static_cast<double>(max);
		}

		double get_percentage() const noexcept
		{
			return  get_completion_ratio() * 100.0;
		}

		bool passed_interval() noexcept
		{
			const auto p = get_percentage();
			if ( p > last_mark + interval )
			{
				last_mark = p;
				return true;
			}
			return false;
		}
	};

	std::ostream& operator << ( std::ostream& os, const Progress_Meter& prog )
	{
		os << std::setw(7) << std::setprecision(1) << std::fixed << prog.get_percentage() << "%";
		return os;
	}

	template<typename Clock = std::chrono::steady_clock>
	class Progress_Meter_With_Timer : public Progress_Meter, public Timer<Clock>
	{
	public :
		using Progress_Meter::Progress_Meter;
		using Timer<Clock>::time_elapsed;

		auto estimate_remaining_time() const noexcept
		{
			const auto elapsed = time_elapsed();
			const auto completion_ratio = get_completion_ratio();

			const auto remaning_ratio = 1.0-completion_ratio;
			
			return remaning_ratio / completion_ratio * elapsed;
		}
	};

	template<typename Clock>
	std::ostream& operator << ( std::ostream& os, const Progress_Meter_With_Timer<Clock>& prog )
	{
		os << static_cast<const Progress_Meter&>(prog);
		os << ". Time elapsed: " << duration_string( prog.time_elapsed() );
		os << ". Time remaning est: " << duration_string( prog.estimate_remaining_time() );
		return os;
	}


	/** Converts a duration to a string representation.*/
	template<std::size_t segment_count=2, typename T, typename D>
	std::string duration_string( const std::chrono::duration<T,D>& d )
	{
		constexpr std::array names = {"d", "h", "m", "s", "ms", "μs", "ns" };

		const auto as_hours = std::chrono::duration_cast<std::chrono::hours>( d ).count();
		const std::array arr = {
			as_hours / 24,
			as_hours % 24,
			std::chrono::duration_cast<std::chrono::minutes>(d).count() % 60,
			std::chrono::duration_cast<std::chrono::seconds>(d).count() % 60,
			std::chrono::duration_cast<std::chrono::milliseconds>(d).count() % 1000,
			std::chrono::duration_cast<std::chrono::microseconds>(d).count() % 1000,
			std::chrono::duration_cast<std::chrono::nanoseconds>(d).count() % 1000
		};
		
		std::string res;

		bool need_sep = false;
		std::size_t c = 0;
		for ( std::size_t i=0; i < arr.size(); ++i )
		{
			if ( arr[i] > 0 )
			{
				if ( need_sep )
				{
					res += " ";
				}
				res += std::to_string(arr[i]) + names[i];
				need_sep = true;
				++c;
				if ( c == segment_count )
				{
					break;
				}
			}
		}

		return res;
	} 
}

#endif