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
#include <deque>

#define BOOST_TEST_MODULE benchmark
#include <boost/test/included/unit_test.hpp>

#include <boost/container/vector.hpp>
#include <boost/container/deque.hpp>

#include <boost/double_ended/devector.hpp>
#include <boost/double_ended/batch_deque.hpp>

#include "benchmark_util.hpp"

namespace {

struct config
{
  config()
    :scale(100, 2, 14)
  {
    pin_thread();
  }

  exp_scale scale;
};

config g_config;

template <typename Vector, typename Scale>
std::vector<double> push_back_impl(const Scale& scale)
{
  Vector v;

  auto init = [&]()
  {
    v.clear();
    const std::size_t max_size = scale.back();
    v.reserve(max_size);
    prefault(v.data(), max_size);
  };

  auto action = [&v]()
  {
    v.push_back(1);
  };

  auto result = measure(
    n_samples(100),
    init,
    action,
    scale
  );

  return result;
}

template <typename Deque, typename Scale>
std::vector<double> deque_push_back_impl(const Scale& scale)
{
  Deque v;

  auto init = [&]()
  {
    v.clear();
  };

  auto action = [&v]()
  {
    v.push_back(1);
  };

  auto result = measure(
    n_samples(25),
    init,
    action,
    scale
  );

  return result;
}

} // namespace

BOOST_AUTO_TEST_SUITE(push_back)

BOOST_AUTO_TEST_CASE(vector)
{
  auto&& scale = g_config.scale;
  auto result = push_back_impl<std::vector<unsigned>>(scale);
  report("push_back_vector", "std::vector", scale, result);
}

BOOST_AUTO_TEST_CASE(devector)
{
  auto&& scale = g_config.scale;
  auto result = push_back_impl<boost::double_ended::devector<unsigned>>(scale);
  report("push_back_devector", "devector", scale, result);
}

BOOST_AUTO_TEST_CASE(bcvector)
{
  auto&& scale = g_config.scale;
  auto result = push_back_impl<boost::container::vector<unsigned>>(scale);
  report("push_back_bcvector", "boost::container::vector", scale, result);
}

BOOST_AUTO_TEST_CASE(deque)
{
  auto&& scale = g_config.scale;
  auto result = deque_push_back_impl<std::deque<unsigned>>(scale);
  report("push_back_deque", "std::deque", scale, result);
}

BOOST_AUTO_TEST_CASE(batch_deque)
{
  auto&& scale = g_config.scale;
  auto result = deque_push_back_impl<boost::double_ended::batch_deque<unsigned>>(scale);
  report("push_back_batch_deque", "batch_deque", scale, result);
}

BOOST_AUTO_TEST_CASE(bcdeque)
{
  auto&& scale = g_config.scale;
  auto result = deque_push_back_impl<boost::container::deque<unsigned>>(scale);
  report("push_back_bcdeque", "boost::container::deque", scale, result);
}

BOOST_AUTO_TEST_SUITE_END()
