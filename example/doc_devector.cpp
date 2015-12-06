//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Benedek Thaler 2015-2015. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://erenon.hu/double_ended for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#include <sys/socket.h> // recv

#include <vector>
#include <string>
#include <iostream>
#include <iterator>

#include <boost/double_ended/devector.hpp>
#include <boost/assert.hpp>

//[doc_devector_growth_policy
struct custom_growth_policy
{
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

using namespace boost::double_ended;

devector<int, devector_small_buffer_policy<0>, custom_growth_policy> custom_growth_devector;
//]

template <typename T>
using custom_allocator = std::allocator<T>;

int main()
{
  (void) custom_growth_devector;

//[doc_devector_replace_vector
  using namespace boost::double_ended; // assumed here and below

  std::vector<int> regular_vector;
  devector<int> regular_devector;

  std::vector<int, custom_allocator<int>> custom_alloc_vector;

  devector<
    int,
    devector_small_buffer_policy<0>,
    devector_growth_policy,
    custom_allocator<int>
  > custom_alloc_devector;
//]

//[doc_devector_sbo
  devector<int, devector_small_buffer_policy<16>> small_devector{1, 2, 3, 4};
  small_devector.push_back(5);
  small_devector.push_back(6);
  small_devector.push_back(7);
  // no dynamic memory allocated
//]

//[doc_devector_ctr_reserve
  devector<int> reserved_devector(32, 16, reserve_only_tag{});

  for (int i = 0; i < 32; ++i) {
    reserved_devector.push_front(i);
  }

  for (int i = 0; i < 16; ++i) {
    reserved_devector.push_back(i);
  }
//]

//[doc_devector_reverse_input
  devector<std::string> reversed_lines;

  std::string line;
  std::getline(std::cin, line);

  while (std::cin)
  {
    reversed_lines.push_front(line);
    std::getline(std::cin, line);
  }

  std::cout << "Reversed lines:\n";
  std::copy(
    reversed_lines.begin(), reversed_lines.end(),
    std::ostream_iterator<std::string>(std::cout, "\n")
  );
//]

//[doc_devector_unsafe_push
  devector<int> dv;
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
  devector<char> buffer(256, unsafe_uninitialized_tag{});
  ssize_t recvSize = recv(sockfd, buffer.data(), buffer.size(), 0);

  if (recvSize >= 0)
  {
    buffer.unsafe_uninitialized_resize_back(recvSize);
    // process contents of buffer
  }
  else { /* handle error */ }
//]

  return 0;
}
