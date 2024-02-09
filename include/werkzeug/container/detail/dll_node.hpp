#ifndef WERKZEUG_GUARD_DETAIL_CONTAINER_DLL_NODE_HPP
#define WERKZEUG_GUARD_DETAIL_CONTAINER_DLL_NODE_HPP

#include <utility>
#include <iostream>

namespace werkzeug::detail
{
	template<typename T>
	concept ll_node = requires ( T t )
	{
		{ t.next } -> std::same_as<T*&>;
	};

	template<typename T>
	concept dll_node = ll_node<T> && requires ( T t )
	{
		{ t.prev } -> std::same_as<T*&>;
	};

	template<typename T>
	struct DLL_Node
	{
		DLL_Node* next = nullptr;
		DLL_Node* prev = nullptr;
		T value;

		template<typename ... Args>
		DLL_Node( DLL_Node* next_, DLL_Node* prev_, Args&& ... args ) noexcept( std::is_nothrow_constructible_v<T,Args...> )
			: next(next_), prev(prev_), value{ std::forward<Args>(args) ... }
		{}
	};

	template<typename Node>
	void print_node( std::ostream& os, Node* const node )
	{
		os << "Node: " << static_cast<void*>(node) << " prev: " << static_cast<void*>(node->prev) << " next: " << static_cast<void*>(node->next);
	}

	template<dll_node Node>
	Node* delink( Node* const node )
	{
		if ( node->prev != nullptr )
		{
			node->prev->next = node->next;
		}
		if ( node->next != nullptr )
		{
			node->next->prev = node->prev;
		}
		node->next = nullptr;
		node->prev = nullptr;

		return node;
	}

	template<ll_node Node>
	Node* enlink_at( Node* const new_node, Node* const link_point )
	{
		new_node->next = link_point->next;
		link_point->next = new_node;
		return new_node;
	}

	template<dll_node Node>
	Node* enlink_at( Node* const new_node, Node* const link_point )
	{
		const auto prev = link_point->prev;

		if ( prev )
		{
			prev->next = new_node;
		}
		link_point->prev = new_node;

		new_node->prev = prev;
		new_node->next = link_point;

		return new_node;
	}
}

#endif