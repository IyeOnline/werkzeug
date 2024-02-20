#ifndef WERKZEUG_GUARD_ARRAY_ND_HPP
#define WERKZEUG_GUARD_ARRAY_ND_HPP

#include <array>
#include <cmath>
#include <functional>
#include <iostream>
#include <numeric>
#include <type_traits>
#include <vector>
#include <span>

namespace werkzeug
{
	namespace detail
	{
		/**
		 * @brief transforms an `array<Contiguous_Container>` into an `array<Span>`
		 */
		template<typename Contiguous_Container, std::size_t Dimensions>
		auto make_view_array( const std::array<Contiguous_Container,Dimensions>& input )
		{
			using result_t = std::array<std::span<const typename Contiguous_Container::value_type>,Dimensions>;

			result_t result;

			std::copy( input.begin(), input.end(), result.begin() );

			return result;
		}

		/**
		 * @brief CRTP base for `Array_Nd` and `Array_Nd::View`
		 * @details exists as a common base to implement member functions
		 * 
		 * @tparam Dimensions Number of Dimensions
		 * @tparam T Type stored in each element of the array
		 * @tparam Concrete_Type Concrete type we are implementing. This is the CRTP part
		 */
		template<typename T, std::size_t Dimensions, typename Concrete_Type>
		class Array_CRTP_Base
		{
		protected :
			const Concrete_Type& as_concrete() const noexcept
			{
				return static_cast<const Concrete_Type&>(*this);
			}

			Concrete_Type& as_concrete() noexcept
			{
				return static_cast<Concrete_Type&>(*this);
			}
		public :
			using int_T = int;
			static_assert( std::is_signed_v<int_T>, "int_T must be signed for some derived classes" );
			using value_type = T;
			using index_type = std::array<int_T,Dimensions>;
			using index_view = std::span<const int_T,Dimensions>;

			[[nodiscard]] std::size_t linear_index( const index_type& idx ) const noexcept
			{
				std::size_t res = 0;
				for ( std::size_t i = 0; i < Dimensions; ++i )
				{
					res += static_cast<std::size_t>( idx[i] ) * as_concrete().size_factors_[i];
				}
				return res;
			}

			[[nodiscard]] index_type nd_index(  std::size_t lin_index ) const noexcept
			{
				index_type idx;
				for ( std::size_t i=0; i<Dimensions; ++i )
				{
					idx[i] = static_cast<int_T>( lin_index / static_cast<std::size_t>( as_concrete().size_factors_[i]) );
					lin_index -= static_cast<std::size_t>( idx[i] * as_concrete().size_factors_[i] );
				}
				return idx;
			}

			/** @brief gets the indices of all neighbouring (including N-diagonal) points. used for debugging */
			[[nodiscard]] std::vector<index_type> surrounding_indices( const index_type& lower_idx ) const
			{
				std::vector<index_type> res;
				res.reserve( std::pow(2,Dimensions) );
				for ( std::size_t np1 = 0; np1 <= Dimensions; ++np1 ) //these are not indices. the `<=` is correct here. Its the "number of plus 1" and "number of minus 1", i.e. the offsets
				{
					for ( std::size_t nm1 = 0; nm1 <= Dimensions-np1; ++nm1 )
					{
						index_type offsets{};
						
						const auto n0 = Dimensions - np1 - nm1;
						std::fill_n( offsets.begin(), nm1, -1 );
						std::fill_n( offsets.begin()+nm1, n0, 0 );
						std::fill_n( offsets.begin()+nm1+n0, np1, 1 );

						do //for all permutations of these offsets
						{
							auto idx = lower_idx;
							for ( std::size_t j = 0; j < Dimensions; ++j )
							{
								idx[j] += offsets[j];
								if ( idx[j] >= as_concrete().size_[j] or idx[j] < 0 )
								{
									goto skip;
								}
							}
							res.push_back(idx);
							skip: ;
						}
						while( std::next_permutation( offsets.begin(), offsets.end() ) );
					}
				}

				return res;
			}

			/** @brief gets the data at a given index */
			[[nodiscard]] inline T& data_at_index( const index_type& key ) noexcept
			{ 
				return as_concrete().data_[linear_index(key)]; 
			}
			/** @brief gets the data at a given index */
			[[nodiscard]] inline std::add_const_t<T>& data_at_index( const index_type& key ) const noexcept
			{ 
				return as_concrete().data_[linear_index(key)];
			}

			/** @brief sets the value at a given index.*/
			void set_value_at_index( const index_type& key, T value ) noexcept
			{
				data_at_index(key) = std::move(value);
			}
			
			/** @brief access the array at an index tuple */
			[[nodiscard]] inline T& operator [] ( const index_type& key ) noexcept
			{ 
				return data_at_index(key); 
			}

			/** @brief access the array at an index tuple */
			[[nodiscard]] inline std::add_const_t<T>& operator [] ( const index_type& key ) const noexcept
			{ 
				return data_at_index(key); 
			}
			
			/** @brief gets the size of the array as an array */
			[[nodiscard]] index_view size_Nd() const noexcept
			{ 
				return as_concrete().extents_; 
			}

			/** @brief gets the size of the array as an array */
			[[nodiscard]] index_view extents() const noexcept
			{ 
				return as_concrete().extents_; 
			}


			/** @brief gets the actual size of the allocation */
			[[nodiscard]] auto raw_size() const noexcept
			{
				return as_concrete().data_.size();
			}

			/** @brief gets the rank/dimensions of the array */
			[[nodiscard]] static constexpr auto rank() noexcept
			{
				return Dimensions;
			}

			/** @brief iterator to the begin of the storage */
			[[nodiscard]] auto begin() const noexcept
			{ 
				return as_concrete().data_.begin(); 
			}

			/** @brief past end iterator of the storage */
			[[nodiscard]] auto end() const noexcept
			{ 
				return as_concrete().data_.end(); 
			}

			/** @brief compares two arrays for equality */
			[[nodiscard]] friend bool operator == ( const Array_CRTP_Base& lhs, const Array_CRTP_Base& rhs ) noexcept
			{
				return lhs.size_ == rhs.size_ and lhs.data_ == rhs.data_;
			}

			/** @brief compares two arrays for inequality */
			[[nodiscard]] friend bool operator != ( const Array_CRTP_Base& lhs, const Array_CRTP_Base& rhs ) noexcept
			{
				return not (lhs == rhs);
			}
		};
	}


	/**
	 * @brief generic Nd array class
	 * 
	 * @tparam dimensions number of dimensions this table has
	 * @tparam T type to be stored 
	 */
	template<std::size_t Dimensions,typename T>
	class Array_Nd : public detail::Array_CRTP_Base<T, Dimensions, Array_Nd<Dimensions,T>>
	{
		static_assert( Dimensions > 0, "An array should really have at least one dimension" );
		using CRTP_Base = detail::Array_CRTP_Base<T, Dimensions, Array_Nd<Dimensions,T>>;
		friend class detail::Array_CRTP_Base<T, Dimensions, Array_Nd<Dimensions,T>>;
	protected:
		// the underlying type used for the index tuple.
		using int_T = typename CRTP_Base::int_T;
		// the index tuple type
		using index_type = typename CRTP_Base::index_type;
		
		// the vector that actually stores the data
		std::vector<T> data_{};
		// the size of the array
		index_type extents_{};

		//array to store the precomputed size offsets for linear indexing
		std::array<std::size_t,Dimensions> size_factors_;
		

		/** @brief determines the size the vector needes. Used in the constructors */
		auto calculate_data_size() const
		{
			return std::accumulate( extents_.begin(), extents_.end(), std::size_t{1}, std::multiplies<std::size_t>() );
		}

		/** @brief sets the size factors for linear index offsets*/
		void set_size_factors() noexcept
		{
			for ( std::size_t i = 0; i < Dimensions; ++i )
			{
				size_factors_[i] = std::accumulate( extents_.begin()+i+1, extents_.end(), std::size_t{1}, std::multiplies<std::size_t>() );
			}
		}

		/** @brief helper function to create an error. Used when reading from streams */
		static void handle_stream_error( const std::istream& ifs, std::string_view text, const std::size_t i, const std::size_t n )
		{
			if ( ifs.eof() )
			{
				throw std::runtime_error( "Error reading " + std::string(text) + ": unexpected EOF at count " + std::to_string(i) + "/" + std::to_string(n));
			}
			else if ( ifs.fail() )
			{
				throw std::runtime_error("Error reading " + std::string(text) + ": invalid value at count " + std::to_string(i) + "/" + std::to_string(n));
			}
			else if ( ifs.bad() )
			{
				throw std::runtime_error("Error reading " + std::string(text) + ": bad bit was set during read of " + std::to_string(i) + "/" + std::to_string(n));
			}
		}

		/** @brief reads N elements from a stream into an range.*/
		template<typename OutputIterator>
		static void read_n( std::istream& ifs, std::string_view text, const std::size_t N, OutputIterator it )
		{
			for ( std::size_t i=0; i < N; ++i )
			{
				ifs >> *it;
				if ( not ifs.good() )
				{
					if ( i == N-1 and ifs.eof() )
					{ 
						continue; 
					}
					else
					{
						handle_stream_error( ifs, text, i+1, N );
					}
				}
				++it;
			}
		}

		/** @brief saves N elements from an iterator into a range - WITHOUT a trailing separator */
		template<typename InputIterator>
		static void save_n( std::ostream& os, const std::size_t N, InputIterator it )
		{
			for ( std::size_t i=0; i < N-1; ++i )
			{
				os << *it << ' ';
				++it;
			}
			os << *it;
		}

	public:

		// This type only defines two custom constructors and no default constructor
		// It makes no sense to default construct an array without a size, so there is no default constructor
		// By not defining any of the other special members (copy and move constructor and assignment operators) they all get created by the compiler
		// The implicitly generated special members will work fine, because all class members are well behaved, i.e. our memory is managed by std::vector

		/**
		 * @brief Construct a new Array_Nd object
		 * 
		 * @param size sizes for all axis
		 * @param value a value to fill the array with. Defaults to to a value initialized value, i.e. 0 for fundamental types
		 */
		Array_Nd( const index_type& size, const T& value = T{} ) noexcept
			: extents_(size)
		{
			data_.resize( calculate_data_size(), value );
			set_size_factors();
		}
		
		/**
		 * @brief Construct a new Array_Nd object from a stream
		 * 
		 * @param ifs stream to read from
		 * 
		 * @throws std::runtime_error if loading failed
		 */
		Array_Nd( std::istream& ifs )
		{
			read_n( ifs, "axis sizes", Dimensions, extents_.begin() ); 
			
			for ( std::size_t i = 0; i < Dimensions; ++i )
			{
				if ( extents_[i] == 0 )
				{ 
					throw std::runtime_error( "axis " + std::to_string(i) + " size is 0." ); 
				}
			}
			data_.resize( calculate_data_size() );
			set_size_factors();

			read_n( ifs, "data", data_.size(), data_.begin() );
		}
		
		/**
		 * @brief saves the array to a file
		 * 
		 * @param ofs stream to save to
		 */
		void save( std::ostream& ofs ) const noexcept
		{
			save_n( ofs, Dimensions, extents_.begin() );
			ofs << '\n';
			save_n( ofs, data_.size(), data_.begin() );
		}

		class View : detail::Array_CRTP_Base<T, Dimensions, View>
		{
			std::span<const T> data_;
			std::span<std::size_t> extents_;
			std::array<std::size_t,Dimensions> size_factors_;

			View( const std::span<const T> data, std::span<std::size_t> extents, const std::array<std::size_t,Dimensions>& size_factors )
				: data_{ data }, extents_{extents}, size_factors_{ size_factors }
			{}
		};

		[[nodiscard]] View make_view() const noexcept
		{
			return View{ data_, extents_, size_factors_ };
		}
	};
}
#endif
