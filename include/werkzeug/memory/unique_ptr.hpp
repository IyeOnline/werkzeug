#ifndef WERKZEUG_GUARD_MEMORY_UNIQUE_PTR
#define WERKZEUG_GUARD_MEMORY_UNIQUE_PTR

#include "werkzeug/memory/concepts.hpp"
#include "werkzeug/memory/resource.hpp"


namespace werkzeug
{
	namespace memory
	{
		/**
		* @brief Generic Delter
		* 
		* @tparam T type to delete
		* @tparam Resource resource type to hold/use
		*/
		template<typename T, memory::concepts::memory_source Resource>
		class Resource_Deleter
		{
			[[no_unique_address]] Resource resource_;

			using resource_block_type = Block_Type_of<Resource>;
			using resource_pointer_type = decltype(resource_block_type::ptr);

			constexpr static bool is_nothrow_delete = noexcept(resource_.deallocate( resource_block_type{} ) ) and std::is_nothrow_destructible_v<T>;

		public :
			Resource_Deleter() 
				requires std::is_default_constructible_v<Resource> 
				= default;
			explicit Resource_Deleter( Resource resource )
				: resource_( std::forward<Resource>(resource))
			{}

			void operator()(Block_Type<T*> blk ) noexcept(is_nothrow_delete)
			{
				for ( std::size_t i=0; i < blk.size; ++i )
				{
					blk.ptr[i].~T();
				}
				assert( resource_.deallocate( block_cast<resource_pointer_type>( blk ) ) );
			}
		};

		template<auto F>
		struct Callable_Deleter_Wrapper
		{
			decltype(auto) operator()( auto obj ) const 
				noexcept(noexcept(F(obj)))
				requires requires { F(obj); }
			{
				return F(obj);
			}
		};


		namespace detail
		{
			template<typename T>
			struct block_compress
			{
				T* ptr_ = nullptr;

				static constexpr std::size_t size() noexcept
				{
					return 1;
				}
			};

			template<typename T>
			struct block_compress<T[]>
			{
				T* ptr_ = nullptr;
				std::size_t size_ = 0;

				constexpr std::size_t size() const noexcept
				{
					return size_;
				}
			};

			template<typename T, std::size_t N>
			struct block_compress<T[N]>
			{
				T* ptr_ = nullptr;

				static constexpr std::size_t size() noexcept
				{
					return N;
				}
			};
		}
	}

	/**
	 * @brief unique pointer implementation. supports arrays and generic deleters. Best created with a version of 'make_unique'
	 * 
	 * @tparam T managed type. May be unbounded array
	 * @tparam Deleter_T Deleter to delete (destroy and deallocate) T
	 */
	template<typename T, typename Deleter_T=memory::Resource_Deleter<std::remove_extent_t<T>,memory::resource::fixed::New_Resource>>
		requires std::is_invocable_v<Deleter_T,memory::Block_Type<std::remove_extent_t<T>*>>
	class unique_ptr
	{
		constexpr static bool is_bounded_array = std::is_bounded_array_v<T>;
		constexpr static bool is_unbounded_array = std::is_unbounded_array_v<T>;
		constexpr static bool is_array = is_bounded_array or is_unbounded_array;

		using plain_managed_type = std::remove_extent_t<T>;
		using pointer = plain_managed_type*;
		using const_pointer = const plain_managed_type*;
		using block_type = memory::Block_Type<pointer>;

		memory::detail::block_compress<T> comp_block_;
		[[no_unique_address]] Deleter_T deleter_{};

		constexpr static bool is_nothrow_delete = noexcept( deleter_(block_type{}) );
	public :
		unique_ptr() = default;

		unique_ptr( const pointer ptr ) noexcept
			requires ( not is_unbounded_array and std::is_default_constructible_v<Deleter_T> )
			: comp_block_{ ptr }
		{ }

		unique_ptr( const pointer ptr, const std::size_t size ) noexcept
			requires ( is_unbounded_array and std::is_default_constructible_v<Deleter_T> )
			: comp_block_{ ptr, size }
		{ }

		unique_ptr( const pointer ptr, Deleter_T deleter ) noexcept
			requires ( not is_unbounded_array )
			: comp_block_{ ptr }, deleter_{ std::move(deleter) }
		{ }

		unique_ptr( const pointer ptr, const std::size_t size, Deleter_T deleter ) noexcept
			requires is_unbounded_array
			: comp_block_{ ptr, size }, deleter_{ std::move(deleter) }
		{ }


		unique_ptr( const unique_ptr& ) = delete;
		unique_ptr& operator= ( const unique_ptr& ) = delete;

		unique_ptr( unique_ptr&& source ) noexcept
			requires std::is_move_constructible_v<Deleter_T>
			: comp_block_{ std::exchange(source.comp_block_, {}) }, deleter_{ std::move(source.deleter_) }
		{ }

		unique_ptr& operator= ( unique_ptr&& source ) noexcept
			requires std::is_move_assignable_v<Deleter_T>
		{
			clear();
			comp_block_ = std::exchange(source.comp_block_, {});
			deleter_ = std::move(source.deleter_);
			return *this;
		}

		template<typename Derived_T>
		unique_ptr( unique_ptr<Derived_T,Deleter_T>&& source ) noexcept
			requires ( not is_array and std::has_virtual_destructor_v<T> and std::derived_from<Derived_T,T> )
			: comp_block_{ std::exchange(source.comp_block_.ptr_, nullptr) } , deleter_(std::move(source.deleter_))
		{ }

		template<typename Derived_T>
		unique_ptr& operator= ( unique_ptr<Derived_T,Deleter_T>&& source ) noexcept
			requires ( not is_array and std::has_virtual_destructor_v<T> and std::derived_from<Derived_T,T> )
		{
			clear();
			comp_block_.ptr_ = std::exchange( source.comp_block_.ptr_, nullptr );
			deleter_ = std::move(source.deleter_);
			return *this;
		}


		void clear() noexcept(is_nothrow_delete)
		{
			if ( comp_block_.ptr_ != nullptr ) //do a check, so that we can ensure a moved from deleter is not invoked.
			{
				deleter_( block_type{comp_block_.ptr_,comp_block_.size() } );
			}
		}

		~unique_ptr() noexcept(is_nothrow_delete)
		{
			clear();
		}

		[[nodiscard]] bool has_value() const noexcept
		{
			return comp_block_.ptr_;
		}

		operator bool() const noexcept
		{
			return has_value();
		}

		/** @brief get pointer to managed object/array */
		pointer get() const noexcept
		{
			return comp_block_.ptr_;
		}

		/** @brief get pointer to managed array */
		pointer data() const noexcept
			requires is_array
		{
			return comp_block_.ptr_;
		}

		/** @brief arrow operator to interact with the manged object */
		pointer operator-> () const noexcept
			requires ( not is_array )
		{
			return comp_block_.ptr_;
		}

		/** @brief dereference operator, returning a reference to the managed object */
		plain_managed_type& operator* () const noexcept
			requires ( not is_array )
		{
			return *comp_block_.ptr_;
		}

		/** @brief member pointer access operator, accessing the managed object */
		auto& operator->*( auto plain_managed_type::* ptr ) const noexcept
			requires ( not is_array )
		{
			return comp_block_.ptr_->*ptr;
		}

		/** @brief subscript operator for array version */
		plain_managed_type& operator[] ( std::size_t idx ) const noexcept
			requires is_array
		{
			assert( idx < comp_block_.size() );
			return comp_block_.ptr_[idx];
		}

		/** @brief gets the size of the managed array */
		std::size_t size() const
			requires is_array
		{
			return comp_block_.size();
		}

		pointer begin() noexcept
			requires is_array
		{
			return comp_block_.ptr_;
		}
		pointer end() noexcept
			requires is_array
		{
			return begin() + comp_block_.size();
		}

		const_pointer begin() const noexcept
			requires is_array
		{
			return comp_block_.ptr_;
		}
		const_pointer end() const noexcept
			requires is_array
		{
			return begin() + comp_block_.size();
		}
	};

	// =============================================================================
	// ==================== Make unique for single object ==========================
	// =============================================================================
	/**
	 * @brief creates a new unique_pointer allocating on 'r' and constructing the object of type T from args...
	 * 
	 * @tparam T type to create and manage
	 * @tparam Resource resource type to use
	 * @tparam Args... arguments to construct 'T' from
	 * 
	 * @param r The concrete resource object/reference to use
	 * @param args... the arguments to forward to the constructor of T
	 */
	template<typename T, memory::concepts::memory_source Resource, typename ... Args>
		requires (not std::is_array_v<T> )
	unique_ptr<T,memory::Resource_Deleter<T,Resource>> make_unique_with_resource( Resource r, Args&& ... args )
	{
		using deleter_t = memory::Resource_Deleter<T,Resource>;

		const auto memory = r.allocate( sizeof(T), alignof(T) );
		assert( memory.ptr != nullptr );

		auto* const ptr = reinterpret_cast<T*>(memory.ptr);
		std::construct_at( ptr, std::forward<Args>(args) ... );

		return { ptr, deleter_t{ std::forward<Resource>(r) } };
	}

	/**
	 * @brief creates a new unique_pointer allocating via 'new'
	 * 
	 * @tparam T type to create and manage
	 * @tparam Args... arguments to construct 'T' from
	 * 
	 * @param args... the arguments to forward to the constructor of T
	 */
	template<typename T, typename ... Args>
		requires (not std::is_array_v<T> )
	auto make_unique( Args&& ... args )
	{
		using resource_t = memory::resource::fixed::New_Resource;
		return make_unique_with_resource<T,resource_t, Args...>( resource_t{}, std::forward<Args>(args) ... );
	}


	/**
	 * @brief creates a new unique_pointer allocating on 'r' and constructing the object of type T from args...
	 * 
	 * @tparam T type to create and manage
	 * @tparam Resource resource type to use
	 * @tparam Args... arguments to construct 'T' from
	 * 
	 * @param r The concrete resource reference to use
	 * @param args... the arguments to forward to the constructor of T
	 */
	template<typename T, memory::concepts::memory_source Resource, typename ... Args>
		requires (not std::is_array_v<T> )
	auto make_unique_with_resource_reference( Resource& r, Args&& ... args )
	{
		return make_unique_with_resource<T,Resource&>( r, std::forward<Args>(args) ... );
	}

	// =============================================================================
	// ==================== Make unique for bounded array ==========================
	// =============================================================================


	/**
	 * @brief creates a new unique_pointer allocating on 'r' and constructing an array of T. The elements are *not* initialized
	 * 
	 * @tparam T array of known bounds
	 * @tparam Resource resource type to use
	 * 
	 * @param r The concrete resource object/reference to use
	 * @param size the size of the array to create
	 */
	template<typename T, memory::concepts::memory_source Resource>
		requires std::is_bounded_array_v<T>
	auto make_unique_with_resource_for_overwrite( Resource r )
	{
		constexpr auto size = std::extent_v<T>;
		using raw_t = std::remove_extent_t<T>;
		using deleter_t = memory::Resource_Deleter<raw_t,Resource>;

		const auto memory = r.allocate( sizeof(raw_t) * size, alignof(raw_t) );
		assert( memory.ptr != nullptr );

		auto* const ptr = reinterpret_cast<raw_t*>(memory.ptr);

		return unique_ptr<T,deleter_t>{ ptr, deleter_t{ std::forward<Resource>(r) } };
	}

	template<typename T, memory::concepts::memory_source Resource>
		requires std::is_bounded_array_v<T>
	auto make_unique_with_resource_reference_for_overwrite( Resource& r )
	{
		return make_unique_with_resource_for_overwrite<T,Resource&>( r );
	}


	/**
	 * @brief creates a new unique_pointer allocating an array of 'T' on 'r' and constructing the objects of type T from args...
	 * 
	 * @tparam T array of known bounds
	 * @tparam Resource resource type to use
	 * @tparam Args... arguments to construct the elements in 'T' from
	 * 
	 * @param r The concrete resource to use
	 * @param args... the arguments to forward to the constructor of T for each element
	 */
	template<typename T, memory::concepts::memory_source Resource, typename ... Args>
		requires std::is_bounded_array_v<T>
	auto make_unique_with_resource( Resource r, const Args& ... args )
	{
		auto&& ptr = make_unique_with_resource_for_overwrite<T,Resource,Args...>(r, args... );

		constexpr auto size = std::extent_v<T>;
		for ( std::size_t i=0; i < size; ++i )
		{
			std::construct_at( ptr.get() + i, args ... );
		}

		return ptr;
	}

	/**
	 * @brief creates a new unique_pointer allocating an array via '::operator new' on 'r' and constructing the objects of type T from args...
	 * 
	 * @tparam T bounded array type
	 * @tparam Args... arguments to construct the elements in 'T' from
	 * 
	 * @param args... the arguments to forward to the constructor of T for each element
	 */
	template<typename T, typename ... Args>
		requires std::is_bounded_array_v<T>
	auto make_unique( const Args& ... args )
	{
		// using raw_t = std::remove_extent_t<T>;
		using resource_t = memory::resource::fixed::New_Resource;
		return make_unique_with_resource<T,resource_t, Args...>( resource_t{}, args ... );
	}

	/**
	 * @brief creates a new unique_pointer allocating an array of 'T' on 'r' and constructing the objects of type T from args...
	 * 
	 * @tparam T bounded array type
	 * @tparam Resource resource type to use
	 * @tparam Args... arguments to construct the elements in 'T' from
	 * 
	 * @param r The concrete resource reference to use
	 * @param args... the arguments to forward to the constructor of T for each element
	 */
	template<typename T, memory::concepts::memory_source Resource, typename ... Args>
		requires std::is_bounded_array_v<T>
	auto make_unique_with_resource_reference( Resource& r, const Args& ... args )
	{
		return make_unique_with_resource<T,Resource&>( r, args... );
	}

	// =============================================================================
	// ==================== Make unique for unbounded array ========================
	// =============================================================================
	
	/**
	 * @brief creates a new unique_pointer allocating on 'r' and constructing an array of T. The elements are *not* initialized
	 * 
	 * @tparam T unbounded array type
	 * @tparam Resource resource type to use
	 * 
	 * @param r The concrete resource object/reference to use
	 * @param size the size of the array to create
	 */
	template<typename T, memory::concepts::memory_source Resource>
		requires std::is_unbounded_array_v<T>
	auto make_unique_with_resource_for_overwrite( Resource r, const std::size_t size )
	{
		using raw_t = std::remove_extent_t<T>;
		using deleter_t = memory::Resource_Deleter<raw_t,Resource>;

		const auto memory = r.allocate( sizeof(raw_t) * size, alignof(raw_t) );
		assert( memory.ptr != nullptr );

		auto* const ptr = reinterpret_cast<raw_t*>(memory.ptr);

		return unique_ptr<T,deleter_t>{ ptr, size, deleter_t{ std::forward<Resource>(r) } };
	}

	/**
	 * @brief creates a new unique_pointer allocating an array of 'T' on 'r' and constructing the objects of type T from args...
	 * 
	 * @tparam T unbounded array type
	 * @tparam Resource resource type to use
	 * @tparam Args... arguments to construct the elements in 'T' from
	 * 
	 * @param r The concrete resource to use
	 * @param size the size of the array to create
	 * @param args... the arguments to forward to the constructor of T for each element
	 */
	template<typename T, memory::concepts::memory_source Resource, typename ... Args>
		requires std::is_unbounded_array_v<T>
	auto make_unique_with_resource( Resource r, const std::size_t size, const Args& ... args )
	{
		auto&& ptr = make_unique_with_resource_for_overwrite<T,Resource,Args...>(r, size, args... );

		for ( std::size_t i=0; i < size; ++i )
		{
			std::construct_at( ptr.get() + i, args ... );
		}

		return ptr;
	}

	/**
	 * @brief creates a new unique_pointer allocating an array via '::operator new' on 'r' and constructing the objects of type T from args...
	 * 
	 * @tparam T unbounded array type
	 * @tparam Args... arguments to construct the elements in 'T' from
	 * 
	 * @param size the size of the array to create
	 * @param args... the arguments to forward to the constructor of T for each element
	 */
	template<typename T, typename ... Args>
		requires std::is_unbounded_array_v<T>
	auto make_unique( const std::size_t size, const Args& ... args )
	{
		// using raw_t = std::remove_extent_t<T>;
		using resource_t = memory::resource::fixed::New_Resource;
		return make_unique_with_resource<T,resource_t, Args...>( resource_t{}, size, args ... );
	}

	
	/**
	 * @brief creates a new unique_pointer allocating an array of 'T' on 'r' and constructing the objects of type T from args...
	 * 
	 * @tparam T unbounded array type
	 * @tparam Resource resource type to use
	 * @tparam Args... arguments to construct the elements in 'T' from
	 * 
	 * @param r The concrete resource reference to use
	 * @param size the size of the array to create
	 * @param args... the arguments to forward to the constructor of T for each element
	 */
	template<typename T, memory::concepts::memory_source Resource, typename ... Args>
		requires std::is_unbounded_array_v<T>
	auto make_unique_with_resource_reference( Resource& r, const std::size_t size, const Args& ... args )
	{
		return make_unique_with_resource<T,Resource&>( r, size, args... );
	}
}

#endif