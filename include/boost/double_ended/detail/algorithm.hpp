//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Benedek Thaler 2015-2016. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://erenon.hu/double_ended for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_DOUBLE_ENDED_DETAIL_ALGORITHM_HPP
#define BOOST_DOUBLE_ENDED_DETAIL_ALGORITHM_HPP

namespace boost {
namespace double_ended {
namespace detail {

template <typename Iterator>
void move_if_noexcept(Iterator first, Iterator last, Iterator dst)
{
  while (first != last)
  {
    *dst++ = std::move_if_noexcept(*first++);
  }
}

template <typename Iterator>
void move_if_noexcept_backward(Iterator first, Iterator last, Iterator dst_last)
{
  while (first != last)
  {
    *(--dst_last) = std::move_if_noexcept(*(--last));
  }
}

}}} // boost::double_ended::detail

#endif // BOOST_DOUBLE_ENDED_DETAIL_ALGORITHM_HPP
