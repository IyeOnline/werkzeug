#ifndef WERKZEUG_GUARD_UTILITY_AXIS_TICK_ARRAY_ND_HPP
#define WERKZEUG_GUARD_UTILITY_AXIS_TICK_ARRAY_ND_HPP

#include <array>
#include <span>

#include "werkzeug/tables/array_nd.hpp"
#include "werkzeug/tables/bounded_array_nd.hpp"

namespace werkzeug
{
	namespace detail
	{
		/**
		 * @brief CRTP base for `Axis_Tick_Array_ND` and `Axis_Tick_Array_ND::View`
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
		class Axis_Tick_Array_Nd_CRTP_Base
		{
			using CRTP_Base = Array_CRTP_Base<Store_Type,Dimensions,Concrete_Type>;
			using int_T = typename CRTP_Base::int_T;
			using index_type = typename CRTP_Base::index_type;
			using pos_T = std::array<Boundary_Type,Dimensions>;
			
			const Concrete_Type& as_concrete() const noexcept
			{
				return static_cast<const Concrete_Type&>(*this);
			}

			Concrete_Type& as_concrete() noexcept
			{
				return static_cast<Concrete_Type&>(*this);
			}
		protected :
			int_T find_lower_idx_bisection( const std::size_t axis, const Boundary_Type where ) const noexcept
			{
				int_T lower = 0;
				int_T upper = as_concrete().extents_[axis]-1;

				while ( (upper - lower) > 1 )
				{
					const int_T mid = lower + (upper - lower)/2;
					if ( where < as_concrete().ticks_[axis][static_cast<std::size_t>(mid)] )
					{
						upper = mid;
					}
					else
					{
						lower = mid;
					}
				}
				return lower;
			}
		public :
			const auto& ticks() const noexcept
			{
				return as_concrete().ticks_;
			}

			[[nodiscard]] inline pos_T index_to_coordinates( const index_type& index ) const noexcept
			{
				pos_T pos;
				for ( std::size_t i=0; i<Dimensions;++i )
				{
					pos[i] = ticks()[i][static_cast<std::size_t>(index[i])];
				}
				return pos;
			}

			struct idx_find_result
			{
				index_type idx{};
				std::array<bool,Dimensions> out_of_bounds{};
			};


			idx_find_result find_lower_index( const pos_T& where ) const noexcept
			{
				idx_find_result result;
				for ( std::size_t i=0; i < Dimensions; ++i )
				{
					if ( where[i] < as_concrete().ticks_[i].front() )
					{
						result.idx[i] = 0;
						result.out_of_bounds[i] = true;
					}
					else if ( where [i] > as_concrete().ticks_[i].back() )
					{
						result.idx[i] = as_concrete().extents_[i]-1;
						result.out_of_bounds[i] = true;
					}

					result.idx[i] = find_lower_idx_bisection(i,where[i]);
				}
				return result;
			}


			friend bool operator == ( const Axis_Tick_Array_Nd_CRTP_Base& lhs, const Axis_Tick_Array_Nd_CRTP_Base& rhs ) noexcept
			{
				if ( lhs.ticks_ != rhs.ticks_ )
				{
					return false;
				}
				return static_cast<const CRTP_Base&>( lhs ) == static_cast<const CRTP_Base&>( rhs );
			}
		};
	}


	/**
	 * @brief Array that also knows positions for each step along its axis
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
	class Axis_Tick_Array_Nd 
		: public Array_Nd<Dimensions,Store_Type>
		, public detail::Axis_Tick_Array_Nd_CRTP_Base<Dimensions, Store_Type, Boundary_Type, Axis_Tick_Array_Nd<Dimensions, Store_Type, Boundary_Type>>
	{
		static_assert( std::is_arithmetic_v<Boundary_Type>, "bondary type must be arithmetic" );
		using Array_Base = Array_Nd<Dimensions,Store_Type>;
		using CRTP_Base = detail::Axis_Tick_Array_Nd_CRTP_Base<Dimensions, Store_Type, Boundary_Type, Axis_Tick_Array_Nd<Dimensions, Store_Type, Boundary_Type>>;

		friend CRTP_Base;

	public:
		using pos_T = std::array<Boundary_Type,Dimensions>;
		using int_T = typename Array_Base::int_T;
		using index_type = typename Array_Base::index_type;

	protected:
		using Array_Base::extents_;
		using Array_Base::data_;
		using Array_Base::read_n;
		using Array_Base::save_n;

	private:
		static index_type determine_size( const std::array<std::vector<Boundary_Type>,Dimensions>& ticks )
		{
			index_type size;
			for ( std::size_t i=0; i < Dimensions; ++i )
			{
				size[i] = static_cast<int_T>( ticks[i].size() );
			}
			return size;
		}
	protected:
		std::array<std::vector<Boundary_Type>,Dimensions> ticks_;

	public :
		Axis_Tick_Array_Nd( std::array<std::vector<Boundary_Type>,Dimensions> ticks, const Store_Type& value = {} )
			: Array_Base{ determine_size(ticks), value }, ticks_( std::move(ticks) )
		{ 
			for ( std::size_t i=0; i < Dimensions; ++i )
			{
				if ( not std::is_sorted(ticks_[i].begin(), ticks_[i].end()) )
				{ throw std::runtime_error( "axis ticks for axis " + std::to_string(i) + "are not sorted"); }
			}
		}

		Axis_Tick_Array_Nd( std::istream& is )
			: Array_Base(is)
		{
			for ( std::size_t i=0; i < Dimensions; ++i )
			{
				ticks_[i].resize( static_cast<std::size_t>( extents_[i] ) );
				read_n( is, "axis ticks", ticks_[i].size(), ticks_[i].begin() );
				
				if ( not std::is_sorted(ticks_[i].begin(), ticks_[i].end()) )
				{ throw std::runtime_error( "ticks for axis " + std::to_string(i) + "are not sorted"); }
			}
		}

		Axis_Tick_Array_Nd( const Bounded_Array_Nd<Dimensions,Store_Type,Boundary_Type>& basic )
			: Array_Base( basic.size_Nd() )
		{
			std::copy( basic.begin(), basic.end(), data_.begin() );

			for ( std::size_t i=0; i < Dimensions; ++i )
			{
				ticks_[i].resize( static_cast<std::size_t>(extents_[i]));
				for ( std::size_t j=0; j < static_cast<std::size_t>(extents_[i]); ++j )
				{
					if ( basic.axis_is_log()[i] )
					{
						ticks_[i][j] = std::exp( std::log(basic.axis_lower()[i]) + basic.axis_delta()[i] * static_cast<Boundary_Type>(j) );
					}
					else
					{
						ticks_[i][j] = basic.axis_lower()[i] + basic.axis_delta()[i] * static_cast<Boundary_Type>(j);
					}
				}
			}
		}

		/** @brief saves the array to a stream */
		void save( std::ostream& ofs ) const
		{
			Array_Base::save( ofs );
			for ( std::size_t i=0; i < Dimensions; ++i)
			{
				ofs << '\n';
				save_n( ofs, ticks_[i].size(), ticks_[i].begin() );
			}
		}

		class View
			: public Array_Base::View
			, public detail::Axis_Tick_Array_Nd_CRTP_Base<Dimensions, Store_Type, Boundary_Type, View>
		{
			std::array<std::span<const Boundary_Type>, Dimensions> ticks_;

			View( const typename Array_Base::View& base_view, const std::array<std::vector<Boundary_Type>,Dimensions>& ticks )
				: Array_Base::View{ base_view }, ticks_{ detail::make_view_array(ticks) }
			{}
		};

		View make_view() const noexcept
		{
			return View{ Array_Base::make_view(), ticks_ };
		}
	};
}


#endif