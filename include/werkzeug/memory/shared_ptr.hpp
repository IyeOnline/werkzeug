#ifndef WERKZEUG_GUARD_MEMORY_SHARED_PTR_HPP
#define WERKZEUG_GUARD_MEMORY_SHARED_PTR_HPP

#include <atomic>
#include <cassert>
#include <mutex>
#include <utility>

#include "werkzeug/manual_lifetime.hpp"
#include "werkzeug/memory/concepts.hpp"

namespace werkzeug
{
	// template<typename T, memory::concepts::memory_source Resource>
	// class reference_counted_pointer
	// {
	// 	struct Allocation
	// 	{
	// 		std::mutex mut;
	// 		std::size_t ref_counter;
	// 		T value;

	// 		std::size_t increment()
	// 		{
	// 			std::scoped_lock _{mut};
	// 			return ++ref_counter;
	// 		}

	// 		std::size_t decrement()
	// 		{
	// 			std::scoped_lock _{mut};
	// 			return --ref_counter;
	// 		}
	// 	};

	// 	Allocation* ptr_ = nullptr;

	// 	reference_counted_pointer() = default;

	// 	reference_counted_pointer( const reference_counted_pointer& src )
	// 		: ptr_{ src.ptr_ }
	// 	{
	// 		if ( ptr_ != nullptr )
	// 		{
	// 			ptr_->increment();
	// 		}
	// 	}

	// 	reference_counted_pointer( reference_counted_pointer&& src )
	// 		: ptr_{ std::exchange( src.ptr_, nullptr) }
	// 	{ }

	// 	reference_counted_pointer& operator=( const reference_counted_pointer& src )
	// 	{
	// 		const std::size_t count = src.ptr_->increment();
	// 		if ( ptr_->decrement() == 0 )
	// 		{
	// 			delete ptr_;
	// 		}
	// 		if ( count == 0 )
	// 		{
	// 			ptr_ = nullptr;
	// 		}
	// 		else 
	// 		{
	// 			ptr_ = src.ptr_;
	// 		}
	// 	}
	// 	reference_counted_pointer& operator=( reference_counted_pointer&& src )
	// 	{
	// 		if ( ptr_->decrement() == 0 )
	// 		{
	// 			delete ptr_;
	// 		}
	// 		ptr_ = src.ptr_;
	// 	}

	// 	~reference_counted_pointer()
	// 	{
	// 		if ( ptr_->decrement() == 0 )
	// 		{
	// 			delete ptr_;
	// 		}
	// 	}

	// 	operator bool() const
	// 	{
	// 		return ptr_ != nullptr;
	// 	}

	// 	T& operator*() const
	// 	{
	// 		return ptr_->value;
	// 	}

	// 	T* operator->() const
	// 	{
	// 		return std::addressof( ptr_->value );
	// 	}

	// 	T& operator[]( const std::size_t idx ) const
	// 		requires std::is_array<T>
	// 	{
	// 		assert( idx < std::size(ptr_->value) );
	// 		return ptr_->value[idx];
	// 	}
	// };


	template<typename T>
	class shared_ptr
	{
		struct allocation_base
		{
			std::mutex mutex_;
			std::size_t strong_counter_{};
			std::size_t weak_counter_{};

			Manual_Lifetime<T> value_;

			std::size_t strong_count() const
			{ return strong_counter_; }

			std::size_t weak_count() const
			{ return weak_counter_; }

			std::size_t increment_strong()
			{ 
				const std::scoped_lock _{mutex_};
				return ++strong_counter_; 
			}

			std::size_t increment_weak()
			{ 
				const std::scoped_lock _{mutex_};
				return ++weak_counter_; 
			}

			std::size_t decrement_strong_and_destroy()
			{
				const std::scoped_lock _{mutex_};
				--strong_counter_;
				if ( strong_counter_ == 0)
				{
					value_.destroy();
					if ( weak_counter_ == 0 )
					{
						deallocate();
					}
					return 0;
				}
				return strong_counter_;
			}
			
			std::size_t decrement_weak()
			{
				const std::scoped_lock _{mutex_};
				--weak_counter_;
				if ( weak_counter_ == 0 and strong_counter_ == 0 )
				{
					deallocate();
					return 0;
				}
				return weak_counter_;
			}

			virtual void deallocate() = 0;
		};

		template<typename Resource>
		struct allocation : allocation_base
		{
			[[no_unique_address]] Resource r;
			virtual void deallocate() override
			{
				
			}
		};

		allocation_base* alloc_ = nullptr;
		T* object_ = nullptr;
	};

}

#endif