//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Benedek Thaler 2015-2016. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://erenon.hu/double_ended for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_DOUBLE_ENDED_DETAIL_GUARDS_HPP
#define BOOST_DOUBLE_ENDED_DETAIL_GUARDS_HPP

#include <boost/move/core.hpp> // BOOST_MOVABLE_BUT_NOT_COPYABLE

namespace boost {
namespace double_ended {
namespace detail {

class null_construction_guard
{
public:
  template <typename... Args>
  null_construction_guard(Args&&...) {}

  void release() {}
  void extend() {}
};

template <typename Allocator>
class construction_guard
{
  typedef typename std::allocator_traits<Allocator>::pointer pointer;
  typedef typename std::allocator_traits<Allocator>::size_type size_type;

  BOOST_MOVABLE_BUT_NOT_COPYABLE(construction_guard)

public:
  construction_guard() = default;

  construction_guard(pointer alloc_ptr, Allocator& allocator)
    :_alloc_ptr(alloc_ptr),
     _elem_count(0),
     _allocator(&allocator)
  {}

  construction_guard(construction_guard&& rhs)
    :_alloc_ptr(rhs._alloc_ptr),
     _elem_count(rhs._elem_count),
     _allocator(rhs._allocator)
  {
    rhs._elem_count = 0;
  }

  ~construction_guard()
  {
    while (_elem_count--)
    {
      std::allocator_traits<Allocator>::destroy(*_allocator, _alloc_ptr++);
    }
  }

  void release()
  {
    _elem_count = 0;
  }

  void extend()
  {
    ++_elem_count;
  }

private:
  pointer _alloc_ptr;
  size_type _elem_count = 0;
  Allocator* _allocator;
};

template <typename Allocator>
using opt_construction_guard = typename std::conditional<
  std::is_trivially_destructible<typename std::allocator_traits<Allocator>::value_type>::value,
  null_construction_guard,
  construction_guard<Allocator>
>::type;

/**
 * Has two ranges
 *
 * On success, destroys the first range (src),
 * on failure, destroys the second range (dst).
 *
 * Can be used when copying/moving a range
 */
template <class Allocator>
class nand_construction_guard
{
  typedef typename std::allocator_traits<Allocator>::pointer pointer;
  typedef typename std::allocator_traits<Allocator>::size_type size_type;

  construction_guard<Allocator> _src;
  construction_guard<Allocator> _dst;
  bool _dst_released = false;

public:
  nand_construction_guard() = default;

  nand_construction_guard(
    pointer src, Allocator& src_alloc,
    pointer dst, Allocator& dst_alloc
  )
    :_src(src, src_alloc),
     _dst(dst, dst_alloc),
     _dst_released(false)
  {}

  nand_construction_guard(nand_construction_guard&& rhs) = default;

  void extend()
  {
    _src.extend();
    _dst.extend();
  }

  void release() // on success
  {
    _dst.release();
    _dst_released = true;
  }

  ~nand_construction_guard()
  {
    if (! _dst_released) { _src.release(); }
  }
};

template <typename Allocator>
using opt_nand_construction_guard = typename std::conditional<
  std::is_trivially_destructible<typename std::allocator_traits<Allocator>::value_type>::value,
  null_construction_guard,
  nand_construction_guard<Allocator>
>::type;

template <typename Allocator>
class allocation_guard
{
  typedef typename std::allocator_traits<Allocator>::pointer pointer;
  typedef typename std::allocator_traits<Allocator>::size_type size_type;

  BOOST_MOVABLE_BUT_NOT_COPYABLE(allocation_guard)

public:
  allocation_guard(pointer alloc_ptr, size_type alloc_size, Allocator& allocator)
    :_alloc_ptr(alloc_ptr),
     _alloc_size(alloc_size),
     _allocator(allocator)
  {}

  ~allocation_guard()
  {
    if (_alloc_ptr)
    {
      std::allocator_traits<Allocator>::deallocate(_allocator, _alloc_ptr, _alloc_size);
    }
  }

  void release()
  {
    _alloc_ptr = nullptr;
  }

private:
  pointer _alloc_ptr;
  size_type _alloc_size;
  Allocator& _allocator;
};

}}} // namespace boost::double_ended::detail

#endif // BOOST_DOUBLE_ENDED_DETAIL_GUARDS_HPP
