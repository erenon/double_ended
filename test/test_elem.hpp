//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Benedek Thaler 2015-2016. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://erenon.hu/double_ended for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#include <boost/utility/compare_pointees.hpp>

struct test_exception {};

struct test_elem_throw
{
  static int throw_on_ctor_after /*= -1*/;
  static int throw_on_copy_after /*= -1*/;
  static int throw_on_move_after /*= -1*/;

  static void on_ctor_after(int x) { throw_on_ctor_after = x; }
  static void on_copy_after(int x) { throw_on_copy_after = x; }
  static void on_move_after(int x) { throw_on_move_after = x; }

  static void do_not_throw()
  {
    throw_on_ctor_after = -1;
    throw_on_copy_after = -1;
    throw_on_move_after = -1;
  }

  static void in_constructor() { maybe_throw(throw_on_ctor_after); }
  static void in_copy() { maybe_throw(throw_on_copy_after); }
  static void in_move() { maybe_throw(throw_on_move_after); }

private:
  static void maybe_throw(int& counter)
  {
    if (counter > 0)
    {
      --counter;
      if (counter == 0)
      {
        --counter;
        throw test_exception();
      }
    }
  }
};

int test_elem_throw::throw_on_ctor_after = -1;
int test_elem_throw::throw_on_copy_after = -1;
int test_elem_throw::throw_on_move_after = -1;

struct test_elem_base
{
  test_elem_base()
  {
    test_elem_throw::in_constructor();
    _index = new int(0);
    ++_live_count;
  }

  test_elem_base(int index)
  {
    test_elem_throw::in_constructor();
    _index = new int(index);
    ++_live_count;
  }

  explicit test_elem_base(const test_elem_base& rhs)
  {
    test_elem_throw::in_copy();
    _index = new int(*rhs._index);
    ++_live_count;
  }

  test_elem_base(test_elem_base&& rhs)
  {
    test_elem_throw::in_move();
    _index = rhs._index;
    rhs._index = nullptr;
    ++_live_count;
  }

  void operator=(const test_elem_base& rhs)
  {
    test_elem_throw::in_copy();
    if (_index) { delete _index; }
    _index = new int(*rhs._index);
  }

  void operator=(test_elem_base&& rhs)
  {
    test_elem_throw::in_move();
    if (_index) { delete _index; }
    _index = rhs._index;
    rhs._index = nullptr;
  }

  ~test_elem_base()
  {
    if (_index) { delete _index; }
    --_live_count;
  }

  friend bool operator==(const test_elem_base& a, const test_elem_base& b)
  {
    return a._index && b._index && *(a._index) == *(b._index);
  }

  friend bool operator<(const test_elem_base& a, const test_elem_base& b)
  {
    return boost::less_pointees(a._index, b._index);
  }

  friend std::ostream& operator<<(std::ostream& out, const test_elem_base& elem)
  {
    if (elem._index) { out << *elem._index; }
    else { out << "null"; }
    return out;
  }

  template <typename Archive>
  void serialize(Archive& ar, unsigned /* version */)
  {
    ar & *_index;
  }

  static bool no_living_elem()
  {
    return _live_count == 0;
  }

private:
  int* _index;

  static int _live_count;
};

int test_elem_base::_live_count = 0;

struct regular_elem : test_elem_base
{
  regular_elem() = default;

  regular_elem(int index) : test_elem_base(index) {}

  regular_elem(const regular_elem& rhs)
    :test_elem_base(rhs)
  {}

  regular_elem(regular_elem&& rhs)
    :test_elem_base(std::move(rhs))
  {}

  void operator=(const regular_elem& rhs)
  {
    static_cast<test_elem_base&>(*this) = rhs;
  }

  void operator=(regular_elem&& rhs)
  {
    static_cast<test_elem_base&>(*this) = std::move(rhs);
  }
};

struct noex_move : test_elem_base
{
  noex_move() = default;

  noex_move(int index) : test_elem_base(index) {}

  noex_move(const noex_move& rhs)
    :test_elem_base(rhs)
  {}

  noex_move(noex_move&& rhs) noexcept
    :test_elem_base(std::move(rhs))
  {}

  void operator=(const noex_move& rhs)
  {
    static_cast<test_elem_base&>(*this) = rhs;
  }

  void operator=(noex_move&& rhs) noexcept
  {
    static_cast<test_elem_base&>(*this) = std::move(rhs);
  }
};

struct noex_copy : test_elem_base
{
  noex_copy() = default;

  noex_copy(int index) : test_elem_base(index) {}

  noex_copy(const noex_copy& rhs) noexcept
    :test_elem_base(rhs)
  {}

  noex_copy(noex_copy&& rhs)
    :test_elem_base(std::move(rhs))
  {}

  void operator=(const noex_copy& rhs) noexcept
  {
    static_cast<test_elem_base&>(*this) = rhs;
  }

  void operator=(noex_copy&& rhs)
  {
    static_cast<test_elem_base&>(*this) = std::move(rhs);
  }
};

struct only_movable : test_elem_base
{
  only_movable() = default;

  only_movable(int index) : test_elem_base(index) {}

  only_movable(only_movable&& rhs)
    :test_elem_base(std::move(rhs))
  {}

  void operator=(only_movable&& rhs)
  {
    static_cast<test_elem_base&>(*this) = std::move(rhs);
  }
};

struct no_default_ctor : test_elem_base
{
  no_default_ctor(int index) : test_elem_base(index) {}

  no_default_ctor(const no_default_ctor& rhs)
    :test_elem_base(rhs)
  {}

  no_default_ctor(no_default_ctor&& rhs)
    :test_elem_base(std::move(rhs))
  {}

  void operator=(const no_default_ctor& rhs)
  {
    static_cast<test_elem_base&>(*this) = rhs;
  }

  void operator=(no_default_ctor&& rhs)
  {
    static_cast<test_elem_base&>(*this) = std::move(rhs);
  }
};
