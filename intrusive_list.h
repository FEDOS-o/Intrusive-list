#pragma once

#include <iostream>
#include <iterator>
#include <utility>

namespace intrusive {
struct default_tag;

struct base_list_element {

  template <typename T, typename Tag>
  friend struct list;

private:
  base_list_element* prev{nullptr};
  base_list_element* next{nullptr};

  void unlink() {
    prev = next = nullptr;
  }

  base_list_element* remove() {
    base_list_element* res = next;
    prev->next = next;
    next->prev = prev;
    unlink();
    return res;
  }

  bool is_linked() const {
    return prev != nullptr;
  }

  void tie() {
    prev = next = this;
  }

public:
  base_list_element() = default;

  base_list_element(base_list_element&& other) {
    if (other.prev) {
      other.prev->next = this;
    }
    if (other.next) {
      other.next->prev = this;
    }
    this->next = other.next;
    this->prev = other.prev;
    other.unlink();
  }

  base_list_element(const base_list_element&) = delete;

  ~base_list_element() {
    if (is_linked()) {
      remove();
    }
    unlink();
  }
};

template <typename Tag = default_tag>
struct list_element : base_list_element {
  list_element() = default;

  list_element(const list_element&) = delete;

  list_element(list_element&& other) : base_list_element(other) {}
};

template <typename T, typename Tag = default_tag>
struct list {

  template <typename S>
  struct base_iterator {
    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = S;
    using pointer = S*;   // or also value_type*
    using reference = S&; // or also value_type&

    using base = std::conditional_t<std::is_const_v<S>, base_list_element const,
                                    base_list_element>;
    using tag = std::conditional_t<std::is_const_v<S>, list_element<Tag> const,
                                   list_element<Tag>>;

    base* ptr;

    operator base_iterator<S const>() const {
      return base_iterator<S const>(ptr);
    }

    base_iterator(base* p) : ptr(p) {}

    base_iterator() = default;

    base_iterator(const base_iterator& other) : ptr(other.ptr) {}

    reference operator*() const {
      return static_cast<reference>(*tag_cast(ptr));
    };

    pointer operator->() const {
      return static_cast<pointer>(tag_cast(ptr));
    }

    tag* tag_cast(base* base) const {
      return static_cast<tag*>(base);
    }

    base_iterator& operator++() {
      ptr = ptr->next;
      return *this;
    }

    base_iterator operator++(int) {
      base_iterator res = *this;
      operator++();
      return res;
    }

    base_iterator& operator--() {
      ptr = ptr->prev;
      return *this;
    }

    base_iterator operator--(int) {
      base_iterator res = *this;
      operator--();
      return res;
    }

    friend bool operator==(const base_iterator<S>& a,
                           const base_iterator<S>& b) {
      return a.ptr == b.ptr;
    };

    friend bool operator!=(const base_iterator<S>& a,
                           const base_iterator<S>& b) {
      return a.ptr != b.ptr;
    };
  };

  using iterator = base_iterator<T>;
  using const_iterator = base_iterator<T const>;

private:
  list_element<Tag> sential;

  void untie_all() {
    while (!empty()) {
      pop_front();
    }
  }

  void move(list&& other) {
    if (!other.empty()) {
      auto last = other.sential.prev;
      other.sential.remove();
      insert_after(last, &sential);
      other.sential.tie();
    } else {
      sential.tie();
    }
  }

  iterator insert_after(base_list_element* first, base_list_element* second) {
    base_list_element* third = first->next;
    if (second->is_linked()) {
      second->remove();
    }
    first->next = second;
    second->prev = first;
    second->next = third;
    third->prev = second;
    return second;
  }

public:
  list() {
    sential.tie();
  }

  list(const list&) = delete;

  list(list&& other) {
    move(std::move(other));
  }

  list& operator=(const list&) = delete;

  list& operator=(list&& other) {
    untie_all();
    move(std::move(other));
    return *this;
  }

  ~list() {
    untie_all();
  }

  bool empty() {
    return begin() == end();
  }

  T& front() {
    return *static_cast<T*>(static_cast<list_element<Tag>*>(sential.next));
  }

  T const& front() const {
    return *static_cast<T const*>(
        static_cast<list_element<Tag>*>(sential.next));
  }

  iterator begin() {
    return sential.next;
  }

  const_iterator begin() const {
    return sential.next;
  }

  T& back() {
    return *static_cast<T*>(static_cast<list_element<Tag>*>(sential.prev));
  }

  T const& back() const {
    return *static_cast<T*>(static_cast<list_element<Tag>*>(sential.prev));
  }

  iterator end() {
    return &sential;
  }

  const_iterator end() const {
    return &sential;
  }

  void push_back(T& object) {
    insert(end(), object);
  }

  void push_front(T& object) {
    insert(begin(), object);
  }

  void pop_back() {
    erase(--end());
  }

  void pop_front() {
    erase(begin());
  }

  iterator erase(iterator element) {
    return element.ptr->remove();
  }

  iterator insert(iterator pos, T& element) {
    base_list_element* third = pos.ptr;
    base_list_element* second = static_cast<list_element<Tag>*>(&element);
    if (second == third || second == third->prev) {
      return second;
    }
    insert_after(third->prev, second);
    return second;
  }

  void splice(const_iterator position, list&, const_iterator begin,
              const_iterator end) {
    if (begin == end) {
      return;
    }
    auto pos = const_cast<base_list_element*>(position.ptr);
    auto first = const_cast<base_list_element*>(begin.ptr);
    auto second = const_cast<base_list_element*>(end.ptr);
    auto last = second->prev;
    first->prev->next = second;
    second->prev = first->prev;
    pos->prev->next = first;
    first->prev = pos->prev;
    last->next = pos;
    pos->prev = last;
  }
};
} // namespace intrusive