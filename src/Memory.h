#pragma once
#include "Misc.h"
#include <stack>
#include <cassert>
#include <span>
template <class PType, class T>
class TempPtr {
private:
	PType* parent = nullptr;
	usize pos;
	usize p_size{ 0 };
public:
	class Iterator {
	private:
		TempPtr* owner;
		usize index;
	public:
		using iterator_category = std::forward_iterator_tag;
		using value_type = T;
		using difference_type = std::ptrdiff_t;
		using pointer = T*;
		using reference = T&;

		Iterator(TempPtr* owner, usize index) : owner(owner), index(index) {}

		reference operator*() { return (((*(owner->parent))[index])); }
		pointer operator->() { return &(operator*()); }

		Iterator& operator++() { ++index; return *this; }
		Iterator operator++(int) { Iterator tmp = *this; ++index; return tmp; }

		bool operator==(const Iterator& other) const { return index == other.index; }
		bool operator!=(const Iterator& other) const { return !(*this == other); }
	};


	TempPtr(PType* parent, usize pos) : parent(parent), pos(pos) {};
	~TempPtr() {
		parent->popPtr();
	}
	inline auto operator[](usize i) -> T& {
		assert(i < p_size);
		return (*parent)[pos + i];
	};
	inline T& front() {
		return (*parent)[pos];
	}
	inline void emplace_back(const T& val) {
		assert(p_size < parent->max_size());
		parent->push(val);
		++p_size;
	}
	inline T pop_back() {
		assert(p_size > 0);
		--p_size;
		return std::move(parent->pop_back());
	}
	inline void resize(int new_size) {
		p_size= new_size;
	}
	inline usize size() const {
		return p_size;
	}
	inline bool empty() const {
		return (p_size == 0);
	}

	// Range-based for loop support
	Iterator begin()  { return Iterator(this, 0); }
	Iterator end()  { return Iterator(this, p_size); }
};

template <class T, usize Capacity>
class SearchArray {
private:
	std::array<T, Capacity> arr{};
	std::stack<usize> pos_arr;
	usize d_size{ 0 };


public:
	explicit operator T*() const { return &arr; }
	inline auto operator[](usize i) -> T& {
		assert(i < d_size);
		return arr[i];
	};
	inline T pop_back() {
		return std::move(arr[--d_size]);
	}
	inline void push(const T& val) {
		assert(d_size < Capacity);
		arr[d_size] = T();
		arr[d_size++] = std::move(val);
	}
	TempPtr<SearchArray, T> getPtr() {
		pos_arr.push(d_size);
		return { this, d_size };
	}
	void popPtr() {
		if (pos_arr.empty()) {
			d_size = 0;
			return;
		}
		d_size = pos_arr.top();
		pos_arr.pop();
	}
	constexpr usize max_size() { return arr.max_size(); }
};

template <class T>
class StaticVector {
private:
	std::array<T, 256> arr{};
	usize d_size{ 0 };
public:
	explicit operator T*() const { return &arr; }
	inline auto operator[](usize i) -> T& {
		assert(i < d_size);
		return arr[i];
	};
	inline T pop_back() {
		assert(d_size > 0);
		return std::move(arr[--d_size]);
	}
	inline void emplace_back(const T& val) {
		assert(d_size < 256);
		arr[d_size++] = std::move(val);
	}
	inline usize size() const {
		return d_size;
	}
	inline void resize(usize size) {
		d_size = size;
	}
	inline bool empty() const {
		return (d_size == 0);
	}
	inline void clear() {
		d_size = 0;
	}
	constexpr usize max_size() { return 256; }

    inline T& front() {
	    assert(d_size > 0);
	    return arr[0];
    }
    inline T& back() {
	    assert(d_size > 0);
	    return arr[d_size - 1];
    }
	// Range-based for loop support using standard iterators
	auto begin() { return arr.begin(); }
	auto end() { return arr.begin() + d_size; }
	auto begin() const { return arr.begin(); }
	auto end() const { return arr.begin() + d_size; }

	// Get a span view of the data
	auto span() { return std::span<T>(arr.data(), d_size); }
	auto span() const { return std::span<const T>(arr.data(), d_size); }
}; 