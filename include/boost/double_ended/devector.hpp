//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Benedek Thaler 2015-2016. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://erenon.hu/double_ended for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_DOUBLE_ENDED_DEVECTOR_HPP
#define BOOST_DOUBLE_ENDED_DEVECTOR_HPP

#include <algorithm>
#include <cstring> // memcpy
#include <stdexcept>
#include <type_traits>

#include <boost/assert.hpp>
#include <boost/aligned_storage.hpp>
#include <boost/throw_exception.hpp>

#include <boost/double_ended/detail/allocator.hpp>
#include <boost/double_ended/detail/iterators.hpp>
#include <boost/double_ended/detail/algorithm.hpp>

namespace boost {
namespace double_ended {

/**
 * Instructs devector to make space for `Size` elements
 * inline with its internals, allowing to store that much elements
 * without allocating memory.
 *
 * Models the `SmallBufferPolicy` concept of the devector class.
 */
template <unsigned Size>
struct small_buffer_size
{
  BOOST_STATIC_CONSTANT(unsigned, size = Size);
};

/**
 * Controls a devectors reallocation policy.
 *
 * Models the `GrowthPolicy` concept of the devector class.
 */
struct devector_growth_policy
{
  /**
   * Sets the `size_type` type of the devector.
   */
  typedef unsigned int size_type;

  /**
   * **Returns**: 4 times the old capacity or 16 if it's 0.
   *
   * @param capacity The current capacity of the devector, equals to `capacity()`.
   */
  static size_type new_capacity(size_type capacity)
  {
    return (capacity) ? capacity * 4u : 16u;
  }

  /**
   * **Returns**: true, if the contents fit in the small buffer.
   *
   * @param size The element count of the devector, equals to `size()`
   * @param capacity The current capacity of the devector, equals to `capacity()`.
   * @param small_buffer_size The size of the small buffer, specified by the `SmallBufferPolicy`.
   */
  static bool should_shrink(size_type size, size_type capacity, size_type small_buffer_size)
  {
    (void)capacity;
    return size <= small_buffer_size;
  }
};

struct reserve_only_tag {};
struct unsafe_uninitialized_tag {};

/**
 * A vector-like sequence container providing front and back operations
 * (e.g: `push_front`/`pop_front`/`push_back`/`pop_back`) with amortized linear complexity,
 * small buffer optimization and unsafe methods geared towards additional performance.
 *
 * Models the [SequenceContainer], [ReversibleContainer], and [AllocatorAwareContainer] concepts.
 *
 * **Requires**:
 *  - `T` shall be [MoveInsertable] into the devector.
 *  - `T` shall be [Erasable] from any `devector<T, Allocator, SBP, GP>`.
 *  - `SmallBufferPolicy`, `GrowthPolicy`, and `Allocator` must model the concepts with the same names.
 *
 * **Definition**: `T` is `NothrowConstructible` if it's either nothrow move constructible or
 * nothrow copy constructible.
 *
 * **Definition**: `T` is `NothrowAssignable` if it's either nothrow move assignable or
 * nothrow copy assignable.
 *
 * **Definition**: A devector `d` is _small_ if it has a non-zero size small buffer and
 * `d.size()` is smaller or equal than the size of the small buffer of `d`.
 *
 * **Exceptions**: The exception specifications assume `T` is nothrow [Destructible].
 *
 * Most methods providing the strong exception guarantee assume `T` either has a move
 * constructor marked noexcept or is [CopyInsertable] into the devector. If it isn't true,
 * and the move constructor throws, the guarantee is waived and the effects are unspecified.
 *
 * In addition to the exceptions specified in the **Throws** clause, the following operations
 * of `T` can throw when any of the specified concept is required (assuming `std::allocator` is used):
 *  - [DefaultInsertable][]: Default constructor
 *  - [MoveInsertable][]: Move constructor
 *  - [CopyInsertable][]: Copy constructor
 *  - [DefaultConstructible][]: Default constructor
 *  - [EmplaceConstructible][]: Constructor selected by the given arguments
 *  - [MoveAssignable][]: Move assignment operator
 *  - [CopyAssignable][]: Copy assignment operator
 *
 * Furthermore, not `noexcept` methods throws whatever the allocator throws
 * if memory allocation fails. Such methods also throw `std::length_error` if the capacity
 * exceeds `max_size()`.
 *
 * **Remark**: If a method invalidates some iterators, it also invalidates references
 * and pointers to the elements pointed by the invalidated iterators.
 *
 * **Policies**:
 *
 * The type `SBP` models the `SmallBufferPolicy` concept if it satisfies the following requirements:
 *
 * Expression | Return type | Description
 * -----------|-------------|------------
 * `SBP::size` | `size_type` | The total number of elements that can be stored without allocation
 *
 * @ref small_buffer_size models the `SmallBufferPolicy` concept.
 *
 * The type `GP` models the `GrowthPolicy` concept if it satisfies the following requirements
 * when `gp` is an instance of such a class:
 *
 * Expression | Return type | Description
 * -----------|-------------|------------
 * `GP::size_type` | Unsigned integral type | Sets the `size_type` type of the `devector`, thus the maximum number of elements it can hold. Must be a compile time constant.
 * `gp.new_capacity(old_capacity)` | `size_type` | Computes the new capacity to be allocated. The returned value must be greater than `old_capacity`. This method is always used when a new buffer gets allocated. `old_capacity` is convertible to `size_type`.
 * `gp.should_shrink(size, capacity, small_buffer_size)` | `bool` | Returns `true`, if superfluous memory should be released. Arguments are convertible to `size_type`.
 *
 * @ref devector_growth_policy models the `GrowthPolicy` concept.
 *
 * [SequenceContainer]: http://en.cppreference.com/w/cpp/concept/SequenceContainer
 * [ReversibleContainer]: http://en.cppreference.com/w/cpp/concept/ReversibleContainer
 * [AllocatorAwareContainer]: http://en.cppreference.com/w/cpp/concept/AllocatorAwareContainer
 * [DefaultInsertable]: http://en.cppreference.com/w/cpp/concept/DefaultInsertable
 * [MoveInsertable]: http://en.cppreference.com/w/cpp/concept/MoveInsertable
 * [CopyInsertable]: http://en.cppreference.com/w/cpp/concept/CopyInsertable
 * [Erasable]: http://en.cppreference.com/w/cpp/concept/Erasable
 * [DefaultConstructible]: http://en.cppreference.com/w/cpp/concept/DefaultConstructible
 * [Destructible]: http://en.cppreference.com/w/cpp/concept/Destructible
 * [EmplaceConstructible]: http://en.cppreference.com/w/cpp/concept/EmplaceConstructible
 * [MoveAssignable]: http://en.cppreference.com/w/cpp/concept/MoveAssignable
 * [CopyAssignable]: http://en.cppreference.com/w/cpp/concept/CopyAssignable
 */
template <
  typename T,
  typename SmallBufferPolicy = small_buffer_size<0>,
  typename GrowthPolicy = devector_growth_policy,
  typename Allocator = std::allocator<T>
>
class devector
  #ifndef BOOST_DOUBLE_ENDED_DOXYGEN_INVOKED
    : Allocator
  #endif // ifndef BOOST_DOUBLE_ENDED_DOXYGEN_INVOKED
{
#ifndef BOOST_DOUBLE_ENDED_DOXYGEN_INVOKED

  typedef detail::allocator_traits<Allocator> allocator_traits;

#endif // ifndef BOOST_DOUBLE_ENDED_DOXYGEN_INVOKED

public:
  // Standard Interface Types:
  typedef T value_type;
  typedef Allocator allocator_type;
  typedef value_type& reference;
  typedef const value_type& const_reference;
  typedef typename allocator_traits::pointer pointer;
  typedef typename allocator_traits::const_pointer const_pointer;
  typedef pointer iterator;
  typedef const_pointer const_iterator;
  typedef typename GrowthPolicy::size_type size_type;
  typedef typename std::make_signed<size_type>::type difference_type;
  typedef std::reverse_iterator<iterator> reverse_iterator;
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

#ifndef BOOST_DOUBLE_ENDED_DOXYGEN_INVOKED
private:

  typedef typename allocator_traits::allocation_guard allocation_guard;
  typedef typename allocator_traits::construction_guard construction_guard;
  typedef typename allocator_traits::move_guard move_guard;

  // Random access pseudo iterator always yielding to the same result
  typedef detail::constant_iterator<T, difference_type> cvalue_iterator;

  static constexpr bool t_is_nothrow_constructible = allocator_traits::t_is_nothrow_constructible;
  static constexpr bool t_is_trivially_copyable = allocator_traits::t_is_trivially_copyable;

  static constexpr size_type sbuffer_size = SmallBufferPolicy::size;
  static constexpr bool no_small_buffer = (sbuffer_size == 0);

#endif // ifndef BOOST_DOUBLE_ENDED_DOXYGEN_INVOKED

// Standard Interface
public:
  // construct/copy/destroy

  /**
   * **Effects**: Constructs an empty devector.
   *
   * **Postcondition**: `empty() && front_free_capacity() == 0
   * && back_free_capacity() == small buffer size`.
   *
   * **Complexity**: Constant.
   */
  devector() noexcept
    :_buffer(_storage.small_buffer_address()),
     _front_index(0),
     _back_index(0)
  {}

  /**
   * **Effects**: Constructs an empty devector, using the specified allocator.
   *
   * **Postcondition**: `empty() && front_free_capacity() == 0
   * && back_free_capacity() == small buffer size`.
   *
   * **Complexity**: Constant.
   */
  explicit devector(const Allocator& allocator) noexcept
    :Allocator(allocator),
     _buffer(_storage.small_buffer_address()),
     _front_index(0),
     _back_index(0)
  {}

  /**
   * **Effects**: Constructs an empty devector, using the specified allocator
   * and reserves `n` slots as if `reserve(n)` was called.
   *
   * **Postcondition**: `empty() && front_free_capacity() == 0
   * && back_free_capacity() >= n`.
   *
   * **Exceptions**: Strong exception guarantee.
   *
   * **Complexity**: Constant.
   */
  devector(size_type n, reserve_only_tag, const Allocator& allocator = Allocator())
    :devector(0, n, reserve_only_tag{}, allocator)
  {}

  /**
   * **Effects**: Constructs an empty devector, using the specified allocator
   * and reserves `front_cap + back_cap` slots as if `reserve_front(front_cap)` and
   * `reserve_back(back_cap)` was called.
   *
   * **Postcondition**: `empty() && front_free_capacity() == front_cap
   * && back_free_capacity() >= back_cap`.
   *
   * **Exceptions**: Strong exception guarantee.
   *
   * **Complexity**: Constant.
   *
   * **Remarks**: This constructor can be used to split the small buffer
   * between expected front and back insertions.
   */
  devector(size_type front_cap, size_type back_cap, reserve_only_tag, const Allocator& allocator = Allocator())
    :Allocator(allocator),
     _storage(front_cap + back_cap),
     _buffer(allocate(_storage._capacity)),
     _front_index(front_cap),
     _back_index(front_cap)
  {}

  /**
   * **Unsafe constructor**, use with care.
   *
   * **Effects**: Constructs a devector containing `n` uninitialized elements.
   *
   * **Postcondition**: `size() == n && front_free_capacity() == 0`.
   *
   * **Exceptions**: Strong exception guarantee.
   *
   * **Complexity**: Constant.
   *
   * **Remarks**: The devector does not keep track of initialization of the elements,
   * the initially provided uninitialized elements must be manually initialized,
   * e.g: through the pointer returned by `data()`. This constructor is unsafe
   * because the user must take care of the initialization, before the destructor
   * is called at latest. Failing that, the destructor would invoke undefined
   * behavior if the destructor of `T` is non-trivial, even if the devectors
   * destructor is called by the stack unwinding effect of an exception.
   */
  devector(size_type n, unsafe_uninitialized_tag, const Allocator& allocator = Allocator())
    :Allocator(allocator),
     _storage(n),
     _buffer(allocate(_storage._capacity)),
     _front_index(),
     _back_index(n)
  {}

  /**
   * [DefaultInsertable]: http://en.cppreference.com/w/cpp/concept/DefaultInsertable
   *
   * **Effects**: Constructs a devector with `n` default-inserted elements using the specified allocator.
   *
   * **Requires**: `T` shall be [DefaultInsertable] into `*this`.
   *
   * **Postcondition**: `size() == n && front_free_capacity() == 0`.
   *
   * **Exceptions**: Strong exception guarantee.
   *
   * **Complexity**: Linear in `n`.
   */
  explicit devector(size_type n, const Allocator& allocator = Allocator())
    :Allocator(allocator),
     _storage(n),
     _buffer(allocate(_storage._capacity)),
     _front_index(),
     _back_index(n)
  {
    // Cannot use construct_from_range/constant_iterator and copy_range,
    // because we are not allowed to default construct T

    allocation_guard buffer_guard(_buffer, _storage._capacity, get_allocator_ref());
    if (is_small()) { buffer_guard.release(); } // avoid disposing small buffer

    construction_guard copy_guard(_buffer, get_allocator_ref());

    for (size_type i = 0; i < n; ++i)
    {
      alloc_construct(_buffer + i);
      copy_guard.extend();
    }

    copy_guard.release();
    buffer_guard.release();

    BOOST_ASSERT(invariants_ok());
  }

  /**
   * [CopyInsertable]: http://en.cppreference.com/w/cpp/concept/CopyInsertable
   *
   * **Effects**: Constructs a devector with `n` copies of `value`, using the specified allocator.
   *
   * **Requires**: `T` shall be [CopyInsertable] into `*this`.
   *
   * **Postcondition**: `size() == n && front_free_capacity() == 0`.
   *
   * **Exceptions**: Strong exception guarantee.
   *
   * **Complexity**: Linear in `n`.
   */
  devector(size_type n, const T& value, const Allocator& allocator = Allocator())
    :Allocator(allocator),
     _storage(n),
     _buffer(allocate(_storage._capacity)),
     _front_index(),
     _back_index(n)
  {
    construct_from_range(cvalue_iterator(value, n), cvalue_iterator());

    BOOST_ASSERT(invariants_ok());
  }

  /**
   * **Effects**: Constructs a devector equal to the range `[first,last)`, using the specified allocator.
   *
   * **Requires**: `T` shall be [EmplaceConstructible] into `*this` from `*first`. If the specified
   * iterator does not meet the forward iterator requirements, `T` shall also be [MoveInsertable]
   * into `*this`.
   *
   * **Postcondition**: `size() == std::distance(first, last)
   *
   * **Exceptions**: Strong exception guarantee.
   *
   * **Complexity**: Makes only `N` calls to the copy constructor of `T` (where `N` is the distance between `first`
   * and `last`), at most one allocation and no reallocations if iterators first and last are of forward,
   * bidirectional, or random access categories. It makes `O(N)` calls to the copy constructor of `T`
   * and `O(log(N)) reallocations if they are just input iterators.
   *
   * **Remarks**: Each iterator in the range `[first,last)` shall be dereferenced exactly once,
   * unless an exception is thrown.
   *
   * [EmplaceConstructible]: http://en.cppreference.com/w/cpp/concept/EmplaceConstructible
   * [MoveInsertable]: http://en.cppreference.com/w/cpp/concept/MoveInsertable
   */
  template <BOOST_DOUBLE_ENDED_REQUIRE_INPUT_ITERATOR(InputIterator)>
  devector(InputIterator first, InputIterator last, const Allocator& allocator = Allocator())
    :devector(allocator) // Use the destructor to clean up on exception
  {
    _front_index = _back_index = 0; // use the full small buffer

    while (first != last)
    {
      emplace_back(*first++);
    }

    BOOST_ASSERT(invariants_ok());
  }

#ifndef BOOST_DOUBLE_ENDED_DOXYGEN_INVOKED

  template <BOOST_DOUBLE_ENDED_REQUIRE_FW_ITERATOR(ForwardIterator)>
  devector(ForwardIterator first, ForwardIterator last, const Allocator& allocator = Allocator())
    :Allocator(allocator),
     _storage(std::distance(first, last)),
     _buffer(allocate(_storage._capacity)),
     _front_index(),
     _back_index(std::distance(first, last))
  {
    construct_from_range(first, last);

    BOOST_ASSERT(invariants_ok());
  }

#endif // ifndef BOOST_DOUBLE_ENDED_DOXYGEN_INVOKED

  /**
   * [CopyInsertable]: http://en.cppreference.com/w/cpp/concept/CopyInsertable
   *
   * **Effects**: Copy constructs a devector.
   *
   * **Requires**: `T` shall be [CopyInsertable] into `*this`.
   *
   * **Postcondition**: `this->size() == x.size() && front_free_capacity() == 0`.
   *
   * **Exceptions**: Strong exception guarantee.
   *
   * **Complexity**: Linear in the size of `x`.
   */
  devector(const devector& x)
    :devector(
      x.begin(), x.end(),
      allocator_traits::select_on_container_copy_construction(x.get_allocator_ref())
    )
  {}

  /**
   * [CopyInsertable]: http://en.cppreference.com/w/cpp/concept/CopyInsertable
   *
   * **Effects**: Copy constructs a devector, using the specified allocator.
   *
   * **Requires**: `T` shall be [CopyInsertable] into `*this`.
   *
   * **Postcondition**: `this->size() == x.size() && front_free_capacity() == 0`.
   *
   * **Exceptions**: Strong exception guarantee.
   *
   * **Complexity**: Linear in the size of `x`.
   */
  devector(const devector& x, const Allocator& allocator)
    :devector(x.begin(), x.end(), allocator)
  {}

  /**
   * **Effects**: Moves `rhs`'s resources to `*this`.
   *
   * **Throws**: If `rhs` _small_ and `T`'s move constructor throws.
   *
   * **Postcondition**: `rhs` is left in an unspecified but valid state.
   *
   * **Exceptions**: Strong exception guarantee if not `noexcept`.
   *
   * **Complexity**: Linear in the size of the small buffer.
   */
  devector(devector&& rhs) noexcept(no_small_buffer || t_is_nothrow_constructible)
    :devector(std::move(rhs), rhs.get_allocator_ref())
  {}

  /**
   * **Effects**: Moves `rhs`'s resources to `*this`, using the specified allocator.
   *
   * **Throws**: If `rhs` _small_ and `T`'s move constructor throws.
   *
   * **Postcondition**: `rhs` is left in an unspecified but valid state.
   *
   * **Exceptions**: Strong exception guarantee if not `noexcept`.
   *
   * **Complexity**: Linear in the size of the small buffer.
   */
  devector(devector&& rhs, const Allocator& allocator) noexcept(
    no_small_buffer || t_is_nothrow_constructible
  )
    :Allocator(allocator),
     _storage(rhs.capacity()),
     _buffer(
       (rhs.is_small()) ? _storage.small_buffer_address() : rhs._buffer
     ),
     _front_index(rhs._front_index),
     _back_index(rhs._back_index)
  {
    // TODO should move elems-by-elems if the two allocators differ
    if (rhs.is_small() == false)
    {
      // buffer is already acquired, reset rhs
      rhs._storage._capacity = sbuffer_size;
      rhs._buffer = rhs._storage.small_buffer_address();
      rhs._front_index = 0;
      rhs._back_index = 0;
    }
    else
    {
      // elems must be moved/copied to small buffer
      opt_move_or_copy(rhs.begin(), rhs.end(), begin());
    }

    BOOST_ASSERT(    invariants_ok());
    BOOST_ASSERT(rhs.invariants_ok());
  }

  /**
   * **Equivalent to**: `devector(il.begin(), il.end())` or `devector(il.begin(), il.end(), allocator)`.
   */
  devector(const std::initializer_list<T>& il, const Allocator& allocator = Allocator())
    :devector(il.begin(), il.end(), allocator)
  {}

  /**
   * **Effects**: Destroys the devector. All stored values are destroyed and
   * used memory, if any, deallocated.
   *
   * **Complexity**: Linear in the size of `*this`.
   */
  ~devector() noexcept
  {
    destroy_elements(_buffer + _front_index, _buffer + _back_index);
    deallocate_buffer();
  }

  /**
   * **Effects**: Copies elements of `x` to `*this`. Previously
   * held elements get copy assigned to or destroyed.
   *
   * **Requires**: `T` shall be [CopyInsertable] into `*this`.
   *
   * **Postcondition**: `this->size() == x.size()`, the elements of
   * `*this` are copies of elements in `x` in the same order.
   *
   * **Returns**: `*this`.
   *
   * **Exceptions**: Strong exception guarantee if `T` is `NothrowConstructible`
   * and the allocator is allowed to be propagated
   * ([propagate_on_container_copy_assignment] is true),
   * Basic exception guarantee otherwise.
   *
   * **Complexity**: Linear in the size of `x` and `*this`.
   *
   * [CopyInsertable]: http://en.cppreference.com/w/cpp/concept/CopyInsertable
   * [propagate_on_container_copy_assignment]: http://en.cppreference.com/w/cpp/memory/allocator_traits
   */
  devector& operator=(const devector& x)
  {
    if (this == &x) { return *this; } // skip self

    if (allocator_traits::propagate_on_container_copy_assignment::value)
    {
      if (get_allocator_ref() != x.get_allocator_ref())
      {
        // new allocator cannot free existing storage
        clear();
        deallocate_buffer();
        _storage._capacity = sbuffer_size;
        _buffer = _storage.small_buffer_address();
      }

      get_allocator_ref() = x.get_allocator_ref();
    }

    size_type n = x.size();
    if (capacity() >= n)
    {
      const_iterator first = x.begin();
      const_iterator last = x.end();

      overwrite_buffer(first, last);
    }
    else
    {
      allocate_and_copy_range(x.begin(), x.end());
    }

    BOOST_ASSERT(invariants_ok());

    return *this;
  }

  /**
   * [MoveInsertable]: http://en.cppreference.com/w/cpp/concept/MoveInsertable
   *
   * **Effects**: Moves elements of `x` to `*this`. Previously
   * held elements get move/copy assigned to or destroyed.
   *
   * **Requires**: `T` shall be [MoveInsertable] into `*this`.
   *
   * **Postcondition**: `x` is left in an unspecified but valid state.
   *
   * **Returns**: `*this`.
   *
   * **Exceptions**: Basic exception guarantee if not `noexcept`.
   *
   * Remark: If `x` is _small_ or the allocator forbids propagation,
   * the contents of `x` is moved one-by-one instead of stealing the buffer.
   *
   * **Complexity**: Linear in the size of the small buffer plus the size of `*this`
   * and the size of `x` if the allocator forbids propagation.
   */
  devector& operator=(devector&& x) noexcept(
      (no_small_buffer || t_is_nothrow_constructible)
   && (allocator_traits::propagate_on_move_assignment || allocator_traits::is_always_equal)
  )
  {
    constexpr bool copy_alloc = allocator_traits::propagate_on_move_assignment;
    const bool equal_alloc = (get_allocator_ref() == x.get_allocator_ref());

    if ((copy_alloc || equal_alloc) && x.is_small() == false)
    {
      clear();
      deallocate_buffer();

      if (copy_alloc)
      {
        get_allocator_ref() = std::move(x.get_allocator_ref());
      }

      _storage._capacity = x._storage._capacity;
      _buffer = x._buffer;
      _front_index = x._front_index;
      _back_index = x._back_index;

      // leave x in valid state
      x._storage._capacity = sbuffer_size;
      x._buffer = _storage.small_buffer_address();
      x._back_index = x._front_index = 0;
    }
    else
    {
      // if the allocator shouldn't be copied and they do not compare equal
      // or the rvalue has a small buffer, we can't steal memory.

      auto xbegin = std::make_move_iterator(x.begin());
      auto xend = std::make_move_iterator(x.end());

      if (copy_alloc)
      {
        get_allocator_ref() = std::move(x.get_allocator_ref());
      }

      if (capacity() >= x.size())
      {
        overwrite_buffer(xbegin, xend);
      }
      else
      {
        allocate_and_copy_range(xbegin, xend);
      }
    }

    BOOST_ASSERT(invariants_ok());

    return *this;
  }

  /**
   * **Effects**: Copies elements of `il` to `*this`. Previously
   * held elements get copy assigned to or destroyed.
   *
   * **Requires**: `T` shall be [CopyInsertable] into `*this` and [CopyAssignable].
   *
   * **Postcondition**: `this->size() == il.size()`, the elements of
   * `*this` are copies of elements in `il` in the same order.
   *
   * **Exceptions**: Strong exception guarantee if `T` is nothrow copy assignable
   * from `T` and `NothrowConstructible`, Basic exception guarantee otherwise.
   *
   * **Returns**: `*this`.
   *
   * **Complexity**: Linear in the size of `il` and `*this`.
   *
   * [CopyInsertable]: http://en.cppreference.com/w/cpp/concept/CopyInsertable
   * [CopyAssignable]: http://en.cppreference.com/w/cpp/concept/CopyAssignable
   */
  devector& operator=(std::initializer_list<T> il)
  {
    assign(il.begin(), il.end());
    return *this;
  }

  /**
   * **Effects**: Replaces elements of `*this` with a copy of `[first,last)`.
   * Previously held elements get copy assigned to or destroyed.
   *
   * **Requires**: `T` shall be [EmplaceConstructible] from `*first`. If the specified iterator
   * does not meet the forward iterator requirements, `T` shall be also [MoveInsertable] into `*this`.
   *
   * **Precondition**: `first` and `last` are not iterators into `*this`.
   *
   * **Postcondition**: `size() == N`, where `N` is the distance between `first` and `last`.
   *
   * **Exceptions**: Strong exception guarantee if `T` is nothrow copy assignable
   * from `*first` and `NothrowConstructible`, Basic exception guarantee otherwise.
   *
   * **Complexity**: Linear in the distance between `first` and `last`.
   * Makes a single reallocation at most if the iterators `first` and `last`
   * are of forward, bidirectional, or random access categories. It makes
   * `O(log(N))` reallocations if they are just input iterators.
   *
   * **Remarks**: Each iterator in the range `[first,last)` shall be dereferenced exactly once,
   * unless an exception is thrown.
   *
   * [EmplaceConstructible]: http://en.cppreference.com/w/cpp/concept/EmplaceConstructible
   * [MoveInsertable]: http://en.cppreference.com/w/cpp/concept/MoveInsertable
   */
  template <BOOST_DOUBLE_ENDED_REQUIRE_INPUT_ITERATOR(InputIterator)>
  void assign(InputIterator first, InputIterator last)
  {
    overwrite_buffer_impl(first, last);
    while (first != last)
    {
      emplace_back(*first++);
    }
  }

#ifndef BOOST_DOUBLE_ENDED_DOXYGEN_INVOKED

  template <BOOST_DOUBLE_ENDED_REQUIRE_FW_ITERATOR(ForwardIterator)>
  void assign(ForwardIterator first, ForwardIterator last)
  {
    const size_type n = std::distance(first, last);

    if (capacity() >= n)
    {
      overwrite_buffer(first, last);
    }
    else
    {
      allocate_and_copy_range(first, last);
    }

    BOOST_ASSERT(invariants_ok());
  }

#endif // ifndef BOOST_DOUBLE_ENDED_DOXYGEN_INVOKED

  /**
   * **Effects**: Replaces elements of `*this` with `n` copies of `u`.
   * Previously held elements get copy assigned to or destroyed.
   *
   * **Requires**: `T` shall be [CopyInsertable] into `*this` and
   * [CopyAssignable].
   *
   * **Precondition**: `u` is not a reference into `*this`.
   *
   * **Postcondition**: `size() == n` and the elements of
   * `*this` are copies of `u`.
   *
   * **Exceptions**: Strong exception guarantee if `T` is nothrow copy assignable
   * from `u` and `NothrowConstructible`, Basic exception guarantee otherwise.
   *
   * **Complexity**: Linear in `n` and the size of `*this`.
   *
   * [CopyInsertable]: http://en.cppreference.com/w/cpp/concept/CopyInsertable
   * [CopyAssignable]: http://en.cppreference.com/w/cpp/concept/CopyAssignable
   */
  void assign(size_type n, const T& u)
  {
    cvalue_iterator first(u, n);
    cvalue_iterator last;

    assign(first, last);
  }

  /** **Equivalent to**: `assign(il.begin(), il.end())`. */
  void assign(std::initializer_list<T> il)
  {
    assign(il.begin(), il.end());
  }

  /**
   * **Returns**: A copy of the allocator associated with the container.
   *
   * **Complexity**: Constant.
   */
  allocator_type get_allocator() const noexcept
  {
    return static_cast<const allocator_type&>(*this);
  }

  // iterators

  /**
   * **Returns**: A iterator pointing to the first element in the devector,
   * or the past the end iterator if the devector is empty.
   *
   * **Complexity**: Constant.
   */
  iterator begin() noexcept
  {
    return _buffer + _front_index;
  }

  /**
   * **Returns**: A constant iterator pointing to the first element in the devector,
   * or the past the end iterator if the devector is empty.
   *
   * **Complexity**: Constant.
   */
  const_iterator begin() const noexcept
  {
    return _buffer + _front_index;
  }

  /**
   * **Returns**: An iterator pointing past the last element of the container.
   *
   * **Complexity**: Constant.
   */
  iterator end() noexcept
  {
    return _buffer + _back_index;
  }

  /**
   * **Returns**: A constant iterator pointing past the last element of the container.
   *
   * **Complexity**: Constant.
   */
  const_iterator end() const noexcept
  {
    return _buffer + _back_index;
  }

  /**
   * **Returns**: A reverse iterator pointing to the first element in the reversed devector,
   * or the reverse past the end iterator if the devector is empty.
   *
   * **Complexity**: Constant.
   */
  reverse_iterator rbegin() noexcept
  {
    return reverse_iterator(_buffer + _back_index);
  }

  /**
   * **Returns**: A constant reverse iterator
   * pointing to the first element in the reversed devector,
   * or the reverse past the end iterator if the devector is empty.
   *
   * **Complexity**: Constant.
   */
  const_reverse_iterator rbegin() const noexcept
  {
    return const_reverse_iterator(_buffer + _back_index);
  }

  /**
   * **Returns**: A reverse iterator pointing past the last element in the
   * reversed container, or to the beginning of the reversed container if it's empty.
   *
   * **Complexity**: Constant.
   */
  reverse_iterator rend() noexcept
  {
    return reverse_iterator(_buffer + _front_index);
  }

  /**
   * **Returns**: A constant reverse iterator pointing past the last element in the
   * reversed container, or to the beginning of the reversed container if it's empty.
   *
   * **Complexity**: Constant.
   */
  const_reverse_iterator rend() const noexcept
  {
    return const_reverse_iterator(_buffer + _front_index);
  }

  /**
   * **Returns**: A constant iterator pointing to the first element in the devector,
   * or the past the end iterator if the devector is empty.
   *
   * **Complexity**: Constant.
   */
  const_iterator cbegin() const noexcept
  {
    return _buffer + _front_index;
  }

  /**
   * **Returns**: A constant iterator pointing past the last element of the container.
   *
   * **Complexity**: Constant.
   */
  const_iterator cend() const noexcept
  {
    return _buffer + _back_index;
  }

  /**
   * **Returns**: A constant reverse iterator
   * pointing to the first element in the reversed devector,
   * or the reverse past the end iterator if the devector is empty.
   *
   * **Complexity**: Constant.
   */
  const_reverse_iterator crbegin() const noexcept
  {
    return const_reverse_iterator(_buffer + _back_index);
  }

  /**
   * **Returns**: A constant reverse iterator pointing past the last element in the
   * reversed container, or to the beginning of the reversed container if it's empty.
   *
   * **Complexity**: Constant.
   */
  const_reverse_iterator crend() const noexcept
  {
    return const_reverse_iterator(_buffer + _front_index);
  }

  // capacity

  /**
   * **Returns**: True, if `size() == 0`, false otherwise.
   *
   * **Complexity**: Constant.
   */
  bool empty() const noexcept
  {
    return _front_index == _back_index;
  }

  /**
   * **Returns**: The number of elements the devector contains.
   *
   * **Complexity**: Constant.
   */
  size_type size() const noexcept
  {
    return _back_index - _front_index;
  }

  /**
   * **Returns**: The maximum number of elements the devector could possibly hold.
   *
   * **Complexity**: Constant.
   */
  size_type max_size() const noexcept
  {
    auto alloc_max = allocator_traits::max_size(get_allocator_ref());
    auto size_type_max = (std::numeric_limits<size_type>::max)();
    return (alloc_max <= size_type_max) ? size_type(alloc_max) : size_type_max;
  }

  /**
   * **Returns**: The total number of elements that the devector can hold without requiring reallocation.
   *
   * **Complexity**: Constant.
   */
  size_type capacity() const noexcept
  {
    return _storage._capacity;
  }

  /**
   * **Returns**: The total number of elements that can be pushed to the front of the
   * devector without requiring reallocation.
   *
   * **Complexity**: Constant.
   */
  size_type front_free_capacity() const noexcept
  {
    return _front_index;
  }

  /**
   * **Returns**: The total number of elements that can be pushed to the back of the
   * devector without requiring reallocation.
   *
   * **Complexity**: Constant.
   */
  size_type back_free_capacity() const noexcept
  {
    return _storage._capacity - _back_index;
  }

  /** **Equivalent to**: `resize_back(sz)` */
  void resize(size_type sz) { resize_back(sz); }

  /** **Equivalent to**: `resize_back(sz, c)` */
  void resize(size_type sz, const T& c) { resize_back(sz, c); }

  /**
   * **Effects**: If `sz` is greater than the size of `*this`,
   * additional value-initialized elements are inserted
   * to the front. Invalidates iterators if reallocation is needed.
   * If `sz` is smaller than than the size of `*this`,
   * elements are popped from the front.
   *
   * **Requires**: T shall be [MoveInsertable] into *this and [DefaultConstructible].
   *
   * **Postcondition**: `sz == size()`.
   *
   * **Exceptions**: Strong exception guarantee.
   *
   * **Complexity**: Linear in the size of `*this` and `sz`.
   *
   * [MoveInsertable]: http://en.cppreference.com/w/cpp/concept/MoveInsertable
   * [DefaultConstructible]: http://en.cppreference.com/w/cpp/concept/DefaultConstructible
   */
  void resize_front(size_type sz)
  {
    resize_front_impl(sz);
    BOOST_ASSERT(invariants_ok());
  }

  /**
   * [CopyInsertable]: http://en.cppreference.com/w/cpp/concept/CopyInsertable
   *
   * **Effects**: If `sz` is greater than the size of `*this`,
   * copies of `c` are inserted to the front.
   * Invalidates iterators if reallocation is needed.
   * If `sz` is smaller than than the size of `*this`,
   * elements are popped from the front.
   *
   * **Postcondition**: `sz == size()`.
   *
   * **Requires**: `T` shall be [CopyInsertable] into `*this`.
   *
   * **Exceptions**: Strong exception guarantee.
   *
   * **Complexity**: Linear in the size of `*this` and `sz`.
   */
  void resize_front(size_type sz, const T& c)
  {
    resize_front_impl(sz, c);
    BOOST_ASSERT(invariants_ok());
  }

  /**
   * **Effects**: If `sz` is greater than the size of `*this`,
   * additional value-initialized elements are inserted
   * to the back. Invalidates iterators if reallocation is needed.
   * If `sz` is smaller than than the size of `*this`,
   * elements are popped from the back.
   *
   * **Requires**: T shall be [MoveInsertable] into *this and [DefaultConstructible].
   *
   * **Postcondition**: `sz == size()`.
   *
   * **Exceptions**: Strong exception guarantee.
   *
   * **Complexity**: Linear in the size of `*this` and `sz`.
   *
   * [MoveInsertable]: http://en.cppreference.com/w/cpp/concept/MoveInsertable
   * [DefaultConstructible]: http://en.cppreference.com/w/cpp/concept/DefaultConstructible
   */
  void resize_back(size_type sz)
  {
    resize_back_impl(sz);
    BOOST_ASSERT(invariants_ok());
  }

  /**
   * [CopyInsertable]: http://en.cppreference.com/w/cpp/concept/CopyInsertable
   *
   * **Effects**: If `sz` is greater than the size of `*this`,
   * copies of `c` are inserted to the back.
   * If `sz` is smaller than than the size of `*this`,
   * elements are popped from the back.
   *
   * **Postcondition**: `sz == size()`.
   *
   * **Requires**: `T` shall be [CopyInsertable] into `*this`.
   *
   * **Exceptions**: Strong exception guarantee.
   *
   * **Complexity**: Linear in the size of `*this` and `sz`.
   */
  void resize_back(size_type sz, const T& c)
  {
    resize_back_impl(sz, c);
    BOOST_ASSERT(invariants_ok());
  }

  // unsafe uninitialized resize methods

  /**
   * **Unsafe method**, use with care.
   *
   * **Effects**: Changes the size of the devector without properly
   * initializing the extra or destroying the superfluous elements.
   * If `n < size()`, elements are removed from the front without
   * getting destroyed; if `n > size()`, uninitialized elements are added
   * before the first element at the front.
   * Invalidates iterators if reallocation is needed.
   *
   * **Postcondition**: `size() == n`.
   *
   * **Exceptions**: Strong exception guarantee.
   *
   * **Complexity**: Linear in `size()` if `capacity() < n`, constant otherwise.
   *
   * **Remarks**: The devector does not keep track of initialization of the elements:
   * Elements without a trivial destructor must be manually destroyed before shrinking,
   * elements without a trivial constructor must be initialized after growing.
   */
  void unsafe_uninitialized_resize_front(size_type n)
  {
    if (n > size())
    {
      unsafe_uninitialized_grow_front(n);
    }
    else
    {
      unsafe_uninitialized_shrink_front(n);
    }
  }

  /**
   * **Unsafe method**, use with care.
   *
   * **Effects**: Changes the size of the devector without properly
   * initializing the extra or destroying the superfluous elements.
   * If `n < size()`, elements are removed from the back without
   * getting destroyed; if `n > size()`, uninitialized elements are added
   * after the last element at the back.
   * Invalidates iterators if reallocation is needed.
   *
   * **Postcondition**: `size() == n`.
   *
   * **Exceptions**: Strong exception guarantee.
   *
   * **Complexity**: Linear in `size()` if `capacity() < n`, constant otherwise.
   *
   * **Remarks**: The devector does not keep track of initialization of the elements:
   * Elements without a trivial destructor must be manually destroyed before shrinking,
   * elements without a trivial constructor must be initialized after growing.
   */
  void unsafe_uninitialized_resize_back(size_type n)
  {
    if (n > size())
    {
      unsafe_uninitialized_grow_back(n);
    }
    else
    {
      unsafe_uninitialized_shrink_back(n);
    }
  }

  // reserve promise:
  // after reserve_[front,back](n), n - size() push_[front,back] will not allocate

  /** **Equivalent to**: `reserve_back(new_capacity)` */
  void reserve(size_type new_capacity) { reserve_back(new_capacity); }

  /**
   * [MoveInsertable]: http://en.cppreference.com/w/cpp/concept/MoveInsertable
   *
   * **Effects**: Ensures that `n` elements can be pushed to the front
   * without requiring reallocation, where `n` is `new_capacity - size()`,
   * if `n` is positive. Otherwise, there are no effects.
   * Invalidates iterators if reallocation is needed.
   *
   * **Requires**: `T` shall be [MoveInsertable] into `*this`.
   *
   * **Complexity**: Linear in the size of *this.
   *
   * **Exceptions**: Strong exception guarantee.
   *
   * **Throws**: `std::length_error` if `new_capacity > max_size()`.
   */
  void reserve_front(size_type new_capacity)
  {
    if (front_capacity() >= new_capacity) { return; }

    reallocate_at(new_capacity + back_free_capacity(), new_capacity - size());

    BOOST_ASSERT(invariants_ok());
  }

  /**
   * [MoveInsertable]: http://en.cppreference.com/w/cpp/concept/MoveInsertable
   *
   * **Effects**: Ensures that `n` elements can be pushed to the back
   * without requiring reallocation, where `n` is `new_capacity - size()`,
   * if `n` is positive. Otherwise, there are no effects.
   * Invalidates iterators if reallocation is needed.
   *
   * **Requires**: `T` shall be [MoveInsertable] into `*this`.
   *
   * **Complexity**: Linear in the size of *this.
   *
   * **Exceptions**: Strong exception guarantee.
   *
   * **Throws**: length_error if `new_capacity > max_size()`.
   */
  void reserve_back(size_type new_capacity)
  {
    if (back_capacity() >= new_capacity) { return; }

    reallocate_at(new_capacity + front_free_capacity(), _front_index);

    BOOST_ASSERT(invariants_ok());
  }


  /**
   * [MoveInsertable]: http://en.cppreference.com/w/cpp/concept/MoveInsertable
   *
   * **Effects**: Reduces `capacity()` to `max(size(), small buffer size)`
   * if the `GrowthPolicy` allows it (`should_shrink` returns `true`),
   * otherwise, there are no effects. Invalidates iterators if shrinks.
   * If shrinks and they fit, moves the contained elements back to
   * the small buffer.
   *
   * **Requires**: `T` shall be [MoveInsertable] into `*this`.
   *
   * **Exceptions**: Strong exception guarantee.
   *
   * **Complexity**: Linear in the size of *this.
   */
  void shrink_to_fit()
  {
    if (
       GrowthPolicy::should_shrink(size(), capacity(), sbuffer_size) == false
    || is_small()
    )
    {
      return;
    }

    if (size() <= sbuffer_size)
    {
      buffer_move_or_copy(_storage.small_buffer_address());

      _buffer = _storage.small_buffer_address();
      _storage._capacity = sbuffer_size;
      _back_index = size();
      _front_index = 0;
    }
    else
    {
      reallocate_at(size(), 0);
    }
  }

  // element access:

  /**
   * **Returns**: A reference to the `n`th element in the devector.
   *
   * **Precondition**: `n < size()`.
   *
   * **Complexity**: Constant.
   */
  reference operator[](size_type n) noexcept
  {
    BOOST_ASSERT(n < size());

    return *(begin() + n);
  }

  /**
   * **Returns**: A constant reference to the `n`th element in the devector.
   *
   * **Precondition**: `n < size()`.
   *
   * **Complexity**: Constant.
   */
  const_reference operator[](size_type n) const noexcept
  {
    BOOST_ASSERT(n < size());

    return *(begin() + n);
  }

  /**
   * **Returns**: A reference to the `n`th element in the devector.
   *
   * **Throws**: `std::out_of_range`, if `n >= size()`.
   *
   * **Complexity**: Constant.
   */
  reference at(size_type n)
  {
    if (size() <= n) { throw_exception(std::out_of_range("devector::at out of range")); }
    return (*this)[n];
  }

  /**
   * **Returns**: A constant reference to the `n`th element in the devector.
   *
   * **Throws**: `std::out_of_range`, if `n >= size()`.
   *
   * **Complexity**: Constant.
   */
  const_reference at(size_type n) const
  {
    if (size() <= n) { throw_exception(std::out_of_range("devector::at out of range")); }
    return (*this)[n];
  }

  /**
   * **Returns**: A reference to the first element in the devector.
   *
   * **Precondition**: `!empty()`.
   *
   * **Complexity**: Constant.
   */
  reference front() noexcept
  {
    BOOST_ASSERT(!empty());

    return *(_buffer + _front_index);
  }

  /**
   * **Returns**: A constant reference to the first element in the devector.
   *
   * **Precondition**: `!empty()`.
   *
   * **Complexity**: Constant.
   */
  const_reference front() const noexcept
  {
    BOOST_ASSERT(!empty());

    return *(_buffer + _front_index);
  }

  /**
   * **Returns**: A reference to the last element in the devector.
   *
   * **Precondition**: `!empty()`.
   *
   * **Complexity**: Constant.
   */
  reference back() noexcept
  {
    BOOST_ASSERT(!empty());

    return *(_buffer + _back_index -1);
  }

  /**
   * **Returns**: A constant reference to the last element in the devector.
   *
   * **Precondition**: `!empty()`.
   *
   * **Complexity**: Constant.
   */
  const_reference back() const noexcept
  {
    BOOST_ASSERT(!empty());

    return *(_buffer + _back_index -1);
  }

  /**
   * **Returns**: A pointer to the underlying array serving as element storage.
   * The range `[data(); data() + size())` is always valid. For a non-empty devector,
   * `data() == &front()`.
   *
   * **Complexity**: Constant.
   */
  T* data() noexcept
  {
    return _buffer + _front_index;
  }

  /**
   * **Returns**: A constant pointer to the underlying array serving as element storage.
   * The range `[data(); data() + size())` is always valid. For a non-empty devector,
   * `data() == &front()`.
   *
   * **Complexity**: Constant.
   */
  const T* data() const noexcept
  {
    return _buffer + _front_index;
  }

  // modifiers:

  /**
   * **Effects**: Pushes a new element to the front of the devector.
   * The element is constructed in-place, using the perfect forwarded `args`
   * as constructor arguments. Invalidates iterators if reallocation is needed.
   * (`front_free_capacity() == 0`)
   *
   * **Requires**: `T` shall be [EmplaceConstructible] from `args` and [MoveInsertable] into `*this`.
   *
   * **Exceptions**: Strong exception guarantee.
   *
   * **Complexity**: Amortized linear in the size of `*this`.
   * (Constant, if `front_free_capacity() > 0`)
   *
   * [EmplaceConstructible]: http://en.cppreference.com/w/cpp/concept/EmplaceConstructible
   * [MoveInsertable]: http://en.cppreference.com/w/cpp/concept/MoveInsertable
   */
  template <class... Args>
  void emplace_front(Args&&... args)
  {
    if (front_free_capacity()) // fast path
    {
      alloc_construct(_buffer + _front_index - 1, std::forward<Args>(args)...);
      --_front_index;
    }
    else
    {
      emplace_reallocating_slow_path(true, 0, std::forward<Args>(args)...);
    }

    BOOST_ASSERT(invariants_ok());
  }

  /**
   * [CopyInsertable]: http://en.cppreference.com/w/cpp/concept/CopyInsertable
   *
   * **Effects**: Pushes the copy of `x` to the front of the devector.
   * Invalidates iterators if reallocation is needed.
   * (`front_free_capacity() == 0`)
   *
   * **Requires**: `T` shall be [CopyInsertable] into `*this`.
   *
   * **Exceptions**: Strong exception guarantee.
   *
   * **Complexity**: Amortized linear in the size of `*this`.
   * (Constant, if `front_free_capacity() > 0`)
   */
  void push_front(const T& x)
  {
    emplace_front(x);
  }

  /**
   * [MoveInsertable]: http://en.cppreference.com/w/cpp/concept/MoveInsertable
   *
   * **Effects**: Move constructs a new element at the front of the devector using `x`.
   * Invalidates iterators if reallocation is needed.
   * (`front_free_capacity() == 0`)
   *
   * **Requires**: `T` shall be [MoveInsertable] into `*this`.
   *
   * **Exceptions**: Strong exception guarantee, not regarding the state of `x`.
   *
   * **Complexity**: Amortized linear in the size of `*this`.
   * (Constant, if `front_free_capacity() > 0`)
   */
  void push_front(T&& x)
  {
    emplace_front(std::move(x));
  }

  /**
   * [CopyInsertable]: http://en.cppreference.com/w/cpp/concept/CopyInsertable
   *
   * **Effects**: Pushes the copy of `x` to the front of the devector.
   *
   * **Requires**: `T` shall be [CopyInsertable] into `*this`.
   *
   * **Precondition**: `front_free_capacity() > 0`.
   *
   * **Postcondition**: `size()` is incremented by 1,
   * `front_free_capacity() is decremented by 1`
   *
   * **Exceptions**: Strong exception guarantee.
   *
   * **Complexity**: Constant.
   */
  void unsafe_push_front(const T& x)
  {
    BOOST_ASSERT(front_free_capacity());

    alloc_construct(_buffer + _front_index - 1, x);
    --_front_index;

    BOOST_ASSERT(invariants_ok());
  }

  /**
   * [MoveInsertable]: http://en.cppreference.com/w/cpp/concept/MoveInsertable
   *
   * **Effects**: Move constructs a new element at the front of the devector using `x`.
   *
   * **Requires**: `T` shall be [MoveInsertable] into `*this`.
   *
   * **Precondition**: `front_free_capacity() > 0`.
   *
   * **Postcondition**: `size()` is incremented by 1,
   * `front_free_capacity() is decremented by 1`
   *
   * **Exceptions**: Strong exception guarantee, not regarding the state of `x`.
   *
   * **Complexity**: Constant.
   */
  void unsafe_push_front(T&& x)
  {
    BOOST_ASSERT(front_free_capacity());

    alloc_construct(_buffer + _front_index - 1, std::forward<T>(x));
    --_front_index;

    BOOST_ASSERT(invariants_ok());
  }

  /**
   * **Effects**: Removes the first element of `*this`.
   *
   * **Precondition**: `!empty()`.
   *
   * **Postcondition**: `front_free_capacity()` is incremented by 1.
   *
   * **Complexity**: Constant.
   */
  void pop_front() noexcept
  {
    BOOST_ASSERT(! empty());
    allocator_traits::destroy(get_allocator_ref(), _buffer + _front_index);
    ++_front_index;
    BOOST_ASSERT(invariants_ok());
  }

  /**
   * **Effects**: Pushes a new element to the back of the devector.
   * The element is constructed in-place, using the perfect forwarded `args`
   * as constructor arguments. Invalidates iterators if reallocation is needed.
   * (`back_free_capacity() == 0`)
   *
   * **Requires**: `T` shall be [EmplaceConstructible] from `args` and [MoveInsertable] into `*this`,
   * and [MoveAssignable].
   *
   * **Exceptions**: Strong exception guarantee.
   *
   * **Complexity**: Amortized linear in the size of `*this`.
   * (Constant, if `back_free_capacity() > 0`)
   *
   * [EmplaceConstructible]: http://en.cppreference.com/w/cpp/concept/EmplaceConstructible
   * [MoveInsertable]: http://en.cppreference.com/w/cpp/concept/MoveInsertable
   * [MoveAssignable]: http://en.cppreference.com/w/cpp/concept/MoveAssignable
   */
  template <class... Args>
  void emplace_back(Args&&... args)
  {
    if (back_free_capacity()) // fast path
    {
      alloc_construct(_buffer + _back_index, std::forward<Args>(args)...);
      ++_back_index;
    }
    else
    {
      emplace_reallocating_slow_path(false, size(), std::forward<Args>(args)...);
    }

    BOOST_ASSERT(invariants_ok());
  }

  /**
   * [CopyInsertable]: http://en.cppreference.com/w/cpp/concept/CopyInsertable
   *
   * **Effects**: Pushes the copy of `x` to the back of the devector.
   * Invalidates iterators if reallocation is needed.
   * (`back_free_capacity() == 0`)
   *
   * **Requires**: `T` shall be [CopyInsertable] into `*this`.
   *
   * **Exceptions**: Strong exception guarantee.
   *
   * **Complexity**: Amortized linear in the size of `*this`.
   * (Constant, if `back_free_capacity() > 0`)
   */
  void push_back(const T& x)
  {
    emplace_back(x);
  }

  /**
   * [MoveInsertable]: http://en.cppreference.com/w/cpp/concept/MoveInsertable
   *
   * **Effects**: Move constructs a new element at the back of the devector using `x`.
   * Invalidates iterators if reallocation is needed.
   * (`back_free_capacity() == 0`)
   *
   * **Requires**: `T` shall be [MoveInsertable] into `*this`.
   *
   * **Exceptions**: Strong exception guarantee, not regarding the state of `x`.
   *
   * **Complexity**: Amortized linear in the size of `*this`.
   * (Constant, if `back_free_capacity() > 0`)
   */
  void push_back(T&& x)
  {
    emplace_back(std::move(x));
  }

  /**
   * [CopyInsertable]: http://en.cppreference.com/w/cpp/concept/CopyInsertable
   *
   * **Effects**: Pushes the copy of `x` to the back of the devector.
   *
   * **Requires**: `T` shall be [CopyInsertable] into `*this`.
   *
   * **Precondition**: `back_free_capacity() > 0`.
   *
   * **Postcondition**: `size()` is incremented by 1,
   * `back_free_capacity() is decremented by 1`
   *
   * **Exceptions**: Strong exception guarantee.
   *
   * **Complexity**: Constant.
   */
  void unsafe_push_back(const T& x)
  {
    BOOST_ASSERT(back_free_capacity());

    alloc_construct(_buffer + _back_index, x);
    ++_back_index;

    BOOST_ASSERT(invariants_ok());
  }

  /**
   * [MoveInsertable]: http://en.cppreference.com/w/cpp/concept/MoveInsertable
   *
   * **Effects**: Move constructs a new element at the back of the devector using `x`.
   *
   * **Requires**: `T` shall be [MoveInsertable] into `*this`.
   *
   * **Precondition**: `back_free_capacity() > 0`.
   *
   * **Postcondition**: `size()` is incremented by 1,
   * `back_free_capacity() is decremented by 1`
   *
   * **Exceptions**: Strong exception guarantee, not regarding the state of `x`.
   *
   * **Complexity**: Constant.
   */
  void unsafe_push_back(T&& x)
  {
    BOOST_ASSERT(back_free_capacity());

    alloc_construct(_buffer + _back_index, std::forward<T>(x));
    ++_back_index;

    BOOST_ASSERT(invariants_ok());
  }

  /**
   * **Effects**: Removes the last element of `*this`.
   *
   * **Precondition**: `!empty()`.
   *
   * **Postcondition**: `back_free_capacity()` is incremented by 1.
   *
   * **Complexity**: Constant.
   */
  void pop_back() noexcept
  {
    BOOST_ASSERT(! empty());
    --_back_index;
    allocator_traits::destroy(get_allocator_ref(), _buffer + _back_index);
    BOOST_ASSERT(invariants_ok());
  }

  /**
   * **Effects**: Constructs a new element before the element pointed by `position`.
   * The element is constructed in-place, using the perfect forwarded `args`
   * as constructor arguments. Invalidates iterators if reallocation is needed.
   *
   * **Requires**: `T` shall be [EmplaceConstructible], and [MoveInsertable] into `*this`,
   * and [MoveAssignable].
   *
   * **Returns**: Iterator pointing to the newly constructed element.
   *
   * **Exceptions**: Strong exception guarantee if `T` is `NothrowConstructible`
   * and `NothrowAssignable`, Basic exception guarantee otherwise.
   *
   * **Complexity**: Linear in the size of `*this`.
   *
   * [EmplaceConstructible]: http://en.cppreference.com/w/cpp/concept/EmplaceConstructible
   * [MoveInsertable]: http://en.cppreference.com/w/cpp/concept/MoveInsertable
   * [MoveAssignable]: http://en.cppreference.com/w/cpp/concept/MoveAssignable
   */
  template <class... Args>
  iterator emplace(const_iterator position, Args&&... args)
  {
    BOOST_ASSERT(position >= begin());
    BOOST_ASSERT(position <= end());

    if (position == end() && back_free_capacity()) // fast path
    {
      alloc_construct(_buffer + _back_index, std::forward<Args>(args)...);
      ++_back_index;
      return end() - 1;
    }
    else if (position == begin() && front_free_capacity()) // secondary fast path
    {
      alloc_construct(_buffer + _front_index - 1, std::forward<Args>(args)...);
      --_front_index;
      return begin();
    }
    else
    {
      size_type new_elem_index = position - begin();
      return emplace_slow_path(new_elem_index, std::forward<Args>(args)...);
    }
  }

  /**
   * **Effects**: Copy constructs a new element before the element pointed by `position`,
   * using `x` as constructor argument. Invalidates iterators if reallocation is needed.
   *
   * **Requires**: `T` shall be [CopyInsertable] into `*this` and and [CopyAssignable].
   *
   * **Returns**: Iterator pointing to the newly constructed element.
   *
   * **Exceptions**: Strong exception guarantee if `T` is `NothrowConstructible`
   * and `NothrowAssignable`, Basic exception guarantee otherwise.
   *
   * **Complexity**: Linear in the size of `*this`.
   *
   * [CopyInsertable]: http://en.cppreference.com/w/cpp/concept/CopyInsertable
   * [CopyAssignable]: http://en.cppreference.com/w/cpp/concept/CopyAssignable
   */
  iterator insert(const_iterator position, const T& x)
  {
    return emplace(position, x);
  }

  /**
   * **Effects**: Move constructs a new element before the element pointed by `position`,
   * using `x` as constructor argument. Invalidates iterators if reallocation is needed.
   *
   * **Requires**: `T` shall be [MoveInsertable] into `*this` and and [CopyAssignable].
   *
   * **Returns**: Iterator pointing to the newly constructed element.
   *
   * **Exceptions**: Strong exception guarantee if `T` is `NothrowConstructible`
   * and `NothrowAssignable` (not regarding the state of `x`),
   * Basic exception guarantee otherwise.
   *
   * **Complexity**: Linear in the size of `*this`.
   *
   * [MoveInsertable]: http://en.cppreference.com/w/cpp/concept/MoveInsertable
   * [CopyAssignable]: http://en.cppreference.com/w/cpp/concept/CopyAssignable
   */
  iterator insert(const_iterator position, T&& x)
  {
    return emplace(position, std::move(x));
  }

  /**
   * **Effects**: Copy constructs `n` elements before the element pointed by `position`,
   * using `x` as constructor argument. Invalidates iterators if reallocation is needed.
   *
   * **Requires**: `T` shall be [CopyInsertable] into `*this` and and [CopyAssignable].
   *
   * **Returns**: Iterator pointing to the first inserted element, or `position`, if `n` is zero.
   *
   * **Exceptions**: Strong exception guarantee if `T` is `NothrowConstructible`
   * and `NothrowAssignable`, Basic exception guarantee otherwise.
   *
   * **Complexity**: Linear in the size of `*this` and `n`.
   *
   * [CopyInsertable]: http://en.cppreference.com/w/cpp/concept/CopyInsertable
   * [CopyAssignable]: http://en.cppreference.com/w/cpp/concept/CopyAssignable
   */
  iterator insert(const_iterator position, size_type n, const T& x)
  {
    cvalue_iterator first(x, n);
    cvalue_iterator last = first + n;
    return insert_range(position, first, last);
  }

  /**
   * **Effects**: Copy constructs elements before the element pointed by position
   * using each element in the rage pointed by `first` and `last` as constructor arguments.
   * Invalidates iterators if reallocation is needed.
   *
   * **Requires**: `T` shall be [EmplaceConstructible] into `*this` from `*first`. If the specified iterator
   * does not meet the forward iterator requirements, `T` shall also be [MoveInsertable] into `*this`
   * and [MoveAssignable].
   *
   * **Precondition**: `first` and `last` are not iterators into `*this`.
   *
   * **Returns**: Iterator pointing to the first inserted element, or `position`, if `first == last`.
   *
   * **Complexity**: Linear in the size of `*this` and `N` (where `N` is the distance between `first` and `last`).
   * Makes only `N` calls to the constructor of `T` and no reallocations if iterators `first` and `last`
   * are of forward, bidirectional, or random access categories. It makes 2N calls to the copy constructor of `T`
   * and allocates memory twice at most if they are just input iterators.
   *
   * **Exceptions**: Strong exception guarantee if `T` is `NothrowConstructible`
   * and `NothrowAssignable`, Basic exception guarantee otherwise.
   *
   * **Remarks**: Each iterator in the range `[first,last)` shall be dereferenced exactly once,
   * unless an exception is thrown.
   *
   * [EmplaceConstructible]: http://en.cppreference.com/w/cpp/concept/EmplaceConstructible
   * [MoveInsertable]: http://en.cppreference.com/w/cpp/concept/MoveInsertable
   * [MoveAssignable]: http://en.cppreference.com/w/cpp/concept/MoveAssignable
   */
  template <BOOST_DOUBLE_ENDED_REQUIRE_INPUT_ITERATOR(InputIterator)>
  iterator insert(const_iterator position, InputIterator first, InputIterator last)
  {
    if (position == end())
    {
      size_type insert_index = size();

      while (first != last)
      {
        push_back(*first++);
      }

      return begin() + insert_index;
    }
    else
    {
      // avoid buffer overflow if original small buffer is too large
      typedef devector<T, small_buffer_size<32>, GrowthPolicy, Allocator> temp_devector;

      temp_devector range(first, last);
      return insert_range(position, range.begin(), range.end());
    }
  }

#ifndef BOOST_DOUBLE_ENDED_DOXYGEN_INVOKED

  template <BOOST_DOUBLE_ENDED_REQUIRE_FW_ITERATOR(ForwardIterator)>
  iterator insert(const_iterator position, ForwardIterator first, ForwardIterator last)
  {
    return insert_range(position, first, last);
  }

#endif // ifndef BOOST_DOUBLE_ENDED_DOXYGEN_INVOKED

  /** **Equivalent to**: `insert(position, il.begin(), il.end())` */
  iterator insert(const_iterator position, std::initializer_list<T> il)
  {
    return insert_range(position, il.begin(), il.end());
  }

  /**
   * [MoveAssignable]: http://en.cppreference.com/w/cpp/concept/MoveAssignable
   *
   * **Effects**: Destroys the element pointed by `position` and removes it from the devector.
   * Invalidates iterators.
   *
   * **Requires**: `T` shall be [MoveAssignable].
   *
   * **Precondition**: `position` must be in the range of `[begin(), end())`.
   *
   * **Returns**: Iterator pointing to the element immediately following the erased element
   * prior to its erasure. If no such element exists, `end()` is returned.
   *
   * **Exceptions**: Strong exception guarantee if `T` is `NothrowAssignable`,
   * Basic exception guarantee otherwise.
   *
   * **Complexity**: Linear in half the size of `*this`.
   */
  iterator erase(const_iterator position)
  {
    return erase(position, position + 1);
  }

  /**
   * [MoveAssignable]: http://en.cppreference.com/w/cpp/concept/MoveAssignable
   *
   * **Effects**: Destroys the range `[first,last)` and removes it from the devector.
   * Invalidates iterators.
   *
   * **Requires**: `T` shall be [MoveAssignable].
   *
   * **Precondition**: `[first,last)` must be in the range of `[begin(), end())`.
   *
   * **Returns**: Iterator pointing to the element pointed to by `last` prior to any elements
   * being erased. If no such element exists, `end()` is returned.
   *
   * **Exceptions**: Strong exception guarantee if `T` is `NothrowAssignable`,
   * Basic exception guarantee otherwise.
   *
   * **Complexity**: Linear in half the size of `*this`
   * plus the distance between `first` and `last`.
   */
  iterator erase(const_iterator first, const_iterator last)
  {
    iterator nc_first = begin() + (first - begin());
    iterator nc_last  = begin() + (last  - begin());
    return erase(nc_first, nc_last);
  }

  /**
   * [MoveAssignable]: http://en.cppreference.com/w/cpp/concept/MoveAssignable
   *
   * **Effects**: Destroys the range `[first,last)` and removes it from the devector.
   * Invalidates iterators.
   *
   * **Requires**: `T` shall be [MoveAssignable].
   *
   * **Precondition**: `[first,last)` must be in the range of `[begin(), end())`.
   *
   * **Returns**: Iterator pointing to the element pointed to by `last` prior to any elements
   * being erased. If no such element exists, `end()` is returned.
   *
   * **Exceptions**: Strong exception guarantee if `T` is `NothrowAssignable`,
   * Basic exception guarantee otherwise.
   *
   * **Complexity**: Linear in half the size of `*this`.
   */
  iterator erase(iterator first, iterator last)
  {
    size_type front_distance = last - begin();
    size_type back_distance = end() - first;
    size_type n = std::distance(first, last);

    if (front_distance < back_distance)
    {
      // move n to the right
      detail::move_if_noexcept_backward(begin(), first, last);

      for (iterator i = begin(); i != begin() + n; ++i)
      {
        allocator_traits::destroy(get_allocator_ref(), i);
      }
      _front_index += n;

      BOOST_ASSERT(invariants_ok());
      return last;
    }
    else
    {
      // move n to the left
      detail::move_if_noexcept(last, end(), first);

      for (iterator i = end() - n; i != end(); ++i)
      {
        allocator_traits::destroy(get_allocator_ref(), i);
      }
      _back_index -= n;

      BOOST_ASSERT(invariants_ok());
      return first;
    }
  }

  /**
   * [MoveInsertable]: http://en.cppreference.com/w/cpp/concept/MoveInsertable
   *
   * **Effects**: exchanges the contents of `*this` and `b`.
   * Invalidates iterators to elements stored in small buffer before swapping.
   *
   * **Requires**: instances of `T` must be swappable by unqualified call of `swap`
   * and `T` must be [MoveInsertable] into `*this`.
   *
   * **Precondition**: The allocators should allow propagation or should compare equal.
   *
   * **Exceptions**: Basic exceptions guarantee if not `noexcept`.
   *
   * **Complexity**: Linear in the size of the small buffer.
   *
   * **Remarks**: If both devectors are _small_, elements are exchanged one by one.
   * If exactly one is small, elements in the small buffer are moved to the
   * small buffer of the other devector and the large buffer is acquired by the
   * previously _small_ devector. If neither are _small_, buffers are exchanged
   * in one step.
   */
  void swap(devector& b) noexcept(no_small_buffer || t_is_nothrow_constructible) // && nothrow_swappable
  {
    BOOST_ASSERT(
       allocator_traits::propagate_on_container_swap::value
    || get_allocator_ref() == b.get_allocator_ref()
    ); // else it's undefined behavior

    if (is_small())
    {
      if (b.is_small())
      {
        swap_small_small(*this, b);
      }
      else
      {
        swap_small_big(*this, b);
      }
    }
    else
    {
      if (b.is_small())
      {
        swap_small_big(b, *this);
      }
      else
      {
        swap_big_big(*this, b);
      }
    }

    // swap indices
    std::swap(_front_index, b._front_index);
    std::swap(_back_index, b._back_index);

    if (allocator_traits::propagate_on_container_swap::value)
    {
      using std::swap;
      swap(get_allocator_ref(), b.get_allocator_ref());
    }

    BOOST_ASSERT(  invariants_ok());
    BOOST_ASSERT(b.invariants_ok());
  }

  /**
   * **Effects**: Destroys all elements in the devector.
   * Invalidates all references, pointers and iterators to the
   * elements of the devector.
   *
   * **Postcondition**: `empty() && front_free_capacity() == 0
   * && back_free_capacity() == small buffer size`.
   *
   * **Complexity**: Linear in the size of `*this`.
   *
   * **Remarks**: Does not free memory.
   */
  void clear() noexcept
  {
    destroy_elements(begin(), end());
    _front_index = _back_index = 0;
  }

private:
#ifndef BOOST_DOUBLE_ENDED_DOXYGEN_INVOKED

  // Allocator wrappers

  allocator_type& get_allocator_ref() noexcept
  {
    return static_cast<allocator_type&>(*this);
  }

  const allocator_type& get_allocator_ref() const noexcept
  {
    return static_cast<const allocator_type&>(*this);
  }

  pointer allocate(size_type capacity)
  {
    if (capacity <= sbuffer_size)
    {
      return _storage.small_buffer_address();
    }
    else
    {
      #ifdef BOOST_DOUBLE_ENDED_DEVECTOR_ALLOC_STATS
      ++capacity_alloc_count;
      #endif // BOOST_DOUBLE_ENDED_DEVECTOR_ALLOC_STATS
      return allocator_traits::allocate(get_allocator_ref(), capacity);
    }
  }

  void destroy_elements(pointer begin, pointer end)
  {
    for (; begin != end; ++begin)
    {
      allocator_traits::destroy(get_allocator_ref(), begin);
    }
  }

  void deallocate_buffer()
  {
    if (! is_small() && _buffer)
    {
      allocator_traits::deallocate(get_allocator_ref(), _buffer, _storage._capacity);
    }
  }

  template <typename... Args>
  void alloc_construct(pointer dst, Args&&... args)
  {
    allocator_traits::construct(
      get_allocator_ref(),
      dst,
      std::forward<Args>(args)...
    );
  }

  size_type front_capacity()
  {
    return _back_index;
  }

  size_type back_capacity()
  {
    return _storage._capacity - _front_index;
  }

  size_type calculate_new_capacity(size_type requested_capacity)
  {
    size_type policy_capacity = GrowthPolicy::new_capacity(_storage._capacity);
    size_type new_capacity = (std::max)(requested_capacity, policy_capacity);

    if (
       new_capacity > max_size()
    || new_capacity < capacity() // overflow
    )
    {
      throw_exception(std::length_error("devector: max_size() exceeded"));
    }

    return new_capacity;
  }

  void buffer_move_or_copy(pointer dst)
  {
    construction_guard guard(dst, get_allocator_ref());

    buffer_move_or_copy(dst, guard);

    guard.release();
  }

  void buffer_move_or_copy(pointer dst, construction_guard& guard)
  {
    opt_move_or_copy(begin(), end(), dst, guard);

    destroy_elements(data(), data() + size());
    deallocate_buffer();
  }

  void opt_move_or_copy(pointer begin, pointer end, pointer dst)
  {
    typedef typename std::conditional<
      t_is_nothrow_constructible,
      detail::null_construction_guard,
      construction_guard
    >::type guard_t;

    guard_t guard(dst, get_allocator_ref());

    opt_move_or_copy(begin, end, dst, guard);

    guard.release();
  }

  template <typename Guard>
  void opt_move_or_copy(pointer begin, pointer end, pointer dst, Guard& guard)
  {
    // if trivial copy and default allocator, memcpy
    if (t_is_trivially_copyable)
    {
      std::memcpy(dst, begin, (end - begin) * sizeof(T));
    }
    else // guard needed
    {
      while (begin != end)
      {
        alloc_construct(dst++, std::move_if_noexcept(*begin++));
        guard.extend();
      }
    }
  }

  template <typename Iterator>
  void opt_copy(Iterator begin, Iterator end, pointer dst)
  {
    typedef typename std::conditional<
      std::is_nothrow_copy_constructible<T>::value,
      detail::null_construction_guard,
      construction_guard
    >::type guard_t;

    guard_t guard(dst, get_allocator_ref());

    opt_copy(begin, end, dst, guard);

    guard.release();
  }

  template <typename Iterator, typename Guard>
  void opt_copy(Iterator begin, Iterator end, pointer dst, Guard& guard)
  {
    while (begin != end)
    {
      alloc_construct(dst++, *begin++);
      guard.extend();
    }
  }

  template <typename Guard>
  void opt_copy(const_pointer begin, const_pointer end, pointer dst, Guard& guard)
  {
    // if trivial copy and default allocator, memcpy
    if (t_is_trivially_copyable)
    {
      std::memcpy(dst, begin, (end - begin) * sizeof(T));
    }
    else // guard needed
    {
      while (begin != end)
      {
        alloc_construct(dst++, *begin++);
        guard.extend();
      }
    }
  }

  template <typename... Args>
  void resize_front_impl(size_type sz, Args&&... args)
  {
    if (sz > size())
    {
      const size_type n = sz - size();

      if (sz <= front_capacity())
      {
        construct_n(_buffer + _front_index - n, n, std::forward<Args>(args)...);
        _front_index -= n;
      }
      else
      {
        resize_front_slow_path(sz, n, std::forward<Args>(args)...);
      }
    }
    else
    {
      while (size() > sz)
      {
        pop_front();
      }
    }
  }

  template <typename... Args>
  void resize_front_slow_path(size_type sz, size_type n, Args&&... args)
  {
    const size_type new_capacity = calculate_new_capacity(sz + back_free_capacity());
    pointer new_buffer = allocate(new_capacity);
    allocation_guard new_buffer_guard(new_buffer, new_capacity, get_allocator_ref());

    const size_type new_old_elem_index = new_capacity - size();
    const size_type new_elem_index = new_old_elem_index - n;

    construction_guard guard(new_buffer + new_elem_index, get_allocator_ref());
    guarded_construct_n(new_buffer + new_elem_index, n, guard, std::forward<Args>(args)...);

    buffer_move_or_copy(new_buffer + new_old_elem_index, guard);

    guard.release();
    new_buffer_guard.release();

    _buffer = new_buffer;
    _storage._capacity = new_capacity;

    _back_index = new_old_elem_index + _back_index - _front_index;
    _front_index = new_elem_index;
  }

  template <typename... Args>
  void resize_back_impl(size_type sz, Args&&... args)
  {
    if (sz > size())
    {
      const size_type n = sz - size();

      if (sz <= back_capacity())
      {
        construct_n(_buffer + _back_index, n, std::forward<Args>(args)...);
        _back_index += n;
      }
      else
      {
        resize_back_slow_path(sz, n, std::forward<Args>(args)...);
      }
    }
    else
    {
      while (size() > sz)
      {
        pop_back();
      }
    }
  }

  template <typename... Args>
  void resize_back_slow_path(size_type sz, size_type n, Args&&... args)
  {
    const size_type new_capacity = calculate_new_capacity(sz + front_free_capacity());
    pointer new_buffer = allocate(new_capacity);
    allocation_guard new_buffer_guard(new_buffer, new_capacity, get_allocator_ref());

    construction_guard guard(new_buffer + _back_index, get_allocator_ref());
    guarded_construct_n(new_buffer + _back_index, n, guard, std::forward<Args>(args)...);

    buffer_move_or_copy(new_buffer + _front_index);

    guard.release();
    new_buffer_guard.release();

    _buffer = new_buffer;
    _storage._capacity = new_capacity;

    _back_index = _back_index + n;
  }

  void unsafe_uninitialized_grow_front(size_type n)
  {
    BOOST_ASSERT(n >= size());

    size_type need = n - size();

    if (need > front_free_capacity())
    {
      reallocate_at(n + back_free_capacity(), need);
    }

    _front_index -= need;
  }

  void unsafe_uninitialized_shrink_front(size_type n)
  {
    BOOST_ASSERT(n <= size());

    size_type doesnt_need = size() - n;
    _front_index += doesnt_need;
  }

  void unsafe_uninitialized_grow_back(size_type n)
  {
    BOOST_ASSERT(n >= size());

    size_type need = n - size();

    if (need > back_free_capacity())
    {
      reallocate_at(n + front_free_capacity(), front_free_capacity());
    }

    _back_index += need;
  }

  void unsafe_uninitialized_shrink_back(size_type n)
  {
    BOOST_ASSERT(n <= size());

    size_type doesnt_need = size() - n;
    _back_index -= doesnt_need;
  }

  template <typename... Args>
  iterator emplace_slow_path(size_type new_elem_index, Args&&... args)
  {
    pointer position = begin() + new_elem_index;

    // prefer moving front to access memory forward if there are less elems to move
    bool prefer_move_front = 2*new_elem_index <= size();

    if (front_free_capacity() && (!back_free_capacity() || prefer_move_front))
    {
      BOOST_ASSERT(size() >= 1);

      // move things closer to the front a bit

      // avoid invalidating any reference in args later
      T tmp(std::forward<Args>(args)...);

      // construct at front - 1 from front (no guard)
      alloc_construct(begin() - 1, std::move(*begin()));

      // move front half left
      std::move(begin() + 1, position, begin());
      --_front_index;

      // move assign new elem before pos
      --position;
      *position = std::move(tmp);

      return position;
    }
    else if (back_free_capacity())
    {
      BOOST_ASSERT(size() >= 1);

      // move things closer to the end a bit

      // avoid invalidating any reference in args later
      T tmp(std::forward<Args>(args)...);

      // construct at back + 1 from back (no guard)
      alloc_construct(end(), std::move(back()));

      // move back half right
      std::move_backward(position, end() - 1, end());
      ++_back_index;

      // move assign new elem to pos
      *position = std::move(tmp);

      return position;
    }
    else
    {
      return emplace_reallocating_slow_path(prefer_move_front, new_elem_index, std::forward<Args>(args)...);
    }
  }

  template <typename... Args>
  pointer emplace_reallocating_slow_path(bool make_front_free, size_type new_elem_index, Args&&... args)
  {
    // reallocate
    size_type new_capacity = calculate_new_capacity(capacity() + 1);
    pointer new_buffer = allocate(new_capacity);

    // guard allocation
    allocation_guard new_buffer_guard(new_buffer, new_capacity, get_allocator_ref());

    size_type new_front_index = (make_front_free)
      ? new_capacity - back_free_capacity() - size() - 1
      : _front_index;

    iterator new_begin = new_buffer + new_front_index;
    iterator new_position = new_begin + new_elem_index;
    iterator old_position = begin() + new_elem_index;

    // construct new element (and guard it)
    alloc_construct(new_position, std::forward<Args>(args)...);

    construction_guard second_half_guard(new_position, get_allocator_ref());
    second_half_guard.extend();

    // move front-pos (possibly guarded)
    construction_guard first_half_guard(new_begin, get_allocator_ref());
    opt_move_or_copy(begin(), old_position, new_begin, first_half_guard);

    // move pos+1-end (possibly guarded)
    opt_move_or_copy(old_position, end(), new_position + 1, second_half_guard);

    // cleanup
    destroy_elements(begin(), end());
    deallocate_buffer();

    // release alloc and other guards
    second_half_guard.release();
    first_half_guard.release();
    new_buffer_guard.release();

    // rebind members
    _storage._capacity = new_capacity;
    _buffer = new_buffer;
    _back_index = new_front_index + size() + 1;
    _front_index = new_front_index;

    return new_position;
  }

  void reallocate_at(size_type new_capacity, size_type buffer_offset)
  {
    BOOST_ASSERT(new_capacity > sbuffer_size);

    pointer new_buffer = allocate(new_capacity);
    allocation_guard new_buffer_guard(new_buffer, new_capacity, get_allocator_ref());

    buffer_move_or_copy(new_buffer + buffer_offset);

    new_buffer_guard.release();

    _buffer = new_buffer;
    _storage._capacity = new_capacity;

    _back_index = _back_index - _front_index + buffer_offset;
    _front_index = buffer_offset;

    BOOST_ASSERT(invariants_ok());
  }

  template <typename ForwardIterator>
  iterator insert_range(const_iterator position, ForwardIterator first, ForwardIterator last)
  {
    size_type n = std::distance(first, last);

    if (position == end() && back_free_capacity() >= n) // fast path
    {
      for (; first != last; ++first)
      {
        unsafe_push_back(*first);
      }
      return end() - n;
    }
    else if (position == begin() && front_free_capacity() >= n) // secondary fast path
    {
      insert_range_slow_path_near_front(begin(), first, n);
      return begin();
    }
    else
    {
      return insert_range_slow_path(position, first, last);
    }
  }

  template <typename ForwardIterator>
  iterator insert_range_slow_path(const_iterator position, ForwardIterator first, ForwardIterator last)
  {
    size_type n = std::distance(first, last);
    size_type index = position - begin();

    if (front_free_capacity() + back_free_capacity() >= n)
    {
      // if we move enough, it can be done without reallocation

      iterator middle = begin() + index;
      n -= insert_range_slow_path_near_front(middle, first, n);

      if (n)
      {
        insert_range_slow_path_near_back(middle, first, n);
      }

      BOOST_ASSERT(first == last);

      return begin() + index;
    }
    else
    {
      const bool prefer_move_front = 2 * index <= size();
      return insert_range_reallocating_slow_path(prefer_move_front, index, first, n);
    }
  }

  template <typename Iterator>
  size_type insert_range_slow_path_near_front(iterator position, Iterator& first, size_type n)
  {
    size_type n_front = (std::min)(front_free_capacity(), n);
    iterator new_begin = begin() - n_front;
    iterator ctr_pos = new_begin;
    construction_guard ctr_guard(ctr_pos, get_allocator_ref());

    while (ctr_pos != begin())
    {
      alloc_construct(ctr_pos++, *(first++));
      ctr_guard.extend();
    }

    std::rotate(new_begin, ctr_pos, position);
    _front_index -= n_front;

    ctr_guard.release();

    BOOST_ASSERT(invariants_ok());

    return n_front;
  }

  template <typename Iterator>
  size_type insert_range_slow_path_near_back(iterator position, Iterator& first, size_type n)
  {
    const size_type n_back = (std::min)(back_free_capacity(), n);
    iterator ctr_pos = end();

    construction_guard ctr_guard(ctr_pos, get_allocator_ref());

    for (size_type i = 0; i < n_back; ++i)
    {
      alloc_construct(ctr_pos++, *first++);
      ctr_guard.extend();
    }

    std::rotate(position, end(), ctr_pos);
    _back_index += n_back;

    ctr_guard.release();

    BOOST_ASSERT(invariants_ok());

    return n_back;
  }

  template <typename Iterator>
  iterator insert_range_reallocating_slow_path(
    bool make_front_free, size_type new_elem_index, Iterator elems, size_type n
  )
  {
    // reallocate
    const size_type new_capacity = calculate_new_capacity(capacity() + n);
    pointer new_buffer = allocate(new_capacity);

    // guard allocation
    allocation_guard new_buffer_guard(new_buffer, new_capacity, get_allocator_ref());

    const size_type new_front_index = (make_front_free)
      ? new_capacity - back_free_capacity() - size() - n
      : _front_index;

    const iterator new_begin = new_buffer + new_front_index;
    const iterator new_position = new_begin + new_elem_index;
    const iterator old_position = begin() + new_elem_index;

    // construct new element (and guard it)
    iterator second_half_position = new_position;
    construction_guard second_half_guard(second_half_position, get_allocator_ref());

    for (size_type i = 0; i < n; ++i)
    {
      alloc_construct(second_half_position++, *(elems++));
      second_half_guard.extend();
    }

    // move front-pos (possibly guarded)
    construction_guard first_half_guard(new_begin, get_allocator_ref());
    opt_move_or_copy(begin(), old_position, new_begin, first_half_guard);

    // move pos+1-end (possibly guarded)
    opt_move_or_copy(old_position, end(), second_half_position, second_half_guard);

    // cleanup
    destroy_elements(begin(), end());
    deallocate_buffer();

    // release alloc and other guards
    second_half_guard.release();
    first_half_guard.release();
    new_buffer_guard.release();

    // rebind members
    _storage._capacity = new_capacity;
    _buffer = new_buffer;
    _back_index = new_front_index + size() + n;
    _front_index = new_front_index;

    return new_position;
  }

  template <typename Iterator>
  void construct_from_range(Iterator begin, Iterator end)
  {
    allocation_guard buffer_guard(_buffer, _storage._capacity, get_allocator_ref());
    if (is_small()) { buffer_guard.release(); } // avoid disposing small buffer

    opt_copy(begin, end, _buffer);

    buffer_guard.release();
  }

  template <typename ForwardIterator>
  void allocate_and_copy_range(ForwardIterator first, ForwardIterator last)
  {
    size_type n = std::distance(first, last);

    pointer new_buffer = allocate(n);
    allocation_guard new_buffer_guard(new_buffer, n, get_allocator_ref());

    opt_copy(first, last, new_buffer);

    destroy_elements(begin(), end());
    deallocate_buffer();

    _storage._capacity = n;
    _buffer = new_buffer;
    _front_index = 0;
    _back_index = n;

    new_buffer_guard.release();
  }

  template <typename... Args>
  void construct_n(pointer buffer, size_type n, Args&&... args)
  {
    construction_guard ctr_guard(buffer, get_allocator_ref());

    guarded_construct_n(buffer, n, ctr_guard, std::forward<Args>(args)...);

    ctr_guard.release();
  }

  template <typename... Args>
  void guarded_construct_n(pointer buffer, size_type n, construction_guard& ctr_guard, Args&&... args)
  {
    for (size_type i = 0; i < n; ++i)
    {
      alloc_construct(buffer + i, std::forward<Args>(args)...);
      ctr_guard.extend();
    }
  }

  static move_guard copy_or_move_front(devector& has_front, devector& needs_front)
    noexcept(t_is_nothrow_constructible)
  {
    pointer src = has_front.begin();
    pointer dst = needs_front._buffer + has_front._front_index;

    move_guard guard(
      src, has_front.get_allocator_ref(),
      dst, needs_front.get_allocator_ref()
    );

    needs_front.opt_move_or_copy(
      src,
      has_front._buffer + (std::min)(has_front._back_index, needs_front._front_index),
      dst,
      guard
    );

    return guard;
  }

  static move_guard copy_or_move_back(devector& has_back, devector& needs_back)
    noexcept(t_is_nothrow_constructible)
  {
    size_type first_pos = (std::max)(has_back._front_index, needs_back._back_index);
    pointer src = has_back._buffer + first_pos;
    pointer dst = needs_back._buffer + first_pos;

    move_guard guard(
      src, has_back.get_allocator_ref(),
      dst, needs_back.get_allocator_ref()
    );

    needs_back.opt_move_or_copy(src, has_back.end(), dst, guard);

    return guard;
  }

  static void swap_small_small(devector& a, devector& b)
    noexcept(t_is_nothrow_constructible) // && nothrow_swappable
  {
    // copy construct elems without pair in the other buffer

    move_guard front_guard =
      (a._front_index < b._front_index) ? copy_or_move_front(a, b) :
      (a._front_index > b._front_index) ? copy_or_move_front(b, a) :
      move_guard{}
    ;

    move_guard back_guard =
      (a._back_index > b._back_index) ? copy_or_move_back(a, b) :
      (a._back_index < b._back_index) ? copy_or_move_back(b, a) :
      move_guard{}
    ;

    // swap elems with pair in the other buffer

    std::swap_ranges(
      a._buffer + (std::min)((std::max)(a._front_index, b._front_index), a._back_index),
      a._buffer + (std::min)(a._back_index, b._back_index),
      b._buffer + (std::max)(a._front_index, b._front_index)
    );

    // no more exceptions
    front_guard.release();
    back_guard.release();
  }

  static void swap_small_big(devector& smll, devector& big)
    noexcept(t_is_nothrow_constructible)
  {
    BOOST_ASSERT(smll.is_small() == true);
    BOOST_ASSERT(big.is_small() == false);

    // small -> big
    big.opt_move_or_copy(
      smll.begin(), smll.end(),
      big._storage.small_buffer_address() + smll._front_index
    );

    smll.destroy_elements(smll.begin(), smll.end());

    // big -> small
    smll._buffer = big._buffer;
    big._buffer = big._storage.small_buffer_address();

    // big <-> small
    std::swap(smll._storage._capacity, big._storage._capacity);
  }

  static void swap_big_big(devector& a, devector& b) noexcept
  {
    BOOST_ASSERT(a.is_small() == false);
    BOOST_ASSERT(b.is_small() == false);

    std::swap(a._storage._capacity, b._storage._capacity);
    std::swap(a._buffer, b._buffer);
  }

  template <typename ForwardIterator>
  void overwrite_buffer(ForwardIterator first, ForwardIterator last)
  {
    if (
       t_is_trivially_copyable
    && std::is_pointer<ForwardIterator>::value
    )
    {
      const size_type n = std::distance(first, last);

      BOOST_ASSERT(capacity() >= n);

      std::memcpy(_buffer, detail::iterator_to_pointer(first), n * sizeof(T));
      _front_index = 0;
      _back_index = n;
    }
    else
    {
      overwrite_buffer_impl(first, last);
    }
  }

  template <typename InputIterator>
  void overwrite_buffer_impl(InputIterator& first, InputIterator last)
  {
    pointer pos = _buffer;
    construction_guard front_guard(pos, get_allocator_ref());

    while (first != last && pos != begin())
    {
      alloc_construct(pos++, *first++);
      front_guard.extend();
    }

    while (first != last && pos != end())
    {
      *pos++ = *first++;
    }

    construction_guard back_guard(pos, get_allocator_ref());

    iterator capacity_end = _buffer + capacity();
    while (first != last && pos != capacity_end)
    {
      alloc_construct(pos++, *first++);
      back_guard.extend();
    }

    pointer destroy_after = (std::min)((std::max)(begin(), pos), end());
    destroy_elements(destroy_after, end());

    front_guard.release();
    back_guard.release();

    _front_index = 0;
    _back_index = pos - begin();
  }

  bool invariants_ok()
  {
    return
       (!_storage._capacity || _buffer)
    && _front_index <= _back_index
    && _back_index <= _storage._capacity
    && sbuffer_size <= _storage._capacity;
  }

  // Small buffer

  bool is_small() const
  {
    return sbuffer_size && _storage._capacity <= sbuffer_size;
  }

  typedef boost::aligned_storage<
    sizeof(T) * sbuffer_size,
    std::alignment_of<T>::value
  > small_buffer_t;

  // Achieve optimal space by leveraging EBO
  struct storage_t : small_buffer_t
  {
    storage_t() : _capacity(sbuffer_size) {}
    storage_t(size_type capacity)
      : _capacity((std::max)(capacity, size_type{sbuffer_size}))
    {}

    T* small_buffer_address()
    {
      return static_cast<T*>(small_buffer_t::address());
    }

    // The only reason _capacity is here to avoid wasting
    // space if `small_buffer_t` is empty.
    size_type _capacity;
  };

  storage_t _storage;
  // 4 bytes padding on 64 bit here
  T* _buffer;

  size_type _front_index;
  size_type _back_index;

#ifdef BOOST_DOUBLE_ENDED_DEVECTOR_ALLOC_STATS
public:
  size_type capacity_alloc_count = 0;

  void reset_alloc_stats()
  {
    capacity_alloc_count = 0;
  }
#endif // ifdef BOOST_DOUBLE_ENDED_DEVECTOR_ALLOC_STATS

#endif // ifndef BOOST_DOUBLE_ENDED_DOXYGEN_INVOKED
};

template <class T, class AllocatorX, class SBPX, class GPX, class AllocatorY, class SBPY, class GPY>
bool operator==(const devector<T, AllocatorX, SBPX, GPX>& x, const devector<T, AllocatorY, SBPY, GPY>& y)
{
  if (x.size() != y.size()) { return false; }
  return std::equal(x.begin(), x.end(), y.begin());
}

template <class T, class AllocatorX, class SBPX, class GPX, class AllocatorY, class SBPY, class GPY>
bool operator< (const devector<T, AllocatorX, SBPX, GPX>& x, const devector<T, AllocatorY, SBPY, GPY>& y)
{
  return std::lexicographical_compare( x.begin(), x.end(),
                                       y.begin(), y.end() );
}

template <class T, class AllocatorX, class SBPX, class GPX, class AllocatorY, class SBPY, class GPY>
bool operator!=(const devector<T, AllocatorX, SBPX, GPX>& x, const devector<T, AllocatorY, SBPY, GPY>& y)
{
  return !(x == y);
}

template <class T, class AllocatorX, class SBPX, class GPX, class AllocatorY, class SBPY, class GPY>
bool operator> (const devector<T, AllocatorX, SBPX, GPX>& x, const devector<T, AllocatorY, SBPY, GPY>& y)
{
  return (y < x);
}

template <class T, class AllocatorX, class SBPX, class GPX, class AllocatorY, class SBPY, class GPY>
bool operator>=(const devector<T, AllocatorX, SBPX, GPX>& x, const devector<T, AllocatorY, SBPY, GPY>& y)
{
  return !(x < y);
}

template <class T, class AllocatorX, class SBPX, class GPX, class AllocatorY, class SBPY, class GPY>
bool operator<=(const devector<T, AllocatorX, SBPX, GPX>& x, const devector<T, AllocatorY, SBPY, GPY>& y)
{
  return !(y < x);
}

template <class T, class Allocator, class SBP, class GP>
void swap(devector<T, Allocator, SBP, GP>& x, devector<T, Allocator, SBP, GP>& y) noexcept(noexcept(x.swap(y)))
{
  x.swap(y);
}

}} // namespace boost::double_ended

#endif // BOOST_DOUBLE_ENDED_DEVECTOR_HPP
