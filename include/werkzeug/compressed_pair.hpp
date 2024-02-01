#ifndef WEKRZEUG_COMPRESSED_PAIR_HPP
#define WEKRZEUG_COMPRESSED_PAIR_HPP

#include <utility>
#include <type_traits>

namespace werkzeug
{
	namespace detail
	{
		template <class T1, class T2>
		class compressed_pair_impl_T1_empty final : private T1 
		{
			T2 val2_;

		public :
			compressed_pair_impl_T1_empty( const T1&, T2 val2 ) noexcept(std::is_nothrow_move_constructible_v<T2> )
				: val2_{std::move(val2)}
			{}

			T1& val1()
			{ return static_cast<T1>(*this); }
			const T1& val1() const
			{ return static_cast<T1>(*this); }

			T2& val2()
			{ return val2_;}
			const T2& val2() const
			{ return val2_; }
		};

		template <class T1, class T2>
		class compressed_pair_impl_T2_empty final : private T2 
		{
			T1 val1_;

		public :
			compressed_pair_impl_T2_empty( T1 val1, const T2& ) noexcept(std::is_nothrow_move_constructible_v<T1> )
				: val1_(std::move(val1))
			{}
			T1& val1()
			{ return val1_;}
			const T1& val1() const
			{ return val1_; }

			T2& val2()
			{ return static_cast<T2>(*this); }
			const T2& val2() const
			{ return static_cast<T2>(*this); }
		};

		template<class T1, class T2>
		class compressed_pair_impl_no_empty
		{
			T1 val1_;
			T2 val2_;
		public :
			compressed_pair_impl_no_empty( T1 val1, T2 val2 ) noexcept( std::is_nothrow_move_constructible_v<T1> && std::is_nothrow_move_constructible_v<T2> )
				: val1_{ std::move(val1) }, val2_{ std::move(val2) }
			{}
			T1& val1()
			{ return val1_;}
			const T1& val1() const
			{ return val1_; }

			T2& val2()
			{ return val2_;}
			const T2& val2() const
			{ return val2_; }
		};
	}

	template<typename T1,typename T2>
	using compressed_pair = std::conditional_t<
		std::is_empty_v<T1> && !std::is_final_v<T1>,
			detail::compressed_pair_impl_T1_empty<T1,T2>,
		std::conditional_t<std::is_empty_v<T2> && !std::is_final_v<T2>,
			detail::compressed_pair_impl_T2_empty<T1,T2>,
			detail::compressed_pair_impl_no_empty<T1,T2>
		>
	>;

}

#endif