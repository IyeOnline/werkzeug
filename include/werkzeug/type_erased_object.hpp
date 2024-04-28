#ifndef WERKKEUG_GUARD_TYPE_ERASED_OBJECT_HPP
#define WERKKEUG_GUARD_TYPE_ERASED_OBJECT_HPP

#include "werkzeug/parameter_pack.hpp"
#include <compare>
#include <concepts>
#include <iostream>
#include <memory>
#include <type_traits>
#include <typeinfo>
#include <utility>


namespace werkzeug
{
	struct Operation
	{
		virtual void* obj_ptr() noexcept = 0;
		// {
		// 	return nullptr;
		// }
		virtual const void* obj_ptr() const = 0;
		// {
		// 	return nullptr;
		// }

		template<typename T>
		T& object_as()
		{
			auto* const ptr = obj_ptr();
			assert( ptr != nullptr );
			return *static_cast<T*>( ptr );
		}

		template<typename T>
		const T& object_as() const
		{
			const auto* ptr = obj_ptr();
			assert( ptr != nullptr );
			return *static_cast<const std::remove_const_t<T>*>( ptr );
		}

		virtual ~Operation() = default;
	};

	struct Stream_Insertion : virtual Operation
	{
		void operator()( std::ostream& os  ) const
		{
			return insert_into( os );
		}
		virtual void insert_into( std::ostream& os ) const = 0;
	};

	template<typename T>
	struct Generic_Stream_Insertion : virtual Stream_Insertion
	{
		virtual void insert_into( std::ostream& os ) const override
		{
			const auto& obj = object_as<T>();
			os << obj << '\n';
		}
	};


	struct Stream_Extraction : virtual Operation
	{
		void operator()( std::istream& is  )
		{
			return extract_from( is );
		}
		virtual void extract_from( std::istream& is ) = 0;
	};

	template<typename T>
	struct Generic_Stream_Extraction : virtual Stream_Extraction
	{
		virtual void extract_from( std::istream& is ) override
		{
			const auto& obj = object_as<T>();
			is >> obj;
		}
	};

	/**
	 * @brief Type erased 'Object' that can support "Operations", which must be provided on construction
	 */
	class Shared_Object
	{
		struct Holder_Base : virtual public Operation
		{
			virtual std::partial_ordering order( const Holder_Base& ) const noexcept = 0;
			virtual const std::type_info& type_id() const noexcept = 0;
		};

		template<typename T>
		struct Holder : Holder_Base
		{
			T value_;

			bool operator==( const Holder& ) const = default;
			auto operator<=>( const Holder& ) const = default;

			virtual void* obj_ptr() noexcept override
			{ return &value_; }

			virtual const void* obj_ptr() const noexcept override
			{ return &value_; }

			virtual std::partial_ordering order( const Holder_Base& other ) const noexcept override
			{
				if constexpr ( std::three_way_comparable<T> )
				{
					if ( type_id() != other.type_id() )
					{
						return std::partial_ordering::unordered;
					}
					else
					{
						return value_ <=> *static_cast<const T*>( other.obj_ptr() );
					}
				}
				else
				{
					return std::partial_ordering::unordered;
				}
			}

			virtual const std::type_info& type_id() const noexcept override
			{ return typeid(T); }
		};

		template<typename T, typename ... Operations>
		struct Combined_Holder final 
			: Holder<T>, virtual Operations ...
		{
			template<typename ... OperationsArg>
			Combined_Holder( T&& obj, OperationsArg&& ... operations )
				: Operations{ std::forward<OperationsArg>(operations) } ...
				, Holder<T>{ std::move(obj) }
			{ }

			template<typename ... OperationsArg>
			Combined_Holder( const T& obj, OperationsArg&& ... operations )
				: Operations{ std::forward<OperationsArg>(operations) } ...
				, Holder<T>{ obj }
			{ }
		};

		std::shared_ptr<Holder_Base> self_{};

	public:
		Shared_Object() noexcept = default;

		template<typename T, std::derived_from<Operation> ... Operations> 
		Shared_Object( T&& obj, Operations&& ... ops )
			: self_{ std::make_shared<Combined_Holder<std::remove_cvref_t<T>,std::remove_cvref_t<Operations>...>>( std::forward<T>(obj), std::forward<Operations>(ops) ... ) }
		{ }


		template<typename T, std::derived_from<Operation> ... Operations> 
		T& assign( T&& obj, Operations&& ... ops )
		{
			self_ = std::make_shared<Combined_Holder<std::remove_cvref_t<T>,std::remove_cvref_t<Operations>...>>( std::forward<T>(obj), std::forward<Operations>(ops) ... );

			return as<T>();
		}


		// gets a specific operation
		template<std::derived_from<Operation> Op>
		[[nodiscard]] Op* try_operation() const noexcept
		{
			return dynamic_cast<Op*>( self_.get() );
		}

		// gets a specific operation
		template<std::derived_from<Operation> Op>
		[[nodiscard]] Op* try_operation() noexcept
		{
			return dynamic_cast<Op*>( self_.get() );
		}

		template<std::derived_from<Operation> Op>
		[[nodiscard]] auto& operation() const noexcept
		{
			const auto* const ptr = try_operation<Op>();
			assert( ptr );
			return *ptr;
		}

		template<std::derived_from<Operation> Op>
		[[nodiscard]] auto& operation() noexcept
		{
			auto* const ptr = try_operation<Op>();
			assert( ptr );
			return *ptr;
		}

		template<typename F, typename ... Ts>
		bool try_invoke_if_contains( F&& functor, type_pack<Ts...> )
		{
			constexpr static auto applicator = []<typename T>( const auto& f, auto* that )
			{
				auto ptr = that->template as<T*>();
				if ( ptr )
				{
					f(*ptr);
					return true;
				}
				return false;
			};

			return ( applicator.template operator()<Ts>(functor, this ) || ... );
		}

		template<typename F, typename ... Ts>
		bool try_invoke_if_contains( F&& functor, type_pack<Ts...> ) const
		{
			constexpr static auto applicator = []<typename T>( const auto& f, auto* that )
			{
				auto ptr = that->template as<T*>();
				if ( ptr )
				{
					f(*ptr);
					return true;
				}
				return false;
			};

			return ( applicator.template operator()<Ts>(functor, this ) || ... );
		}


		[[nodiscard]] bool is_empty() const noexcept
		{ return !self_; }

		void destroy() noexcept
		{ self_.reset(); }


		template<typename T>
		[[nodiscard]] bool is() const noexcept
		{ return not is_empty() and type_id() == typeid(T); }
		
		template<typename T>
			requires std::is_pointer_v<T>
		auto* as() noexcept
		{
			using U = std::remove_const_t<std::remove_pointer_t<T>>;
			auto ptr = dynamic_cast<Holder<U>*>( self_.get() );
			if ( ptr )
			{
				return ptr->value_;
			}
			else
			{
				return nullptr;
			}
		}

		template<typename T>
			requires std::is_pointer_v<T>
		const auto* as() const noexcept
		{ 
			using U = std::remove_const_t<std::remove_pointer_t<T>>;
			auto ptr = dynamic_cast<Holder<U>*>( self_.get() );
			if ( ptr )
			{
				return ptr->value_;
			}
			else
			{
				return nullptr;
			}
		}

		template<typename T>
			requires ( not std::is_pointer_v<T> )
		auto& as()
		{ 
			auto ptr = as<T*>();
			assert( ptr );
			return *ptr;
		}

		template<typename T>
			requires ( not std::is_pointer_v<T> )
		const auto& as() const
		{ 
			auto ptr = as<T*>();
			assert( ptr );
			return *ptr;
		}


		[[nodiscard]] const std::type_info& type_id() const noexcept
		{ return self_->type_id(); }

		operator const std::type_info&() const noexcept
		{ return type_id(); }

		[[nodiscard]] std::partial_ordering operator<=>( const Shared_Object& other ) const
		{ 
			return self_->order( *other.self_.get() );
		}

		[[nodiscard]] bool operator==( const Shared_Object& other ) const = default;
	};
}

#endif