#ifndef WERKZEUG_GUARD_CONTAINER_POLYMORPHIC_LIST_HPP
#define WERKZEUG_GUARD_CONTAINER_POLYMORPHIC_LIST_HPP

#include "werkzeug/container/crtp_range_bases.hpp"
#include "werkzeug/memory/concepts.hpp"
#include "werkzeug/memory/resource.hpp"
#include "werkzeug/assertion.hpp"

namespace werkzeug
{
	template<typename Base, memory::concepts::memory_source Resource = memory::resource::fixed::New_Resource>
	class polymorphic_list
		: public detail::Range_Stream_CRTP_Base<polymorphic_list<Base,Resource>>
		// , public detail::Range_Threeway_CRTP_Base<polymorphic_list<Base,Resource>,Base>
	{
		struct size_align_t
		{
			std::size_t size;
			std::size_t alignment;
		};

		struct Node_Base
		{
			Node_Base* prev = nullptr;
			Node_Base* next = nullptr;

			Node_Base() = default;
			Node_Base( Node_Base* prev_, Node_Base* next_ )
				: prev{prev_}, next{next_}
			{ }
			Node_Base( const Node_Base& ) = delete;

			virtual Base* get_pointer() noexcept = 0;
			virtual const Base* get_pointer() const noexcept = 0;

			virtual size_align_t get_size_align() const noexcept = 0;
			virtual Node_Base* clone_into( memory::Pointer_Type_of<Resource> ) const = 0;
			virtual ~Node_Base() = default;
		};


		template<typename U>
		struct Node final : Node_Base
		{
			U value;

			template<typename ... Args>
			Node( Node_Base* prev, Node_Base* next, Args&& ... args )
				: Node_Base{ prev, next }, value{ std::forward<Args>(args) ... }
			{ };


			Node( const U& value )
				: value{ value }
			{ };

			Base* get_pointer() noexcept override
			{ return &value; }
			const Base* get_pointer() const noexcept override
			{ return &value; }

			size_align_t get_size_align() const noexcept override
			{ return { sizeof(Node), alignof(Node) }; }

			Node_Base* clone_into( memory::Pointer_Type_of<Resource> dest ) const override
			{
				return new( dest ) Node{ value };
			}
		};

		using resource_traits = memory::Resource_Traits<Resource>;

		using node_t = Node_Base;

		using node_ptr_t = node_t*;

		node_ptr_t head = nullptr;
		node_ptr_t tail = nullptr;
		std::size_t size_ = 0;

		[[no_unique_address]] Resource resource;

		static auto as_resource_pointer( auto ptr ) noexcept
		{
			return reinterpret_cast<memory::Pointer_Type_of<Resource>>( ptr );
		}

		node_ptr_t copy_single( node_ptr_t source_node )
		{
			const auto [ size, alignment ] = source_node->get_size_align();

			const auto block = resource.allocate( size, alignment );
			WERKZEUG_ASSERT( block, "allocation required" );

			auto* node = source_node->clone_into( block.ptr );

			return node;
		}

		// doesnt update size
		void copy_from( const polymorphic_list& other )
		{
			if ( other.head == nullptr )
			{
				return;
			}
			head = copy_single( other.head );
			auto current_tail = head;
			// for ( auto source_ptr = other.head->next, dest_ptr = head; source_ptr!=nullptr; source_ptr = source_ptr->next )
			for ( auto source_ptr = other.head->next; source_ptr!=nullptr; source_ptr = source_ptr->next )
			{
				auto new_node = copy_single( source_ptr );
				current_tail->next = new_node;
				new_node->prev = current_tail;
				current_tail = new_node;
			}
			tail = current_tail;
		}

		template<bool is_const, bool is_backward>
		class iterator_impl
		{
			using ptr_t = std::conditional_t<is_const, const node_t*, node_t*>;

			ptr_t ptr = nullptr;
			bool is_end = false;

			friend class polymorphic_list;

			friend class iterator_impl<true,true>;
			friend class iterator_impl<true,false>;
			
		public :
			iterator_impl( const iterator_impl& ) = default;
			iterator_impl( const iterator_impl<false, is_backward>& other )
				requires ( is_const )
				: ptr{ other.ptr }, is_end{ other.is_end }
			{ }

			iterator_impl( ptr_t p, bool ie )
				: ptr{ p }, is_end{ ie }
			{ }

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
				return *(ptr->get_pointer());
			}

			auto* operator -> () const noexcept
			{
				WERKZEUG_ASSERT( not is_end, "Must not dereference end iterator" );
				return ptr->get_pointer();
			}
		};

	public :
		using value_type = Base;
		using iterator = iterator_impl<false,false>;
		using const_iterator = iterator_impl<true,false>;
		using reverse_iterator = iterator_impl<false,true>;
		using const_reverse_iterator = iterator_impl<true,true>;

		polymorphic_list() noexcept = default;

		polymorphic_list( Resource resource ) noexcept
			: resource{ std::forward<Resource>(resource) }
		{ }

		polymorphic_list( const polymorphic_list& other )
			: size_{other.size_}, resource{ other.resource }
		{
			copy_from( other );
		}

		polymorphic_list( polymorphic_list&& other ) noexcept
			: head{ std::exchange(other.head, nullptr) }, tail{ std::exchange(other.tail,nullptr) }, size_{ std::exchange(other.size_,0) }
			, resource{ std::forward<Resource>(resource) }
		{ }

		polymorphic_list& operator=( const polymorphic_list& other ) 
		{
			clear();
			copy_from( other );
		}

		polymorphic_list& operator=( polymorphic_list&& other ) noexcept
		{
			resource = std::move(other.resource);
			head = std::exchange( other.head, head );
			tail = std::exchange( other.tail, tail );
			size_ = std::exchange( other.size_, size_ );
			return *this;
		}

		~polymorphic_list() noexcept(noexcept(clear()))
		{
			clear();
		}

		void clear()
		{
			while ( tail != nullptr )
			{
				tail->get_pointer()->~Base();
				const auto prev = tail->prev;
				const auto [ size, alignment ] = tail->get_size_align();
				assert( resource.deallocate( { as_resource_pointer(tail), size }, alignment ) );
				tail = prev;
			}
			head = nullptr;
			tail = nullptr;
			size_ = 0;
		}

		template<typename Derived = Base, typename ... Args>
		Derived& emplace_back( Args&& ... args )
			noexcept ( type_traits<Derived>::template construct_from<Args...>::nothrow and resource_traits::allocate::nothrow )
			requires ( std::is_base_of_v<Base,Derived> )
		{
			using Node_T = Node<Derived>;
			const auto block = resource.allocate( sizeof(Node_T), alignof(Node_T) );
			assert( block );

			auto new_node = new (block.ptr) Node_T{ tail, nullptr, std::forward<Args>(args) ... };

			if ( head == nullptr )
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

			return new_node->value;
		}


		template<typename Derived = Base, typename ... Args>
		Derived& emplace_front( Args&& ... args )
			noexcept ( type_traits<Derived>::template construct_from<Args...>::nothrow and resource_traits::allocate::nothrow )
			requires ( std::is_base_of_v<Base,Derived> )
		{
			using Node_T = Node<Derived>;
			const auto block = resource.allocate( sizeof(Node_T), alignof(Node_T) );
			assert( block );

			auto new_node = new (block.ptr) Node_T{ nullptr, head, std::forward<Args>(args) ... };

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

			return new_node->value;
		}

		template<typename Derived = Base, typename ... Args>
		Derived& emplace_at( iterator position, Args&& ... args )
			noexcept ( type_traits<Derived>::template construct_from<Args...>::nothrow and resource_traits::allocate::nothrow )
			requires ( std::is_base_of_v<Base,Derived> )
		{
			if ( position == end() )
			{
				return emplace_back<Derived>( std::forward<Args>( args ) ... );
			}
			else if ( position == begin() )
			{
				return emplace_front<Derived>( std::forward<Args>( args ) ... );
			}

			auto* insertion_point = position.ptr;

			using Node_T = Node<Derived>;
			const auto block = resource.allocate( sizeof(Node_T), alignof(Node_T) );
			assert( block );

			auto* new_node = new (block.ptr) Node_T{ nullptr, nullptr, std::forward<Args>(args) ... };

			detail::enlink_at( static_cast<Node_Base*>(new_node), insertion_point );

			if ( new_node->prev == nullptr )
			{
				head = new_node;
			}

			++size_;

			return new_node->value;
		}

		void splice_at( const iterator it, polymorphic_list&& other ) noexcept
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

		void print_links( std::ostream& os = std::cout ) const noexcept
		{
			for ( auto curr = head; curr != nullptr; curr = curr->next )
			{
				detail::print_node( os, curr );
				os << '\n';
			}
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
}

#endif