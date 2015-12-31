//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Benedek Thaler 2015-2015. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://erenon.hu/double_ended for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#include <cstring> // memcmp
#include <iostream>
#include <vector>
#include <limits>
#include <forward_list>

#define BOOST_DOUBLE_ENDED_DEVECTOR_ALLOC_STATS
#include <boost/double_ended/devector.hpp>
#undef BOOST_DOUBLE_ENDED_DEVECTOR_ALLOC_STATS

#include <boost/algorithm/cxx14/equal.hpp>

#include "test_elem.hpp"
#include "input_iterator.hpp"

using namespace boost::double_ended;

#define BOOST_TEST_MODULE devector
#include <boost/test/included/unit_test.hpp>

#include <boost/mpl/list.hpp>
#include <boost/mpl/filter_view.hpp>
#include <boost/mpl/is_sequence.hpp>
#include <boost/mpl/placeholders.hpp>

template <typename T>
using small_devector = devector<T, devector_small_buffer_policy<16>>;

template <typename T>
struct different_allocator : public std::allocator<T>
{
  bool operator==(const different_allocator&) const { return false; }

  using propagate_on_container_copy_assignment = std::true_type;
  using propagate_on_container_move_assignment = std::false_type;
  using propagate_on_container_swap = std::true_type;
};

template <typename SizeType>
struct different_growth_policy
{
  typedef SizeType size_type;

  static size_type new_capacity(size_type capacity)
  {
    return (capacity) ? capacity * 4u : 32u;
  }

  static bool should_shrink(size_type, size_type, size_type)
  {
    return false;
  }
};

#if 1
typedef boost::mpl::list<
  devector<unsigned>,
  devector<regular_elem>,
  devector<noex_move>,
  devector<noex_copy>,
  devector<only_movable>,
  devector<no_default_ctor>,

  small_devector<unsigned>,
  small_devector<regular_elem>,
  small_devector<noex_move>,
  small_devector<noex_copy>,
  small_devector<only_movable>,
  small_devector<no_default_ctor>,

  devector<unsigned, devector_small_buffer_policy<0>, devector_growth_policy, different_allocator<unsigned>>,
  devector<unsigned, devector_small_buffer_policy<16>, devector_growth_policy, different_allocator<unsigned>>,

  devector<unsigned, devector_small_buffer_policy<8>, different_growth_policy<std::size_t>>
> all_devectors;
#else
typedef boost::mpl::list<
  devector<regular_elem>
> all_devectors;
#endif

template <template<typename> class Predicate>
struct if_value_type
{
  template <typename Container>
  struct apply : Predicate<typename Container::value_type> {};
};

typedef boost::mpl::filter_view<all_devectors, if_value_type<std::is_default_constructible>>::type
  t_is_default_constructible;

typedef boost::mpl::filter_view<all_devectors, if_value_type<std::is_copy_constructible>>::type
  t_is_copy_constructible;

template <typename Container>
Container get_range(int count)
{
  Container c;
  c.reserve(count);

  for (int i = 1; i <= count; ++i)
  {
    c.emplace_back(i);
  }

  return c;
}

template <typename Devector>
Devector get_range(int fbeg, int fend, int bbeg, int bend)
{
  Devector c(fend - fbeg, bend - bbeg, reserve_only_tag{});

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

template <class T, class SBP, class GP, class A>
std::ostream& operator<<(std::ostream& out, const devector<T, SBP, GP, A>& devec)
{
  print_range(out, devec);
  return out;
}

template <typename T>
std::ostream& operator<<(std::ostream& out, const std::vector<T>& vec)
{
  print_range(out, vec);
  return out;
}

template <typename Devector>
void assert_equals(const Devector& actual, const std::vector<typename Devector::value_type>& expected)
{
  bool equals = boost::algorithm::equal(actual.begin(), actual.end(), expected.begin(), expected.end());

  if (!equals)
  {
    std::cerr << actual << " != " << expected << " (actual != expected)" << std::endl;

    BOOST_TEST(false);
  }
}

template <typename Devector>
void assert_equals(const Devector& actual, std::initializer_list<unsigned> inits)
{
  using T = typename Devector::value_type;
  std::vector<T> expected;
  for (unsigned init : inits)
  {
    expected.push_back(T(init));
  }

  assert_equals(actual, expected);
}

template <typename>
struct small_buffer_size;

template <typename U, typename SBP, typename GP, typename A>
struct small_buffer_size<devector<U, SBP, GP, A>>
{
  static constexpr unsigned value = SBP::size;
};

// END HELPERS

BOOST_AUTO_TEST_CASE_TEMPLATE(constructor_default, Devector, all_devectors)
{
  Devector a;

  BOOST_TEST(a.empty());
  BOOST_TEST(a.capacity_alloc_count == 0u);
  BOOST_TEST(a.capacity() == unsigned{small_buffer_size<Devector>::value});
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_constructor_allocator, Devector, all_devectors)
{
  typename Devector::allocator_type alloc_template;

  Devector a(alloc_template);

  BOOST_TEST(a.empty());
  BOOST_TEST(a.capacity_alloc_count == 0u);
  BOOST_TEST(a.capacity() == unsigned{small_buffer_size<Devector>::value});
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_constructor_reserve_only, Devector, all_devectors)
{
  {
    Devector a(16, reserve_only_tag{});
    BOOST_TEST(a.size() == 0u);
    BOOST_TEST(a.capacity() >= 16u);
  }

  {
    Devector b(0, reserve_only_tag{});
    BOOST_TEST(b.capacity_alloc_count == 0u);
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_constructor_reserve_only_front_back, Devector, all_devectors)
{
  {
    Devector a(8, 8, reserve_only_tag{});
    BOOST_TEST(a.size() == 0u);
    BOOST_TEST(a.capacity() >= 16u);

    for (int i = 8; i; --i)
    {
      a.emplace_front(i);
    }

    for (int i = 9; i < 17; ++i)
    {
      a.emplace_back(i);
    }

    assert_equals(a, {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16});
    BOOST_TEST(a.capacity_alloc_count <= 1u);
  }

  {
    Devector b(0, 0, reserve_only_tag{});
    BOOST_TEST(b.capacity_alloc_count == 0u);
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_constructor_unsafe_uninitialized, Devector, all_devectors)
{
  using T = typename Devector::value_type;

  {
    Devector a(8, unsafe_uninitialized_tag{});
    BOOST_TEST(a.size() == 8u);

    for (int i = 0; i < 8; ++i)
    {
      new (a.data() + i) T(i+1);
    }

    assert_equals(a, {1, 2, 3, 4, 5, 6, 7, 8});
  }

  {
    Devector b(0, unsafe_uninitialized_tag{});
    BOOST_TEST(b.capacity_alloc_count == 0u);
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_constructor_n, Devector, t_is_default_constructible)
{
  using T = typename Devector::value_type;

  {
    Devector a(8);

    assert_equals(a, {0, 0, 0, 0, 0, 0, 0, 0});
  }

  {
    Devector b(0);

    assert_equals(b, {});
    BOOST_TEST(b.capacity_alloc_count == 0u);
  }

  if (! std::is_nothrow_constructible<T>::value)
  {
    test_elem_throw::on_ctor_after(4);

    BOOST_CHECK_THROW(Devector a(8), test_exception);

    BOOST_TEST(test_elem_base::no_living_elem());
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_constructor_n_copy, Devector, t_is_copy_constructible)
{
  using T = typename Devector::value_type;

  {
    const T x(9);
    Devector a(8, x);

    assert_equals(a, {9, 9, 9, 9, 9, 9, 9, 9});
  }

  {
    const T x(9);
    Devector b(0, x);

    assert_equals(b, {});
    BOOST_TEST(b.capacity_alloc_count == 0u);
  }

  if (! std::is_nothrow_copy_constructible<T>::value)
  {
    test_elem_throw::on_copy_after(4);
    const T x(404);

    BOOST_CHECK_THROW(Devector a(8, x), test_exception);
  }

  BOOST_TEST(test_elem_base::no_living_elem());
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_constructor_input_range, Devector, t_is_copy_constructible)
{
  using T = typename Devector::value_type;

  {
    const devector<T> expected = get_range<devector<T>>(16);
    devector<T> input = expected;

    auto input_begin = make_input_iterator(input, input.begin());
    auto input_end   = make_input_iterator(input, input.end());

    Devector a(input_begin, input_end);
    BOOST_TEST(a == expected);
  }

  { // empty range
    devector<T> input;
    auto input_begin = make_input_iterator(input, input.begin());

    Devector b(input_begin, input_begin);

    assert_equals(b, {});
    BOOST_TEST(b.capacity_alloc_count == 0u);
  }

  BOOST_TEST(test_elem_base::no_living_elem());

  if (! std::is_nothrow_copy_constructible<T>::value)
  {
    devector<T> input = get_range<devector<T>>(16);

    auto input_begin = make_input_iterator(input, input.begin());
    auto input_end   = make_input_iterator(input, input.end());

    test_elem_throw::on_copy_after(4);

    BOOST_CHECK_THROW(Devector c(input_begin, input_end), test_exception);
  }

  BOOST_TEST(test_elem_base::no_living_elem());
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_constructor_forward_range, Devector, t_is_copy_constructible)
{
  using T = typename Devector::value_type;

  const std::forward_list<T> x{1, 2, 3, 4, 5, 6, 7, 8};

  {
    Devector a(x.begin(), x.end());

    assert_equals(a, {1, 2, 3, 4, 5, 6, 7, 8});
    BOOST_TEST(a.capacity_alloc_count <= 1u);
  }

  {
    Devector b(x.begin(), x.begin());

    assert_equals(b, {});
    BOOST_TEST(b.capacity_alloc_count == 0u);
  }

  if (! std::is_nothrow_copy_constructible<T>::value)
  {
    test_elem_throw::on_copy_after(4);

    BOOST_CHECK_THROW(Devector c(x.begin(), x.end()), test_exception);
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_constructor_pointer_range, Devector, t_is_copy_constructible)
{
  using T = typename Devector::value_type;

  const std::vector<T> x = get_range<std::vector<T>>(8);
  const T* xbeg = x.data();
  const T* xend = x.data() + x.size();

  {
    Devector a(xbeg, xend);

    assert_equals(a, {1, 2, 3, 4, 5, 6, 7, 8});
    BOOST_TEST(a.capacity_alloc_count <= 1u);
  }

  {
    Devector b(xbeg, xbeg);

    assert_equals(b, {});
    BOOST_TEST(b.capacity_alloc_count == 0u);
  }

  if (! std::is_nothrow_copy_constructible<T>::value)
  {
    test_elem_throw::on_copy_after(4);

    BOOST_CHECK_THROW(Devector c(xbeg, xend), test_exception);
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_copy_constructor, Devector, t_is_copy_constructible)
{
  using T = typename Devector::value_type;

  {
    Devector a;
    Devector b(a);

    assert_equals(b, {});
    BOOST_TEST(b.capacity_alloc_count == 0u);
  }

  {
    Devector a = get_range<Devector>(8);
    Devector b(a);

    assert_equals(b, {1, 2, 3, 4, 5, 6, 7, 8});
    BOOST_TEST(b.capacity_alloc_count <= 1u);
  }

  if (! std::is_nothrow_copy_constructible<T>::value)
  {
    Devector a = get_range<Devector>(8);

    test_elem_throw::on_copy_after(4);

    BOOST_CHECK_THROW(Devector b(a), test_exception);
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_move_constructor, Devector, all_devectors)
{
  using T = typename Devector::value_type;

  { // empty
    Devector a;
    Devector b(std::move(a));

    BOOST_TEST(a.empty());
    BOOST_TEST(b.empty());
  }

  { // maybe small
    Devector a = get_range<Devector>(1, 5, 5, 9);
    Devector b(std::move(a));

    assert_equals(b, {1, 2, 3, 4, 5, 6, 7, 8});

    // a is unspecified but valid state
    a.clear();
    BOOST_TEST(a.empty());
  }

  { // big
    Devector a = get_range<Devector>(32);
    Devector b(std::move(a));

    std::vector<T> exp = get_range<std::vector<T>>(32);
    assert_equals(b, exp);

    // a is unspecified but valid state
    a.clear();
    BOOST_TEST(a.empty());
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_destructor, Devector, all_devectors)
{
  Devector a;

  Devector b = get_range<Devector>(3);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_assignment, Devector, t_is_copy_constructible)
{
  using T = typename Devector::value_type;

  { // assign to empty (maybe small)
    Devector a;
    const Devector b = get_range<Devector>(6);

    a = b;

    assert_equals(a, {1, 2, 3, 4, 5, 6});
  }

  { // assign from empty
    Devector a = get_range<Devector>(6);
    const Devector b;

    a = b;

    assert_equals(a, {});
  }

  { // assign to non-empty
    Devector a = get_range<Devector>(11, 15, 15, 19);
    const Devector b = get_range<Devector>(6);

    a = b;

    assert_equals(a, {1, 2, 3, 4, 5, 6});
  }

  { // assign to free front
    Devector a = get_range<Devector>(11, 15, 15, 19);
    a.reserve_front(8);
    a.reset_alloc_stats();

    const Devector b = get_range<Devector>(6);

    a = b;

    assert_equals(a, {1, 2, 3, 4, 5, 6});
    BOOST_TEST(a.capacity_alloc_count == 0u);
  }

  { // assignment overlaps contents
    Devector a = get_range<Devector>(11, 15, 15, 19);
    a.reserve_front(12);
    a.reset_alloc_stats();

    const Devector b = get_range<Devector>(6);

    a = b;

    assert_equals(a, {1, 2, 3, 4, 5, 6});
    BOOST_TEST(a.capacity_alloc_count == 0u);
  }

  { // assignment exceeds contents
    Devector a = get_range<Devector>(11, 13, 13, 15);
    a.reserve_front(8);
    a.reserve_back(8);
    a.reset_alloc_stats();

    const Devector b = get_range<Devector>(12);

    a = b;

    assert_equals(a, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12});
    BOOST_TEST(a.capacity_alloc_count == 0u);
  }

  if (! std::is_nothrow_copy_constructible<T>::value)
  {
    // strong guarantee if reallocation is needed (no guarantee otherwise)
    Devector a = get_range<Devector>(6);
    const Devector b = get_range<Devector>(12);

    test_elem_throw::on_copy_after(3);

    BOOST_CHECK_THROW(a = b, test_exception);

    assert_equals(a, {1, 2, 3, 4, 5, 6});
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_move_assignment, Devector, all_devectors)
{
  using T = typename Devector::value_type;

  { // assign to empty (maybe small)
    Devector a;
    Devector b = get_range<Devector>(6);

    a = std::move(b);

    assert_equals(a, {1, 2, 3, 4, 5, 6});

    // b is in unspecified but valid state
    b.clear();
    assert_equals(b, {});
  }

  { // assign from empty
    Devector a = get_range<Devector>(6);
    Devector b;

    a = std::move(b);

    assert_equals(a, {});
    assert_equals(b, {});
  }

  { // assign to non-empty
    Devector a = get_range<Devector>(11, 15, 15, 19);
    Devector b = get_range<Devector>(6);

    a = std::move(b);

    assert_equals(a, {1, 2, 3, 4, 5, 6});

    b.clear();
    assert_equals(b, {});
  }

  // move should be used on the slow path
  if (std::is_nothrow_move_constructible<T>::value)
  {
    Devector a = get_range<Devector>(11, 15, 15, 19);
    Devector b = get_range<Devector>(6);

    test_elem_throw::on_copy_after(3);

    a = std::move(b);

    test_elem_throw::do_not_throw();

    assert_equals(a, {1, 2, 3, 4, 5, 6});

    b.clear();
    assert_equals(b, {});
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_il_assignment, Devector, t_is_copy_constructible)
{
  using T = typename Devector::value_type;

  { // assign to empty (maybe small)
    Devector a;

    a = {1, 2, 3, 4, 5, 6};

    assert_equals(a, {1, 2, 3, 4, 5, 6});
  }

  { // assign from empty
    Devector a = get_range<Devector>(6);

    a = {};

    assert_equals(a, {});
  }

  { // assign to non-empty
    Devector a = get_range<Devector>(11, 15, 15, 19);

    a = {1, 2, 3, 4, 5, 6};

    assert_equals(a, {1, 2, 3, 4, 5, 6});
  }

  { // assign to free front
    Devector a = get_range<Devector>(11, 15, 15, 19);
    a.reserve_front(8);
    a.reset_alloc_stats();

    a = {1, 2, 3, 4, 5, 6};

    assert_equals(a, {1, 2, 3, 4, 5, 6});
    BOOST_TEST(a.capacity_alloc_count == 0u);
  }

  { // assignment overlaps contents
    Devector a = get_range<Devector>(11, 15, 15, 19);
    a.reserve_front(12);
    a.reset_alloc_stats();

    a = {1, 2, 3, 4, 5, 6};

    assert_equals(a, {1, 2, 3, 4, 5, 6});
    BOOST_TEST(a.capacity_alloc_count == 0u);
  }

  { // assignment exceeds contents
    Devector a = get_range<Devector>(11, 13, 13, 15);
    a.reserve_front(8);
    a.reserve_back(8);
    a.reset_alloc_stats();

    a = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};

    assert_equals(a, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12});
    BOOST_TEST(a.capacity_alloc_count == 0u);
  }

  if (! std::is_nothrow_copy_constructible<T>::value)
  {
    // strong guarantee if reallocation is needed (no guarantee otherwise)
    Devector a = get_range<Devector>(6);

    test_elem_throw::on_copy_after(3);

    try
    {
      a = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
      BOOST_TEST(false);
    } catch(const test_exception&) {}

    assert_equals(a, {1, 2, 3, 4, 5, 6});
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_assign_input_range, Devector, t_is_copy_constructible)
{
  using T = typename Devector::value_type;

  { // assign to empty, keep it small
    const devector<T> expected = get_range<devector<T>>(small_buffer_size<Devector>::value);
    devector<T> input = expected;

    auto input_begin = make_input_iterator(input, input.begin());
    auto input_end   = make_input_iterator(input, input.end());

    Devector a;

    a.assign(input_begin, input_end);

    BOOST_TEST(a == expected);
    BOOST_TEST(a.capacity_alloc_count == 0u);
  }

  { // assign to empty (maybe small)
    devector<T> input = get_range<devector<T>>(6);

    auto input_begin = make_input_iterator(input, input.begin());
    auto input_end   = make_input_iterator(input, input.end());

    Devector a;

    a.assign(input_begin, input_end);

    assert_equals(a, {1, 2, 3, 4, 5, 6});
  }

  { // assign from empty
    devector<T> input = get_range<devector<T>>(6);
    auto input_begin = make_input_iterator(input, input.begin());

    Devector a = get_range<Devector>(6);
    a.assign(input_begin, input_begin);

    assert_equals(a, {});
  }

  { // assign to non-empty
    devector<T> input = get_range<devector<T>>(6);
    auto input_begin = make_input_iterator(input, input.begin());
    auto input_end   = make_input_iterator(input, input.end());

    Devector a = get_range<Devector>(11, 15, 15, 19);
    a.assign(input_begin, input_end);

    assert_equals(a, {1, 2, 3, 4, 5, 6});
  }

  { // assign to free front
    devector<T> input = get_range<devector<T>>(6);
    auto input_begin = make_input_iterator(input, input.begin());
    auto input_end   = make_input_iterator(input, input.end());

    Devector a = get_range<Devector>(11, 15, 15, 19);
    a.reserve_front(8);
    a.reset_alloc_stats();

    a.assign(input_begin, input_end);

    assert_equals(a, {1, 2, 3, 4, 5, 6});
    BOOST_TEST(a.capacity_alloc_count == 0u);
  }

  { // assignment overlaps contents
    devector<T> input = get_range<devector<T>>(6);
    auto input_begin = make_input_iterator(input, input.begin());
    auto input_end   = make_input_iterator(input, input.end());

    Devector a = get_range<Devector>(11, 15, 15, 19);
    a.reserve_front(12);
    a.reset_alloc_stats();

    a.assign(input_begin, input_end);

    assert_equals(a, {1, 2, 3, 4, 5, 6});
    BOOST_TEST(a.capacity_alloc_count == 0u);
  }

  { // assignment exceeds contents
    devector<T> input = get_range<devector<T>>(12);
    auto input_begin = make_input_iterator(input, input.begin());
    auto input_end   = make_input_iterator(input, input.end());

    Devector a = get_range<Devector>(11, 13, 13, 15);
    a.reserve_front(8);
    a.reserve_back(8);
    a.reset_alloc_stats();

    a.assign(input_begin, input_end);

    assert_equals(a, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12});
    BOOST_TEST(a.capacity_alloc_count == 0u);
  }

  if (! std::is_nothrow_copy_constructible<T>::value)
  {
    // strong guarantee if reallocation is needed (no guarantee otherwise)

    devector<T> input = get_range<devector<T>>(12);
    auto input_begin = make_input_iterator(input, input.begin());
    auto input_end   = make_input_iterator(input, input.end());

    Devector a = get_range<Devector>(6);

    test_elem_throw::on_copy_after(3);

    BOOST_CHECK_THROW(a.assign(input_begin, input_end), test_exception);

    assert_equals(a, {1, 2, 3, 4, 5, 6});
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_assign_forward_range, Devector, t_is_copy_constructible)
{
  using T = typename Devector::value_type;

  const std::forward_list<T> x{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
  auto one = x.begin();
  auto six = one;
  auto twelve = one;

  std::advance(six, 6);
  std::advance(twelve, 12);

  { // assign to empty (maybe small)
    Devector a;

    a.assign(one, six);

    assert_equals(a, {1, 2, 3, 4, 5, 6});
  }

  { // assign from empty
    Devector a = get_range<Devector>(6);

    a.assign(one, one);

    assert_equals(a, {});
  }

  { // assign to non-empty
    Devector a = get_range<Devector>(11, 15, 15, 19);

    a.assign(one, six);

    assert_equals(a, {1, 2, 3, 4, 5, 6});
  }

  { // assign to free front
    Devector a = get_range<Devector>(11, 15, 15, 19);
    a.reserve_front(8);
    a.reset_alloc_stats();

    a.assign(one, six);

    assert_equals(a, {1, 2, 3, 4, 5, 6});
    BOOST_TEST(a.capacity_alloc_count == 0u);
  }

  { // assignment overlaps contents
    Devector a = get_range<Devector>(11, 15, 15, 19);
    a.reserve_front(12);
    a.reset_alloc_stats();

    a.assign(one, six);

    assert_equals(a, {1, 2, 3, 4, 5, 6});
    BOOST_TEST(a.capacity_alloc_count == 0u);
  }

  { // assignment exceeds contents
    Devector a = get_range<Devector>(11, 13, 13, 15);
    a.reserve_front(8);
    a.reserve_back(8);
    a.reset_alloc_stats();

    a.assign(one, twelve);

    assert_equals(a, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12});
    BOOST_TEST(a.capacity_alloc_count == 0u);
  }

  if (! std::is_nothrow_copy_constructible<T>::value)
  {
    // strong guarantee if reallocation is needed (no guarantee otherwise)
    Devector a = get_range<Devector>(6);

    test_elem_throw::on_copy_after(3);

    BOOST_CHECK_THROW(a.assign(one, twelve), test_exception);

    assert_equals(a, {1, 2, 3, 4, 5, 6});
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_assign_pointer_range, Devector, t_is_copy_constructible)
{
  using T = typename Devector::value_type;

  const std::vector<T> x = get_range<std::vector<T>>(12);
  const T* one = x.data();
  const T* six = one + 6;
  const T* twelve = one + 12;

  { // assign to empty (maybe small)
    Devector a;

    a.assign(one, six);

    assert_equals(a, {1, 2, 3, 4, 5, 6});
  }

  { // assign from empty
    Devector a = get_range<Devector>(6);

    a.assign(one, one);

    assert_equals(a, {});
  }

  { // assign to non-empty
    Devector a = get_range<Devector>(11, 15, 15, 19);

    a.assign(one, six);

    assert_equals(a, {1, 2, 3, 4, 5, 6});
  }

  { // assign to free front
    Devector a = get_range<Devector>(11, 15, 15, 19);
    a.reserve_front(8);
    a.reset_alloc_stats();

    a.assign(one, six);

    assert_equals(a, {1, 2, 3, 4, 5, 6});
    BOOST_TEST(a.capacity_alloc_count == 0u);
  }

  { // assignment overlaps contents
    Devector a = get_range<Devector>(11, 15, 15, 19);
    a.reserve_front(12);
    a.reset_alloc_stats();

    a.assign(one, six);

    assert_equals(a, {1, 2, 3, 4, 5, 6});
    BOOST_TEST(a.capacity_alloc_count == 0u);
  }

  { // assignment exceeds contents
    Devector a = get_range<Devector>(11, 13, 13, 15);
    a.reserve_front(8);
    a.reserve_back(8);
    a.reset_alloc_stats();

    a.assign(one, twelve);

    assert_equals(a, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12});
    BOOST_TEST(a.capacity_alloc_count == 0u);
  }

  if (! std::is_nothrow_copy_constructible<T>::value)
  {
    // strong guarantee if reallocation is needed (no guarantee otherwise)
    Devector a = get_range<Devector>(6);

    test_elem_throw::on_copy_after(3);

    BOOST_CHECK_THROW(a.assign(one, twelve), test_exception);

    assert_equals(a, {1, 2, 3, 4, 5, 6});
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_assign_n, Devector, t_is_copy_constructible)
{
  using T = typename Devector::value_type;

  { // assign to empty (maybe small)
    Devector a;

    a.assign(6, T(9));

    assert_equals(a, {9, 9, 9, 9, 9, 9});
  }

  { // assign from empty
    Devector a = get_range<Devector>(6);

    a.assign(0, T(404));

    assert_equals(a, {});
  }

  { // assign to non-empty
    Devector a = get_range<Devector>(11, 15, 15, 19);

    a.assign(6, T(9));

    assert_equals(a, {9, 9, 9, 9, 9, 9});
  }

  { // assign to free front
    Devector a = get_range<Devector>(11, 15, 15, 19);
    a.reserve_front(8);
    a.reset_alloc_stats();

    a.assign(6, T(9));

    assert_equals(a, {9, 9, 9, 9, 9, 9});
    BOOST_TEST(a.capacity_alloc_count == 0u);
  }

  { // assignment overlaps contents
    Devector a = get_range<Devector>(11, 15, 15, 19);
    a.reserve_front(12);
    a.reset_alloc_stats();

    a.assign(6, T(9));

    assert_equals(a, {9, 9, 9, 9, 9, 9});
    BOOST_TEST(a.capacity_alloc_count == 0u);
  }

  { // assignment exceeds contents
    Devector a = get_range<Devector>(11, 13, 13, 15);
    a.reserve_front(8);
    a.reserve_back(8);
    a.reset_alloc_stats();

    a.assign(12, T(9));

    assert_equals(a, {9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9});
    BOOST_TEST(a.capacity_alloc_count == 0u);
  }

  if (! std::is_nothrow_copy_constructible<T>::value)
  {
    // strong guarantee if reallocation is needed (no guarantee otherwise)
    Devector a = get_range<Devector>(6);

    test_elem_throw::on_copy_after(3);

    BOOST_CHECK_THROW(a.assign(32, T(9)), test_exception);

    assert_equals(a, {1, 2, 3, 4, 5, 6});
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_assign_il, Devector, t_is_copy_constructible)
{
  using T = typename Devector::value_type;

  { // assign to empty (maybe small)
    Devector a;

    a.assign({1, 2, 3, 4, 5, 6});

    assert_equals(a, {1, 2, 3, 4, 5, 6});
  }

  { // assign from empty
    Devector a = get_range<Devector>(6);

    a.assign({});

    assert_equals(a, {});
  }

  { // assign to non-empty
    Devector a = get_range<Devector>(11, 15, 15, 19);

    a.assign({1, 2, 3, 4, 5, 6});

    assert_equals(a, {1, 2, 3, 4, 5, 6});
  }

  { // assign to free front
    Devector a = get_range<Devector>(11, 15, 15, 19);
    a.reserve_front(8);
    a.reset_alloc_stats();

    a.assign({1, 2, 3, 4, 5, 6});

    assert_equals(a, {1, 2, 3, 4, 5, 6});
    BOOST_TEST(a.capacity_alloc_count == 0u);
  }

  { // assignment overlaps contents
    Devector a = get_range<Devector>(11, 15, 15, 19);
    a.reserve_front(12);
    a.reset_alloc_stats();

    a.assign({1, 2, 3, 4, 5, 6});

    assert_equals(a, {1, 2, 3, 4, 5, 6});
    BOOST_TEST(a.capacity_alloc_count == 0u);
  }

  { // assignment exceeds contents
    Devector a = get_range<Devector>(11, 13, 13, 15);
    a.reserve_front(8);
    a.reserve_back(8);
    a.reset_alloc_stats();

    a.assign({1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12});

    assert_equals(a, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12});
    BOOST_TEST(a.capacity_alloc_count == 0u);
  }

  if (! std::is_nothrow_copy_constructible<T>::value)
  {
    // strong guarantee if reallocation is needed (no guarantee otherwise)
    Devector a = get_range<Devector>(6);

    test_elem_throw::on_copy_after(3);

    BOOST_CHECK_THROW(a.assign({1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18}), test_exception);

    assert_equals(a, {1, 2, 3, 4, 5, 6});
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_begin_end, Devector, all_devectors)
{
  using T = typename Devector::value_type;

  std::vector<T> expected = get_range<std::vector<T>>(10);
  Devector actual = get_range<Devector>(10);

  BOOST_TEST(boost::algorithm::equal(expected.begin(), expected.end(), actual.begin(), actual.end()));
  BOOST_TEST(boost::algorithm::equal(expected.rbegin(), expected.rend(), actual.rbegin(), actual.rend()));
  BOOST_TEST(boost::algorithm::equal(expected.cbegin(), expected.cend(), actual.cbegin(), actual.cend()));
  BOOST_TEST(boost::algorithm::equal(expected.crbegin(), expected.crend(), actual.crbegin(), actual.crend()));

  const Devector cactual = get_range<Devector>(10);

  BOOST_TEST(boost::algorithm::equal(expected.begin(), expected.end(), cactual.begin(), cactual.end()));
  BOOST_TEST(boost::algorithm::equal(expected.rbegin(), expected.rend(), cactual.rbegin(), cactual.rend()));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_empty, Devector, all_devectors)
{
  using T = typename Devector::value_type;

  Devector a;
  BOOST_TEST(a.empty());

  a.push_front(T(1));
  BOOST_TEST(! a.empty());

  a.pop_back();
  BOOST_TEST(a.empty());

  Devector b(16, reserve_only_tag{});
  BOOST_TEST(b.empty());

  Devector c = get_range<Devector>(3);
  BOOST_TEST(! c.empty());
}

template <typename ST>
using gp_devector = devector<unsigned, devector_small_buffer_policy<8>, different_growth_policy<ST>>;

BOOST_AUTO_TEST_CASE(test_max_size)
{
  gp_devector<unsigned char> a;
  BOOST_TEST(a.max_size() == (std::numeric_limits<unsigned char>::max)());

  gp_devector<unsigned short> b;
  BOOST_TEST(b.max_size() == (std::numeric_limits<unsigned short>::max)());

  gp_devector<unsigned int> c;
  BOOST_TEST(c.max_size() >= b.max_size());

  gp_devector<unsigned long long> d;
  BOOST_TEST(d.max_size() >= c.max_size());
}

BOOST_AUTO_TEST_CASE(test_exceeding_max_size)
{
  using Devector = gp_devector<unsigned char>;

  Devector a((std::numeric_limits<typename Devector::size_type>::max)());
  BOOST_CHECK_THROW(a.emplace_back(404), std::length_error);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_size, Devector, all_devectors)
{
  using T = typename Devector::value_type;

  Devector a;
  BOOST_TEST(a.size() == 0u);

  a.push_front(T(1));
  BOOST_TEST(a.size() == 1u);

  a.pop_back();
  BOOST_TEST(a.size() == 0u);

  Devector b(16, reserve_only_tag{});
  BOOST_TEST(b.size() == 0u);

  Devector c = get_range<Devector>(3);
  BOOST_TEST(c.size() == 3u);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_capacity, Devector, all_devectors)
{
  Devector a;
  BOOST_TEST(a.capacity() == unsigned{small_buffer_size<Devector>::value});

  Devector b(128, reserve_only_tag{});
  BOOST_TEST(b.capacity() >= 128u);

  Devector c = get_range<Devector>(10);
  BOOST_TEST(c.capacity() >= 10u);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_resize_front, Devector, t_is_default_constructible)
{
  using T = typename Devector::value_type;

  // size < required, alloc needed
  {
    Devector a = get_range<Devector>(5);
    a.resize_front(8);
    assert_equals(a, {0, 0, 0, 1, 2, 3, 4, 5});
  }

  // size < required, but capacity provided
  {
    Devector b = get_range<Devector>(5);
    b.reserve_front(16);
    b.resize_front(8);
    assert_equals(b, {0, 0, 0, 1, 2, 3, 4, 5});
  }

  // size < required, move would throw
  if (! std::is_nothrow_move_constructible<T>::value && std::is_copy_constructible<T>::value)
  {
    Devector c = get_range<Devector>(5);
    test_elem_throw::on_move_after(3);

    c.resize_front(8); // shouldn't use the throwing move

    test_elem_throw::do_not_throw();
    assert_equals(c, {0, 0, 0, 1, 2, 3, 4, 5});
  }

  // size < required, constructor throws
  if (! std::is_nothrow_constructible<T>::value)
  {
    Devector d = get_range<Devector>(5);
    std::vector<T> d_origi = get_range<std::vector<T>>(5);
    auto origi_begin = d.begin();
    test_elem_throw::on_ctor_after(3);

    BOOST_CHECK_THROW(d.resize_front(256), test_exception);

    assert_equals(d, d_origi);
    BOOST_TEST(origi_begin == d.begin());
  }

  // size >= required
  {
    Devector e = get_range<Devector>(6);
    e.resize_front(4);
    assert_equals(e, {3, 4, 5, 6});
  }

  // size < required, does not fit front small buffer
  {
    std::vector<T> expected(128);
    Devector g;
    g.resize_front(128);
    assert_equals(g, expected);
  }

  // size = required
  {
    Devector e = get_range<Devector>(6);
    e.resize_front(6);
    assert_equals(e, {1, 2, 3, 4, 5, 6});
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_resize_front_copy, Devector, t_is_copy_constructible)
{
  using T = typename Devector::value_type;

  // size < required, alloc needed
  {
    Devector a = get_range<Devector>(5);
    a.resize_front(8, T(9));
    assert_equals(a, {9, 9, 9, 1, 2, 3, 4, 5});
  }

  // size < required, but capacity provided
  {
    Devector b = get_range<Devector>(5);
    b.reserve_front(16);
    b.resize_front(8, T(9));
    assert_equals(b, {9, 9, 9, 1, 2, 3, 4, 5});
  }

  // size < required, copy throws
  if (! std::is_nothrow_copy_constructible<T>::value)
  {
    Devector c = get_range<Devector>(5);
    std::vector<T> c_origi = get_range<std::vector<T>>(5);
    test_elem_throw::on_copy_after(3);

    BOOST_CHECK_THROW(c.resize_front(256, T(404)), test_exception);

    assert_equals(c, c_origi);
  }

  // size < required, copy throws, but later
  if (! std::is_nothrow_copy_constructible<T>::value)
  {
    Devector c = get_range<Devector>(5);
    std::vector<T> c_origi = get_range<std::vector<T>>(5);
    auto origi_begin = c.begin();
    test_elem_throw::on_copy_after(7);

    BOOST_CHECK_THROW(c.resize_front(256, T(404)), test_exception);

    assert_equals(c, c_origi);
    BOOST_TEST(origi_begin == c.begin());
  }

  // size >= required
  {
    Devector e = get_range<Devector>(6);
    e.resize_front(4, T(404));
    assert_equals(e, {3, 4, 5, 6});
  }

  // size < required, does not fit front small buffer
  {
    std::vector<T> expected(128, T(9));
    Devector g;
    g.resize_front(128, T(9));
    assert_equals(g, expected);
  }

  // size = required
  {
    Devector e = get_range<Devector>(6);
    e.resize_front(6, T(9));
    assert_equals(e, {1, 2, 3, 4, 5, 6});
  }

  // size < required, tmp is already inserted
  {
    Devector f = get_range<Devector>(8);
    const T& tmp = *(f.begin() + 1);
    f.resize_front(16, tmp);
    assert_equals(f, {2,2,2,2,2,2,2,2,1,2,3,4,5,6,7,8});
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_resize_back, Devector, t_is_default_constructible)
{
  using T = typename Devector::value_type;

  // size < required, alloc needed
  {
    Devector a = get_range<Devector>(5);
    a.resize_back(8);
    assert_equals(a, {1, 2, 3, 4, 5, 0, 0, 0});
  }

  // size < required, but capacity provided
  {
    Devector b = get_range<Devector>(5);
    b.reserve_back(16);
    b.resize_back(8);
    assert_equals(b, {1, 2, 3, 4, 5, 0, 0, 0});
  }

  // size < required, move would throw
  if (! std::is_nothrow_move_constructible<T>::value && std::is_copy_constructible<T>::value)
  {
    Devector c = get_range<Devector>(5);
    test_elem_throw::on_move_after(3);

    c.resize_back(8); // shouldn't use the throwing move

    test_elem_throw::do_not_throw();
    assert_equals(c, {1, 2, 3, 4, 5, 0, 0, 0});
  }

  // size < required, constructor throws
  if (! std::is_nothrow_constructible<T>::value)
  {
    Devector d = get_range<Devector>(5);
    std::vector<T> d_origi = get_range<std::vector<T>>(5);
    auto origi_begin = d.begin();
    test_elem_throw::on_ctor_after(3);

    BOOST_CHECK_THROW(d.resize_back(256), test_exception);

    assert_equals(d, d_origi);
    BOOST_TEST(origi_begin == d.begin());
  }

  // size >= required
  {
    Devector e = get_range<Devector>(6);
    e.resize_back(4);
    assert_equals(e, {1, 2, 3, 4});
  }

  // size < required, does not fit front small buffer
  {
    std::vector<T> expected(128);
    Devector g;
    g.resize_back(128);
    assert_equals(g, expected);
  }

  // size = required
  {
    Devector e = get_range<Devector>(6);
    e.resize_back(6);
    assert_equals(e, {1, 2, 3, 4, 5, 6});
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_resize_back_copy, Devector, t_is_copy_constructible)
{
  using T = typename Devector::value_type;

  // size < required, alloc needed
  {
    Devector a = get_range<Devector>(5);
    a.resize_back(8, T(9));
    assert_equals(a, {1, 2, 3, 4, 5, 9, 9, 9});
  }

  // size < required, but capacity provided
  {
    Devector b = get_range<Devector>(5);
    b.reserve_back(16);
    b.resize_back(8, T(9));
    assert_equals(b, {1, 2, 3, 4, 5, 9, 9, 9});
  }

  // size < required, copy throws
  if (! std::is_nothrow_copy_constructible<T>::value)
  {
    Devector c = get_range<Devector>(5);
    std::vector<T> c_origi = get_range<std::vector<T>>(5);
    test_elem_throw::on_copy_after(3);

    BOOST_CHECK_THROW(c.resize_back(256, T(404)), test_exception);

    assert_equals(c, c_origi);
  }

  // size < required, copy throws, but later
  if (! std::is_nothrow_copy_constructible<T>::value)
  {
    Devector c = get_range<Devector>(5);
    std::vector<T> c_origi = get_range<std::vector<T>>(5);
    auto origi_begin = c.begin();
    test_elem_throw::on_copy_after(7);

    BOOST_CHECK_THROW(c.resize_back(256, T(404)), test_exception);

    assert_equals(c, c_origi);
    BOOST_TEST(origi_begin == c.begin());
  }

  // size < required, copy throws
  if (! std::is_nothrow_copy_constructible<T>::value)
  {
    Devector c = get_range<Devector>(5);
    std::vector<T> c_origi = get_range<std::vector<T>>(5);
    auto origi_begin = c.begin();
    test_elem_throw::on_copy_after(3);

    BOOST_CHECK_THROW(c.resize_back(256, T(404)), test_exception);

    assert_equals(c, c_origi);
    BOOST_TEST(origi_begin == c.begin());
  }

  // size >= required
  {
    Devector e = get_range<Devector>(6);
    e.resize_back(4, T(404));
    assert_equals(e, {1, 2, 3, 4});
  }

  // size < required, does not fit front small buffer
  {
    std::vector<T> expected(128, T(9));
    Devector g;
    g.resize_back(128, T(9));
    assert_equals(g, expected);
  }

  // size = required
  {
    Devector e = get_range<Devector>(6);
    e.resize_back(6, T(9));
    assert_equals(e, {1, 2, 3, 4, 5, 6});
  }

  // size < required, tmp is already inserted
  {
    Devector f = get_range<Devector>(8);
    const T& tmp = *(f.begin() + 1);
    f.resize_back(16, tmp);
    assert_equals(f, {1,2,3,4,5,6,7,8,2,2,2,2,2,2,2,2});
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_unsafe_uninitialized_resize_front, Devector, all_devectors)
{
  using T = typename Devector::value_type;

  { // noop
    Devector a = get_range<Devector>(8);
    a.reset_alloc_stats();

    a.unsafe_uninitialized_resize_front(a.size());

    assert_equals(a, {1, 2, 3, 4, 5, 6, 7, 8});
    BOOST_TEST(a.capacity_alloc_count == 0u);
  }

  { // grow (maybe has enough capacity)
    Devector b = get_range<Devector>(0, 0, 5, 9);

    b.unsafe_uninitialized_resize_front(8);

    for (int i = 0; i < 4; ++i)
    {
      new (b.data() + i) T(i+1);
    }

    assert_equals(b, {1, 2, 3, 4, 5, 6, 7, 8});
  }

  { // shrink uninitialized
    Devector c = get_range<Devector>(8);

    c.unsafe_uninitialized_resize_front(16);
    c.unsafe_uninitialized_resize_front(8);

    assert_equals(c, {1, 2, 3, 4, 5, 6, 7, 8});
  }

  if (std::is_trivially_destructible<T>::value)
  {
    // shrink
    Devector d = get_range<Devector>(8);

    d.unsafe_uninitialized_resize_front(4);

    assert_equals(d, {5, 6, 7, 8});
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_unsafe_uninitialized_resize_back, Devector, all_devectors)
{
  using T = typename Devector::value_type;

  { // noop
    Devector a = get_range<Devector>(8);
    a.reset_alloc_stats();

    a.unsafe_uninitialized_resize_back(a.size());

    assert_equals(a, {1, 2, 3, 4, 5, 6, 7, 8});
    BOOST_TEST(a.capacity_alloc_count == 0u);
  }

  { // grow (maybe has enough capacity)
    Devector b = get_range<Devector>(1, 5, 0, 0);

    b.unsafe_uninitialized_resize_back(8);

    for (int i = 0; i < 4; ++i)
    {
      new (b.data() + 4 + i) T(i+5);
    }

    assert_equals(b, {1, 2, 3, 4, 5, 6, 7, 8});
  }

  { // shrink uninitialized
    Devector c = get_range<Devector>(8);

    c.unsafe_uninitialized_resize_back(16);
    c.unsafe_uninitialized_resize_back(8);

    assert_equals(c, {1, 2, 3, 4, 5, 6, 7, 8});
  }

  if (std::is_trivially_destructible<T>::value)
  {
    // shrink
    Devector d = get_range<Devector>(8);

    d.unsafe_uninitialized_resize_back(4);

    assert_equals(d, {1, 2, 3, 4});
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_reserve_front, Devector, all_devectors)
{
  Devector a;

  a.reserve_front(100);
  for (unsigned i = 0; i < 100u; ++i)
  {
    a.push_front(i);
  }

  BOOST_TEST(a.capacity_alloc_count == 1u);

  Devector b;
  b.reserve_front(4);
  b.reserve_front(6);
  b.reserve_front(4);
  b.reserve_front(8);
  b.reserve_front(16);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_reserve_back, Devector, all_devectors)
{
  Devector a;

  a.reserve_back(100);
  for (unsigned i = 0; i < 100; ++i)
  {
    a.push_back(i);
  }

  BOOST_TEST(a.capacity_alloc_count == 1u);

  Devector b;
  b.reserve_back(4);
  b.reserve_back(6);
  b.reserve_back(4);
  b.reserve_back(8);
  b.reserve_back(16);
}

template <typename Devector>
void test_shrink_to_fit_always()
{
  Devector a;
  a.reserve(100);

  a.push_back(1);
  a.push_back(2);
  a.push_back(3);

  a.shrink_to_fit();

  std::vector<unsigned> expected{1, 2, 3};
  assert_equals(a, expected);

  unsigned sb_size = small_buffer_size<Devector>::value;
  BOOST_TEST(a.capacity() == (std::max)(sb_size, 3u));
}

template <typename Devector>
void test_shrink_to_fit_never()
{
  Devector a;
  a.reserve(100);

  a.push_back(1);
  a.push_back(2);
  a.push_back(3);

  a.shrink_to_fit();

  std::vector<unsigned> expected{1, 2, 3};
  assert_equals(a, expected);
  BOOST_TEST(a.capacity() == 100u);
}

void test_shrink_to_fit()
{
  struct always_shrink : public devector_growth_policy
  {
    static bool should_shrink(unsigned, unsigned, unsigned)
    {
      return true;
    }
  };

  struct never_shrink : public devector_growth_policy
  {
    static bool should_shrink(unsigned, unsigned, unsigned)
    {
      return false;
    }
  };

  using devector_u_shr       = devector<unsigned, devector_small_buffer_policy<0>, always_shrink>;
  using small_devector_u_shr = devector<unsigned, devector_small_buffer_policy<3>, always_shrink>;

  test_shrink_to_fit_always<devector_u_shr>();
  test_shrink_to_fit_always<small_devector_u_shr>();

  using devector_u           = devector<unsigned, devector_small_buffer_policy<0>, never_shrink>;
  using small_devector_u     = devector<unsigned, devector_small_buffer_policy<3>, never_shrink>;

  test_shrink_to_fit_never<devector_u>();
  test_shrink_to_fit_never<small_devector_u>();
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_index_operator, Devector, all_devectors)
{
  using T = typename Devector::value_type;

  { // non-const []
    Devector a = get_range<Devector>(5);

    BOOST_TEST(a[0] == T(1));
    BOOST_TEST(a[4] == T(5));
    BOOST_TEST(&a[3] == &a[0] + 3);

    a[0] = T(100);
    BOOST_TEST(a[0] == T(100));
  }

  { // const []
    const Devector a = get_range<Devector>(5);

    BOOST_TEST(a[0] == T(1));
    BOOST_TEST(a[4] == T(5));
    BOOST_TEST(&a[3] == &a[0] + 3);
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_at, Devector, all_devectors)
{
  using T = typename Devector::value_type;

  { // non-const at
    Devector a = get_range<Devector>(3);

    BOOST_TEST(a.at(0) == T(1));
    a.at(0) = T(100);
    BOOST_TEST(a.at(0) == T(100));

    BOOST_CHECK_THROW(a.at(3), std::out_of_range);
  }

  { // const at
    const Devector a = get_range<Devector>(3);

    BOOST_TEST(a.at(0) == T(1));

    BOOST_CHECK_THROW(a.at(3), std::out_of_range);
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_front, Devector, all_devectors)
{
  using T = typename Devector::value_type;

  { // non-const front
    Devector a = get_range<Devector>(3);
    BOOST_TEST(a.front() == T(1));
    a.front() = T(100);
    BOOST_TEST(a.front() == T(100));
  }

  { // const front
    const Devector a = get_range<Devector>(3);
    BOOST_TEST(a.front() == T(1));
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_back, Devector, all_devectors)
{
  using T = typename Devector::value_type;

  { // non-const back
    Devector a = get_range<Devector>(3);
    BOOST_TEST(a.back() == T(3));
    a.back() = T(100);
    BOOST_TEST(a.back() == T(100));
  }

  { // const back
    const Devector a = get_range<Devector>(3);
    BOOST_TEST(a.back() == T(3));
  }
}

void test_data()
{
  unsigned c_array[] = {1, 2, 3, 4};

  { // non-const data
    devector<unsigned> a(c_array, c_array + 4);
    BOOST_TEST(a.data() == &a.front());

    BOOST_TEST(std::memcmp(c_array, a.data(), 4 * sizeof(unsigned)) == 0);

    *(a.data()) = 100;
    BOOST_TEST(a.front() == 100u);
  }

  { // const data
    const devector<unsigned> a(c_array, c_array + 4);
    BOOST_TEST(a.data() == &a.front());

    BOOST_TEST(std::memcmp(c_array, a.data(), 4 * sizeof(unsigned)) == 0);
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_emplace_front, Devector, all_devectors)
{
  using T = typename Devector::value_type;

  {
    Devector a;

    a.emplace_front(3);
    a.emplace_front(2);
    a.emplace_front(1);

    std::vector<T> expected = get_range<std::vector<T>>(3);

    assert_equals(a, expected);
  }

  if (! std::is_nothrow_constructible<T>::value)
  {
    Devector b = get_range<Devector>(4);
    auto origi_begin = b.begin();

    test_elem_throw::on_ctor_after(1);

    BOOST_CHECK_THROW(b.emplace_front(404), test_exception);

    auto new_begin = b.begin();

    BOOST_TEST(origi_begin == new_begin);
    BOOST_TEST(b.size() == 4u);
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_push_front, Devector, t_is_copy_constructible)
{
  using T = typename Devector::value_type;

  {
    std::vector<T> expected = get_range<std::vector<T>>(16);
    std::reverse(expected.begin(), expected.end());
    Devector a;

    for (std::size_t i = 1; i <= 16; ++i)
    {
      T elem(i);
      a.push_front(elem);
    }

    assert_equals(a, expected);
  }

  if (! std::is_nothrow_copy_constructible<T>::value)
  {
    Devector b = get_range<Devector>(4);
    auto origi_begin = b.begin();

    const T elem(404);
    test_elem_throw::on_copy_after(1);

    BOOST_CHECK_THROW(b.push_front(elem), test_exception);

    auto new_begin = b.begin();

    BOOST_TEST(origi_begin == new_begin);
    BOOST_TEST(b.size() == 4u);
  }

  // test when tmp is already inserted
  {
    Devector c = get_range<Devector>(4);
    const T& tmp = *(c.begin() + 1);
    c.push_front(tmp);
    assert_equals(c, {2, 1, 2, 3, 4});
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_push_front_rvalue, Devector, all_devectors)
{
  using T = typename Devector::value_type;

  {
    std::vector<T> expected = get_range<std::vector<T>>(16);
    std::reverse(expected.begin(), expected.end());
    Devector a;

    for (std::size_t i = 1; i <= 16; ++i)
    {
      T elem(i);
      a.push_front(std::move(elem));
    }

    assert_equals(a, expected);
  }

  if (! std::is_nothrow_move_constructible<T>::value)
  {
    Devector b = get_range<Devector>(4);
    auto origi_begin = b.begin();

    test_elem_throw::on_move_after(1);

    BOOST_CHECK_THROW(b.push_front(T(404)), test_exception);

    auto new_begin = b.begin();

    BOOST_TEST(origi_begin == new_begin);
    BOOST_TEST(b.size() == 4u);
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_unsafe_push_front, Devector, t_is_copy_constructible)
{
  using T = typename Devector::value_type;

  {
    std::vector<T> expected = get_range<std::vector<T>>(16);
    std::reverse(expected.begin(), expected.end());
    Devector a;
    a.reserve_front(16);

    for (std::size_t i = 1; i <= 16; ++i)
    {
      T elem(i);
      a.unsafe_push_front(elem);
    }

    assert_equals(a, expected);
  }

  if (! std::is_nothrow_copy_constructible<T>::value)
  {
    Devector b = get_range<Devector>(4);
    b.reserve_front(5);
    auto origi_begin = b.begin();

    const T elem(404);
    test_elem_throw::on_copy_after(1);

    BOOST_CHECK_THROW(b.unsafe_push_front(elem), test_exception);

    auto new_begin = b.begin();

    BOOST_TEST(origi_begin == new_begin);
    BOOST_TEST(b.size() == 4u);
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_unsafe_push_front_rvalue, Devector, all_devectors)
{
  using T = typename Devector::value_type;

  {
    std::vector<T> expected = get_range<std::vector<T>>(16);
    std::reverse(expected.begin(), expected.end());
    Devector a;
    a.reserve_front(16);

    for (std::size_t i = 1; i <= 16; ++i)
    {
      T elem(i);
      a.unsafe_push_front(std::move(elem));
    }

    assert_equals(a, expected);
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_pop_front, Devector, all_devectors)
{
  {
    Devector a;
    a.emplace_front(1);
    a.pop_front();
    BOOST_TEST(a.empty());
  }

  {
    Devector b;

    b.emplace_back(2);
    b.pop_front();
    BOOST_TEST(b.empty());

    b.emplace_front(3);
    b.pop_front();
    BOOST_TEST(b.empty());
  }

  {
    Devector c = get_range<Devector>(20);
    for (int i = 0; i < 20; ++i)
    {
      BOOST_TEST(!c.empty());
      c.pop_front();
    }
    BOOST_TEST(c.empty());
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_emplace_back, Devector, all_devectors)
{
  using T = typename Devector::value_type;

  {
    Devector a;

    a.emplace_back(1);
    a.emplace_back(2);
    a.emplace_back(3);

    std::vector<T> expected = get_range<std::vector<T>>(3);

    assert_equals(a, expected);
  }

  if (! std::is_nothrow_constructible<T>::value)
  {
    Devector b = get_range<Devector>(4);
    auto origi_begin = b.begin();

    test_elem_throw::on_ctor_after(1);

    BOOST_CHECK_THROW(b.emplace_back(404), test_exception);

    auto new_begin = b.begin();

    BOOST_TEST(origi_begin == new_begin);
    BOOST_TEST(b.size() == 4u);
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_push_back, Devector, t_is_copy_constructible)
{
  using T = typename Devector::value_type;

  {
    std::vector<T> expected = get_range<std::vector<T>>(16);
    Devector a;

    for (std::size_t i = 1; i <= 16; ++i)
    {
      T elem(i);
      a.push_back(elem);
    }

    assert_equals(a, expected);
  }

  if (! std::is_nothrow_copy_constructible<T>::value)
  {
    Devector b = get_range<Devector>(4);
    auto origi_begin = b.begin();

    const T elem(404);
    test_elem_throw::on_copy_after(1);

    BOOST_CHECK_THROW(b.push_back(elem), test_exception);

    auto new_begin = b.begin();

    BOOST_TEST(origi_begin == new_begin);
    BOOST_TEST(b.size() == 4u);
  }

  // test when tmp is already inserted
  {
    Devector c = get_range<Devector>(4);
    const T& tmp = *(c.begin() + 1);
    c.push_back(tmp);
    assert_equals(c, {1, 2, 3, 4, 2});
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_push_back_rvalue, Devector, all_devectors)
{
  using T = typename Devector::value_type;

  {
    std::vector<T> expected = get_range<std::vector<T>>(16);
    Devector a;

    for (std::size_t i = 1; i <= 16; ++i)
    {
      T elem(i);
      a.push_back(std::move(elem));
    }

    assert_equals(a, expected);
  }

  if (! std::is_nothrow_move_constructible<T>::value)
  {
    Devector b = get_range<Devector>(4);
    auto origi_begin = b.begin();

    test_elem_throw::on_move_after(1);

    BOOST_CHECK_THROW(b.push_back(T(404)), test_exception);

    auto new_begin = b.begin();

    BOOST_TEST(origi_begin == new_begin);
    BOOST_TEST(b.size() == 4u);
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_unsafe_push_back, Devector, t_is_copy_constructible)
{
  using T = typename Devector::value_type;

  {
    std::vector<T> expected = get_range<std::vector<T>>(16);
    Devector a;
    a.reserve(16);

    for (std::size_t i = 1; i <= 16; ++i)
    {
      T elem(i);
      a.unsafe_push_back(elem);
    }

    assert_equals(a, expected);
  }

  if (! std::is_nothrow_copy_constructible<T>::value)
  {
    Devector b = get_range<Devector>(4);
    b.reserve(5);
    auto origi_begin = b.begin();

    const T elem(404);
    test_elem_throw::on_copy_after(1);

    BOOST_CHECK_THROW(b.unsafe_push_back(elem), test_exception);

    auto new_begin = b.begin();

    BOOST_TEST(origi_begin == new_begin);
    BOOST_TEST(b.size() == 4u);
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_unsafe_push_back_rvalue, Devector, all_devectors)
{
  using T = typename Devector::value_type;

  {
    std::vector<T> expected = get_range<std::vector<T>>(16);
    Devector a;
    a.reserve(16);

    for (std::size_t i = 1; i <= 16; ++i)
    {
      T elem(i);
      a.unsafe_push_back(std::move(elem));
    }

    assert_equals(a, expected);
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_pop_back, Devector, all_devectors)
{
  {
    Devector a;
    a.emplace_back(1);
    a.pop_back();
    BOOST_TEST(a.empty());
  }

  {
    Devector b;

    b.emplace_front(2);
    b.pop_back();
    BOOST_TEST(b.empty());

    b.emplace_back(3);
    b.pop_back();
    BOOST_TEST(b.empty());
  }

  {
    Devector c = get_range<Devector>(20);
    for (int i = 0; i < 20; ++i)
    {
      BOOST_TEST(!c.empty());
      c.pop_back();
    }
    BOOST_TEST(c.empty());
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_emplace, Devector, all_devectors)
{
  using T = typename Devector::value_type;

  {
    Devector a = get_range<Devector>(16);
    auto it = a.emplace(a.begin(), 123);
    assert_equals(a, {123, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16});
    BOOST_TEST(*it == T(123));
  }

  {
    Devector b = get_range<Devector>(16);
    auto it = b.emplace(b.end(), 123);
    assert_equals(b, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 123});
    BOOST_TEST(*it == T(123));
  }

  {
    Devector c = get_range<Devector>(16);
    c.pop_front();
    auto it = c.emplace(c.begin(), 123);
    assert_equals(c, {123, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16});
    BOOST_TEST(*it == T(123));
  }

  {
    Devector d = get_range<Devector>(16);
    d.pop_back();
    auto it = d.emplace(d.end(), 123);
    assert_equals(d, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 123});
    BOOST_TEST(*it == T(123));
  }

  {
    Devector e = get_range<Devector>(16);
    auto it = e.emplace(e.begin() + 5, 123);
    assert_equals(e, {1, 2, 3, 4, 5, 123, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16});
    BOOST_TEST(*it == T(123));
  }

  {
    Devector f = get_range<Devector>(16);
    f.pop_front();
    f.pop_back();
    auto valid = f.begin() + 1;
    auto it = f.emplace(f.begin() + 1, 123);
    assert_equals(f, {2, 123, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15});
    BOOST_TEST(*it == T(123));
    BOOST_TEST(*valid == T(3));
  }

  {
    Devector g = get_range<Devector>(16);
    g.pop_front();
    g.pop_back();
    auto valid = g.end() - 2;
    auto it = g.emplace(g.end() - 1, 123);
    assert_equals(g, {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 123, 15});
    BOOST_TEST(*it == T(123));
    BOOST_TEST(*valid == T(14));
  }

  {
    Devector h = get_range<Devector>(16);
    h.pop_front();
    h.pop_back();
    auto valid = h.begin() + 7;
    auto it = h.emplace(h.begin() + 7, 123);
    assert_equals(h, {2, 3, 4, 5, 6, 7, 8, 123, 9, 10, 11, 12, 13, 14, 15});
    BOOST_TEST(*it == T(123));
    BOOST_TEST(*valid == T(9));
  }

  {
    Devector i;
    i.emplace(i.begin(), 1);
    i.emplace(i.end(), 10);
    for (int j = 2; j < 10; ++j)
    {
      i.emplace(i.begin() + (j-1), j);
    }
    assert_equals(i, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10});
  }

  if (! std::is_nothrow_constructible<T>::value)
  {
    Devector j = get_range<Devector>(4);
    auto origi_begin = j.begin();

    test_elem_throw::on_ctor_after(1);

    BOOST_CHECK_THROW(j.emplace(j.begin() + 2, 404), test_exception);

    assert_equals(j, {1, 2, 3, 4});
    BOOST_TEST(origi_begin == j.begin());
  }

  // It's not required to pass the following test per C++11 23.3.6.5/1
  // If an exception is thrown other than by the copy constructor, move constructor,
  // assignment operator, or move assignment operator of T or by any InputIterator operation
  // there are no effects. If an exception is thrown by the move constructor of a non-CopyInsertable T,
  // the effects are unspecified.

//  if (! std::is_nothrow_move_constructible<T>::value && ! std::is_nothrow_copy_constructible<T>::value)
//  {
//    Devector k = get_range<Devector>(8);
//    auto origi_begin = k.begin();
//
//    test_elem_throw::on_copy_after(3);
//    test_elem_throw::on_move_after(3);
//
//    BOOST_CHECK_THROW(k.emplace(k.begin() + 4, 404), test_exception);
//
//    test_elem_throw::do_not_throw();
//
//    assert_equals(k, {1, 2, 3, 4, 5, 6, 7, 8});
//    BOOST_TEST(origi_begin == k.begin());
//  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_insert, Devector, t_is_copy_constructible)
{
  using T = typename Devector::value_type;

  T test_elem(123);

  {
    Devector a = get_range<Devector>(16);
    auto it = a.insert(a.begin(), test_elem);
    assert_equals(a, {123, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16});
    BOOST_TEST(*it == T(123));
  }

  {
    Devector b = get_range<Devector>(16);
    auto it = b.insert(b.end(), test_elem);
    assert_equals(b, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 123});
    BOOST_TEST(*it == T(123));
  }

  {
    Devector c = get_range<Devector>(16);
    c.pop_front();
    auto it = c.insert(c.begin(), test_elem);
    assert_equals(c, {123, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16});
    BOOST_TEST(*it == T(123));
  }

  {
    Devector d = get_range<Devector>(16);
    d.pop_back();
    auto it = d.insert(d.end(), test_elem);
    assert_equals(d, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 123});
    BOOST_TEST(*it == T(123));
  }

  {
    Devector e = get_range<Devector>(16);
    auto it = e.insert(e.begin() + 5, test_elem);
    assert_equals(e, {1, 2, 3, 4, 5, 123, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16});
    BOOST_TEST(*it == T(123));
  }

  {
    Devector f = get_range<Devector>(16);
    f.pop_front();
    f.pop_back();
    auto valid = f.begin() + 1;
    auto it = f.insert(f.begin() + 1, test_elem);
    assert_equals(f, {2, 123, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15});
    BOOST_TEST(*it == T(123));
    BOOST_TEST(*valid == T(3));
  }

  {
    Devector g = get_range<Devector>(16);
    g.pop_front();
    g.pop_back();
    auto valid = g.end() - 2;
    auto it = g.insert(g.end() - 1, test_elem);
    assert_equals(g, {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 123, 15});
    BOOST_TEST(*it == T(123));
    BOOST_TEST(*valid == T(14));
  }

  {
    Devector h = get_range<Devector>(16);
    h.pop_front();
    h.pop_back();
    auto valid = h.begin() + 7;
    auto it = h.insert(h.begin() + 7, test_elem);
    assert_equals(h, {2, 3, 4, 5, 6, 7, 8, 123, 9, 10, 11, 12, 13, 14, 15});
    BOOST_TEST(*it == T(123));
    BOOST_TEST(*valid == T(9));
  }

  {
    Devector i;
    i.insert(i.begin(), 1);
    i.insert(i.end(), 10);
    for (int j = 2; j < 10; ++j)
    {
      T x(j);
      i.insert(i.begin() + (j-1), x);
    }
    assert_equals(i, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10});
  }

  if (! std::is_nothrow_copy_constructible<T>::value)
  {
    Devector j = get_range<Devector>(4);
    auto origi_begin = j.begin();

    test_elem_throw::on_copy_after(1);

    BOOST_CHECK_THROW(j.insert(j.begin() + 2, test_elem), test_exception);

    assert_equals(j, {1, 2, 3, 4});
    BOOST_TEST(origi_begin == j.begin());
  }

  // test when tmp is already inserted and there's free capacity
  {
    Devector c = get_range<Devector>(6);
    c.pop_back();
    const T& tmp = *(c.begin() + 2);
    c.insert(c.begin() + 1, tmp);
    assert_equals(c, {1, 3, 2, 3, 4, 5});
  }

  // test when tmp is already inserted and maybe there's no free capacity
  {
    Devector c = get_range<Devector>(6);
    const T& tmp = *(c.begin() + 2);
    c.insert(c.begin() + 1, tmp);
    assert_equals(c, {1, 3, 2, 3, 4, 5, 6});
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_insert_rvalue, Devector, all_devectors)
{
  using T = typename Devector::value_type;

  {
    Devector a = get_range<Devector>(16);
    auto it = a.insert(a.begin(), 123);
    assert_equals(a, {123, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16});
    BOOST_TEST(*it == T(123));
  }

  {
    Devector b = get_range<Devector>(16);
    auto it = b.insert(b.end(), 123);
    assert_equals(b, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 123});
    BOOST_TEST(*it == T(123));
  }

  {
    Devector c = get_range<Devector>(16);
    c.pop_front();
    auto it = c.insert(c.begin(), 123);
    assert_equals(c, {123, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16});
    BOOST_TEST(*it == T(123));
  }

  {
    Devector d = get_range<Devector>(16);
    d.pop_back();
    auto it = d.insert(d.end(), 123);
    assert_equals(d, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 123});
    BOOST_TEST(*it == T(123));
  }

  {
    Devector e = get_range<Devector>(16);
    auto it = e.insert(e.begin() + 5, 123);
    assert_equals(e, {1, 2, 3, 4, 5, 123, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16});
    BOOST_TEST(*it == T(123));
  }

  {
    Devector f = get_range<Devector>(16);
    f.pop_front();
    f.pop_back();
    auto valid = f.begin() + 1;
    auto it = f.insert(f.begin() + 1, 123);
    assert_equals(f, {2, 123, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15});
    BOOST_TEST(*it == T(123));
    BOOST_TEST(*valid == T(3));
  }

  {
    Devector g = get_range<Devector>(16);
    g.pop_front();
    g.pop_back();
    auto valid = g.end() - 2;
    auto it = g.insert(g.end() - 1, 123);
    assert_equals(g, {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 123, 15});
    BOOST_TEST(*it == T(123));
    BOOST_TEST(*valid == T(14));
  }

  {
    Devector h = get_range<Devector>(16);
    h.pop_front();
    h.pop_back();
    auto valid = h.begin() + 7;
    auto it = h.insert(h.begin() + 7, 123);
    assert_equals(h, {2, 3, 4, 5, 6, 7, 8, 123, 9, 10, 11, 12, 13, 14, 15});
    BOOST_TEST(*it == T(123));
    BOOST_TEST(*valid == T(9));
  }

  {
    Devector i;
    i.insert(i.begin(), 1);
    i.insert(i.end(), 10);
    for (int j = 2; j < 10; ++j)
    {
      i.insert(i.begin() + (j-1), j);
    }
    assert_equals(i, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10});
  }

  if (! std::is_nothrow_constructible<T>::value)
  {
    Devector j = get_range<Devector>(4);
    auto origi_begin = j.begin();

    test_elem_throw::on_ctor_after(1);

    BOOST_CHECK_THROW(j.insert(j.begin() + 2, 404), test_exception);

    assert_equals(j, {1, 2, 3, 4});
    BOOST_TEST(origi_begin == j.begin());
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_insert_n, Devector, t_is_copy_constructible)
{
  using T = typename Devector::value_type;

  {
    Devector a;
    const T x(123);
    auto ret = a.insert(a.end(), 5, x);
    assert_equals(a, {123, 123, 123, 123, 123});
    BOOST_TEST(ret == a.begin());
  }

  {
    Devector b = get_range<Devector>(8);
    const T x(9);
    auto ret = b.insert(b.begin(), 3, x);
    assert_equals(b, {9, 9, 9, 1, 2, 3, 4, 5, 6, 7, 8});
    BOOST_TEST(ret == b.begin());
  }

  {
    Devector c = get_range<Devector>(8);
    const T x(9);
    auto ret = c.insert(c.end(), 3, x);
    assert_equals(c, {1, 2, 3, 4, 5, 6, 7, 8, 9, 9, 9});
    BOOST_TEST(ret == c.begin() + 8);
  }

  {
    Devector d = get_range<Devector>(8);

    d.pop_front();
    d.pop_front();
    d.pop_front();

    const T x(9);
    auto origi_end = d.end();
    auto ret = d.insert(d.begin(), 3, x);

    assert_equals(d, {9, 9, 9, 4, 5, 6, 7, 8});
    BOOST_TEST(origi_end == d.end());
    BOOST_TEST(ret == d.begin());
  }

  {
    Devector e = get_range<Devector>(8);

    e.pop_back();
    e.pop_back();
    e.pop_back();

    const T x(9);
    auto origi_begin = e.begin();
    auto ret = e.insert(e.end(), 3, x);

    assert_equals(e, {1, 2, 3, 4, 5, 9, 9, 9});
    BOOST_TEST(origi_begin == e.begin());
    BOOST_TEST(ret == e.begin() + 5);
  }

  {
    Devector f = get_range<Devector>(8);
    f.reset_alloc_stats();

    f.pop_front();
    f.pop_front();
    f.pop_back();
    f.pop_back();

    const T x(9);
    auto ret = f.insert(f.begin() + 2, 4, x);

    assert_equals(f, {3, 4, 9, 9, 9, 9, 5, 6});
    BOOST_TEST(f.capacity_alloc_count == 0u);
    BOOST_TEST(ret == f.begin() + 2);
  }

  {
    Devector g = get_range<Devector>(8);
    g.reset_alloc_stats();

    g.pop_front();
    g.pop_front();
    g.pop_back();
    g.pop_back();
    g.pop_back();

    const T x(9);
    auto ret = g.insert(g.begin() + 2, 5, x);

    assert_equals(g, {3, 4, 9, 9, 9, 9, 9, 5});
    BOOST_TEST(g.capacity_alloc_count == 0u);
    BOOST_TEST(ret == g.begin() + 2);
  }

  {
    Devector g = get_range<Devector>(8);

    const T x(9);
    auto ret = g.insert(g.begin() + 2, 5, x);

    assert_equals(g, {1, 2, 9, 9, 9, 9, 9, 3, 4, 5, 6, 7, 8});
    BOOST_TEST(ret == g.begin() + 2);
  }

  { // n == 0
    Devector h = get_range<Devector>(8);
    h.reset_alloc_stats();

    const T x(9);

    auto ret = h.insert(h.begin(), 0, x);
    BOOST_TEST(ret == h.begin());

    ret = h.insert(h.begin() + 4, 0, x);
    BOOST_TEST(ret == h.begin() + 4);

    ret = h.insert(h.end(), 0, x);
    BOOST_TEST(ret == h.end());

    assert_equals(h, {1, 2, 3, 4, 5, 6, 7, 8});
    BOOST_TEST(h.capacity_alloc_count == 0u);
  }

  { // test insert already inserted
    Devector i = get_range<Devector>(8);
    i.reset_alloc_stats();

    i.pop_front();
    i.pop_front();

    auto ret = i.insert(i.end() - 1, 2, *i.begin());

    assert_equals(i, {3, 4, 5, 6, 7, 3, 3, 8});
    BOOST_TEST(i.capacity_alloc_count == 0u);
    BOOST_TEST(ret == i.begin() + 5);
  }

  if (! std::is_nothrow_copy_constructible<T>::value)
  {
    // insert at begin
    {
      Devector j = get_range<Devector>(4);

      const T x(404);
      test_elem_throw::on_copy_after(3);

    BOOST_CHECK_THROW(j.insert(j.begin(), 4, x), test_exception);
    }

    // insert at end
    {
      Devector k = get_range<Devector>(4);

      const T x(404);
      test_elem_throw::on_copy_after(3);

    BOOST_CHECK_THROW(k.insert(k.end(), 4, x), test_exception);
    }
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_insert_input_range, Devector, t_is_copy_constructible)
{
  using T = typename Devector::value_type;

  const devector<T> x{T(9), T(9), T(9), T(9), T(9)};

  {
    devector<T> input = x;

    auto input_begin = make_input_iterator(input, input.begin());
    auto input_end   = make_input_iterator(input, input.begin() + 5);

    Devector a;
    auto ret = a.insert(a.end(), input_begin, input_end);
    assert_equals(a, {9, 9, 9, 9, 9});
    BOOST_TEST(ret == a.begin());
  }

  {
    devector<T> input = x;

    auto input_begin = make_input_iterator(input, input.begin());
    auto input_end   = make_input_iterator(input, input.begin() + 3);

    Devector b = get_range<Devector>(8);
    auto ret = b.insert(b.begin(), input_begin, input_end);
    assert_equals(b, {9, 9, 9, 1, 2, 3, 4, 5, 6, 7, 8});
    BOOST_TEST(ret == b.begin());
  }

  {
    devector<T> input = x;

    auto input_begin = make_input_iterator(input, input.begin());
    auto input_end   = make_input_iterator(input, input.begin() + 3);

    Devector c = get_range<Devector>(8);
    auto ret = c.insert(c.end(), input_begin, input_end);
    assert_equals(c, {1, 2, 3, 4, 5, 6, 7, 8, 9, 9, 9});
    BOOST_TEST(ret == c.begin() + 8);
  }

  {
    devector<T> input = x;

    auto input_begin = make_input_iterator(input, input.begin());
    auto input_end   = make_input_iterator(input, input.begin() + 3);

    Devector d = get_range<Devector>(8);

    d.pop_front();
    d.pop_front();
    d.pop_front();

    auto origi_end = d.end();
    auto ret = d.insert(d.begin(), input_begin, input_end);

    assert_equals(d, {9, 9, 9, 4, 5, 6, 7, 8});
    BOOST_TEST(origi_end == d.end());
    BOOST_TEST(ret == d.begin());
  }

  {
    devector<T> input = x;

    auto input_begin = make_input_iterator(input, input.begin());
    auto input_end   = make_input_iterator(input, input.begin() + 3);

    Devector e = get_range<Devector>(8);

    e.pop_back();
    e.pop_back();
    e.pop_back();

    auto origi_begin = e.begin();
    auto ret = e.insert(e.end(), input_begin, input_end);

    assert_equals(e, {1, 2, 3, 4, 5, 9, 9, 9});
    BOOST_TEST(origi_begin == e.begin());
    BOOST_TEST(ret == e.begin() + 5);
  }

  {
    devector<T> input = x;

    auto input_begin = make_input_iterator(input, input.begin());
    auto input_end   = make_input_iterator(input, input.begin() + 4);

    Devector f = get_range<Devector>(8);
    f.reset_alloc_stats();

    f.pop_front();
    f.pop_front();
    f.pop_back();
    f.pop_back();

    auto ret = f.insert(f.begin() + 2, input_begin, input_end);

    assert_equals(f, {3, 4, 9, 9, 9, 9, 5, 6});
    BOOST_TEST(f.capacity_alloc_count == 0u);
    BOOST_TEST(ret == f.begin() + 2);
  }

  {
    devector<T> input = x;

    auto input_begin = make_input_iterator(input, input.begin());
    auto input_end   = make_input_iterator(input, input.begin() + 5);

    Devector g = get_range<Devector>(8);
    g.reset_alloc_stats();

    g.pop_front();
    g.pop_front();
    g.pop_back();
    g.pop_back();
    g.pop_back();

    auto ret = g.insert(g.begin() + 2, input_begin, input_end);

    assert_equals(g, {3, 4, 9, 9, 9, 9, 9, 5});
    BOOST_TEST(g.capacity_alloc_count == 0u);
    BOOST_TEST(ret == g.begin() + 2);
  }

  {
    devector<T> input = x;

    auto input_begin = make_input_iterator(input, input.begin());
    auto input_end   = make_input_iterator(input, input.begin() + 5);

    Devector g = get_range<Devector>(8);

    auto ret = g.insert(g.begin() + 2, input_begin, input_end);

    assert_equals(g, {1, 2, 9, 9, 9, 9, 9, 3, 4, 5, 6, 7, 8});
    BOOST_TEST(ret == g.begin() + 2);
  }

  { // n == 0
    devector<T> input = x;

    auto input_begin = make_input_iterator(input, input.begin());

    Devector h = get_range<Devector>(8);
    h.reset_alloc_stats();

    auto ret = h.insert(h.begin(), input_begin, input_begin);
    BOOST_TEST(ret == h.begin());

    ret = h.insert(h.begin() + 4, input_begin, input_begin);
    BOOST_TEST(ret == h.begin() + 4);

    ret = h.insert(h.end(), input_begin, input_begin);
    BOOST_TEST(ret == h.end());

    assert_equals(h, {1, 2, 3, 4, 5, 6, 7, 8});
    BOOST_TEST(h.capacity_alloc_count == 0u);
  }

  if (! std::is_nothrow_copy_constructible<T>::value)
  {
    // insert at begin
    {
      devector<T> input = x;

      auto input_begin = make_input_iterator(input, input.begin());
      auto input_end   = make_input_iterator(input, input.begin() + 4);

      Devector j = get_range<Devector>(4);

      test_elem_throw::on_copy_after(3);

    BOOST_CHECK_THROW(j.insert(j.begin(), input_begin, input_end), test_exception);
    }

    // insert at end
    {
      devector<T> input = x;

      auto input_begin = make_input_iterator(input, input.begin());
      auto input_end   = make_input_iterator(input, input.begin() + 4);

      Devector k = get_range<Devector>(4);

      test_elem_throw::on_copy_after(3);

    BOOST_CHECK_THROW(k.insert(k.end(), input_begin, input_end), test_exception);
    }
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_insert_range, Devector, t_is_copy_constructible)
{
  using T = typename Devector::value_type;

  std::vector<T> x{T(9), T(9), T(9), T(9), T(9)};
  auto xb = x.begin();

  {
    Devector a;
    auto ret = a.insert(a.end(), xb, xb+5);
    assert_equals(a, {9, 9, 9, 9, 9});
    BOOST_TEST(ret == a.begin());
  }

  {
    Devector b = get_range<Devector>(8);
    auto ret = b.insert(b.begin(), xb, xb+3);
    assert_equals(b, {9, 9, 9, 1, 2, 3, 4, 5, 6, 7, 8});
    BOOST_TEST(ret == b.begin());
  }

  {
    Devector c = get_range<Devector>(8);
    auto ret = c.insert(c.end(), xb, xb+3);
    assert_equals(c, {1, 2, 3, 4, 5, 6, 7, 8, 9, 9, 9});
    BOOST_TEST(ret == c.begin() + 8);
  }

  {
    Devector d = get_range<Devector>(8);

    d.pop_front();
    d.pop_front();
    d.pop_front();

    auto origi_end = d.end();
    auto ret = d.insert(d.begin(), xb, xb+3);

    assert_equals(d, {9, 9, 9, 4, 5, 6, 7, 8});
    BOOST_TEST(origi_end == d.end());
    BOOST_TEST(ret == d.begin());
  }

  {
    Devector e = get_range<Devector>(8);

    e.pop_back();
    e.pop_back();
    e.pop_back();

    auto origi_begin = e.begin();
    auto ret = e.insert(e.end(), xb, xb+3);

    assert_equals(e, {1, 2, 3, 4, 5, 9, 9, 9});
    BOOST_TEST(origi_begin == e.begin());
    BOOST_TEST(ret == e.begin() + 5);
  }

  {
    Devector f = get_range<Devector>(8);
    f.reset_alloc_stats();

    f.pop_front();
    f.pop_front();
    f.pop_back();
    f.pop_back();

    auto ret = f.insert(f.begin() + 2, xb, xb+4);

    assert_equals(f, {3, 4, 9, 9, 9, 9, 5, 6});
    BOOST_TEST(f.capacity_alloc_count == 0u);
    BOOST_TEST(ret == f.begin() + 2);
  }

  {
    Devector g = get_range<Devector>(8);
    g.reset_alloc_stats();

    g.pop_front();
    g.pop_front();
    g.pop_back();
    g.pop_back();
    g.pop_back();

    auto ret = g.insert(g.begin() + 2, xb, xb+5);

    assert_equals(g, {3, 4, 9, 9, 9, 9, 9, 5});
    BOOST_TEST(g.capacity_alloc_count == 0u);
    BOOST_TEST(ret == g.begin() + 2);
  }

  {
    Devector g = get_range<Devector>(8);

    auto ret = g.insert(g.begin() + 2, xb, xb+5);

    assert_equals(g, {1, 2, 9, 9, 9, 9, 9, 3, 4, 5, 6, 7, 8});
    BOOST_TEST(ret == g.begin() + 2);
  }

  { // n == 0
    Devector h = get_range<Devector>(8);
    h.reset_alloc_stats();

    auto ret = h.insert(h.begin(), xb, xb);
    BOOST_TEST(ret == h.begin());

    ret = h.insert(h.begin() + 4, xb, xb);
    BOOST_TEST(ret == h.begin() + 4);

    ret = h.insert(h.end(), xb, xb);
    BOOST_TEST(ret == h.end());

    assert_equals(h, {1, 2, 3, 4, 5, 6, 7, 8});
    BOOST_TEST(h.capacity_alloc_count == 0u);
  }

  if (! std::is_nothrow_copy_constructible<T>::value)
  {
    // insert at begin
    {
      Devector j = get_range<Devector>(4);

      test_elem_throw::on_copy_after(3);

    BOOST_CHECK_THROW(j.insert(j.begin(), xb, xb+4), test_exception);
    }

    // insert at end
    {
      Devector k = get_range<Devector>(4);

      test_elem_throw::on_copy_after(3);

    BOOST_CHECK_THROW(k.insert(k.end(), xb, xb+4), test_exception);
    }
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_insert_init_list, Devector, t_is_copy_constructible)
{
  using T = typename Devector::value_type;

  {
    Devector a;
    auto ret = a.insert(a.end(), {T(123), T(123), T(123), T(123), T(123)});
    assert_equals(a, {123, 123, 123, 123, 123});
    BOOST_TEST(ret == a.begin());
  }

  {
    Devector b = get_range<Devector>(8);
    auto ret = b.insert(b.begin(), {T(9), T(9), T(9)});
    assert_equals(b, {9, 9, 9, 1, 2, 3, 4, 5, 6, 7, 8});
    BOOST_TEST(ret == b.begin());
  }

  {
    Devector c = get_range<Devector>(8);
    auto ret = c.insert(c.end(), {T(9), T(9), T(9)});
    assert_equals(c, {1, 2, 3, 4, 5, 6, 7, 8, 9, 9, 9});
    BOOST_TEST(ret == c.begin() + 8);
  }

  {
    Devector d = get_range<Devector>(8);

    d.pop_front();
    d.pop_front();
    d.pop_front();

    auto origi_end = d.end();
    auto ret = d.insert(d.begin(), {T(9), T(9), T(9)});

    assert_equals(d, {9, 9, 9, 4, 5, 6, 7, 8});
    BOOST_TEST(origi_end == d.end());
    BOOST_TEST(ret == d.begin());
  }

  {
    Devector e = get_range<Devector>(8);

    e.pop_back();
    e.pop_back();
    e.pop_back();

    auto origi_begin = e.begin();
    auto ret = e.insert(e.end(), {T(9), T(9), T(9)});

    assert_equals(e, {1, 2, 3, 4, 5, 9, 9, 9});
    BOOST_TEST(origi_begin == e.begin());
    BOOST_TEST(ret == e.begin() + 5);
  }

  {
    Devector f = get_range<Devector>(8);
    f.reset_alloc_stats();

    f.pop_front();
    f.pop_front();
    f.pop_back();
    f.pop_back();

    auto ret = f.insert(f.begin() + 2, {T(9), T(9), T(9), T(9)});

    assert_equals(f, {3, 4, 9, 9, 9, 9, 5, 6});
    BOOST_TEST(f.capacity_alloc_count == 0u);
    BOOST_TEST(ret == f.begin() + 2);
  }

  {
    Devector g = get_range<Devector>(8);
    g.reset_alloc_stats();

    g.pop_front();
    g.pop_front();
    g.pop_back();
    g.pop_back();
    g.pop_back();

    auto ret = g.insert(g.begin() + 2, {T(9), T(9), T(9), T(9), T(9)});

    assert_equals(g, {3, 4, 9, 9, 9, 9, 9, 5});
    BOOST_TEST(g.capacity_alloc_count == 0u);
    BOOST_TEST(ret == g.begin() + 2);
  }

  {
    Devector g = get_range<Devector>(8);

    auto ret = g.insert(g.begin() + 2, {T(9), T(9), T(9), T(9), T(9)});

    assert_equals(g, {1, 2, 9, 9, 9, 9, 9, 3, 4, 5, 6, 7, 8});
    BOOST_TEST(ret == g.begin() + 2);
  }

  if (! std::is_nothrow_copy_constructible<T>::value)
  {
    // insert at begin
    {
      Devector j = get_range<Devector>(4);

      test_elem_throw::on_copy_after(3);

    BOOST_CHECK_THROW(j.insert(j.begin(), {T(9), T(9), T(9), T(9), T(9)}), test_exception);
    }

    // insert at end
    {
      Devector k = get_range<Devector>(4);

      test_elem_throw::on_copy_after(3);

    BOOST_CHECK_THROW(k.insert(k.end(), {T(9), T(9), T(9), T(9), T(9)}), test_exception);
    }
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_erase, Devector, all_devectors)
{
  {
    Devector a = get_range<Devector>(4);
    auto ret = a.erase(a.begin());
    assert_equals(a, {2, 3, 4});
    BOOST_TEST(ret == a.begin());
  }

  {
    Devector b = get_range<Devector>(4);
    auto ret = b.erase(b.end() - 1);
    assert_equals(b, {1, 2, 3});
    BOOST_TEST(ret == b.end());
  }

  {
    Devector c = get_range<Devector>(6);
    auto ret = c.erase(c.begin() + 2);
    assert_equals(c, {1, 2, 4, 5, 6});
    BOOST_TEST(ret == c.begin() + 2);
    BOOST_TEST(c.front_free_capacity() > 0u);
  }

  {
    Devector d = get_range<Devector>(6);
    auto ret = d.erase(d.begin() + 4);
    assert_equals(d, {1, 2, 3, 4, 6});
    BOOST_TEST(ret == d.begin() + 4);
    BOOST_TEST(d.back_free_capacity() > 0u);
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_erase_range, Devector, all_devectors)
{
  {
    Devector a = get_range<Devector>(4);
    a.erase(a.end(), a.end());
    a.erase(a.begin(), a.begin());
  }

  {
    Devector b = get_range<Devector>(8);
    auto ret = b.erase(b.begin(), b.begin() + 2);
    assert_equals(b, {3, 4, 5, 6, 7, 8});
    BOOST_TEST(ret == b.begin());
    BOOST_TEST(b.front_free_capacity() > 0u);
  }

  {
    Devector c = get_range<Devector>(8);
    auto ret = c.erase(c.begin() + 1, c.begin() + 3);
    assert_equals(c, {1, 4, 5, 6, 7, 8});
    BOOST_TEST(ret == c.begin() + 1);
    BOOST_TEST(c.front_free_capacity() > 0u);
  }

  {
    Devector d = get_range<Devector>(8);
    auto ret = d.erase(d.end() - 2, d.end());
    assert_equals(d, {1, 2, 3, 4, 5, 6});
    BOOST_TEST(ret == d.end());
    BOOST_TEST(d.back_free_capacity() > 0u);
  }

  {
    Devector e = get_range<Devector>(8);
    auto ret = e.erase(e.end() - 3, e.end() - 1);
    assert_equals(e, {1, 2, 3, 4, 5, 8});
    BOOST_TEST(ret == e.end() - 1);
    BOOST_TEST(e.back_free_capacity() > 0u);
  }

  {
    Devector f = get_range<Devector>(8);
    auto ret = f.erase(f.begin(), f.end());
    assert_equals(f, {});
    BOOST_TEST(ret == f.end());
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_swap, Devector, all_devectors)
{
  using T = typename Devector::value_type;
  using std::swap; // test if ADL works

  // empty-empty
  {
    Devector a;
    Devector b;

    swap(a, b);

    BOOST_TEST(a.empty());
    BOOST_TEST(b.empty());
  }

  // empty-not empty
  {
    Devector a;
    Devector b = get_range<Devector>(4);

    swap(a, b);

    BOOST_TEST(b.empty());
    assert_equals(a, {1, 2, 3, 4});

    swap(a, b);

    BOOST_TEST(a.empty());
    assert_equals(b, {1, 2, 3, 4});
  }

  // small-small / big-big
  {
    Devector a = get_range<Devector>(1, 5, 5, 7);
    Devector b = get_range<Devector>(13, 15, 15, 19);

    swap(a, b);

    assert_equals(a, {13, 14, 15, 16, 17, 18});
    assert_equals(b, {1, 2, 3, 4, 5, 6});

    swap(a, b);

    assert_equals(b, {13, 14, 15, 16, 17, 18});
    assert_equals(a, {1, 2, 3, 4, 5, 6});
  }

  // big-small + small-big
  {
    Devector a = get_range<Devector>(10);
    Devector b = get_range<Devector>(9, 11, 11, 17);

    swap(a, b);

    assert_equals(b, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10});
    assert_equals(a, {9, 10, 11, 12, 13, 14, 15, 16});

    swap(a, b);

    assert_equals(a, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10});
    assert_equals(b, {9, 10, 11, 12, 13, 14, 15, 16});
  }

  // self swap
  {
    Devector a = get_range<Devector>(10);

    swap(a, a);

    assert_equals(a, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10});
  }

  // no overlap
  {
    Devector a = get_range<Devector>(1, 9, 0, 0);
    Devector b = get_range<Devector>(0, 0, 11, 17);

    a.pop_back();
    a.pop_back();

    b.pop_front();
    b.pop_front();

    swap(a, b);

    assert_equals(a, {13, 14, 15, 16});
    assert_equals(b, {1, 2, 3, 4, 5, 6});
  }

  // exceptions

  bool can_throw =
     ! std::is_nothrow_move_constructible<T>::value
  && ! std::is_nothrow_copy_constructible<T>::value;

  // small-small with exception

  if (small_buffer_size<Devector>::value && can_throw)
  {
    Devector a = get_range<Devector>(8);
    Devector b = get_range<Devector>(1,6,7,8);

    test_elem_throw::on_copy_after(4);
    test_elem_throw::on_move_after(4);

    BOOST_CHECK_THROW(swap(a, b), test_exception);

    test_elem_throw::do_not_throw();
  }

  // failure in small-small swap has unspecified results but does not leak
  BOOST_TEST(test_elem_base::no_living_elem());

  // small-big with exception

  if (small_buffer_size<Devector>::value && can_throw)
  {
    Devector small = get_range<Devector>(8);
    Devector big = get_range<Devector>(32);

    std::vector<T> big_ex = get_range<std::vector<T>>(32);

    test_elem_throw::on_copy_after(4);
    test_elem_throw::on_move_after(4);

    BOOST_CHECK_THROW(swap(small, big), test_exception);

    test_elem_throw::do_not_throw();

    // content of small might be moved
    assert_equals(big, big_ex);
  }

  // big-big does not copy or move
  {
    Devector a = get_range<Devector>(32);
    Devector b = get_range<Devector>(32);
    std::vector<T> c = get_range<std::vector<T>>(32);

    test_elem_throw::on_copy_after(1);
    test_elem_throw::on_move_after(1);

    swap(a, b);

    test_elem_throw::do_not_throw();

    assert_equals(a, c);
    assert_equals(b, c);
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_clear, Devector, all_devectors)
{
  {
    Devector a;
    a.clear();
    BOOST_TEST(a.empty());
  }

  {
    Devector a = get_range<Devector>(8);
    a.clear();
    BOOST_TEST(a.empty());
    a.reset_alloc_stats();

    for (unsigned i = 0; i < small_buffer_size<Devector>::value; ++i)
    {
      a.emplace_back(i);
    }

    BOOST_TEST(a.capacity_alloc_count == 0u);
    a.clear();
    BOOST_TEST(a.empty());
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_op_eq, Devector, all_devectors)
{
  { // equal
    Devector a = get_range<Devector>(8);
    Devector b = get_range<Devector>(8);

    BOOST_TEST(a == b);
  }

  { // diff size
    Devector a = get_range<Devector>(8);
    Devector b = get_range<Devector>(9);

    BOOST_TEST(!(a == b));
  }

  { // diff content
    Devector a = get_range<Devector>(8);
    Devector b = get_range<Devector>(2,6,6,10);

    BOOST_TEST(!(a == b));
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_op_lt, Devector, all_devectors)
{
  { // little than
    Devector a = get_range<Devector>(7);
    Devector b = get_range<Devector>(8);

    BOOST_TEST(a < b);
  }

  { // equal
    Devector a = get_range<Devector>(8);
    Devector b = get_range<Devector>(8);

    BOOST_TEST(!(a < b));
  }

  { // greater than
    Devector a = get_range<Devector>(8);
    Devector b = get_range<Devector>(7);

    BOOST_TEST(!(a < b));
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_op_ne, Devector, all_devectors)
{
  { // equal
    Devector a = get_range<Devector>(8);
    Devector b = get_range<Devector>(8);

    BOOST_TEST(!(a != b));
  }

  { // diff size
    Devector a = get_range<Devector>(8);
    Devector b = get_range<Devector>(9);

    BOOST_TEST(a != b);
  }

  { // diff content
    Devector a = get_range<Devector>(8);
    Devector b = get_range<Devector>(2,6,6,10);

    BOOST_TEST(a != b);
  }
}


BOOST_AUTO_TEST_CASE_TEMPLATE(test_op_gt, Devector, all_devectors)
{
  { // little than
    Devector a = get_range<Devector>(7);
    Devector b = get_range<Devector>(8);

    BOOST_TEST(!(a > b));
  }

  { // equal
    Devector a = get_range<Devector>(8);
    Devector b = get_range<Devector>(8);

    BOOST_TEST(!(a > b));
  }

  { // greater than
    Devector a = get_range<Devector>(8);
    Devector b = get_range<Devector>(7);

    BOOST_TEST(a > b);
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_op_ge, Devector, all_devectors)
{
  { // little than
    Devector a = get_range<Devector>(7);
    Devector b = get_range<Devector>(8);

    BOOST_TEST(!(a >= b));
  }

  { // equal
    Devector a = get_range<Devector>(8);
    Devector b = get_range<Devector>(8);

    BOOST_TEST(a >= b);
  }

  { // greater than
    Devector a = get_range<Devector>(8);
    Devector b = get_range<Devector>(7);

    BOOST_TEST(a >= b);
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_op_le, Devector, all_devectors)
{
  { // little than
    Devector a = get_range<Devector>(7);
    Devector b = get_range<Devector>(8);

    BOOST_TEST(a <= b);
  }

  { // equal
    Devector a = get_range<Devector>(8);
    Devector b = get_range<Devector>(8);

    BOOST_TEST(a <= b);
  }

  { // greater than
    Devector a = get_range<Devector>(8);
    Devector b = get_range<Devector>(7);

    BOOST_TEST(!(a <= b));
  }
}
