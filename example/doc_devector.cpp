//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Benedek Thaler 2015-2016. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://erenon.hu/double_ended for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#include <vector>
#include <string>
#include <iostream>
#include <iterator>

#include <boost/assert.hpp>

//[doc_devector_preamble
#include <boost/double_ended/devector.hpp>
#include <boost/double_ended/batch_deque.hpp>

namespace de = boost::double_ended;
//]

// sys/socket.h, missing on non-POSIX systems
long recv(int, void*, size_t size, int) { return size; }

//[doc_devector_growth_policy
struct custom_growth_policy
{
  using size_type = unsigned int;

  static constexpr unsigned growth_factor = 2u;
  static constexpr unsigned initial_size = 16u;

  static unsigned new_capacity(unsigned capacity)
  {
    return (capacity) ? capacity * growth_factor : initial_size;
  }

  static bool should_shrink(unsigned size, unsigned capacity, unsigned small_buffer_size)
  {
    (void)capacity;
    return size <= small_buffer_size;
  }
};

de::devector<int, de::devector_small_buffer_policy<0>, custom_growth_policy> custom_growth_devector;
//]

template <typename T>
using custom_allocator = std::allocator<T>;

int main()
{
  (void) custom_growth_devector;

//[doc_devector_sbo
  de::devector<int, de::devector_small_buffer_policy<16>> small_devector;

  BOOST_ASSERT(small_devector.capacity() == 16); // but no dynamic memory was allocated

  small_devector.push_back(2);
  small_devector.push_back(3);
  small_devector.push_back(4);

  small_devector.push_front(1); // allocates
//]

//[doc_devector_ctr_reserve
  de::devector<int> reserved_devector(32, 16, de::reserve_only_tag{});

  for (int i = 0; i < 32; ++i) {
    reserved_devector.push_front(i);
  }

  for (int i = 0; i < 16; ++i) {
    reserved_devector.push_back(i);
  }
//]

  std::cin.setstate(std::ios_base::eofbit);

//[doc_devector_reverse_input
  de::devector<std::string> reversed_lines;
  reversed_lines.reserve_front(24);

  std::string line;

  while (std::getline(std::cin, line))
  {
    reversed_lines.push_front(line);
  }

  std::cout << "Reversed lines:\n";
  for (auto&& line : reversed_lines)
  {
    std::cout << line << "\n";
  }
//]

//[doc_devector_unsafe_push
  de::devector<int> dv;
  dv.reserve_front(2);
  dv.reserve_back(2); // the previous reserve_front is still in effect

  dv.unsafe_push_front(2);
  dv.unsafe_push_front(1);
  dv.unsafe_push_back(3);
  dv.unsafe_push_back(4);
//]

  int sockfd = 0;
//[doc_devector_unsafe_uninit
  // int sockfd = socket(...); connect(...);
  de::devector<char> buffer(256, de::unsafe_uninitialized_tag{});
  long recvSize = recv(sockfd, buffer.data(), buffer.size(), 0);

  if (recvSize >= 0)
  {
    buffer.unsafe_uninitialized_resize_back(recvSize);
    // process contents of buffer
  }
  else { /* handle error */ }
//]

//[doc_devector_replace_vector
  std::vector<int> regular_vector;
  de::devector<int> regular_devector;

  std::vector<int, custom_allocator<int>> custom_alloc_vector;

  de::devector<
    int,
    de::devector_small_buffer_policy<0>,
    de::devector_growth_policy,
    custom_allocator<int>
  > custom_alloc_devector;
//]

  return 0;
}
