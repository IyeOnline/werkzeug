#ifndef WERKZEUG_GUARD_UTILITY_INTERPOLATING_TABLES_HPP
#define WERKZEUG_GUARD_UTILITY_INTERPOLATING_TABLES_HPP

#include <array>
#include <type_traits>
#include <algorithm>
#include <cmath>

#include "werkzeug/tables/bounded_array_nd.hpp"
#include "werkzeug/tables/axis_tick_array_nd.hpp"
#include "werkzeug/algorithm/constexpr_math.hpp"

namespace werkzeug
{
	enum class interpolation_value_mode
    {
		lin,
		log,
		mix,
	};

	enum class interpolation_axis_mode
	{
		lin,
		log
	};


	namespace detail
	{
		template<typename T>
		struct interpolation_result
		{
			T value;
			bool trustable;
		};

		
		/**
		* @brief common CRTP base for interpolating tables
		* @details exists as a common base to implement member functions
		* 
		* @tparam dimensions number of dims the table has
		* @tparam T type stored in the table
		* @tparam Concrete_Type Concrete type we are implementing. This is the CRTP part
		*/
		template<std::size_t Dimensions,typename T, typename Concrete_Type>
		class Interpolation_CRTP_Base
		{
			using Array_Base = Array_CRTP_Base<T, Dimensions, Interpolation_CRTP_Base>;

		public :
			using int_T = typename Array_Base::int_T;
			using index_type = typename Array_Base::index_type;
			using pos_T = std::array<T,Dimensions>;

		protected :
			const Concrete_Type& as_concrete() const noexcept
			{
				return static_cast<const Concrete_Type&>(*this);
			}

			Concrete_Type& as_concrete() noexcept
			{
				return static_cast<Concrete_Type&>(*this);
			}
			struct idx_dx_result
			{
				index_type lower_index;
				pos_T dx;
			};
		public :
			struct interpolation_result
			{
				T value;
				bool trustable;
			};

			/**
			* @brief ND-interpolation between the edges of the hypercube that `where` is located in.
			* 
			* @param where position to interpolate at
			* @param interp_mode interpolation mode to use
			*/
			interpolation_result interpolate_value_at( const pos_T& where, const interpolation_value_mode interp_mode = interpolation_value_mode::lin ) const noexcept
			{
				const auto [ lower_idx, dx ] = as_concrete().find_lower_idx_dx(where);

				constexpr std::size_t dimensions = Concrete_Type::rank();

				T res_lin{0};
				T res_log{0};
				std::size_t n_zeros{0};
				
				//for i zeroes and dimensions-i ones. 
				for ( std::size_t i = 0; i <= dimensions; ++i ) //this is not an index. '<=' is correct and intended here.
				{
					index_type offsets;
					
					//generate offsets, note the zeroes have to be filled first due to how next_permutation only goes through lexicographically greater permutations
					std::fill_n( offsets.begin(), i, 0 );
					std::fill( offsets.begin()+i, offsets.end(), 1 );
					
					do //for all permutations of these offsets
					{
						T f{ 1 };
						
						auto idx = lower_idx;
						for ( std::size_t j = 0; j < dimensions; ++j )
						{
							//guard against out of bound indices
							if ( idx[j] < as_concrete().size_Nd()[j]-1 )
							{ idx[j] += offsets[j]; }
							const auto tmp = (offsets[j]==1) ? dx[j] : 1 - dx[j];
							f *= tmp;
						}
						
						const auto v = as_concrete().data_at_index( idx );

						if ( interp_mode == interpolation_value_mode::lin || interp_mode == interpolation_value_mode::mix )
						{
							res_lin += v * f;
						}

						// A value of 0 gives no contribution to the interpolation.
						// However, we have to check here, since otherwise for 'f==0' we have 'log(0)*0', which is '-inf * 0', leading to 'NaN', poisoning the entire result
						if ( interp_mode == interpolation_value_mode::log || interp_mode == interpolation_value_mode::mix )
						{
							if ( v == 0.0 )
							{
								++n_zeros;
							}
							else
							{
								res_log += std::log(v) * f;
							}
						}
					}
					while( std::next_permutation( offsets.begin(), offsets.end() ) );
				}

				constexpr std::size_t n_permutations = math::pow<dimensions>( 2u );

				const bool trustable_log = ( n_zeros == n_permutations ) or ( static_cast<double>(n_zeros) / static_cast<double>(n_permutations) < 0.75 );

				if ( interp_mode == interpolation_value_mode::log and trustable_log )
				{
					res_log = std::exp(res_log);
				}
				else if ( interp_mode == interpolation_value_mode::mix and res_log != 0.0 )
				{
					res_log = std::exp(res_log);
				}
				else
				{
					res_log = T{0};
				}


				constexpr T tiny_number = 1e-10;

				switch( interp_mode )
				{
					case interpolation_value_mode::lin :	
						return interpolation_result{ res_lin, true };
					case interpolation_value_mode::log :
						return interpolation_result{ res_log, trustable_log };
					case interpolation_value_mode::mix :
					{
						if ( not trustable_log )
						{
							return interpolation_result{ res_lin, false };
						}
						const auto res_mix = (res_lin+res_log)/T{2};
						const auto trustable_mix = res_lin < tiny_number or std::abs(res_lin-res_log) / (res_lin+res_log) < 0.3;

						return interpolation_result{ res_mix, trustable_mix };
					}
				}

				return interpolation_result{ T{}, false }; //to "silence" the warning
			}
		};


		/**
		* @brief common CRTP base for `Basic_Interpolating_Array` and `Basic_Interpolating_Array::View`
		* @details exists as a common base to implement member functions
		* 
		* @tparam Dimensions number of dims the table has
		* @tparam T type stored in the table
		* @tparam Concrete_Type Concrete type we are implementing. This is the CRTP part
		*/
		template<std::size_t Dimensions,typename T, typename Concrete_Type>
		class Basic_Interpolating_Array_CRTP_Base : public Interpolation_CRTP_Base<Dimensions, T, Concrete_Type>
		{
			using Interpolation_Base = Interpolation_CRTP_Base<Dimensions, T, Concrete_Type>;
		public :
			using int_T = typename Interpolation_Base::int_T;
			using index_type = typename Interpolation_Base::index_type;
			using pos_T = typename Interpolation_Base::pos_T;

			friend Interpolation_Base;
			using Interpolation_Base::as_concrete;

			using idx_dx_result = typename Interpolation_Base::idx_dx_result;
			idx_dx_result find_lower_idx_dx( const pos_T& where ) const noexcept
			{
				//get information
				std::array<bool,Dimensions> in_bounds; //will know if the axis access is within the known area
				index_type lower_idx; //will be the index of the point "before" the access position
				pos_T dx; // will be the difference between the value of the lower point and the access point
				
				for ( std::size_t i=0; i < Dimensions; ++i )
				{
					T log_where;
					if ( as_concrete().axis_is_log_[i] )
					{
						log_where = std::log(where[i]);
					}

					int_T tmp;
					if ( as_concrete().axis_is_log_[i] )
					{
						tmp = static_cast<int_T>( ( log_where - as_concrete().axis_lower_log_[i])/as_concrete().axis_delta_[i] );
					}
					else
					{
						tmp = static_cast<int_T>( ( where[i] - as_concrete().axis_lower_[i] ) / as_concrete().axis_delta_[i] );
					}
					
					if ( tmp < 0 ) //guard against lower out of bounds
					{ 
						lower_idx[i] = 0;
						in_bounds[i] = false;
					}
					else if ( tmp >= as_concrete().extents_[i] ) //guard against upper out of bounds
					{ 
						lower_idx[i] = as_concrete().extents_[i]-1;
						in_bounds[i] = false;
					}
					else
					{ 
						lower_idx[i] = tmp;
						in_bounds[i] = tmp < as_concrete().extents_[i]-1;
					}
					
					if ( in_bounds[i] )
					{
						if ( as_concrete().axis_is_log_[i] )
						{
							dx[i] = (log_where - as_concrete().axis_lower_log_[i] - lower_idx[i]*as_concrete().axis_delta_[i]) / as_concrete().axis_delta_[i]; //TODO consider numerically better option.
						}
						else
						{
							dx[i] = (where[i] - as_concrete().axis_lower_[i] - lower_idx[i]*as_concrete().axis_delta_[i]) / as_concrete().axis_delta_[i]; 
						}
					}
					else
					{
						dx[i] = T{0};
					}
				}
				return idx_dx_result{ lower_idx, dx };
			}
		};

		/**
		* @brief common CRTP base for `Advanced_Interpolating_Array` and `Advanced_Interpolating_Array::View`
		* @details exists as a common base to implement member functions
		* 
		* @tparam Dimensions number of dims the table has
		* @tparam T type stored in the table
		* @tparam Concrete_Type Concrete type we are implementing. This is the CRTP part
		*/
		template<std::size_t Dimensions,typename T, typename Concrete_Type>
		class Advanced_Interpolating_Array_CRTP_Base : public Interpolation_CRTP_Base<Dimensions, T, Concrete_Type>
		{
			using Interpolation_Base = Interpolation_CRTP_Base<Dimensions, T, Concrete_Type>;
		public :
			using int_T = typename Interpolation_Base::int_T;
			using index_type = typename Interpolation_Base::index_type;
			using pos_T = typename Interpolation_Base::pos_T;

			friend Interpolation_Base;
			using Interpolation_Base::as_concrete;
		protected :
			using idx_dx_result = typename Interpolation_Base::idx_dx_result;
			idx_dx_result find_lower_idx_dx( const pos_T& where ) const noexcept
			{
				const auto& ticks_ = as_concrete().ticks_;
				const auto& extents_ = as_concrete().extents_;

				pos_T dx;
				index_type lower_idx;

				for ( std::size_t i=0; i < Dimensions; ++i )
				{
					if ( where[i] < ticks_[i].front() ) //guard against lower out of bounds
					{ 
						lower_idx[i] = 0; //TODO consider linear extrapolation
						dx[i] = T{};
					}
					else if (  where[i] > ticks_[i].back() ) //guard against upper out of bounds
					{ 
						lower_idx[i] = extents_[i]-1; //TODO consider linear extrapoltion
						dx[i] = T{};
					}
					else
					{
						lower_idx[i] = as_concrete().find_lower_idx_bisection( i, where[i] );
						const auto dist_high_low = ticks_[i][static_cast<std::size_t>(lower_idx[i]+1)] - ticks_[i][static_cast<std::size_t>(lower_idx[i])];
						const auto dist_where_low = where[i] - ticks_[i][static_cast<std::size_t>(lower_idx[i])];
						dx[i] = dist_where_low / dist_high_low;
					}
				}

				return { lower_idx, dx };
			}
		};
	}

	/**
	 * @brief an interpolating table. Will return the boundary value if out of bounds. This table only has a tick count and axis bounds, so its an evenly spaced grid
	 *
	 * @param Dimensions: the dimensionality of the table
	 * @param T=double: the underlying data type. must be arithmetic
	 */
	template<
		std::size_t Dimensions,
		typename T = double
	>
	class Basic_Interpolating_Array 
		: public Bounded_Array_Nd<Dimensions,T>
		, public detail::Basic_Interpolating_Array_CRTP_Base<Dimensions, T, Basic_Interpolating_Array<Dimensions,T>>
	{
		static_assert( std::is_arithmetic_v<T>, "Held type must be arithmetic" );
		
		using Array_Base = Bounded_Array_Nd<Dimensions,T>;
		using Interpolation_Base = detail::Basic_Interpolating_Array_CRTP_Base<Dimensions, T, Basic_Interpolating_Array<Dimensions,T>>;
		friend Interpolation_Base;
		
		using Array_Base::extents_;
		using Array_Base::data_;
		using Array_Base::axis_lower_;
		using Array_Base::axis_upper_;
		using Array_Base::axis_delta_;
		using Array_Base::axis_is_log_;
		using Array_Base::axis_lower_log_;
	
	public:
		using value_type = T;
		using int_T = typename Array_Base::int_T;
		using index_type = typename Array_Base::index_type;
		using pos_T = typename Array_Base::pos_T;
		using b_arr_T = typename Array_Base::b_arr_T;
		using interpolation_result = detail::interpolation_result<T>;

		using Array_Base::Array_Base;

		class View 
			: public Array_Base::View
			, public detail::Basic_Interpolating_Array_CRTP_Base<Dimensions, T, View>
		{
		public :
			View( const typename Array_Base::View& base_view )
				: Array_Base::View{ base_view }
			{}
		};

		View make_view() const noexcept
		{
			return Array_Base::make_view();
		}
	};

	/**
	 * @brief interpolating table. Will return the boundary value if out of bounds. This table stores the values of all ticks along all axis, so it can be dynamically spaced
	 *
	 * @param Dimensions the dimensionality of the table
	 * @param T=double the underlying data type. must be arithmetic
	 */
	template<std::size_t Dimensions, typename T>
	class Advanced_Interpolating_Array 
		: public Axis_Tick_Array_Nd<Dimensions, T, T>
		, public detail::Advanced_Interpolating_Array_CRTP_Base<Dimensions, T, Advanced_Interpolating_Array<Dimensions,T>>
	{
		using Array_Base = Axis_Tick_Array_Nd<Dimensions,T,T>;
		using Interpolation_Base = detail::Advanced_Interpolating_Array_CRTP_Base<Dimensions, T, Advanced_Interpolating_Array<Dimensions,T>>;
		friend Interpolation_Base;

		using Array_Base::extents_;
		using Array_Base::data_;
		using Array_Base::ticks_;
		using Array_Base::find_lower_idx_bisection;

	public :
		using Array_Base::rank;
		using value_type = T;
		using interpolation_result = detail::interpolation_result<T>;
		using int_T = typename Array_Base::int_T;
		using index_type = typename Array_Base::index_type;
		using pos_T = typename Array_Base::pos_T;

		using Array_Base::Array_Base;

		class View 
			: public Array_Base::View
			, public detail::Advanced_Interpolating_Array_CRTP_Base<Dimensions, T, View>
		{
		public :
			View( const typename Array_Base::View& base_view )
				: Array_Base::View{ base_view }
			{}
		};

		View make_view() const noexcept
		{
			return Array_Base::make_view();
		}
	};
}
#endif
