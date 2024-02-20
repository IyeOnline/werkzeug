#ifndef WERKZEUG_GUARD_ALGORITHM_ANALYSIS_TOOLS_HPP
#define WERKZEUG_GUARD_ALGORITHM_ANALYSIS_TOOLS_HPP

#include <array>
#include <iostream>
#include <span>

namespace werkzeug
{
	template<typename T = double>
    struct Running_Average
    {
        static_assert( std::is_arithmetic_v<T> );
        T sum_{};
        std::size_t count_ = 0;

        auto sum() const
        { return sum_; }

        auto count() const
        { return count_; }

        auto average() const
        {
            return static_cast<double>(sum_)/static_cast<double>(count_);
        }

        void add_value( const T value )
        {
            sum_ += value;
            ++count_;
        }

		friend std::ostream& operator << ( std::ostream& os, const Running_Average& avg )
		{
			os << "average : ";
			if ( avg.count() != 0 )
			{
				os << avg.average() << '\n';
			}
			else
			{
				os << "---\n";
			}
			os << "count   : " << avg.count();
			return os;
		}
    };

	template<typename T, std::size_t N_max=1, std::size_t N_min=0>
	class Continous_Min_Max_Finder
	{
		std::array<T,N_min> lowest_values_;
		std::array<T,N_max> largest_values_;

		std::size_t lowest_count_ = 0;
		std::size_t largest_count_ = 0;

	public :

 		void add_value( T value ) noexcept
		{
			if constexpr ( N_min > 0 )
			{
				std::size_t insert_index = lowest_count_ == N_min ? lowest_values_.size() : 0;
				for ( std::size_t i=0; i < lowest_count_; ++i )
				{
					if ( value < lowest_values_[i] )
					{
						insert_index = i;
					}
					else
					{
						break;
					}
				}
			
				if ( insert_index < lowest_values_.size() )
				{
					if ( lowest_count_ < N_min )
					{
						const auto end = std::min(lowest_count_,N_min-1);
						for ( std::size_t i=insert_index; i < end; ++i )
						{
							lowest_values_[i+1] = lowest_values_[i];
						}
						++lowest_count_;
					}
					else
					{
						for ( std::size_t i=1; i < insert_index; ++i )
						{
							lowest_values_[i-1] = lowest_values_[i];
						}
					}
					lowest_values_[insert_index] = value;

					if constexpr ( N_max > 0 )
					{
						if ( largest_count_ == N_max )
						{
							return;
						}
					}
				}
			}

			if constexpr( N_max > 0 )
			{
				std::size_t insert_index = largest_count_ == N_max ? largest_values_.size() : 0;
				for ( std::size_t i=0; i < largest_count_; ++i )
				{
					if ( value > largest_values_[i] )
					{
						insert_index = i;
					}
					else
					{
						break;
					}
				}
			
				if ( insert_index < largest_values_.size() )
				{
					if ( largest_count_ < N_max )
					{
						const auto end = std::min(largest_count_,N_max-1);
						for ( std::size_t i=insert_index; i < end; ++i )
						{
							largest_values_[i+1] = largest_values_[i];
						}
						++largest_count_;
					}
					else
					{
						for ( std::size_t i=1; i < insert_index; ++i )
						{
							largest_values_[i-1] = largest_values_[i];
						}
					}
					largest_values_[insert_index] = value;
				}
			}
		}

		template<typename R>
		void add_range( const R& range ) noexcept
		{
			for ( const auto v : range )
			{
				add_value( v );
			}
		}

		[[nodiscard]] auto top() const noexcept
		{
			return largest_values_[largest_count_-1];
		}

		[[nodiscard]] auto bottom() const noexcept
		{
			return lowest_values_[lowest_count_-1];
		}

		[[nodiscard]] std::span<const T> largest() const noexcept
		{
			return { largest_values_.begin(), largest_count_ };
		}

		[[nodiscard]] std::span<const T> lowest() const noexcept
		{
			return { lowest_values_.begin(), lowest_count_ };
		}

		friend std::ostream& operator << ( std::ostream& os, const Continous_Min_Max_Finder& minmax )
		{
			os << "lowest values : { ";
			for ( const auto min : minmax.lowest() )
			{
				os << min << ' ';
			}
			os << "}\n";
			os << "largest values : { ";
			for ( const auto max : minmax.largest() )
			{
				os << max << ' ';
			}
			os << "}";
			return os;
		}
	};

	template<typename T, typename Tool0, typename ... ToolRest>
	struct Joined_Tool : public Tool0, public ToolRest ...
	{
		Joined_Tool() = default;
		void add_value( const T value ) noexcept
		{
			Tool0::add_value(value);
			( ToolRest::add_value(value), ... );
		}

		template<typename R>
		void add_range( const R& range )
		{
			Tool0::add_range(range);
			( ToolRest::add_range(range), ... );
		}

		friend std::ostream& operator << ( std::ostream& os, const Joined_Tool& tool )
		{
			os << static_cast<const Tool0&>(tool);
			const auto print = [&os](const auto& x ){ std::cout << '\n' << x; };
			( print( static_cast<const ToolRest&>(tool) ), ... );
			return os;
		}
	};
}

#endif