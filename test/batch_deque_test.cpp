//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Benedek Thaler 2015-2015. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://erenon.hu/double_ended for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <forward_list>

#define BOOST_DOUBLE_ENDED_TEST
#include <boost/double_ended/batch_deque.hpp>
#undef BOOST_DOUBLE_ENDED_TEST

using namespace boost::double_ended;

#define BOOST_TEST_MODULE batch_deque
#include <boost/test/included/unit_test.hpp>

#include <boost/mpl/list.hpp>
#include <boost/mpl/filter_view.hpp>
#include <boost/mpl/is_sequence.hpp>
#include <boost/mpl/placeholders.hpp>

#include <boost/algorithm/cxx14/equal.hpp>

#include "test_elem.hpp"
#include "input_iterator.hpp"

template <typename T>
using make_batch_deque = batch_deque<T, batch_deque_policy<8>, std::allocator<T>>;

#if 1
typedef boost::mpl::list<
  make_batch_deque<unsigned>,
  make_batch_deque<regular_elem>,
  make_batch_deque<noex_move>,
  make_batch_deque<noex_copy>,
  make_batch_deque<only_movable>,
  make_batch_deque<no_default_ctor>
> all_deques;
#else
typedef boost::mpl::list<
  //make_batch_deque<unsigned>
  //make_batch_deque<regular_elem>
> all_deques;
#endif

template <template<typename> class Predicate>
struct if_value_type
{
  template <typename Container>
  struct apply : Predicate<typename Container::value_type> {};
};

typedef boost::mpl::filter_view<all_deques, if_value_type<std::is_default_constructible>>::type
  t_is_default_constructible;

typedef boost::mpl::filter_view<all_deques, if_value_type<std::is_copy_constructible>>::type
  t_is_copy_constructible;

typedef boost::mpl::filter_view<all_deques, if_value_type<std::is_trivial>>::type
  t_is_trivial;

template <typename Container>
Container get_range(int fbeg, int fend, int bbeg, int bend)
{
  Container c;

  for (int i = fend; i > fbeg ;)
  {
    c.emplace_front(--i);
  }

  for (int i = bbeg; i < bend; ++i)
  {
    c.emplace_back(i);
  }

  return c;
}

template <typename Container>
Container get_range(int count)
{
  Container c;

  for (int i = 1; i <= count; ++i)
  {
    c.emplace_back(i);
  }

  return c;
}

template <typename Container>
Container get_range()
{
  return get_range<Container>(1, 13, 13, 25);
}

template <typename T, typename P, typename A, typename C2>
bool operator==(const batch_deque<T, P, A>& a, const C2& b)
{
  return std::lexicographical_compare(
    a.begin(), a.end(),
    b.begin(), b.end()
  );
}

template <typename Range>
void print_range(std::ostream& out, const Range& range)
{
  out << '[';
  bool first = true;
  for (auto&& elem : range)
  {
    if (first) { first = false; }
    else { out << ','; }

    out << elem;
  }
  out << ']';
}

template <typename T, typename P, typename A>
std::ostream& operator<<(std::ostream& out, const batch_deque<T, P, A>& deque)
{
  print_range(out, deque);
  return out;
}

template <typename C1, typename C2>
void test_equal_range(const C1& a, const C2&b)
{
  bool equals = boost::algorithm::equal(
    a.begin(), a.end(),
    b.begin(), b.end()
  );

  BOOST_TEST(equals);

  if (!equals)
  {
    print_range(std::cerr, a);
    std::cerr << "\n";
    print_range(std::cerr, b);
    std::cerr << "\n";
  }
}

// support initializer_list
template <typename C>
void test_equal_range(const C& a, std::initializer_list<unsigned> il)
{
  typedef typename C::value_type T;
  std::vector<T> b;

  for (auto&& elem : il)
  {
    b.emplace_back(elem);
  }

  test_equal_range(a, b);
}

// END HELPERS

// TODO iterator operations

BOOST_AUTO_TEST_CASE_TEMPLATE(segment_iterator, Deque, t_is_trivial)
{
  typedef typename Deque::value_type T;

  devector<T> expected = get_range<devector<T>>();
  Deque a = get_range<Deque>();
  a.reserve_front(139);
  a.reserve_back(147);

  expected.pop_front();
  expected.pop_front();
  a.pop_front();
  a.pop_front();

  T* p_expected = expected.data();

  auto a_first = a.segment_begin();
  auto a_last = a.segment_end();

  while (a_first != a_last)
  {
    auto size = a_first.data_size();
    bool ok = std::memcmp(a_first.data(), p_expected, size) == 0;
    BOOST_TEST(ok);

    ++a_first;
    p_expected += size;
  }

  BOOST_TEST(p_expected == expected.data() + expected.size());
}

BOOST_AUTO_TEST_CASE_TEMPLATE(consturctor_default, Deque, all_deques)
{
  {
    Deque a;

    BOOST_TEST(a.size() == 0u);
    BOOST_TEST(a.empty());
  }

  {
    typename Deque::allocator_type allocator;
    Deque b(allocator);

    BOOST_TEST(b.size() == 0u);
    BOOST_TEST(b.empty());
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(constructor_n_value, Deque, t_is_default_constructible)
{
  typedef typename Deque::value_type T;

  {
    Deque a(0);
    BOOST_TEST(a.empty());
  }

  {
    Deque b(18);
    BOOST_TEST(b.size() == 18u);

    const T tmp{};

    for (auto&& elem : b)
    {
      BOOST_TEST(elem == tmp);
    }
  }

  {
    Deque b(8);
    BOOST_TEST(b.size() == 8u);

    const T tmp{};

    for (auto&& elem : b)
    {
      BOOST_TEST(elem == tmp);
    }
  }

  if (! std::is_nothrow_constructible<T>::value)
  {
    test_elem_throw::on_ctor_after(10);

    try
    {
      Deque a(12);
      BOOST_TEST(false);
    } catch (const test_exception&) {}

    BOOST_TEST(test_elem_base::no_living_elem());
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(constructor_n_copy, Deque, t_is_copy_constructible)
{
  typedef typename Deque::value_type T;

  {
    const T x(9);
    Deque a(0, x);
    BOOST_TEST(a.empty());
  }

  {
    const T x(9);
    Deque b(18, x);
    BOOST_TEST(b.size() == 18u);

    for (auto&& elem : b)
    {
      BOOST_TEST(elem == x);
    }
  }

  {
    const T x(9);
    Deque b(8, x);
    BOOST_TEST(b.size() == 8u);

    for (auto&& elem : b)
    {
      BOOST_TEST(elem == x);
    }
  }

  if (! std::is_nothrow_copy_constructible<T>::value)
  {
    test_elem_throw::on_copy_after(10);

    try
    {
      const T x(9);
      Deque a(12, x);
      BOOST_TEST(false);
    } catch (const test_exception&) {}

    BOOST_TEST(test_elem_base::no_living_elem());
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(constructor_input_range, Deque, t_is_copy_constructible)
{
  typedef typename Deque::value_type T;

  {
    const devector<T> expected = get_range<devector<T>>(18);
    devector<T> input = expected;

    auto input_begin = make_input_iterator(input, input.begin());
    auto input_end   = make_input_iterator(input, input.end());

    Deque a(input_begin, input_end);
    BOOST_TEST(a == expected, boost::test_tools::per_element());
  }

  { // empty range
    devector<T> input;
    auto input_begin = make_input_iterator(input, input.begin());

    Deque b(input_begin, input_begin);

    BOOST_TEST(b.empty());
  }

  if (! std::is_nothrow_copy_constructible<T>::value)
  {
    devector<T> input = get_range<devector<T>>(18);

    auto input_begin = make_input_iterator(input, input.begin());
    auto input_end   = make_input_iterator(input, input.end());

    test_elem_throw::on_copy_after(17);

    try
    {
      Deque c(input_begin, input_end);
      BOOST_TEST(false);
    } catch (const test_exception&) {}
  }

  BOOST_TEST(test_elem_base::no_living_elem());
}

BOOST_AUTO_TEST_CASE_TEMPLATE(constructor_forward_range, Deque, t_is_copy_constructible)
{
  typedef typename Deque::value_type T;
  const std::forward_list<T> x{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};

  {
    Deque a(x.begin(), x.end());
    test_equal_range(a, x);
  }

  {
    Deque b(x.begin(), x.begin());
    BOOST_TEST(b.empty());
  }

  if (! std::is_nothrow_copy_constructible<T>::value)
  {
    test_elem_throw::on_copy_after(10);

    try
    {
      Deque c(x.begin(), x.end());
      BOOST_TEST(false);
    } catch (const test_exception&) {}
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(copy_constructor, Deque, t_is_copy_constructible)
{
  typedef typename Deque::value_type T;

  {
    const Deque a;
    Deque b(a);

    BOOST_TEST(b.empty());
  }

  {
    const Deque a = get_range<Deque>();
    Deque b(a);

    test_equal_range(a, b);
  }

  if (! std::is_nothrow_copy_constructible<T>::value)
  {
    Deque a = get_range<Deque>();

    test_elem_throw::on_copy_after(12);

    try
    {
      Deque b(a);
      BOOST_TEST(false);
    } catch (const test_exception&) {}
  }

  BOOST_TEST(test_elem_base::no_living_elem());
}

BOOST_AUTO_TEST_CASE_TEMPLATE(move_constructor, Deque, all_deques)
{
  { // empty
    Deque a;
    Deque b(std::move(a));

    BOOST_TEST(a.empty());
    BOOST_TEST(b.empty());
  }

  {
    Deque a = get_range<Deque>(1, 5, 5, 9);
    Deque b(std::move(a));

    test_equal_range(b, get_range<Deque>(1, 5, 5, 9));

    // a is unspecified but valid state
    a.clear();
    BOOST_TEST(a.empty());
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(constructor_il, Deque, t_is_copy_constructible)
{
  {
    Deque a({});
    BOOST_TEST(a.empty());
  }

  {
    Deque b{1, 2, 3, 4, 5, 6, 7, 8};
    test_equal_range(b, get_range<Deque>(8));
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(destructor, Deque, all_deques)
{
  { Deque a; }
  {
    Deque b = get_range<Deque>();
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(op_copy_assign, Deque, t_is_copy_constructible)
{
  // assign to empty
  {
    Deque a;

    const Deque empty;
    a = empty;
    BOOST_TEST(a.empty());

    const Deque input{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
    a = input;
    test_equal_range(a, {1,2,3,4,5,6,7,8,9,10,11,12});
  }

  // input exceeds content
  {
    Deque b = get_range<Deque>(64,68,68,74);
    Deque input{1,2,3,4,5,6,7,8,9,10,11,12};

    b = input;
    test_equal_range(b, {1,2,3,4,5,6,7,8,9,10,11,12});
  }

  // input fits content
  {
    Deque c = get_range<Deque>(64,72,72,80);
    Deque input{1,2,3,4};

    c = input;
    test_equal_range(c, {1,2,3,4});
  }

  // input fits free front
  {
    Deque d = get_range<Deque>(10,16,16,16);
    d.pop_front();
    d.pop_front();
    d.pop_front();
    d.pop_front();

    Deque input{1,2,3,4};
    d = input;
    test_equal_range(d, {1,2,3,4});
  }

  // throw while assign to free front
  typedef typename Deque::value_type T;

  bool can_throw =
     ! std::is_nothrow_move_constructible<T>::value
  && ! std::is_nothrow_copy_constructible<T>::value;

  if (can_throw)
  {
    Deque e = get_range<Deque>(10);
    e.pop_front();
    e.pop_front();
    e.pop_front();
    e.pop_front();

    Deque input{1,2,3,4};

    test_elem_throw::on_copy_after(2);
    test_elem_throw::on_move_after(2);

    try
    {
      e = input;
      BOOST_TEST(false);
    } catch (const test_exception&) {}

    test_elem_throw::do_not_throw();
  }

  BOOST_TEST(test_elem_base::no_living_elem());
}

BOOST_AUTO_TEST_CASE_TEMPLATE(op_move_assign, Deque, all_deques)
{
  // assign to empty
  {
    Deque a;
    Deque b;

    a = std::move(b);
    b.clear(); // b is in an unspecified but valid state

    BOOST_TEST(b.empty());
    BOOST_TEST(a.empty());
  }

  // assign to non-empty
  {
    Deque a = get_range<Deque>();
    Deque b = get_range<Deque>(9);

    a = std::move(b);
    b.clear();

    test_equal_range(a, {1,2,3,4,5,6,7,8,9});
    BOOST_TEST(b.empty());
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(op_assign_il, Deque, t_is_copy_constructible)
{
  typedef typename Deque::value_type T;

  // assign to empty
  {
    Deque a;

    a = {};
    BOOST_TEST(a.empty());

    a = {1,2,3,4,5,6,7,8,9,10,11,12};
    test_equal_range(a, {1,2,3,4,5,6,7,8,9,10,11,12});
  }

  // input exceeds content
  {
    Deque b = get_range<Deque>(64,68,68,74);

    b = {1,2,3,4,5,6,7,8,9,10,11,12,13};
    test_equal_range(b, {1,2,3,4,5,6,7,8,9,10,11,12,13});
  }

  // input fits content
  {
    Deque c = get_range<Deque>(64,72,72,80);

    c = {1,2,3,4,5};
    test_equal_range(c, {1,2,3,4,5});
  }

  // input fits free front
  {
    Deque d = get_range<Deque>(10,16,16,16);
    d.pop_front();
    d.pop_front();
    d.pop_front();
    d.pop_front();

    d = {1,2,3,4};
    test_equal_range(d, {1,2,3,4});
  }

  // throw while assign to free front
  bool can_throw =
     ! std::is_nothrow_move_constructible<T>::value
  && ! std::is_nothrow_copy_constructible<T>::value;

  if (can_throw)
  {
    Deque e = get_range<Deque>(10);
    e.pop_front();
    e.pop_front();
    e.pop_front();
    e.pop_front();

    test_elem_throw::on_copy_after(2);
    test_elem_throw::on_move_after(2);

    try
    {
      e = {404,404,404,404,404};
      BOOST_TEST(false);
    } catch (const test_exception&) {}

    test_elem_throw::do_not_throw();
  }

  BOOST_TEST(test_elem_base::no_living_elem());
}

BOOST_AUTO_TEST_CASE_TEMPLATE(assign_input_range, Deque, t_is_copy_constructible)
{
  typedef typename Deque::value_type T;

  // assign to empty
  {
    Deque a;
    devector<T> input = get_range<devector<T>>(12);

    auto input_begin = make_input_iterator(input, input.begin());
    auto input_end   = make_input_iterator(input, input.end());

    a.assign(input_begin, input_begin);
    BOOST_TEST(a.empty());

    a.assign(input_begin, input_end);
    test_equal_range(a, {1,2,3,4,5,6,7,8,9,10,11,12});
  }

  // input exceeds content
  {
    Deque b = get_range<Deque>(64,68,68,74);
    devector<T> input = get_range<devector<T>>(13);

    auto input_begin = make_input_iterator(input, input.begin());
    auto input_end   = make_input_iterator(input, input.end());

    b.assign(input_begin, input_end);
    test_equal_range(b, {1,2,3,4,5,6,7,8,9,10,11,12,13});
  }

  // input fits content
  {
    Deque c = get_range<Deque>(64,72,72,80);
    devector<T> input = get_range<devector<T>>(6);

    auto input_begin = make_input_iterator(input, input.begin());
    auto input_end   = make_input_iterator(input, input.end());

    c.assign(input_begin, input_end);
    test_equal_range(c, {1,2,3,4,5,6});
  }

  // input fits free front
  {
    Deque d = get_range<Deque>(10,16,16,16);
    d.pop_front();
    d.pop_front();
    d.pop_front();
    d.pop_front();

    devector<T> input = get_range<devector<T>>(3);

    auto input_begin = make_input_iterator(input, input.begin());
    auto input_end   = make_input_iterator(input, input.end());

    d.assign(input_begin, input_end);
    test_equal_range(d, {1,2,3});
  }

  // throw while assign to free front
  bool can_throw =
     ! std::is_nothrow_move_constructible<T>::value
  && ! std::is_nothrow_copy_constructible<T>::value;

  if (can_throw)
  {
    Deque e = get_range<Deque>(10);
    e.pop_front();
    e.pop_front();
    e.pop_front();
    e.pop_front();

    devector<T> input = get_range<devector<T>>(3);

    auto input_begin = make_input_iterator(input, input.begin());
    auto input_end   = make_input_iterator(input, input.end());

    test_elem_throw::on_copy_after(2);
    test_elem_throw::on_move_after(2);

    try
    {
      e.assign(input_begin, input_end);
      BOOST_TEST(false);
    } catch (const test_exception&) {}

    test_elem_throw::do_not_throw();
  }

  BOOST_TEST(test_elem_base::no_living_elem());
}

BOOST_AUTO_TEST_CASE_TEMPLATE(assign_forward_range, Deque, t_is_copy_constructible)
{
  typedef typename Deque::value_type T;

  { // scope of input

  const std::forward_list<T> input{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
  auto input_begin = input.begin();
  auto input_mid = input.begin();
  auto input_end = input.end();

  std::advance(input_mid, 4);

  // assign to empty
  {
    Deque a;

    a.assign(input_begin, input_begin);
    BOOST_TEST(a.empty());

    a.assign(input_begin, input_end);
    test_equal_range(a, {1,2,3,4,5,6,7,8,9,10,11,12});
  }

  // input exceeds content
  {
    Deque b = get_range<Deque>(64,68,68,74);

    b.assign(input_begin, input_end);
    test_equal_range(b, {1,2,3,4,5,6,7,8,9,10,11,12});
  }

  // input fits content
  {
    Deque c = get_range<Deque>(64,72,72,80);

    c.assign(input_begin, input_mid);
    test_equal_range(c, {1,2,3,4});
  }

  // input fits free front
  {
    Deque d = get_range<Deque>(10,16,16,16);
    d.pop_front();
    d.pop_front();
    d.pop_front();
    d.pop_front();

    d.assign(input_begin, input_mid);
    test_equal_range(d, {1,2,3,4});
  }

  // throw while assign to free front
  bool can_throw =
     ! std::is_nothrow_move_constructible<T>::value
  && ! std::is_nothrow_copy_constructible<T>::value;

  if (can_throw)
  {
    Deque e = get_range<Deque>(10);
    e.pop_front();
    e.pop_front();
    e.pop_front();
    e.pop_front();

    test_elem_throw::on_copy_after(2);
    test_elem_throw::on_move_after(2);

    try
    {
      e.assign(input_begin, input_end);
      BOOST_TEST(false);
    } catch (const test_exception&) {}

    test_elem_throw::do_not_throw();
  }

  } // end scope of input

  BOOST_TEST(test_elem_base::no_living_elem());
}

BOOST_AUTO_TEST_CASE_TEMPLATE(assign_n_copy, Deque, t_is_copy_constructible)
{
  typedef typename Deque::value_type T;

  { // scope of input

  const T input(9);

  // assign to empty
  {
    Deque a;

    a.assign(0, input);
    BOOST_TEST(a.empty());

    a.assign(12, input);
    test_equal_range(a, {9,9,9,9,9,9,9,9,9,9,9,9});
  }

  // input exceeds content
  {
    Deque b = get_range<Deque>(64,68,68,74);

    b.assign(12, input);
    test_equal_range(b, {9,9,9,9,9,9,9,9,9,9,9,9});
  }

  // input fits content
  {
    Deque c = get_range<Deque>(64,72,72,80);

    c.assign(4, input);
    test_equal_range(c, {9,9,9,9});
  }

  // input fits free front
  {
    Deque d = get_range<Deque>(10,16,16,16);
    d.pop_front();
    d.pop_front();
    d.pop_front();
    d.pop_front();

    d.assign(4, input);
    test_equal_range(d, {9,9,9,9});
  }

  // throw while assign to free front
  bool can_throw =
     ! std::is_nothrow_move_constructible<T>::value
  && ! std::is_nothrow_copy_constructible<T>::value;

  if (can_throw)
  {
    Deque e = get_range<Deque>(10);
    e.pop_front();
    e.pop_front();
    e.pop_front();
    e.pop_front();

    test_elem_throw::on_copy_after(2);
    test_elem_throw::on_move_after(2);

    try
    {
      e.assign(10, input);
      BOOST_TEST(false);
    } catch (const test_exception&) {}

    test_elem_throw::do_not_throw();
  }

  } // end scope of input

  BOOST_TEST(test_elem_base::no_living_elem());
}

BOOST_AUTO_TEST_CASE_TEMPLATE(assign_il, Deque, t_is_copy_constructible)
{
  typedef typename Deque::value_type T;

  // assign to empty
  {
    Deque a;

    a.assign({});
    BOOST_TEST(a.empty());

    a.assign({1,2,3,4,5,6,7,8,9,10,11,12});
    test_equal_range(a, {1,2,3,4,5,6,7,8,9,10,11,12});
  }

  // input exceeds content
  {
    Deque b = get_range<Deque>(64,68,68,74);

    b.assign({1,2,3,4,5,6,7,8,9,10,11,12,13});
    test_equal_range(b, {1,2,3,4,5,6,7,8,9,10,11,12,13});
  }

  // input fits content
  {
    Deque c = get_range<Deque>(64,72,72,80);

    c.assign({1,2,3,4,5});
    test_equal_range(c, {1,2,3,4,5});
  }

  // input fits free front
  {
    Deque d = get_range<Deque>(10,16,16,16);
    d.pop_front();
    d.pop_front();
    d.pop_front();
    d.pop_front();

    d.assign({1,2,3,4});
    test_equal_range(d, {1,2,3,4});
  }

  // throw while assign to free front
  bool can_throw =
     ! std::is_nothrow_move_constructible<T>::value
  && ! std::is_nothrow_copy_constructible<T>::value;

  if (can_throw)
  {
    Deque e = get_range<Deque>(10);
    e.pop_front();
    e.pop_front();
    e.pop_front();
    e.pop_front();

    test_elem_throw::on_copy_after(2);
    test_elem_throw::on_move_after(2);

    try
    {
      e.assign({404,404,404,404,404});
      BOOST_TEST(false);
    } catch (const test_exception&) {}

    test_elem_throw::do_not_throw();
  }

  BOOST_TEST(test_elem_base::no_living_elem());
}

BOOST_AUTO_TEST_CASE_TEMPLATE(get_allocator, Deque, all_deques)
{
  Deque a;
  (void)a.get_allocator();
}

template <typename Deque, typename MutableDeque>
void test_begin_end_impl()
{
  {
    Deque a;
    BOOST_TEST(a.begin() == a.end());
  }

  {
    Deque b = get_range<MutableDeque>(1, 13, 13, 25);
    auto expected = get_range<std::vector<typename Deque::value_type>>(24);

    BOOST_TEST(boost::algorithm::equal(
      b.begin(), b.end(),
      expected.begin(), expected.end()
    ));
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(begin_end, Deque, all_deques)
{
  test_begin_end_impl<      Deque, Deque>();
  test_begin_end_impl<const Deque, Deque>();
}

template <typename Deque, typename MutableDeque>
void test_rbegin_rend_impl()
{
  {
    Deque a;
    BOOST_TEST(a.rbegin() == a.rend());
  }

  {
    Deque b = get_range<MutableDeque>(1, 13, 13, 25);
    auto expected = get_range<std::vector<typename Deque::value_type>>(24);

    BOOST_TEST(boost::algorithm::equal(
      b.rbegin(), b.rend(),
      expected.rbegin(), expected.rend()
    ));
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(rbegin_rend, Deque, all_deques)
{
  test_rbegin_rend_impl<      Deque, Deque>();
  test_rbegin_rend_impl<const Deque, Deque>();
}

BOOST_AUTO_TEST_CASE_TEMPLATE(cbegin_cend, Deque, all_deques)
{
  {
    Deque a;
    BOOST_TEST(a.cbegin() == a.cend());
  }

  {
    Deque b = get_range<Deque>(1, 13, 13, 25);
    auto expected = get_range<std::vector<typename Deque::value_type>>(24);

    BOOST_TEST(boost::algorithm::equal(
      b.cbegin(), b.cend(),
      expected.cbegin(), expected.cend()
    ));
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(crbegin_crend, Deque, all_deques)
{
  {
    Deque a;
    BOOST_TEST(a.crbegin() == a.crend());
  }

  {
    Deque b = get_range<Deque>(1, 13, 13, 25);
    auto expected = get_range<std::vector<typename Deque::value_type>>(24);

    BOOST_TEST(boost::algorithm::equal(
      b.crbegin(), b.crend(),
      expected.crbegin(), expected.crend()
    ));
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(empty, Deque, all_deques)
{
  Deque a;
  BOOST_TEST(a.empty());

  a.emplace_front(1);

  BOOST_TEST(! a.empty());
}

BOOST_AUTO_TEST_CASE_TEMPLATE(size, Deque, all_deques)
{
  Deque a;
  BOOST_TEST(a.size() == 0u);

  a.emplace_front(1);
  a.emplace_front(2);
  a.emplace_front(3);

  BOOST_TEST(a.size() == 3u);

  a.pop_front();
  a.pop_front();

  BOOST_TEST(a.size() == 1u);

  a.emplace_back(2);
  a.emplace_back(3);
  a.emplace_back(4);
  a.emplace_back(5);
  a.emplace_back(6);

  BOOST_TEST(a.size() == 6u);

  a.emplace_back(7);
  a.emplace_back(8);

  a.emplace_back(9);
  a.emplace_back(10);
  a.emplace_back(11);

  BOOST_TEST(a.size() == 11u);

  Deque b = get_range<Deque>(1,9,0,0);
  BOOST_TEST(b.size() == 8u);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(max_size, Deque, all_deques)
{
  Deque a;
  (void)a.max_size();
}

BOOST_AUTO_TEST_CASE_TEMPLATE(resize_value, Deque, t_is_default_constructible)
{
  {
    Deque a;
    a.resize(0);
    BOOST_TEST(a.empty());

    a.resize(10);
    BOOST_TEST(a.size() == 10u);
    test_equal_range(a, {0,0,0,0,0,0,0,0,0,0});

    a.resize(10);
    BOOST_TEST(a.size() == 10u);
    test_equal_range(a, {0,0,0,0,0,0,0,0,0,0});

    a.resize(5);
    BOOST_TEST(a.size() == 5u);
    test_equal_range(a, {0,0,0,0,0});

    a.resize(0);
    BOOST_TEST(a.empty());
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(resize_copy, Deque, t_is_copy_constructible)
{
  typedef typename Deque::value_type T;
  const T x(9);

  {
    Deque a;
    a.resize(0, x);
    BOOST_TEST(a.empty());

    a.resize(10, x);
    BOOST_TEST(a.size() == 10u);
    test_equal_range(a, {9,9,9,9,9,9,9,9,9,9});

    a.resize(10, x);
    BOOST_TEST(a.size() == 10u);
    test_equal_range(a, {9,9,9,9,9,9,9,9,9,9});

    a.resize(5, x);
    BOOST_TEST(a.size() == 5u);
    test_equal_range(a, {9,9,9,9,9});

    a.resize(0, x);
    BOOST_TEST(a.empty());
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(reserve_front, Deque, all_deques)
{
  Deque a;

  a.reserve_front(100);
  for (unsigned i = 0; i < 100u; ++i)
  {
    a.push_front(i);
  }

  Deque b;
  b.reserve_front(4);
  b.reserve_front(6);
  b.reserve_front(4);
  b.reserve_front(8);
  b.reserve_front(16);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(reserve_back, Deque, all_deques)
{
  Deque a;

  a.reserve_back(100);
  for (unsigned i = 0; i < 100; ++i)
  {
    a.push_back(i);
  }

  Deque b;
  b.reserve_back(4);
  b.reserve_back(6);
  b.reserve_back(4);
  b.reserve_back(8);
  b.reserve_back(16);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(shrink_to_fit, Deque, all_deques)
{
  Deque a;
  a.shrink_to_fit();
  BOOST_TEST(a.front_free_capacity() == 0u);

  a.emplace_front(1);
  a.pop_front();
  a.shrink_to_fit();
  BOOST_TEST(a.front_free_capacity() == 0u);

  a.emplace_front(1);
  a.shrink_to_fit();

  const auto min_cap = a.front_free_capacity();
  a.reserve_front(123);
  a.shrink_to_fit();
  BOOST_TEST(a.front_free_capacity() == min_cap);
}

template <typename Deque, typename MutableDeque>
void test_op_at_impl()
{
  typedef typename Deque::value_type T;

  MutableDeque a = get_range<MutableDeque>(26);

  a.pop_front();
  a.pop_front();

  Deque b(std::move(a));

  BOOST_TEST(b[0] == T(3));
  BOOST_TEST(b[8] == T(11));
  BOOST_TEST(b[14] == T(17));
  BOOST_TEST(b[23] == T(26));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(op_at, Deque, all_deques)
{
  test_op_at_impl<      Deque, Deque>();
  test_op_at_impl<const Deque, Deque>();
}

template <typename Deque, typename MutableDeque>
void test_at_impl()
{
  typedef typename Deque::value_type T;

  MutableDeque a = get_range<MutableDeque>(26);

  a.pop_front();
  a.pop_front();

  Deque b(std::move(a));

  BOOST_TEST(b.at(0) == T(3));
  BOOST_TEST(b.at(8) == T(11));
  BOOST_TEST(b.at(14) == T(17));
  BOOST_TEST(b.at(23) == T(26));

  BOOST_CHECK_THROW(b.at(24), std::out_of_range);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(at, Deque, all_deques)
{
  test_at_impl<      Deque, Deque>();
  test_at_impl<const Deque, Deque>();
}

BOOST_AUTO_TEST_CASE_TEMPLATE(front, Deque, all_deques)
{
  typedef typename Deque::value_type T;

  { // non-const front
    Deque a = get_range<Deque>(3);
    BOOST_TEST(a.front() == T(1));
    a.front() = T(100);
    BOOST_TEST(a.front() == T(100));
  }

  { // const front
    const Deque a = get_range<Deque>(3);
    BOOST_TEST(a.front() == T(1));
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(back, Deque, all_deques)
{
  typedef typename Deque::value_type T;

  { // non-const back
    Deque a = get_range<Deque>(3);
    BOOST_TEST(a.back() == T(3));
    a.back() = T(100);
    BOOST_TEST(a.back() == T(100));
  }

  { // const back
    const Deque a = get_range<Deque>(3);
    BOOST_TEST(a.back() == T(3));
  }

  { // at segment boundary
    Deque a = get_range<Deque>(8);
    BOOST_TEST(a.back() == T(8));
    a.push_back(T(9));
    BOOST_TEST(a.back() == T(9));
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(emplace_front, Deque, all_deques)
{
  typedef typename Deque::value_type T;

  {
    Deque a;

    a.emplace_front(3);
    a.emplace_front(2);
    a.emplace_front(1);

    test_equal_range(a, {1, 2, 3});
  }

  if (! std::is_nothrow_constructible<T>::value)
  {
    Deque b = get_range<Deque>(8);

    try
    {
      test_elem_throw::on_ctor_after(1);
      b.emplace_front(404);
      BOOST_TEST(false);
    }
    catch (const test_exception&) {}

    test_equal_range(b, {1, 2, 3, 4, 5, 6, 7, 8});
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(push_front_copy, Deque, t_is_copy_constructible)
{
  typedef typename Deque::value_type T;

  {
    Deque a;

    for (std::size_t i = 1; i <= 12; ++i)
    {
      const T elem(i);
      a.push_front(elem);
    }

    test_equal_range(a, {12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1});
  }

  if (! std::is_nothrow_copy_constructible<T>::value)
  {
    Deque b = get_range<Deque>(10);

    try
    {
      const T elem(404);
      test_elem_throw::on_copy_after(1);
      b.push_front(elem);
      BOOST_TEST(false);
    }
    catch (const test_exception&) {}

    test_equal_range(b, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10});
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(push_front_rvalue, Deque, all_deques)
{
  typedef typename Deque::value_type T;

  {
    Deque a;

    for (std::size_t i = 1; i <= 12; ++i)
    {
      T elem(i);
      a.push_front(std::move(elem));
    }

    test_equal_range(a, {12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1});
  }

  if (! std::is_nothrow_move_constructible<T>::value)
  {
    Deque b = get_range<Deque>(8);

    try
    {
      test_elem_throw::on_move_after(1);
      b.push_front(T(404));
      BOOST_TEST(false);
    }
    catch (const test_exception&) {}

    test_equal_range(b, {1, 2, 3, 4, 5, 6, 7, 8});
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(unsafe_push_front_copy, Deque, t_is_copy_constructible)
{
  typedef typename Deque::value_type T;

  {
    Deque a;
    a.reserve_front(12);

    for (int i = 1; i <= 12; ++i)
    {
      const T elem(i);
      a.unsafe_push_front(elem);
    }

    test_equal_range(a, {12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1});
  }

  if (! std::is_nothrow_copy_constructible<T>::value)
  {
    Deque b = get_range<Deque>(10);
    b.reserve_front(11);

    try
    {
      const T elem(404);
      test_elem_throw::on_copy_after(1);
      b.unsafe_push_front(elem);
      BOOST_TEST(false);
    }
    catch (const test_exception&) {}

    test_equal_range(b, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10});
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(unsafe_push_front_rvalue, Deque, all_deques)
{
  typedef typename Deque::value_type T;

  {
    Deque a;
    a.reserve_front(12);

    for (int i = 1; i <= 12; ++i)
    {
      T elem(i);
      a.unsafe_push_front(std::move(elem));
    }

    test_equal_range(a, {12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1});
  }

  if (! std::is_nothrow_move_constructible<T>::value)
  {
    Deque b = get_range<Deque>(8);
    b.reserve_front(9);

    try
    {
      test_elem_throw::on_move_after(1);
      b.unsafe_push_front(T(404));
      BOOST_TEST(false);
    }
    catch (const test_exception&) {}

    test_equal_range(b, {1, 2, 3, 4, 5, 6, 7, 8});
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(emplace_back, Deque, all_deques)
{
  typedef typename Deque::value_type T;

  {
    Deque a;

    a.emplace_back(1);
    a.emplace_back(2);
    a.emplace_back(3);

    test_equal_range(a, {1, 2, 3});
  }

  if (! std::is_nothrow_constructible<T>::value)
  {
    Deque b = get_range<Deque>(8);

    try
    {
      test_elem_throw::on_ctor_after(1);
      b.emplace_back(404);
      BOOST_TEST(false);
    }
    catch (const test_exception&) {}

    test_equal_range(b, {1, 2, 3, 4, 5, 6, 7, 8});
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(push_back_copy, Deque, t_is_copy_constructible)
{
  typedef typename Deque::value_type T;

  {
    Deque a;

    for (std::size_t i = 1; i <= 12; ++i)
    {
      const T elem(i);
      a.push_back(elem);
    }

    test_equal_range(a, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12});
  }

  if (! std::is_nothrow_copy_constructible<T>::value)
  {
    Deque b = get_range<Deque>(10);

    try
    {
      const T elem(404);
      test_elem_throw::on_copy_after(1);
      b.push_back(elem);
      BOOST_TEST(false);
    }
    catch (const test_exception&) {}

    test_equal_range(b, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10});
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(push_back_rvalue, Deque, all_deques)
{
  typedef typename Deque::value_type T;

  {
    Deque a;

    for (std::size_t i = 1; i <= 12; ++i)
    {
      T elem(i);
      a.push_back(std::move(elem));
    }

    test_equal_range(a, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12});
  }

  if (! std::is_nothrow_move_constructible<T>::value)
  {
    Deque b = get_range<Deque>(8);

    try
    {
      test_elem_throw::on_move_after(1);
      b.push_back(T(404));
      BOOST_TEST(false);
    }
    catch (const test_exception&) {}

    test_equal_range(b, {1, 2, 3, 4, 5, 6, 7, 8});
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(unsafe_push_back_copy, Deque, t_is_copy_constructible)
{
  typedef typename Deque::value_type T;

  {
    Deque a;
    a.reserve_back(12);

    for (int i = 1; i <= 12; ++i)
    {
      const T elem(i);
      a.unsafe_push_back(elem);
    }

    test_equal_range(a, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12});
  }

  if (! std::is_nothrow_copy_constructible<T>::value)
  {
    Deque b = get_range<Deque>(10);
    b.reserve_back(11);

    try
    {
      const T elem(404);
      test_elem_throw::on_copy_after(1);
      b.unsafe_push_back(elem);
      BOOST_TEST(false);
    }
    catch (const test_exception&) {}

    test_equal_range(b, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10});
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(unsafe_push_back_rvalue, Deque, all_deques)
{
  typedef typename Deque::value_type T;

  {
    Deque a;
    a.reserve_back(12);

    for (std::size_t i = 1; i <= 12; ++i)
    {
      T elem(i);
      a.unsafe_push_back(std::move(elem));
    }

    test_equal_range(a, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12});
  }

  if (! std::is_nothrow_move_constructible<T>::value)
  {
    Deque b = get_range<Deque>(8);
    b.reserve_back(9);

    try
    {
      test_elem_throw::on_move_after(1);
      b.unsafe_push_back(T(404));
      BOOST_TEST(false);
    }
    catch (const test_exception&) {}

    test_equal_range(b, {1, 2, 3, 4, 5, 6, 7, 8});
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(emplace, Deque, all_deques)
{
  typedef typename Deque::value_type T;

  // emplace to empty begin
  {
    Deque a;
    auto res = a.emplace(a.begin(), 1);
    test_equal_range(a, {1});
    BOOST_TEST(*res == T(1));
  }

  // to empty end
  {
    Deque b;
    auto res = b.emplace(b.end(), 2);
    test_equal_range(b, {2});
    BOOST_TEST(*res == T(2));
  }

  // to front
  {
    Deque c = get_range<Deque>(8);
    auto res = c.emplace(c.begin(), 9);
    test_equal_range(c, {9,1,2,3,4,5,6,7,8});
    BOOST_TEST(*res == T(9));
  }

  // to back
  {
    Deque d = get_range<Deque>(8);
    auto res = d.emplace(d.end(), 9);
    test_equal_range(d, {1,2,3,4,5,6,7,8,9});
    BOOST_TEST(*res == T(9));
  }

  // to near front
  {
    Deque e = get_range<Deque>(8);
    auto res = e.emplace(e.begin() + 3, 9);
    test_equal_range(e, {1,2,3,9,4,5,6,7,8});
    BOOST_TEST(*res == T(9));
  }

  // to near back
  {
    Deque f = get_range<Deque>(8);
    auto res = f.emplace(f.end() - 3, 9);
    test_equal_range(f, {1,2,3,4,5,9,6,7,8});
    BOOST_TEST(*res == T(9));
  }

  // middle with ctor exception
  if (! std::is_nothrow_constructible<T>::value)
  {
    Deque g = get_range<Deque>(8);
    auto begin = g.begin();

    test_elem_throw::on_ctor_after(1);

    try
    {
      g.emplace(g.begin() + 4, 404);
      BOOST_TEST(false);
    } catch (const test_exception&) {}

    BOOST_TEST(begin == g.begin());
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(insert_copy, Deque, t_is_copy_constructible)
{
  typedef typename Deque::value_type T;

  const T x(9);

  // insert to empty begin
  {
    Deque a;
    auto res = a.insert(a.begin(), x);
    test_equal_range(a, {9});
    BOOST_TEST(*res == x);
  }

  // to empty end
  {
    Deque b;
    auto res = b.insert(b.end(), x);
    test_equal_range(b, {9});
    BOOST_TEST(*res == x);
  }

  // to front
  {
    Deque c = get_range<Deque>(8);
    auto res = c.insert(c.begin(), x);
    test_equal_range(c, {9,1,2,3,4,5,6,7,8});
    BOOST_TEST(*res == x);
  }

  // to back
  {
    Deque d = get_range<Deque>(8);
    auto res = d.insert(d.end(), x);
    test_equal_range(d, {1,2,3,4,5,6,7,8,9});
    BOOST_TEST(*res == x);
  }

  // to near front
  {
    Deque e = get_range<Deque>(8);
    auto res = e.insert(e.begin() + 3, x);
    test_equal_range(e, {1,2,3,9,4,5,6,7,8});
    BOOST_TEST(*res == x);
  }

  // to near back
  {
    Deque f = get_range<Deque>(8);
    auto res = f.insert(f.end() - 3, x);
    test_equal_range(f, {1,2,3,4,5,9,6,7,8});
    BOOST_TEST(*res == x);
  }

  // middle with copy ctor exception
  if (! std::is_nothrow_copy_constructible<T>::value)
  {
    Deque g = get_range<Deque>(8);
    auto begin = g.begin();

    test_elem_throw::on_copy_after(1);

    try
    {
      g.insert(g.begin() + 4, x);
      BOOST_TEST(false);
    } catch (const test_exception&) {}

    BOOST_TEST(begin == g.begin());
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(insert_rvalue, Deque, all_deques)
{
  typedef typename Deque::value_type T;

  // insert to empty begin
  {
    Deque a;
    auto res = a.insert(a.begin(), T(1));
    test_equal_range(a, {1});
    BOOST_TEST(*res == T(1));
  }

  // to empty end
  {
    Deque b;
    auto res = b.insert(b.end(), T(2));
    test_equal_range(b, {2});
    BOOST_TEST(*res == T(2));
  }

  // to front
  {
    Deque c = get_range<Deque>(8);
    auto res = c.insert(c.begin(), T(9));
    test_equal_range(c, {9,1,2,3,4,5,6,7,8});
    BOOST_TEST(*res == T(9));
  }

  // to back
  {
    Deque d = get_range<Deque>(8);
    auto res = d.insert(d.end(), T(9));
    test_equal_range(d, {1,2,3,4,5,6,7,8,9});
    BOOST_TEST(*res == T(9));
  }

  // to near front
  {
    Deque e = get_range<Deque>(8);
    auto res = e.insert(e.begin() + 3, T(9));
    test_equal_range(e, {1,2,3,9,4,5,6,7,8});
    BOOST_TEST(*res == T(9));
  }

  // to near back
  {
    Deque f = get_range<Deque>(8);
    auto res = f.insert(f.end() - 3, T(9));
    test_equal_range(f, {1,2,3,4,5,9,6,7,8});
    BOOST_TEST(*res == T(9));
  }

  // middle with ctor exception
  if (! std::is_nothrow_constructible<T>::value)
  {
    Deque g = get_range<Deque>(8);
    auto begin = g.begin();

    test_elem_throw::on_ctor_after(1);

    try
    {
      g.insert(g.begin() + 4, T(404));
      BOOST_TEST(false);
    } catch (const test_exception&) {}

    BOOST_TEST(begin == g.begin());
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(insert_n_copy, Deque, t_is_copy_constructible)
{
  typedef typename Deque::value_type T;

  { // scope of x

  const T x(9);

  // empty to empty
  {
    Deque a;

    auto ret = a.insert(a.begin(), 0, x);
    BOOST_TEST(a.empty());
    BOOST_TEST(ret == a.begin());

    ret = a.insert(a.end(), 0, x);
    BOOST_TEST(a.empty());
    BOOST_TEST(ret == a.end());
  }

  // range to empty
  {
    Deque b;

    auto ret = b.insert(b.end(), 10, x);
    test_equal_range(b, {9,9,9,9,9,9,9,9,9,9});
    BOOST_TEST(ret == b.begin());
  }

  // range to non-empty front
  {
    Deque c = get_range<Deque>(6);

    auto ret = c.insert(c.begin(), 4, x);
    test_equal_range(c, {9,9,9,9,1,2,3,4,5,6});
    BOOST_TEST(ret == c.begin());
  }

  // range to non-empty back
  {
    Deque d = get_range<Deque>(6);

    auto ret = d.insert(d.end(), 4, x);
    test_equal_range(d, {1,2,3,4,5,6,9,9,9,9});
    BOOST_TEST(ret == d.begin() + 6);
  }

  // range to non-empty near front
  {
    Deque e = get_range<Deque>(12);

    auto ret = e.insert(e.begin() + 2, 3, x);
    test_equal_range(e, {1,2,9,9,9,3,4,5,6,7,8,9,10,11,12});
    BOOST_TEST(ret == e.begin() + 2);
  }

  // range to non-empty near back
  {
    Deque f = get_range<Deque>(12);

    auto ret = f.insert(f.begin() + 8, 3, x);
    test_equal_range(f, {1,2,3,4,5,6,7,8,9,9,9,9,10,11,12});
    BOOST_TEST(ret == f.begin() + 8);
  }

  // exception in the middle
  if (! std::is_nothrow_copy_constructible<T>::value)
  {
    test_elem_throw::on_copy_after(10);

    try
    {
      Deque g = get_range<Deque>(8);
      g.insert(g.begin() + 4, 12, x);
      BOOST_TEST(false);
    } catch (const test_exception&) {}
  }

  } // scope of x

  BOOST_TEST(test_elem_base::no_living_elem());
}

BOOST_AUTO_TEST_CASE_TEMPLATE(insert_input_range, Deque, t_is_copy_constructible)
{
  typedef typename Deque::value_type T;

  // empty to empty
  {
    Deque a;

    devector<T> input;

    auto input_begin = make_input_iterator(input, input.begin());
    auto input_end   = make_input_iterator(input, input.end());

    auto ret = a.insert(a.begin(), input_begin, input_end);
    BOOST_TEST(a.empty());
    BOOST_TEST(ret == a.begin());

    ret = a.insert(a.end(), input_begin, input_end);
    BOOST_TEST(a.empty());
    BOOST_TEST(ret == a.end());
  }

  // range to empty
  {
    Deque b;

    devector<T> input = get_range<devector<T>>(10);

    auto input_begin = make_input_iterator(input, input.begin());
    auto input_end   = make_input_iterator(input, input.end());

    auto ret = b.insert(b.begin(), input_begin, input_end);
    test_equal_range(b, {1,2,3,4,5,6,7,8,9,10});
    BOOST_TEST(ret == b.begin());
  }

  // range to non-empty front
  {
    Deque c = get_range<Deque>(6);

    devector<T> input = get_range<devector<T>>(4);

    auto input_begin = make_input_iterator(input, input.begin());
    auto input_end   = make_input_iterator(input, input.end());

    auto ret = c.insert(c.begin(), input_begin, input_end);
    test_equal_range(c, {1,2,3,4,1,2,3,4,5,6});
    BOOST_TEST(ret == c.begin());
  }

  // range to non-empty back
  {
    Deque d = get_range<Deque>(6);

    devector<T> input = get_range<devector<T>>(4);

    auto input_begin = make_input_iterator(input, input.begin());
    auto input_end   = make_input_iterator(input, input.end());

    auto ret = d.insert(d.end(), input_begin, input_end);
    test_equal_range(d, {1,2,3,4,5,6,1,2,3,4});
    BOOST_TEST(ret == d.begin() + 6);
  }

  // range to non-empty near front
  {
    Deque e = get_range<Deque>(12);

    devector<T> input = get_range<devector<T>>(4);

    auto input_begin = make_input_iterator(input, input.begin());
    auto input_end   = make_input_iterator(input, input.end());

    auto ret = e.insert(e.begin() + 2, input_begin, input_end);
    test_equal_range(e, {1,2,1,2,3,4,3,4,5,6,7,8,9,10,11,12});
    BOOST_TEST(ret == e.begin() + 2);
  }

  // range to non-empty near back
  {
    Deque f = get_range<Deque>(12);

    devector<T> input = get_range<devector<T>>(4);

    auto input_begin = make_input_iterator(input, input.begin());
    auto input_end   = make_input_iterator(input, input.end());

    auto ret = f.insert(f.begin() + 8, input_begin, input_end);
    test_equal_range(f, {1,2,3,4,5,6,7,8,1,2,3,4,9,10,11,12});
    BOOST_TEST(ret == f.begin() + 8);
  }

  // exception in the middle
  if (! std::is_nothrow_copy_constructible<T>::value)
  {
    devector<T> input = get_range<devector<T>>(12);

    auto input_begin = make_input_iterator(input, input.begin());
    auto input_end   = make_input_iterator(input, input.end());

    test_elem_throw::on_copy_after(10);

    try
    {
      Deque g = get_range<Deque>(8);
      g.insert(g.begin() + 4, input_begin, input_end);
      BOOST_TEST(false);
    } catch (const test_exception&) {}
  }

  BOOST_TEST(test_elem_base::no_living_elem());
}

BOOST_AUTO_TEST_CASE_TEMPLATE(insert_forward_range, Deque, t_is_copy_constructible)
{
  typedef typename Deque::value_type T;

  { // scope of input

  const std::forward_list<T> input{1,2,3,4,5,6,7,8,9,10,11,12};
  auto four = input.begin();
  std::advance(four, 4);

  // empty to empty
  {
    Deque a;

    auto ret = a.insert(a.begin(), input.begin(), input.begin());
    BOOST_TEST(a.empty());
    BOOST_TEST(ret == a.begin());

    ret = a.insert(a.end(), input.begin(), input.begin());
    BOOST_TEST(a.empty());
    BOOST_TEST(ret == a.end());
  }

  // range to empty
  {
    Deque b;

    auto ret = b.insert(b.begin(), input.begin(), input.end());
    test_equal_range(b, {1,2,3,4,5,6,7,8,9,10,11,12});
    BOOST_TEST(ret == b.begin());
  }

  // range to non-empty front
  {
    Deque c = get_range<Deque>(6);

    auto ret = c.insert(c.begin(), input.begin(), input.end());
    test_equal_range(c, {1,2,3,4,5,6,7,8,9,10,11,12,1,2,3,4,5,6});
    BOOST_TEST(ret == c.begin());
  }

  // range to non-empty back
  {
    Deque d = get_range<Deque>(6);

    auto ret = d.insert(d.end(), input.begin(), four);
    test_equal_range(d, {1,2,3,4,5,6,1,2,3,4});
    BOOST_TEST(ret == d.begin() + 6);
  }

  // range to non-empty near front
  {
    Deque e = get_range<Deque>(12);

    auto ret = e.insert(e.begin() + 2, input.begin(), four);
    test_equal_range(e, {1,2,1,2,3,4,3,4,5,6,7,8,9,10,11,12});
    BOOST_TEST(ret == e.begin() + 2);
  }

  // range to non-empty near back
  {
    Deque f = get_range<Deque>(12);

    auto ret = f.insert(f.begin() + 8, input.begin(), four);
    test_equal_range(f, {1,2,3,4,5,6,7,8,1,2,3,4,9,10,11,12});
    BOOST_TEST(ret == f.begin() + 8);
  }

  // exception in the middle
  if (! std::is_nothrow_copy_constructible<T>::value)
  {
    test_elem_throw::on_copy_after(10);

    try
    {
      Deque g = get_range<Deque>(8);
      g.insert(g.begin() + 4, input.begin(), input.end());
      BOOST_TEST(false);
    } catch (const test_exception&) {}
  }

  } // scope of input

  BOOST_TEST(test_elem_base::no_living_elem());
}

BOOST_AUTO_TEST_CASE_TEMPLATE(insert_il, Deque, t_is_copy_constructible)
{
  // empty to empty
  {
    Deque a;

    auto ret = a.insert(a.begin(), {});
    BOOST_TEST(a.empty());
    BOOST_TEST(ret == a.begin());

    ret = a.insert(a.end(), {});
    BOOST_TEST(a.empty());
    BOOST_TEST(ret == a.end());
  }

  // range to empty
  {
    Deque b;

    auto ret = b.insert(b.begin(), {1,2,3,4,5,6,7,8,9,10,11,12});
    test_equal_range(b, {1,2,3,4,5,6,7,8,9,10,11,12});
    BOOST_TEST(ret == b.begin());
  }

  // range to non-empty front
  {
    Deque c = get_range<Deque>(6);

    auto ret = c.insert(c.begin(), {1,2,3,4,5,6,7,8,9,10,11,12});
    test_equal_range(c, {1,2,3,4,5,6,7,8,9,10,11,12,1,2,3,4,5,6});
    BOOST_TEST(ret == c.begin());
  }

  // range to non-empty back
  {
    Deque d = get_range<Deque>(6);

    auto ret = d.insert(d.end(), {1,2,3,4});
    test_equal_range(d, {1,2,3,4,5,6,1,2,3,4});
    BOOST_TEST(ret == d.begin() + 6);
  }

  // range to non-empty near front
  {
    Deque e = get_range<Deque>(12);

    auto ret = e.insert(e.begin() + 2, {1,2,3,4});
    test_equal_range(e, {1,2,1,2,3,4,3,4,5,6,7,8,9,10,11,12});
    BOOST_TEST(ret == e.begin() + 2);
  }

  // range to non-empty near back
  {
    Deque f = get_range<Deque>(12);

    auto ret = f.insert(f.begin() + 8, {1,2,3,4});
    test_equal_range(f, {1,2,3,4,5,6,7,8,1,2,3,4,9,10,11,12});
    BOOST_TEST(ret == f.begin() + 8);
  }

  typedef typename Deque::value_type T;

  // exception in the middle
  if (! std::is_nothrow_copy_constructible<T>::value)
  {
    test_elem_throw::on_copy_after(10);

    try
    {
      Deque g = get_range<Deque>(8);
      g.insert(g.begin() + 4, {1,2,3,4,5,6,7,8,9,10,11,12});
      BOOST_TEST(false);
    } catch (const test_exception&) {}
  }

  BOOST_TEST(test_elem_base::no_living_elem());
}

BOOST_AUTO_TEST_CASE_TEMPLATE(stable_insert, Deque, t_is_default_constructible)
{
  typedef typename Deque::value_type T;
  typedef typename Deque::iterator iterator;

  devector<const T*> valid_pointers;

  auto save_pointers = [&](const Deque& d)
  {
    valid_pointers.clear();
    valid_pointers.reserve(d.size());
    for (auto&& e : d) { valid_pointers.unsafe_push_back(&e); }
  };

  auto check_pointers = [&]()
  {
    unsigned i = 0;
    for (const T* p : valid_pointers) { BOOST_TEST(*p == T(++i)); }
  };

  auto check_inserted = [](iterator f, iterator l)
  {
    unsigned i = 0;
    while (f != l) { BOOST_TEST(*f++ == T(++i)); }
  };

  // empty to empty
  {
    devector<T> x;
    auto xb = std::make_move_iterator(x.begin());
    auto xe = std::make_move_iterator(x.end());

    Deque a;
    auto res = a.stable_insert(a.begin(), xb, xb);
    BOOST_TEST(a.empty());
    BOOST_TEST(res == a.begin());

    res = a.stable_insert(a.end(), xe, xe);
    BOOST_TEST(a.empty());
    BOOST_TEST(res == a.end());
  }

  // range to empty
  {
    devector<T> x = get_range<devector<T>>(12);
    auto xb = std::make_move_iterator(x.begin());
    auto xe = std::make_move_iterator(x.end());

    Deque b;
    auto res = b.stable_insert(b.end(), xb, xe);
    test_equal_range(b, {1,2,3,4,5,6,7,8,9,10,11,12});
    BOOST_TEST(res == b.begin());
  }

  // range to begin
  {
    devector<T> x = get_range<devector<T>>(12);
    auto xb = std::make_move_iterator(x.begin());
    auto xe = std::make_move_iterator(x.end());

    Deque c = get_range<Deque>(4);
    save_pointers(c);

    auto res = c.stable_insert(c.begin(), xb, xe);

    check_pointers();
    check_inserted(res, res + x.size());
    BOOST_TEST(c.size() >= 16);
  }

  // range to end
  {
    devector<T> x = get_range<devector<T>>(12);
    auto xb = std::make_move_iterator(x.begin());
    auto xe = std::make_move_iterator(x.end());

    Deque d = get_range<Deque>(4);
    save_pointers(d);

    auto res = d.stable_insert(d.end(), xb, xe);

    check_pointers();
    check_inserted(res, res + x.size());
    BOOST_TEST(d.size() >= 16);
  }

  // range to mid mid-segment
  {
    devector<T> x = get_range<devector<T>>(4);
    auto xb = std::make_move_iterator(x.begin());
    auto xe = std::make_move_iterator(x.end());

    Deque e = get_range<Deque>(14);
    save_pointers(e);

    auto res = e.stable_insert(e.begin() + 6, xb, xe);

    check_pointers();
    check_inserted(res, res + x.size());
    BOOST_TEST(e.size() >= 18);
  }

  // range to mid segment boundary
  {
    devector<T> x = get_range<devector<T>>(6);
    auto xb = std::make_move_iterator(x.begin());
    auto xe = std::make_move_iterator(x.end());

    Deque f = get_range<Deque>(11);
    save_pointers(f);

    auto res = f.stable_insert(f.begin() + 8, xb, xe);

    check_pointers();
    check_inserted(res, res + x.size());
    BOOST_TEST(f.size() >= 17);
  }

  // range to mid segment, no default ctr
  {
    devector<T> x = get_range<devector<T>>(8);
    auto xb = std::make_move_iterator(x.begin());
    auto xe = std::make_move_iterator(x.end());

    Deque g = get_range<Deque>(14);
    save_pointers(g);

    auto res = g.stable_insert(g.begin() + 6, xb, xe);

    check_pointers();
    check_inserted(res, res + x.size());
    BOOST_TEST(g.size() >= 22);
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(pop_front, Deque, all_deques)
{
  {
    Deque a;
    a.emplace_front(1);
    a.pop_front();
    BOOST_TEST(a.empty());

    a.emplace_back(2);
    a.pop_front();
    BOOST_TEST(a.empty());

    a.emplace_front(3);
    a.pop_front();
    BOOST_TEST(a.empty());
  }

  {
    Deque b = get_range<Deque>(20);
    for (int i = 0; i < 20; ++i)
    {
      BOOST_TEST(!b.empty());
      b.pop_front();
    }
    BOOST_TEST(b.empty());
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(pop_back, Deque, all_deques)
{
  {
    Deque a;
    a.emplace_front(1);
    a.pop_back();
    BOOST_TEST(a.empty());

    a.emplace_back(2);
    a.pop_back();
    BOOST_TEST(a.empty());

    a.emplace_front(3);
    a.pop_back();
    BOOST_TEST(a.empty());
  }

  {
    Deque b = get_range<Deque>(20);
    for (int i = 0; i < 20; ++i)
    {
      BOOST_TEST(!b.empty());
      b.pop_back();
    }
    BOOST_TEST(b.empty());
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(erase, Deque, all_deques)
{
  typedef typename Deque::value_type T;

  {
    Deque a = get_range<Deque>(10);

    auto it = a.erase(a.begin());
    test_equal_range(a, {2,3,4,5,6,7,8,9,10});
    BOOST_TEST(*it == T(2));

    it = a.erase(a.begin() + 8);
    test_equal_range(a, {2,3,4,5,6,7,8,9});
    BOOST_TEST(it == a.end());

    it = a.erase(a.begin() + 4);
    test_equal_range(a, {2,3,4,5,7,8,9});
    BOOST_TEST(*it == T(7));
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(erase_range, Deque, all_deques)
{
  typedef typename Deque::value_type T;

  {
    Deque a;
    a.erase(a.begin(), a.begin());
    BOOST_TEST(a.empty());
    a.erase(a.begin(), a.end());
    BOOST_TEST(a.empty());
    a.erase(a.end(), a.end());
    BOOST_TEST(a.empty());
  }

  {
    Deque b = get_range<Deque>(18);
    auto it = b.erase(b.begin() + 1, b.begin() + 4);
    test_equal_range(b, {1,5,6,7,8,9,10,11,12,13,14,15,16,17,18});
    BOOST_TEST(*it == T(5));
  }

  {
    Deque c = get_range<Deque>(18);
    auto it = c.erase(c.begin() + 3, c.begin() + 14);
    test_equal_range(c, {1,2,3,15,16,17,18});
    BOOST_TEST(*it == T(15));
  }

  {
    Deque d = get_range<Deque>(22);
    d.erase(d.begin(), d.end());
    BOOST_TEST(d.empty());
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(member_swap, Deque, all_deques)
{
  {
    Deque a;
    Deque b;

    a.swap(b);

    BOOST_TEST(a.empty());
    BOOST_TEST(b.empty());
  }

  {
    Deque a;
    Deque b = get_range<Deque>(4);

    a.swap(b);

    test_equal_range(a, {1, 2, 3, 4});
    BOOST_TEST(b.empty());
  }

  {
    Deque a = get_range<Deque>(5, 9, 9, 13);
    Deque b = get_range<Deque>(4);

    a.swap(b);

    test_equal_range(a, {1, 2, 3, 4});
    test_equal_range(b, {5, 6, 7, 8, 9, 10, 11, 12});
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(clear, Deque, all_deques)
{
  {
    Deque a;
    a.clear();
    BOOST_TEST(a.empty());
  }

  {
    Deque b = get_range<Deque>();
    b.clear();
    BOOST_TEST(b.empty());
  }

  {
    Deque c = get_range<Deque>();
    c.clear();
    c.emplace_back(1);
    c.emplace_back(2);
    c.emplace_back(3);
    c.emplace_back(4);
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(op_eq, Deque, all_deques)
{
  { // equal
    Deque a = get_range<Deque>(8);
    Deque b = get_range<Deque>(8);

    BOOST_TEST(a == b);
  }

  { // diff size
    Deque a = get_range<Deque>(8);
    Deque b = get_range<Deque>(9);

    BOOST_TEST(!(a == b));
  }

  { // diff content
    Deque a = get_range<Deque>(8);
    Deque b = get_range<Deque>(2,6,6,10);

    BOOST_TEST(!(a == b));
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(op_lt, Deque, all_deques)
{
  { // little than
    Deque a = get_range<Deque>(7);
    Deque b = get_range<Deque>(8);

    BOOST_TEST(a < b);
  }

  { // equal
    Deque a = get_range<Deque>(8);
    Deque b = get_range<Deque>(8);

    BOOST_TEST(!(a < b));
  }

  { // greater than
    Deque a = get_range<Deque>(8);
    Deque b = get_range<Deque>(7);

    BOOST_TEST(!(a < b));
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(op_ne, Deque, all_deques)
{
  { // equal
    Deque a = get_range<Deque>(8);
    Deque b = get_range<Deque>(8);

    BOOST_TEST(!(a != b));
  }

  { // diff size
    Deque a = get_range<Deque>(8);
    Deque b = get_range<Deque>(9);

    BOOST_TEST(a != b);
  }

  { // diff content
    Deque a = get_range<Deque>(8);
    Deque b = get_range<Deque>(2,6,6,10);

    BOOST_TEST(a != b);
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(op_gt, Deque, all_deques)
{
  { // little than
    Deque a = get_range<Deque>(7);
    Deque b = get_range<Deque>(8);

    BOOST_TEST(!(a > b));
  }

  { // equal
    Deque a = get_range<Deque>(8);
    Deque b = get_range<Deque>(8);

    BOOST_TEST(!(a > b));
  }

  { // greater than
    Deque a = get_range<Deque>(8);
    Deque b = get_range<Deque>(7);

    BOOST_TEST(a > b);
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(op_ge, Deque, all_deques)
{
  { // little than
    Deque a = get_range<Deque>(7);
    Deque b = get_range<Deque>(8);

    BOOST_TEST(!(a >= b));
  }

  { // equal
    Deque a = get_range<Deque>(8);
    Deque b = get_range<Deque>(8);

    BOOST_TEST(a >= b);
  }

  { // greater than
    Deque a = get_range<Deque>(8);
    Deque b = get_range<Deque>(7);

    BOOST_TEST(a >= b);
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(op_le, Deque, all_deques)
{
  { // little than
    Deque a = get_range<Deque>(7);
    Deque b = get_range<Deque>(8);

    BOOST_TEST(a <= b);
  }

  { // equal
    Deque a = get_range<Deque>(8);
    Deque b = get_range<Deque>(8);

    BOOST_TEST(a <= b);
  }

  { // greater than
    Deque a = get_range<Deque>(8);
    Deque b = get_range<Deque>(7);

    BOOST_TEST(!(a <= b));
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(free_swap, Deque, all_deques)
{
  using std::swap; // test the ADL trick

  {
    Deque a;
    Deque b;

    swap(a, b);

    BOOST_TEST(a.empty());
    BOOST_TEST(b.empty());
  }

  {
    Deque a;
    Deque b = get_range<Deque>(4);

    swap(a, b);

    test_equal_range(a, {1, 2, 3, 4});
    BOOST_TEST(b.empty());
  }

  {
    Deque a = get_range<Deque>(5, 9, 9, 13);
    Deque b = get_range<Deque>(4);

    swap(a, b);

    test_equal_range(a, {1, 2, 3, 4});
    test_equal_range(b, {5, 6, 7, 8, 9, 10, 11, 12});
  }
}
