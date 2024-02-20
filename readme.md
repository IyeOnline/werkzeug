# `werkzeug::`

This is a collection of tools/utilities I wrote over time. Most parts are independent of eachother, some rely on eachother. 

It is sort of a general utilities library, but you are unlikely go use every part of it.

"Werkzeug" means "tool" in german, hence the name.

### Who is this for?

Primarily me and anybody who is interested. I wrote these things out of curiosity/because they seemed interesting.

Maybe you can take something from it.

### How does it work?

While its technically a lot of seperate parts, there are common utilities that are used across many different files.

The library currently provides a simple `CMake` script allowing its in-tree usage, but no installation utilities.

## Interesting things in here:

* [Expected](#Expected): A slightly improved alternative to `std::expected`
* [inheritance_variant](#inheritance_variant): A variant-like sum type 
* [overload](#overload): A tool to easily create overloads from multiple callables
* [aggregate_ordering](#aggregate_ordering): Utility to ad-hoc create a complex ordering relation, e.g. for use in sorting algorithms
* [type_pack, value_pack](#parameter_pack): Wrappers to enable easy handling of parameter packs
* [type_traits](#type_traits): A large wrapper for many type traits
* [Callable_Wrapper](#Callable_Wrapper): A wrapper to allow passing C++ callables to C-APIs
* [explicit_](#explicit_): A wrapper that disallows implicit conversions
* [select_overload](#select_overload)': Utility to select a specific overload form an overload set 
* [try_constexpr_invoke](#try_constexpr_invoke): Utility to potentially evaluate a function at compile time
* [State_Machine](#State_Machine): Utility to create simple state machines
* [Nd array types](#ND_Array): Nd linearly indexed arrays/tables (similar to C++23 `std::mdarray`). Multiple versions with different axis settings, as well as interpolating versions exist.

## Overview:

The `container/` and `memory/` parts are mainly rewrites of existing (standard library) tools for exploration/technical learning purposes. The things in `simple_vector_implementation/` are iterative, simpler versions of `vector`. They mostly exist for learners to look at with less "feature related clutter". A complete and advanced vector implementation is in `container/dynamic_array.hpp`.

Note that not everything in here is of similar implementation quality/completeness. Some things are as complete as can be with correct `noexcept` specifiers, others may lack all basic functionality.

This is an overview of all the seperate files and their contents:

For additional examples you can look at the `tests/` (if they exist...)

## Callable_Wrapper


This exists to easiy create wrappers allowing using a generic C++ callable with a C language API that accepts a `void*` for any user data.

This is mostly useful when wanting to use established C libraries in C++ code. A prime example would be GNU scientific libraries.

#### Example:

```cpp
// some C API.In this example it just invokes our function
double c_api_mid( double a, double b, double(*f)(double a, void* ud, double b), void* ud )
{
	return f(a,ud,b);
}

// Your C++ callable. Capture only to make it non-trivial to convert to a function pointer.
constexpr auto callable = [&]( double a, double b )
{
	return a+b;
};

// create the callable wrapper
auto wrapper = werkzeug::make_wrapper<double(double,double,user_data)>( callable );
// calling the C API gives us the exact same value as calling
REQUIRE( callable(0,1) == c_api_tail( 0, 1, wrapper, &wrapper ) );
```

## Expected


This is an extended version of `std::expected`. Notably it allows you to directly specifc multiple error types (instead of having to manually interact with a `variant`).

Its monadic interface automatically transforms values and transfers errors along.

#### Example:
```cpp
enum class err_a { a1 }; // some error codes for demo purposes. Could also be any other type.

enum class err_b { b1 };

Expected<int,err_a> expected_a = 0; // expected now contains an int
Expected<int,err_a> expected_b = err_a::a1; // expected now contains an err_a

{ // using monadic interface transforming the result
	constexpr auto transform = []( int i ) -> long
	{
		return i;
	}

	Expected<long,err_a> res_a = expected_a.and_then( transform ); // res_a now contains the long 0

	Expected<long,err_a> res_b = expected_b.and_then( transform ); // res_a now contains the err_a
}

{
	constexpr auto transform_to_expected( int i ) -> Expected<long,err_b> 
	{
		if ( i == 0 )
		{
			return i;
		}
		else
		{
			return err_b::b1;
		}
	}

	Expected<long,err_a,err_b> res_a = expected_a.and_then( transform_to_expected ); // res_a now contains the long 0

	expected_a.emplace( 1 ); // changes the value in `expected_a` to 1
	Expected<long,err_a,err_b> res_a2 = expected_a.and_then( transform_to_expected ); // res_a2 now contains err_b::b1

	Expected<long,err_a,err_b> res_b = expected_b.and_then( transform_to_expected ); // res_b now contains err_a::a1
}
```

## explicit_


A helper type that does not allow implicit conversions

## inheritance_variant


This is a variant-like sum type that only allows storage of a compile time known list of types derived from a common `Base`. It provieds access to a base pointer, allowing for slightly cleaner usage without having to go through `std::visit`.

It does *not* require a virtual destructor. The objects are stored within the variant and destroyed according to their index.

The provided API is largely identical to that of `std::variant`

#### Example:

```cpp
enum class type
{
	base, derived
};


struct Base
{
	virtual type type_id() const
	{
		return type::base;
	}

	// no virtual destructor required for the usage
};

struct Derived : Base
{
	virtual type type_id() const override
	{
		return type::derived;
	}
};

using variant_t = werkzeug::inheritance_variant<Base, Derived>;

variant_t var{ Base{} };
REQUIRE( var->type_id() == type::base );

var.emplace<Derived>();
REQUIRE( var->type_id() == type::derived );
```

## overload


This is an extension upon the classic pattern

```cpp
template<typename...Ts>
struct overload : Ts...
{
	using Ts::operator() ...;
};
```

Notably this overload also takes both member function and member data pointers, allowing a usage similar to `std::invoke`, but with less overhead:

#### Example:

```cpp
struct A
{
	int f();
};

struct B
{
	int i = 0;
};

constexpr overload my_overload = {
	&A::f,
	&B::i,
}

int a = my_overload(A{}); // invokes A::f
int b = my_overload(B{}); // accesses and returns B::i
```

If the provided projectors do not have a common return type, the return type of `overload::operator()` will be `void` and potential results will be discarded.

## select_overload

This is a helper to select a selecfic signature from an overload set.

#### Example:

```cpp
int f( int );
void f( double );

constexpr auto f_int = select_overload<int>( &f );
```


## parameter_pack


This header contains utilities for dealing with parameter packs. For this it primarily uses `type_pack<Ts...>` and `value_pack<auto...>`.

Both provide the same functionalities:

List of provided utilities:

* `type_pack_c<T>`: concept to check whether `T` is an instantiation of `type_pack`
* `value_pack_c<T>`: concept to check whether `T` is an instantiation of `value_pack`
* `pack::size`: static member containing the size of the pack
* `pack::contains<X>()`: static member returning  whether `pack` contains `X` (type or value)
* `pack::index_of<X>()`: static member returning the index of `X` in `pack`
* `type_pack::type_at<I>`: static member typedef, giving the type at index `I`
* `value_pack::value_at<I>`: static data member, giving the value at index `I`
* `type_at_index<I,type_pack>`: freestanding metafunction get thet type at `I`. Can also be used as `type_at_index<I,Ts...>`.
* `value_at_index<I,valze_pack>`: freestanding metafunction to get the value at index `I`. Can also be used as `value_at_index<I,Vs...>`
* `all_unique<Ts...>`, `all_unique<type_pack>`, `all_unique<value_pack>`: freestanding metafunction to querry if all elements in a pack are unique.
* `pack_inclues<pack1,pack2>`: freestanding metafunction to check whether `pack1` fully includes `pack2`
* `append<pack1,X>`: freestanding metafunction that appends `X` to `pack1`. Can also be used as `append<pack1,pack2>` to append the entire pack
* `append_if_unique<pack1,X>`: freestanding metafunction that appends `X` to `pack1`, if `pack1` does not already contain `X`
* `static_for<pack>( Callable&& callable )`: function that invokes `Callable` for every type or value in `pack`, passing it as a distinct type, so it can be used as a constant expression


## State_Machine

This provides a utility to create a rudimentary state machine from an enum and functions.

#### Example:

```cpp
// an enum for our states
enum class states : unsigned char
{
    A, B
};


// the machine itself
struct Flip_Flop_Machine
    : werkzeug::State_Machine<states,2,Flip_Flop_Machine>
{
	// some custom members
    int i = 0;

    using Base = werkzeug::State_Machine<states,2,Flip_Flop_Machine>;

	// just a default ctor
    constexpr Flip_Flop_Machine()
        : Base{ states::A }
    { };

	// declare the state function template.
    template<states>
    states func_for() = delete; //delete its default. That means that the compiler catches if we have not implemented all of them
};

// implement the state function for states::A
template<>
states Flip_Flop_Machine::func_for<states::A>()
{
    std::cout << "FLIP\n";
    ++i;
    return states::B; // returns the next state we want to go to.
}

// implement the state function for states::B
template<>
states Flip_Flop_Machine::func_for<states::B>()
{
    std::cout << "flop\n";
    ++i;
    return states::A; // returns the next state we want to go to.
}

int main()
{
	Flip_Flop_Machine m;

	m.execute_active() // prints "FLIP"
	m.execute_active() // prints "flop"

	REQUIRE( m.i == 2 );
}
```

## type_traits

This header provides a generic type trait `type_traits<T>`, that contains almost all information about a type.

For every operation, there is named member type trait, that tells you if the operation is possible, trivial or nothrow.

#### Examples

* `traits<T>::default_construct::possible`
* `traits<T>::swap::nothrow`
* `traits<T>::copy_construct::trivial`
* `traits<T>::template invocable<Args...>::possible`
* `traits<T>::subscript::nothrow`

All member traits follow the pattern of providing `possible`, `trivial` and `nothrow` to check the respective property.

`type_constant`
---

Pretty much `std::integral_constant` with a better name.

Provides a user defined literal `_tc` that automatically creates a `size_t` or `long double` `type_constant`:

```cpp
constexpr auto A = 0_tc; // type_constant<0,size_t>{}
constexpr auto B = 0.0_tc; // type_constant<0.0l,long double>{}
```

`type_erased_object`
---

An exploration into creating a type erased wrapper that provides specifications for an interface.

```cpp
struct S
{ 
	int i;
};

std::ostream& operator<<( std::ostream& os, const S& s )
{
	return os << s.i;
}

std::stream& operator>>( std::istream& is, const S& s )
{
	return is >> s.i;
}

Shared_Object obj{ S, Generic_Stream_Extraction<S>{}, Generic_Stream_Insertion<S>{} }; // create an erased object with its set of supported operations

std::string_stream ss("42");
obj.operation<Stream_Extraction>()(ss); // `Stream_Extraction` is a predefined interface that is implemented by the `Generic_Stream_Extraction<T>`

obj.operation<Stream_Insertion>()(std::cout); // prints 42


if ( auto op_ptr = obj.try_operation<Unsupported_Operation>() )
{
	// never happens, because no `Unsupported_Operation` was provided.
}
if ( auto op_ptr = obj.try_operation<Stream_Insertion>() )
{
	(*ptr)( std::cout ); // prints 42 again
}
```

## try_constexpr_invoke

This utility will try to invoke a callable at compile time. If that is possible, it provides the value. Otherwise it does not.


```cpp
constexpr int yes()
{ return 0; }

constexpr auto result = try_constexpr_invoke( yes );

static_assert( result.has_value() );
static_assert( result.value() == 0 );
```

```cpp
int no()
{ return 0; }

constexpr auto result = try_constexpr_invoke( no );

static_assert( not result.has_value() );
```

## aggregate_ordering


This is a fancy little utility that easily lets you create custom ordering ad-hoc

```cpp
// some data structure
struct S
{
	int i;
	int j;
	double d;
};

constexpr auto ordering = aggregate_ordering{  // sort by
	by{ &S::i, less{} }, 	// ascending i first
	by{ &S::d, less{} },	// ascending d next
	by{ &S::j, greater{} } 	// descending j last
};

S arr[] = {
	{ .i = 0, .j= 1, .d=2.0 },
	{ .i = 2, .j= 2, .d=3.0 },
	{ .i = 2, .j= 2, .d=2.0 },
	{ .i = 0, .j= 3, .d=2.0 },
	{ .i = 3, .j= 4, .d=2.0 }
};

std::ranges::stable_sort( arr, ordering );

REQUIRE( arr[0].i == 3 );

REQUIRE( arr[1].i == 2 );
REQUIRE( arr[1].d == 3.0 );

REQUIRE( arr[2].i == 2 );
REQUIRE( arr[2].d == 2.0 );

REQUIRE( arr[3].j == 1 );
```

## Nd_Array

These array types are similar to `std::mdarray`, but are C++17 compatible. Their primary purpose is the implementation of the `Interpolating_` table types.

* `Array_ND` is simply an ND array, supporting indexing via `operator()`
* `Bounded_Array_Nd` also has lower and upper bounds attachted to each axis
* `Axis_Tick_Array_Nd` instead has ticks for every dimension saved, meaning they can be non-linearly/logarythmically spaced.
* `Basic_Interpolating_Array_Nd` is a `Bounded_Array_Nd` that can interpolate at an arbitrary position in the hypercube, or do simple extrapolation outside of it.
* `Advanced_Interpolating_Array_Nd` is an `Axist_Tick_Array_Nd` that supports interpolation.

Notable features:

* All of them also provide a member function `make_view()` that creats a `std::span` like object to *view* the array/table. These objects support the exact same functionalities as the owning array itself.
* These types can be saved to an `std::ostream` and constructed from an `std::ostream`
* Interpolation works in N dimensions, either as lin, log or linlog interpolation.

The internal data structre is as follows:
```
Array_Nd:
[ axis lengths ] [ data ]

Bounded_Array_Nd
[ Array_Nd ] [ axis_lower_bounds ] [ axis_upper_bounds ] [ axis_is_log ]

Basic_Interpolating_Table
[ Bounded_Array_Nd ]

Axis_Tick_Array_Nd
[ Array_Nd ] [ axis_ticks ]

[ Advanced_Interpolating_Table ]
[ Axis_Tick_Array_Nd ]
```

## Achknowledgements

* The tests use [doctest](https://github.com/doctest/doctest) under the MIT license