//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Benedek Thaler 2015-2016. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://erenon.hu/double_ended for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#include <boost/mpl/filter_view.hpp>
#include <boost/mpl/is_sequence.hpp>
#include <boost/mpl/placeholders.hpp>

// get_range

template <typename DeContainer>
DeContainer get_range(int fbeg, int fend, int bbeg, int bend)
{
  DeContainer c;

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

// Allocator with different propagation policies than std::allocator

template <typename T>
struct different_allocator : public std::allocator<T>
{
  bool operator==(const different_allocator&) const { return false; }
  bool operator!=(const different_allocator&) const { return true; }

  using propagate_on_container_copy_assignment = std::true_type;
  using propagate_on_container_move_assignment = std::false_type;
  using propagate_on_container_swap = std::true_type;
};

// MPL sequence util

template <template<typename> class Predicate>
struct if_value_type
{
  template <typename Container>
  struct apply : Predicate<typename Container::value_type> {};
};

template <typename Seq>
using if_t_is_default_constructible =
  typename boost::mpl::filter_view<Seq, if_value_type<std::is_default_constructible>>::type;

template <typename Seq>
using if_t_is_copy_constructible =
  typename boost::mpl::filter_view<Seq, if_value_type<std::is_copy_constructible>>::type;

template <typename Seq>
using if_t_is_trivial =
  typename boost::mpl::filter_view<Seq, if_value_type<std::is_trivial>>::type;

// Container comparison

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

