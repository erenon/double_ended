//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Benedek Thaler 2015-2016. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://erenon.hu/double_ended for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_DOUBLE_ENDED_BENCHMARK_BENCHMARK_UTIL_HPP_HPP
#define BOOST_DOUBLE_ENDED_BENCHMARK_BENCHMARK_UTIL_HPP_HPP

#include <deque>
#include <vector>
#include <cstdint> // uint64_t
#include <atomic> // atomic_thread_fence
#include <iostream>
#include <cstring> // strerror

#if defined __GNUC__
  #include <x86intrin.h> // __rdtsc
#elif defined _MSVC_VER
  #include <intrin.h> // __rdtsc
#endif

#if defined _GNU_SOURCE
  #include <pthread.h>
#endif

#include <boost/iterator/iterator_facade.hpp>
#include <boost/test/included/unit_test.hpp> // access cli args

inline void pin_thread()
{
#if defined _GNU_SOURCE
  pthread_t thread = pthread_self();

  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(0, &cpuset); // pin to CPU0

  int ok = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
  if (ok != 0)
  {
    std::cerr << "Failed to pin thread: " << strerror(errno) << "\n";
  }
#elif defined _MSVC_VER
  auto ok = SetThreadAffinityMask(GetCurrentThread(), 1);
  if (! ok)
  {
    std::cerr << "Failed to pin thread: " << GetLastError() << "\n";
  }
#else
  std::cerr << "pin_thread() not implemented on this platform\n";
#endif
}

template <typename T>
void prefault(T* buffer, const std::size_t size)
{
  const std::size_t page_size = 4 * 1024;
  char* cbuffer = reinterpret_cast<char*>(buffer);
  const std::size_t csize = size * sizeof(T);

  for (std::size_t i = 0; i < csize; i += page_size)
  {
    cbuffer[i] = 'X';
  }
}

using clock_type = uint64_t;

inline clock_type get_clock()
{
  return __rdtsc();
}

using sample_container = std::deque<std::vector<clock_type>>;

template <typename SamplesPredicate, typename Callable1, typename Callable2, typename Scale>
std::vector<double> measure(
  const SamplesPredicate& samples_ready,
  Callable1& init,
  Callable2& action,
  const Scale& scale
)
{
  sample_container samples;

  while (! samples_ready(samples))
  {
    samples.emplace_back();
    auto& sample = samples.back();
    sample.reserve(scale.size());

    init();

    std::atomic_thread_fence(std::memory_order_seq_cst);

    const clock_type base_clock = get_clock();

    std::size_t action_idx = 0;
    for (std::size_t step : scale)
    {
      for (; action_idx < step; ++action_idx)
      {
        action();
      }

      sample.push_back(get_clock() - base_clock);
    }
  }

  std::vector<double> result(scale.size());
  for (std::size_t clock = 0; clock < scale.size(); ++clock)
  {
    std::size_t sum = 0;
    for (auto&& sample : samples)
    {
      sum += sample[clock];
    }
    result[clock] = double(sum) / samples.size();
  }

  return result;
}

// SamplesPredicates

struct n_samples
{
  const std::size_t _n;
  n_samples(std::size_t n) : _n(n) {}
  bool operator()(sample_container& sc) const { return sc.size() >= _n; }
};

// alternative: gather until std deviation is below a specified limit

// Scales

class exp_scale
{
  class exp_scale_iterator : public boost::iterator_facade<
      exp_scale_iterator,
      const std::size_t,
      boost::forward_traversal_tag
    >
  {
  public:
    exp_scale_iterator() = default;
    exp_scale_iterator(std::size_t value, std::size_t mult, std::size_t remaining)
      :_value(value),
       _mult(mult),
       _remaining(remaining)
    {}

  private:
    friend class boost::iterator_core_access;

    const std::size_t& dereference() const { return _value; }
    bool equal(const exp_scale_iterator& rhs) const { return _remaining == rhs._remaining; }
    void increment() { _value *= _mult; --_remaining; }

    std::size_t _value;
    const std::size_t _mult = 0;
    std::size_t _remaining = 0;
  };

public:
  using iterator = exp_scale_iterator;

  exp_scale(std::size_t initial, std::size_t mult, std::size_t size)
    :_initial(initial),
     _mult(mult),
     _size(size)
  {}

  iterator begin() const { return iterator{_initial, _mult, _size}; }
  iterator end() const { return iterator{}; }
  std::size_t size() const { return _size; }

  std::size_t back() const
  {
    auto i = begin();
    std::size_t last_val = 0;
    while (i != end()) { last_val = *i; ++i; }
    return last_val;
  }

private:
  const std::size_t _initial;
  const std::size_t _mult;
  const std::size_t _size;
};

std::string report_dir()
{
  auto&& ms = boost::unit_test::framework::master_test_suite();
  return (ms.argc < 2) ? "/tmp/" : std::string(ms.argv[1]) + '/';
}

template <typename Scale>
void report(
  const std::string& report_name,
  const std::string& seria_name,
  const Scale& scale,
  const std::vector<double>& seria
)
{
  const std::string report_path = report_dir() + report_name + ".dat";
  std::fstream out(report_path, std::ios_base::out);

  out << "X " << seria_name << "\n0 0\n";
  auto y = seria.begin();
  for (auto x = scale.begin(); x != scale.end(); ++x, ++y)
  {
    out << *x << " " << std::fixed << *y << "\n";
  }

  if (out)
  {
    BOOST_TEST_MESSAGE("Report written to: ") << report_path;
  }
  else
  {
    BOOST_FAIL("Failed to write report: " << report_path);
  }
}

#endif // BOOST_DOUBLE_ENDED_BENCHMARK_BENCHMARK_UTIL_HPP_HPP
