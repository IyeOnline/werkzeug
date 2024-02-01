#ifndef WERKZEUG_GUARD_ITERATOR_HPP
#define WERKZEUG_GUARD_ITERATOR_HPP

#include <compare>
#include <utility>

#include "werkzeug/traits.hpp"
#include "werkzeug/algorithm/sorting.hpp"

namespace werkzeug
{
	template<typename T>
	struct iterator_traits : type_traits<T>
	{
		using equality_compare = detail::equality_compare_trait<T>;
		using three_way_compare = detail::three_way_compare_trait<T>;
		using pre_increment = detail::pre_increment_trait<T>;
		using pre_decrement = detail::pre_decrement_trait<T>;
		using post_increment = detail::post_increment_trait<T>;
		using post_decrement = detail::post_decrement_trait<T>;
		using add = detail::add_trait<T,ptrdiff_t>;
		using subtract = detail::sub_trait<T,ptrdiff_t>;
		using difference = detail::difference_trait<T>;
		using deref = detail::deref_trait<T>;
		using subscript = detail::subscript_trait<T>;
	};


	template<typename T>
	concept dereferencable = requires ( T t )
	{
		{ *t } -> reference;
	};

	namespace detail
	{
		template<typename R>
		concept iterator_interface_member = requires ( R r )
		{
			{ r.begin() } -> dereferencable;
			{ r.begin() != r.end() } -> std::same_as<bool>;
		};

		template<typename R>
		concept iterator_interface_adl = requires ( R r )
		{
			{ begin(r) } -> dereferencable;
			{ begin(r) != end(r) } -> std::same_as<bool>;
		};

		template<typename R>
		concept iterator_interface_std = requires ( R r )
		{
			{ std::begin(r) } -> dereferencable;
			{ std::begin(r) != std::end(r) } -> std::same_as<bool>;
		};
	}


	template<typename C>
		requires ( 
			detail::iterator_interface_member<C> or
			detail::iterator_interface_adl<C> or
			detail::iterator_interface_std<C>
		)
	decltype(auto) begin( C&& c )
	{ 
		if constexpr ( detail::iterator_interface_member<C> )
		{
			return std::forward<C>(c).begin(); 
		}
		else 
		{
			using std::begin;
			return begin(std::forward<C>(c));
		}
	}
	template<typename C>
		requires ( 
			detail::iterator_interface_member<C> or
			detail::iterator_interface_adl<C> or
			detail::iterator_interface_std<C>
		)
	decltype(auto) end( C&& c )
	{ 
		if constexpr ( detail::iterator_interface_member<C> )
		{
			return std::forward<C>(c).end(); 
		}
		else 
		{
			using std::end;
			return end(std::forward<C>(c));
		}
	}

	template<typename Iterator, typename Tag>
	class Tagged_Iterator_Wrapper
	{
		Iterator it{};

		using traits = iterator_traits<Iterator>;
		using std_iterator_traits = std::iterator_traits<Iterator>;

	public :
		using difference_type = typename std_iterator_traits::difference_type;
		using value_type = typename std_iterator_traits::value_type;
		using reference = typename std_iterator_traits::reference;
		using iterator_category = typename std_iterator_traits::iterator_category;
		// using iterator_concept = typename std_iterator_traits::iteartor_concept;

		constexpr Tagged_Iterator_Wrapper() = default;

		constexpr Tagged_Iterator_Wrapper( const Iterator& p ) noexcept
			: it{ p }
		{}
		
		constexpr Tagged_Iterator_Wrapper( Iterator&& p ) noexcept
			: it{ std::move(p) }
		{}


		[[nodiscard]] constexpr auto operator-( const Tagged_Iterator_Wrapper& other ) const
			noexcept( traits::difference::nothrow )
			requires( traits::difference::possible )
		{
			return this->it - other.it;
		}

		constexpr Tagged_Iterator_Wrapper& operator++()
			noexcept( traits::pre_increment::nothrow )
		{
			++it;
			return *this; 
		}

		[[nodiscard]] constexpr Tagged_Iterator_Wrapper operator++(int)
			noexcept( traits::post_increment::nothrow )
		{
			return { it++ };
		}


		constexpr Tagged_Iterator_Wrapper& operator--()
			noexcept( traits::pre_decrement::nothrow )
			requires( traits::pre_decrement::possible )
		{
			--it;
			return *this;
		}

		[[nodiscard]] constexpr Tagged_Iterator_Wrapper operator--(int)
			noexcept( traits::post_decrement::nothrow )
			requires( traits::post_decrement::possible )
		{
			return { it-- };
		}

		constexpr Tagged_Iterator_Wrapper& operator+=( const difference_type distance )
			noexcept( traits::add::nothrow )
			requires( traits::add::possible )
		{
			it += distance;
			return *this;
		}

		constexpr Tagged_Iterator_Wrapper& operator-=( const difference_type distance ) 
			noexcept( traits::subtract::nothrow )
			requires( traits::subtract::possible )
		{
			it -= distance;
			return *this;
		}

		[[nodiscard]] constexpr friend Tagged_Iterator_Wrapper operator+( const Tagged_Iterator_Wrapper& it_, const difference_type distance ) 
			noexcept( traits::add::nothrow )
			requires(traits::add::possible )
		{
			return Tagged_Iterator_Wrapper(it_) += distance;
		}

		[[nodiscard]] constexpr friend Tagged_Iterator_Wrapper operator+( const difference_type distance, const Tagged_Iterator_Wrapper& it_ )
			noexcept( traits::add::nothrow )
			requires(traits::add::possible )
		{
			return Tagged_Iterator_Wrapper(it_) += distance;
		}

		[[nodiscard]] friend Tagged_Iterator_Wrapper operator-( const Tagged_Iterator_Wrapper& it_, const difference_type distance )
			noexcept( traits::subtract::nothrow )
			requires( traits::subtract::possible )
		{
			return Tagged_Iterator_Wrapper(it_) -= distance;
		}

		[[nodiscard]] constexpr auto& operator[]( const difference_type distance) const
			noexcept( traits::subscript::nothrow )
			requires( traits::subscript::possible )
		{
			return *( Iterator{it} + distance );
		}

		[[nodiscard]] constexpr auto& operator*() const
			noexcept( traits::deref::nothrow )
		{
			return *it;
		}

		[[nodiscard]] constexpr auto* operator->() const
			noexcept( traits::deref::nothrow )
		{
			return std::addressof(*it);
		}

		[[nodiscard]] constexpr auto operator<=>( const Tagged_Iterator_Wrapper& ) const noexcept = default;
		[[nodiscard]] constexpr bool operator == ( const Tagged_Iterator_Wrapper& ) const noexcept = default;
	};



	template<typename Iterator>
		requires requires (Iterator it) { --it; }
	class Reverse_Iterator_Wrapper
	{
		Iterator it;

		using std_iterator_traits = std::iterator_traits<Iterator>;
		using traits = iterator_traits<Iterator>;

	public :
		using difference_type = typename std_iterator_traits::difference_type;
		using value_type = typename std_iterator_traits::value_type;
		using reference = typename std_iterator_traits::reference;
		using iterator_category = typename std_iterator_traits::iterator_category; //TODO not sure if this is a good idea. Is a range contigous if its a reverse range?

		constexpr Reverse_Iterator_Wrapper( const Iterator& i ) 
			noexcept ( traits::copy::nothrow )
			: it{ i }
		{}

		constexpr Reverse_Iterator_Wrapper( Iterator&& i )
			noexcept ( traits::copy_construct::nothrow )
			: it{ std::move(i) }
		{}
		constexpr Reverse_Iterator_Wrapper& operator++() 
			noexcept( traits::pre_decrement::nothrow )
		{
			--it;
			return *this; 
		}

		constexpr Reverse_Iterator_Wrapper& operator+=( std::ptrdiff_t distance )
			noexcept( traits::subtract::nothrow )
			requires( traits::subtract::possible )
		{
			it -= distance;
			return *this;
		}

		[[nodiscard]] constexpr Reverse_Iterator_Wrapper operator+( const difference_type dist ) const
			noexcept( traits::subtract::nothrow )
			requires( traits::subtract::possible )
		{
			return { it - dist };
		}

		[[nodiscard]] constexpr auto& operator*() const
			noexcept( traits::deref::nothrow )
		{
			return *it;
		}

		[[nodiscard]] constexpr auto* operator->() const
			noexcept( traits::deref::nothrow )
		{
			return std::addressof(*it);
		}

		[[nodiscard]] constexpr auto operator<=>( const Reverse_Iterator_Wrapper& other ) const noexcept
		{
			return negate_ordering( it <=> other.it );
		}
		[[nodiscard]] constexpr bool operator==( const Reverse_Iterator_Wrapper& ) const noexcept = default;
	};

	template<typename R>
	class Stable_Contigous_Iterator
	{
		R* range_ = nullptr;
		size_t index_ = 0;

		Stable_Contigous_Iterator( R& container, size_t index )
			requires type_traits<R>::subscript::possible
			: range_{ container }, index_{ index }
		{ }

		[[nodiscard]] auto& operator*() const noexcept
		{
			return (*range_)[index_];
		}

		[[nodiscard]] auto& operator[]( size_t idx ) const
		{
			return (*range_)[index_+idx];
		}


		[[nodiscard]] std::partial_ordering operator<=>( const Stable_Contigous_Iterator& other ) const
		{
			if ( this->range_ != other.range_ )
			{
				return std::partial_ordering::unordered;
			}
			else
			{
				return this->index_ <=> other.index_;
			}
		}

		[[nodiscard]] bool operator==( const Stable_Contigous_Iterator& other ) const
		{
			return ( *this <=> other ) == std::partial_ordering::equivalent;
		}
		

		auto& operator++() 
		{
			++index_;
			return *this;
		}

		[[nodiscard]] auto operator++(int) noexcept
		{
			auto copy{*this};
			++index_;
			return copy;
		}


		auto& operator--() noexcept
		{
			--index_;
			return *this;
		}

		[[nodiscard]] auto operator--(int) noexcept
		{
			auto copy{*this};
			--index_;
			return copy;
		}


		auto& operator+=( std::ptrdiff_t offset ) noexcept
		{
			index_ += offset;
			return *this;
		}
		auto& operator-=( std::ptrdiff_t offset) noexcept
		{
			index_ -= offset;
			return *this;
		}


		[[nodiscard]] friend Stable_Contigous_Iterator operator+( const Stable_Contigous_Iterator& it, ptrdiff_t offset ) noexcept
		{
			return Stable_Contigous_Iterator{it} += offset;
		}

		[[nodiscard]]friend Stable_Contigous_Iterator operator-( const Stable_Contigous_Iterator& it, ptrdiff_t offset ) noexcept
		{
			return Stable_Contigous_Iterator{it} -= offset;
		}
	};

	template<typename R>
		requires type_traits<R>::subscript::possible
	Stable_Contigous_Iterator<R> stable_begin( R& r ) noexcept
	{
		return { r, 0 };
	}

	template<typename R>
		requires type_traits<R>::subscript::possible
	Stable_Contigous_Iterator<R> stable_end( R& r ) noexcept
	{
		return { r, std::ranges::size(r) };
	}
}

#endif