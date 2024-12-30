#ifndef WERKZEUG_GUARD_CONTAINER_LIST_HPP
#define WERKZEUG_GUARD_CONTAINER_LIST_HPP

#include "werkzeug/concepts.hpp"
#include "werkzeug/container/crtp_range_bases.hpp"
#include "werkzeug/memory/concepts.hpp"
#include "werkzeug/memory/allocator.hpp"
#include "werkzeug/container/detail/dll_node.hpp"

namespace werkzeug
{
	template<typename T, memory::concepts::memory_source Resource = memory::resource::fixed::New_Resource>
	class list
		: public detail::Range_Stream_CRTP_Base<list<T,Resource>>
		, public detail::Range_Threeway_CRTP_Base<list<T,Resource>,T>
	{
		using node_t = detail::DLL_Node<T>;
		using node_ptr_t = node_t*;
		
		node_ptr_t head = nullptr;
		node_ptr_t tail = nullptr;
		std::size_t size_ = 0;

		using Allocator =  memory::Allocator<node_t,Resource>;
		using resource_traits = memory::Resource_Traits<Resource>;
		using traits = type_traits<T>;
		[[no_unique_address]] Allocator alloc;

		template<bool is_const, bool is_backward>
		class iterator_impl
		{
			using ptr_t = std::conditional_t<is_const, const node_t*, node_t*>;

			ptr_t ptr = nullptr;
			bool is_end = false;

			friend class list;
			
		public :
			iterator_impl( ptr_t ptr_, bool is_end_ )
				: ptr{ ptr_ }, is_end{ is_end_ }
			{}

			bool operator == ( const iterator_impl& other ) const noexcept = default;

			auto& operator ++ () noexcept
			{
				if constexpr ( is_backward )
				{
					if ( ptr->prev != nullptr )
					{ ptr = ptr->prev; }
					else
					{ is_end = true; }
				}
				else
				{
					if ( ptr->next != nullptr )
					{ ptr = ptr->next; }
					else
					{ is_end = true; }
				}
				return *this;
			}

			auto& operator -- () noexcept
			{
				if constexpr ( is_backward )
				{
					if ( ptr->next != nullptr )
					{ ptr = ptr->next; }
					else
					{ is_end = true; }
				}
				else
				{
					if ( ptr->prev != nullptr )
					{ ptr = ptr->prev; }
					else
					{ is_end = true; }
				}
				return *this;
			}

			auto& operator * () const noexcept
			{
				WERKZEUG_ASSERT( not is_end, "Must not dereference end iterator" );
				return ptr->value;
			}

			auto* operator -> () const noexcept
			{
				WERKZEUG_ASSERT( not is_end, "Must not dereference end iterator" );
				return &(ptr->value);
			}
		};

	public :
		using value_type = T;
		using iterator = iterator_impl<false,false>;
		using const_iterator = iterator_impl<true,false>;
		using reverse_iterator = iterator_impl<false,true>;
		using const_reverse_iterator = iterator_impl<true,true>;


		list() noexcept
			requires std::is_default_constructible_v<Resource>
			= default;


		explicit list( Resource r ) noexcept
			: alloc{ r }
		{ }


		list( const list& other ) noexcept(Allocator::is_nothrow_alloc && std::is_nothrow_copy_constructible_v<T> )
			: alloc{ other.alloc }
		{
			for( auto ptr = other.head; ptr != nullptr; ptr = ptr->next )
			{
				emplace_back( ptr->value );
			}
		}
		

		list ( range_of_type<T> auto&& source, Resource r )
			: alloc{ r }
		{
			for ( decltype(auto) value : source )
			{
				emplace_back( std::forward<decltype(value)>(value) );
			}
		}


		list( list&& other ) noexcept
			: head{ std::exchange(other.head,nullptr) }, tail{ std::exchange(other.tail, nullptr) }, size_{ std::exchange(other.size_, 0) }
			, alloc{ other.alloc }
		{ }


		list& operator=( list&& other ) noexcept
		{
			alloc = std::move(other.allocator);
			head = std::exchange( other.head, head );
			tail = std::exchange( other.tail, tail );
			size_ = std::exchange( other.size_, size_ );
			return *this;
		}


		~list() noexcept( resource_traits::deallocate::nothrow and traits::destroy::nothrow )
		{
			clear();
		}


		void clear() noexcept( resource_traits::deallocate::nothrow and traits::destroy::nothrow )
		{
			while ( tail != nullptr )
			{
				const auto prev = tail->prev;
				if constexpr ( not std::is_trivially_destructible_v<node_t> )
				{
					tail->~node_t();
				}
				WERKZEUG_ASSERT( alloc.deallocate_single( tail ), "deallocation must succeed" );
				tail = prev;
			}
			head = nullptr;
			tail = nullptr;
			size_ = 0;
		}

		
		template<typename ... Args>
		T& emplace_front( Args&& ... args )
			noexcept ( Allocator::is_nothrow_alloc && std::is_nothrow_constructible_v<T,Args...> )
			requires ( std::is_constructible_v<T,Args...> )
		{
			const auto memory = alloc.allocate_single();
			WERKZEUG_ASSERT( memory!=nullptr, "Allocation failure" );
			const auto new_node = std::construct_at( memory, head, nullptr, std::forward<Args>(args) ... );
			if ( head == nullptr )
			{
				head = new_node;
				tail = new_node;
			}
			else
			{
				head->prev = new_node;
				head = new_node;
			}
			++size_;

			return head->value;
		}


		template<typename ... Args>
		T& emplace_back( Args&& ... args ) 
			noexcept ( Allocator::is_nothrow_alloc && std::is_nothrow_constructible_v<T,Args...> )
			requires ( std::is_constructible_v<T,Args...> )
		{
			const auto memory = alloc.allocate_single();
			WERKZEUG_ASSERT( memory!=nullptr, "Allocation failure" );
			const auto new_node = std::construct_at( memory, nullptr, tail, std::forward<Args>(args) ... );
			if ( tail == nullptr )
			{
				head = new_node;
				tail = new_node;
			}
			else
			{
				tail->next = new_node;
				tail = new_node;
			}
			++size_;

			return tail->value;
		}


		template<typename ... Args>
		T& emplace_at( iterator it, Args&& ... args )
			noexcept( resource_traits::allocate::nothrow and std::is_nothrow_constructible_v<T,Args...> )
			requires ( std::is_constructible_v<T,Args...> )
		{
			const auto memory = alloc.allocate_single();
			WERKZEUG_ASSERT( memory!=nullptr, "Allocation failure" );
			const auto new_node = std::construct_at( memory, nullptr, tail, std::forward<Args>(args) ... );

			if ( tail == nullptr )
			{
				tail = new_node;
				head = new_node;
			}
			else 
			{
				detail::enlink_at(new_node, it.ptr );
				if ( it == begin() )
				{
					head = new_node;
				}
				else if ( it == end() )
				{
					tail = new_node;
				}
			}
			++size_;
			
			return new_node->value;
		}


		void splice_at( const iterator it, list&& other ) noexcept
		{
			WERKZEUG_ASSERT( &other != this, "Must not splice list into itself" );
			if ( other.size_ == 0 )
			{
				return;
			}

			if ( it == begin() )
			{
				other.tail->next = head;
				head->prev = other.tail;
				head = other.head;
			}
			else if ( it == end() )
			{
				tail->next = other.head;
				other.head->prev = tail;
				tail = other.tail;
			}
			else
			{
				const auto infront = it.ptr->prev;
				const auto after = it.ptr;

				infront->next = other.head;
				other.head->prev = infront;
				after->prev = other.tail;
				other.tail->next = after;
			}

			other.head = nullptr;
			other.tail = nullptr; 
			size_ += other.size_;
			other.size_ = 0;
		}


		[[nodiscard]] auto size() const noexcept
		{ return size_; }


		[[nodiscard]] iterator begin() noexcept
		{ return { head, head==nullptr }; }
		[[nodiscard]] const_iterator begin() const noexcept
		{ return { head, head==nullptr }; }


		[[nodiscard]] iterator end() noexcept
		{ return { tail, true }; }
		[[nodiscard]] const_iterator end() const noexcept
		{ return { tail, true }; }
	};


	template<typename Range, typename Resource>
	list( Range, Resource&& ) -> list<std::remove_cvref_t<decltype(*std::begin(std::declval<Range>()))>,Resource>;
}

#endif