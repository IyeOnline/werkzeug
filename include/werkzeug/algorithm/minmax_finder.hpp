#ifndef WERKZEUG_GUARD_MIN_MAX_FINDER_HPP
#define WERKZEUG_GUARD_MIN_MAX_FINDER_HPP


#include <span>
#include <iostream>
#include <array>


namespace werkzeug
{
	template<typename T, std::size_t N_max=1, std::size_t N_min=0>
	#if __cplusplus >= 202002L 
		requires requires (T t)
		{
			T{};
			t < t;
			t > t;
		}
	#endif
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
					else if ( lowest_count_ < N_min ) [[unlikely]]
					{
						break;
					}
				}
			
				if ( insert_index < lowest_values_.size() )
				{
					if ( lowest_count_ < N_min ) [[unlikely]]
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
					else if ( largest_count_ < N_max ) [[unlikely]]
					{
						break;
					}
				}
			
				if ( insert_index < largest_values_.size() )
				{
					if ( largest_count_ < N_max ) [[unlikely]]
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

		[[nodiscard]] const auto& top() const noexcept
		{
			return largest_values_[largest_count_-1];
		}

		[[nodiscard]] const auto& bottom() const noexcept
		{
			return lowest_values_[lowest_count_-1];
		}

		[[nodiscard]] auto largest() const noexcept
		{
			return std::span{ largest_values_.begin(), largest_count_};
		}

		[[nodiscard]] auto lowest() const noexcept
		{
			return std::span{ lowest_values_.begin(), lowest_count_};
		}

		friend std::ostream& operator << ( std::ostream& os, const Continous_Min_Max_Finder& minmax )
		{
			os << "lowest values : {";
			for ( const auto min : minmax.lowest() )
			{
				os << min << ' ';
			}
			os << "}\n";
			os << "largest values : {";
			for ( const auto max : minmax.largest() )
			{
				os << max << ' ';
			}
			os << "}";
			return os;
		}
	};

}

#endif