//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Benedek Thaler 2015-2015. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://erenon.hu/double_ended for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#include <vector>

#include <boost/double_ended/batch_deque.hpp>
#include <boost/assert.hpp>

namespace de = boost::double_ended;

// unistd.h, missing on non-POSIX systems
int write(int, const void*, size_t) { return -1; }

void iterate_segments()
{
//[doc_batch_deque_sizing
  de::batch_deque<char, de::batch_deque_policy<256>> deque;
//]

  int fd = 1;

//[doc_batch_deque_segment_iterator
  for (auto it = deque.segment_begin(); it != deque.segment_end(); ++it)
  {
    write(fd, it.data(), it.data_size());
  }
//]
}

void insert_stable()
{
//[doc_batch_deque_stable_insert
  de::batch_deque<int> deque{1, 2, 3, 4, 5, 6, 7, 8};
  auto four_it = deque.begin() + 3;
  int& four = *four_it;

  std::vector<int> input(100, 9);
  deque.stable_insert(four_it, input.begin(), input.end());

  BOOST_ASSERT(four == 4); // stable_insert ensures reference stability
//]
}

int main()
{
  iterate_segments();
  insert_stable();

  return 0;
}
