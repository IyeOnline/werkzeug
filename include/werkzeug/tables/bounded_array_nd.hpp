#ifndef WERKZEUG_GUARD_UTILITY_BOUNDED_ARRAY_ND_HPP
#define WERKZEUG_GUARD_UTILITY_BOUNDED_ARRAY_ND_HPP

#include <optional>

#include "werkzeug/tables/array_nd.hpp"

namespace werkzeug
{
	namespace detail
	{
		/**
		 * @brief CRTP base for `Bounded_Array_Nd` and `Bounded_Array_Nd::View`
		 * @details exists as a common base to implement member functions
		 * 
		 * @tparam Dimensions Number of Dimensions
		 * @tparam Store_Type Type stored in each element of the array
		 * @tparam Boundary_Type Type for the bounaries (could differ from stored type for histograms)
		 * @tparam Concrete_Type Concrete type we are implementing. This is the CRTP part
		 */
		template<
			std::size_t Dimensions,
			typename Store_Type,
			typename Boundary_Type,
			typename Concrete_Type
		>
		class Bounded_Array_Nd_CRTP_Base
		{
			using CRTP_Base = Array_CRTP_Base<Store_Type,Dimensions,Concrete_Type>;
			
			const Concrete_Type& as_concrete() const noexcept
			{
				return static_cast<const Concrete_Type&>(*this);
			}

			Concrete_Type& as_concrete() noexcept
			{
				return static_cast<Concrete_Type&>(*this);
			}
		public :
			using int_T = typename CRTP_Base::int_T;
			using index_type = typename CRTP_Base::index_type;
			using b_arr_T = std::array<bool,Dimensions>;
			using pos_T = std::array<Boundary_Type,Dimensions>;

			// using CRTP_Base::size_Nd;
			const pos_T& axis_lower() const noexcept
			{
				return as_concrete().axis_lower_;
			}
			const pos_T& axis_upper() const noexcept
			{
				return as_concrete().axis_upper_;
			}
			const pos_T& axis_delta() const noexcept
			{
				return as_concrete().axis_delta_;
			}
			const b_arr_T& axis_is_log() const noexcept
			{
				return as_concrete().axis_is_log_;
			}
			const pos_T& axis_lower_log() const noexcept
			{
				return as_concrete().axis_lower_log_;
			}

			[[nodiscard]] inline std::optional<index_type> position_to_index( const pos_T& position ) const noexcept
			{
				index_type idx;
				for ( std::size_t i=0; i < Dimensions; ++i )
				{
					int_T x;
					if ( axis_is_log()[i] )
					{
						x = static_cast<int_T>( ( std::log(position[i])-axis_lower_log()[i] ) / axis_delta()[i] );
					}
					else
					{
						x = static_cast<int_T>( ( position[i]-axis_lower()[i] ) / axis_delta()[i] );
					}
					if ( x < 0 || x >= as_concrete().size_Nd_[i] )
					{ return {}; }
					idx[i] = x;
				}
				return idx;
			}

			[[nodiscard]] inline pos_T index_to_coordinates( const index_type& index ) const noexcept
			{
				pos_T pos;
				for ( std::size_t i=0; i < Dimensions;++i )
				{
					if ( axis_is_log()[i] )
					{
						pos[i] = std::exp( axis_lower_log()[i] + index[i] * axis_delta()[i] );
					}
					else
					{
						pos[i] = axis_lower()[i] + index[i] * axis_delta()[i];
					}
				}
				return pos;
			}

			
			friend bool operator == ( const Bounded_Array_Nd_CRTP_Base& lhs, const Bounded_Array_Nd_CRTP_Base& rhs ) noexcept
			{
				if ( lhs.axis_is_log_ != rhs.axis_is_log_ ) { return false; }
				if ( lhs.axis_lower_ != rhs.axis_lower_ ) { return false; }
				if ( lhs.axis_upper_ != rhs.axis_upper_ ) { return false; }
				return static_cast<const CRTP_Base&>( lhs ) == static_cast<const CRTP_Base&>( rhs );
			}
		};
	}


	/**
	 * @brief Array that also knows value bounds on each axis. Use as a base for histograms and interpolation tables
	 * 
	 * @tparam dimensions number of dims the array has
	 * @tparam Store_Type type stored in the array
	 * @tparam Boundary_Type type of the bounds (could differ from stored type for histograms)
	 */
	template<
		std::size_t Dimensions,
		typename Store_Type,
		typename Boundary_Type = Store_Type
	>
	class Bounded_Array_Nd 
		: public Array_Nd<Dimensions,Store_Type>
		, public detail::Bounded_Array_Nd_CRTP_Base<Dimensions, Store_Type, Boundary_Type, Bounded_Array_Nd<Dimensions,Store_Type,Boundary_Type>>
	{
		static_assert( std::is_arithmetic_v<Boundary_Type>, "bondary type must be arithmetic" );
		using Array_Base = Array_Nd<Dimensions,Store_Type>;
		using CRTP_Base = detail::Bounded_Array_Nd_CRTP_Base<Dimensions, Store_Type, Boundary_Type, Bounded_Array_Nd<Dimensions,Store_Type,Boundary_Type>>;
	protected:
		using Array_Base::extents_;
		using Array_Base::data_;
		using Array_Base::read_n;
		using Array_Base::save_n;

	public:
		using pos_T = typename CRTP_Base::pos_T;
		using int_T = typename CRTP_Base::int_T;
		using index_type = typename CRTP_Base::index_type;
		using b_arr_T = typename CRTP_Base::b_arr_T;
		using Array_Base::begin;
		using Array_Base::end;
		using Array_Base::rank;
		using Array_Base::data_at_index;

	protected:
		pos_T axis_lower_{};
		pos_T axis_upper_{};
		pos_T axis_delta_{};
		b_arr_T axis_is_log_{};
		pos_T axis_lower_log_{};

		void set_axis_deltas()
		{
			for( std::size_t i = 0; i < Dimensions; ++i )
			{
				if ( axis_is_log_[i] )
				{
					if ( axis_lower_[i] == Boundary_Type{} )
					{
						axis_lower_log_[i] = Boundary_Type{};
						axis_delta_[i] = std::log(axis_upper_[i]) / (extents_[i] - 1);
						continue;
					}
					else if ( axis_upper_[i] == Boundary_Type{} )
					{
						axis_delta_[i] = std::log(std::abs(axis_upper_[i])) / (extents_[i]-1);
					}
					else
					{
						axis_delta_[i] = ( std::log(axis_upper_[i]) - std::log(axis_lower_[i]) ) / (extents_[i]-1);
					}
					axis_lower_log_[i] = std::log(axis_lower_[i]);
				}
				else
				{
					axis_delta_[i] = ( axis_upper_[i] - axis_lower_[i] ) / (extents_[i]-1);
				}
			}
		}
	public:
		Bounded_Array_Nd() = default;
		
		/**
		 * @brief Construct a new Bounded_Array_Nd object
		 * 
		 * @param size size of the array
		 * @param axis_lower_bounds lower bounds of each axis
		 * @param axis_upper_bounds upper bounds of each axis
		 * @param axis_is_log_ whether each axis is in a log scale
		 */
		Bounded_Array_Nd( const index_type& size, const pos_T& axis_lower_bounds, const pos_T& axis_upper_bounds, const b_arr_T& axis_is_log={} )
			: Array_Base( size ), axis_lower_( axis_lower_bounds ), axis_upper_( axis_upper_bounds ), axis_is_log_( axis_is_log )	
		{
			set_axis_deltas();
		}
		
		/**
		 * @brief Construct a new Bounded_Array_Nd object from a stream
		 * 
		 * @param ifs 
		 */
		Bounded_Array_Nd( std::istream& ifs )
			: Array_Base( ifs )
		{
			read_n( ifs, "axis lower bounds", Dimensions, axis_lower_.begin() );
			read_n( ifs, "axis upper bounds", Dimensions, axis_upper_.begin() );
			read_n( ifs, "axis log state", Dimensions, axis_is_log_.begin() );
			
			set_axis_deltas();
		}

		/** @brief saves the array to a stream */
		void save( std::ostream& ofs ) const
		{
			Array_Base::save( ofs );
			ofs << '\n';
			save_n( ofs, Dimensions, axis_lower_.begin() );
			ofs << '\n';
			save_n( ofs, Dimensions, axis_upper_.begin() );
			ofs << '\n';
			save_n( ofs, Dimensions, axis_is_log_.begin() );
		}


		class View 
			: public Array_Base::View
			, public detail::Bounded_Array_Nd_CRTP_Base<Dimensions, Store_Type, Boundary_Type, View>
		{
		protected :
			pos_T axis_lower_;
			pos_T axis_upper_;
			pos_T axis_delta_;
			pos_T axis_lower_log_;
			b_arr_T axis_is_log_;
		public :
			View( const typename Array_Base::View& base_view, const pos_T& axis_lower, const pos_T& axis_upper, const pos_T& axis_delta, const pos_T& axis_lower_log, const b_arr_T& axis_is_log )
				: Array_Base::View{ base_view }, axis_lower_{axis_lower}, axis_upper_{axis_upper}, axis_delta_{axis_delta}, axis_lower_log_{axis_lower_log}, axis_is_log_{axis_is_log}
			{}
		};

		View make_view() const noexcept
		{
			return View{ Array_Base::make_view(), axis_lower_, axis_upper_, axis_delta_, axis_is_log_ };
		}
	};
}

#endif
