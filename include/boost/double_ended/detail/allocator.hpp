//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Benedek Thaler 2015-2016. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://erenon.hu/double_ended for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_DOUBLE_ENDED_DETAIL_ALLOCATOR_HPP
#define BOOST_DOUBLE_ENDED_DETAIL_ALLOCATOR_HPP

#include <memory> // allocator_traits
#include <type_traits>

#include <boost/double_ended/detail/guards.hpp>

namespace boost {
namespace double_ended {
namespace detail {

template <typename Allocator>
struct allocator_traits : public std::allocator_traits<Allocator>
{
  using T = typename Allocator::value_type;

  // before C++14, std::allocator does not specify propagate_on_container_move_assignment,
  // std::allocator_traits sets it to false. Not something we would like to use.
  static constexpr bool propagate_on_move_assignment =
       std::allocator_traits<Allocator>::propagate_on_container_move_assignment::value
    || std::is_same<Allocator, std::allocator<T>>::value;

  // poor emulation of a C++17 feature
  static constexpr bool is_always_equal =
       std::is_same<Allocator, std::allocator<T>>::value
    || std::is_empty<Allocator>::value;

  // true, if there is a way to reconstruct t' from t in a noexcept way
  static constexpr bool t_is_nothrow_constructible =
       std::is_nothrow_copy_constructible<T>::value
    || std::is_nothrow_move_constructible<T>::value;

  // TODO use is_trivially_copyable instead of is_pod when available
  static constexpr bool t_is_trivially_copyable = std::is_pod<T>::value;

  // Guard to deallocate buffer on exception
  typedef typename detail::allocation_guard<Allocator> allocation_guard;

  // Guard to destroy already constructed elements on exception
  // TODO enable null guard if required opearations are noexcept
  typedef typename detail::opt_construction_guard<Allocator> construction_guard;

  // Guard to destroy either source or target, on success or on exception
  typedef typename detail::opt_nand_construction_guard<Allocator> move_guard;
};

}}} // namespace boost::double_ended::detail

#endif // BOOST_DOUBLE_ENDED_DETAIL_ALLOCATOR_HPP
