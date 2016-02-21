//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Benedek Thaler 2015-2016. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://erenon.hu/double_ended for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_DOUBLE_ENDED_SERIALIZE_DEVECTOR_HPP
#define BOOST_DOUBLE_ENDED_SERIALIZE_DEVECTOR_HPP

#include <boost/double_ended/devector.hpp>
#include <boost/serialization/split_free.hpp>
#include <boost/core/no_exceptions_support.hpp>

namespace boost {
namespace serialization {

using double_ended::devector;

template <typename A>
using de_alloc_traits = double_ended::detail::allocator_traits<A>;

template <typename Archive, typename T, typename SBP, typename GP, typename A>
void save(Archive& ar, const devector<T, SBP, GP, A>& c, unsigned /*version*/)
{
  auto size = c.size();
  ar << size;

  if (de_alloc_traits<A>::t_is_trivially_copyable)
  {
    ar.save_binary(c.data(), c.size() * sizeof(T));
  }
  else
  {
    for (const T& t : c)
    {
      ar << t;
    }
  }
}

namespace double_ended_detail {

template <typename Archive, typename T, typename SBP, typename GP, typename A, typename ST>
void load_trivial(Archive& ar, devector<T, SBP, GP, A>& c, ST new_size)
{
  const bool fits = c.capacity() >= new_size;

  if (fits)
  {
    const auto to_front = (std::min)(c.front_free_capacity() + c.size(), new_size);
    c.unsafe_uninitialized_resize_front(to_front);
    c.unsafe_uninitialized_resize_back(new_size - to_front + c.size());
  }
  else
  {
    c.clear();
    c.unsafe_uninitialized_resize_back(new_size);
  }

  BOOST_TRY
  {
    ar.load_binary(c.data(), new_size * sizeof(T));
  }
  BOOST_CATCH(...)
  {
    c.unsafe_uninitialized_resize_back(0);
    BOOST_RETHROW;
  }
  BOOST_CATCH_END
}

template <typename Archive, typename T, typename SBP, typename GP, typename A, typename ST>
void load_non_trivial(Archive& ar, devector<T, SBP, GP, A>& c, ST new_size)
{
  const bool fits = c.capacity() >= new_size;

  if (fits)
  {
    const auto to_front = (std::min)(c.front_free_capacity() + c.size(), new_size);
    c.resize_front(to_front);
    c.resize_back(new_size - to_front + c.size());
  }
  else
  {
    c.clear();
    c.resize(new_size);
  }

  for (T& t : c)
  {
    ar >> t;
  }
}

} // namespace double_ended_detail

template <typename Archive, typename T, typename SBP, typename GP, typename A>
void load(Archive& ar, devector<T, SBP, GP, A>& c, unsigned /*version*/)
{
  using Devector = devector<T, SBP, GP, A>;
  using size_type = typename Devector::size_type;

  size_type new_size;
  ar >> new_size;

  if (de_alloc_traits<A>::t_is_trivially_copyable)
  {
    double_ended_detail::load_trivial(ar, c, new_size);
  }
  else
  {
    double_ended_detail::load_non_trivial(ar, c, new_size);
  }
}

template <typename Archive, typename T, typename SBP, typename GP, typename A>
void serialize(Archive& ar, devector<T, SBP, GP, A>& c, unsigned version)
{
  boost::serialization::split_free(ar, c, version);
}

}} // namespace boost::serialization

#endif // BOOST_DOUBLE_ENDED_SERIALIZE_DEVECTOR_HPP
