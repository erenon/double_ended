//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Benedek Thaler 2015-2015. Distributed under the Boost
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

namespace boost {
namespace serialization {

using double_ended::devector;

template <typename Archive, typename T, typename SBP, typename GP, typename A>
void save(Archive& ar, const devector<T, SBP, GP, A>& c, unsigned /*version*/)
{
  auto size = c.size();
  ar << size;

  for (const T& t : c)
  {
    ar << t;
  }
}

template <typename Archive, typename T, typename SBP, typename GP, typename A>
void load(Archive& ar, devector<T, SBP, GP, A>& c, unsigned /*version*/)
{
  using Devector = devector<T, SBP, GP, A>;
  using size_type = typename Devector::size_type;

  size_type new_size;
  ar >> new_size;

  const bool fits = c.capacity() >= new_size;

  if (fits)
  {
    const size_type to_front = (std::min)(c.front_free_capacity() + c.size(), new_size);
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

template <typename Archive, typename T, typename SBP, typename GP, typename A>
void serialize(Archive& ar, devector<T, SBP, GP, A>& c, unsigned version)
{
  boost::serialization::split_free(ar, c, version);
}

}} // namespace boost::serialization

#endif // BOOST_DOUBLE_ENDED_SERIALIZE_DEVECTOR_HPP
