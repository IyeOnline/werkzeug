#ifndef WERKZEUG_GUARD_ALGORITHM_ITERATION_HPP
#define WERKZEUG_GUARD_ALGORITHM_ITERATION_HPP

#include <concepts>

#include "werkzeug/concepts.hpp"

namespace werkzeug
{
	template<reverse_iterable T>
	struct reverse
	{
		T value;

		[[nodiscard]] auto begin() 
		{
			return value.rbegin();
		}

		[[nodiscard]] auto end()
		{
			return value.rend();
		}
	};
	template<typename T>
	reverse( T& ) -> reverse<T&>;
	template<typename T>
	reverse( T&& ) -> reverse<T>;

	template<iterable T>
	struct enumerate
	{
		T value;

		using iterator_t = decltype(std::declval<T>().begin());
		using value_t = decltype(*(std::declval<T>().begin()));

		struct binding
		{
			std::size_t count;
			value_t& value;
		};

		struct iterator
		{
			std::size_t count;
			iterator_t it;

			iterator& operator++()
			{
				++count;
				++it;
				return *this;
			}

			bool operator == ( const iterator& other ) const noexcept
			{
				return it == other.it;
			}

			bool operator != ( const iterator& other ) const noexcept
			{
				return it != other.it;
			}

			binding operator * () const
			{
				return binding{ count, *it };
			}
		};

		[[nodiscard]] auto begin()
		{
			return iterator{ 0, value.begin() };
		}

		[[nodiscard]] auto end()
		{
			return iterator{ 0, value.end() };
		}
	};
	template<typename T>
	enumerate( T& ) -> enumerate<T&>;
	template<typename T>
	enumerate( T&& ) -> enumerate<T>;
}

#endif