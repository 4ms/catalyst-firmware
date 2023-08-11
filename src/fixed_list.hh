#pragma once
#include <array>
#include <cstdint>

namespace Catalyst2
{

template<typename T, unsigned max_size>
class FixedFwList {
public:
	FixedFwList(T val_0, T val_1)
	{
		head = &node[0];
		tail = &node[1];
		head->data = val_0;
		tail->data = val_1;
		head->next = tail;
		tail->next = head;
		count = 2;
		update_fast_access();
	}

	unsigned size() const
	{
		return count;
	}

	T read(unsigned index) const
	{
		return fast_access[index % count];
	}

	void replace(unsigned index, const T &d)
	{
		index %= count;

		fast_access[index] = d;

		Node *in = get_node(index);

		in->data = d;
	}

	bool insert(const T &d)
	{
		// insert after previous node.
		Node *empty = allocate();
		if (empty == nullptr)
			return false;

		count += 1;

		if (previous == tail)
			tail = empty;

		empty->data = d;
		empty->next = previous->next;
		previous->next = empty;

		previous = empty;

		update_fast_access();

		return true;
	}

	bool insert(unsigned index, const T &d)
	{
		Node *empty = allocate();

		if (empty == nullptr)
			return false;

		count += 1;

		previous = empty;

		Node *before = get_node(index);

		empty->next = before->next;
		before->next = empty;
		empty->data = d;

		if (before == tail)
			tail = empty;

		update_fast_access();
		return true;
	}

	bool erase(unsigned index)
	{
		if (count == 2)
			return false;

		count -= 1;

		if (index == 0) {
			// erasing head.
			tail->next = head->next;
			head->next = nullptr;
			head = tail->next;
		} else {
			Node *before = get_node(index - 1);

			if (before->next == tail) {
				// erasing tail.
				tail = before;
			}

			Node *temp = before->next->next;
			before->next->next = nullptr;
			before->next = temp;
		}

		update_fast_access();
		return true;
	}

private:
	struct Node {
		T data;
		Node *next = nullptr;
	};
	Node *allocate()
	{
		Node *out = nullptr;

		for (auto &n : node) {
			if (n.next == nullptr) {
				out = &n;
				break;
			}
		}
		return out;
	}
	Node *get_node(unsigned index)
	{
		Node *out = head;

		while (index--) {
			out = out->next;
		}
		return out;
	}
	void update_fast_access()
	{
		Node *n = head;

		for (unsigned i = 0; i < count; i++) {
			fast_access[i] = n->data;
			n = n->next;
		}
	}

	std::array<Node, max_size> node;
	std::array<T, max_size> fast_access;
	Node *head;
	Node *tail;
	Node *previous;
	unsigned count;
};
} // namespace Catalyst2