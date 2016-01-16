//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Benedek Thaler 2015-2016. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://erenon.hu/double_ended for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_DOUBLE_ENDED_DETAIL_ITERATORS_HPP
#define BOOST_DOUBLE_ENDED_DETAIL_ITERATORS_HPP

#include <iterator>
#include <type_traits>

#include <boost/iterator/iterator_facade.hpp>

namespace boost {
namespace double_ended {
namespace detail {

template <typename It>
class is_input_iterator
{
  template <typename In>
  static typename std::iterator_traits<In>::iterator_category f(int);

  template <typename NotIn>
  static void f(...);

public:
  enum { value = std::is_same< decltype(f<It>(0)), std::input_iterator_tag>::value };
};

template <typename It>
struct is_not_input_iterator
{
  enum { value = ! is_input_iterator<It>::value };
};

template <typename It>
class is_forward_iterator
{
  template <typename Fw>
  static typename std::iterator_traits<Fw>::iterator_category f(int);

  template <typename NotFw>
  static void f(...);

public:
  enum { value = std::is_same< decltype(f<It>(0)), std::forward_iterator_tag>::value };
};

#ifndef BOOST_DOUBLE_ENDED_DOXYGEN_INVOKED

  #define BOOST_DOUBLE_ENDED_REQUIRE_INPUT_ITERATOR(Iterator) \
    typename Iterator,                                        \
    typename std::enable_if<                                  \
      detail::is_input_iterator<Iterator>::value              \
    ,int>::type = 0

  #define BOOST_DOUBLE_ENDED_REQUIRE_FW_ITERATOR(Iterator)    \
    typename Iterator,                                        \
    typename std::enable_if<                                  \
      detail::is_not_input_iterator<Iterator>::value          \
    ,int>::type = 0

#else

  #define BOOST_DOUBLE_ENDED_REQUIRE_INPUT_ITERATOR(Iterator) typename Iterator
  #define BOOST_DOUBLE_ENDED_REQUIRE_FW_ITERATOR(Iterator) typename Iterator

#endif // ifndef BOOST_DOUBLE_ENDED_DOXYGEN_INVOKED

template <typename T>
T* iterator_to_pointer(T* p) { return p; }

template <typename It>
typename std::iterator_traits<It>::pointer
iterator_to_pointer(const It& it)
{
  return it.operator->();
}

template <typename T, typename Difference>
class constant_iterator : public iterator_facade<
  constant_iterator<T, Difference>,
  const T,
  random_access_traversal_tag,
  const T&,
  Difference
>
{
public:
  constant_iterator() = default;
  constant_iterator(const T& value, Difference size)
    :_value(&value),
     _size(size)
  {}

private:
  friend class boost::iterator_core_access;

  const T& dereference() const noexcept
  {
    return *_value;
  }

  bool equal(const constant_iterator& rhs) const noexcept
  {
    return _size == rhs._size;
  }

  void increment() noexcept
  {
    --_size;
  }

  void decrement() noexcept
  {
    ++_size;
  }

  void advance(Difference n) noexcept
  {
    _size -= n;
  }

  Difference distance_to(const constant_iterator& b) const noexcept
  {
    return _size - b._size;
  }

  const T* _value;
  Difference _size = 0;
};

}}} // namespace boost::double_ended::detail

#endif // BOOST_DOUBLE_ENDED_DETAIL_ITERATORS_HPP
