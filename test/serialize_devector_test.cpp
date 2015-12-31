//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Benedek Thaler 2015-2015. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://erenon.hu/double_ended for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#include <sstream>

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

#include <boost/double_ended/serialize_devector.hpp>

#include "test_elem.hpp"

using namespace boost::double_ended;

#define BOOST_TEST_MODULE serialize_devector
#include <boost/test/included/unit_test.hpp>

#include <boost/mpl/list.hpp>

using all_devectors = boost::mpl::list<
  devector<unsigned>,
  devector<regular_elem>
>;

BOOST_AUTO_TEST_CASE_TEMPLATE(empty, Devector, all_devectors)
{
  const Devector a;

  std::stringstream stream;

  {
    boost::archive::text_oarchive out(stream);
    out << a;
  }

  Devector b;
  boost::archive::text_iarchive in(stream);
  in >> b;

  BOOST_TEST(b.empty());
}

BOOST_AUTO_TEST_CASE_TEMPLATE(into_empty, Devector, all_devectors)
{
  const Devector a{1, 2, 3, 4, 5, 6, 7, 8};

  std::stringstream stream;

  {
    boost::archive::text_oarchive out(stream);
    out << a;
  }

  Devector b;
  boost::archive::text_iarchive in(stream);
  in >> b;

  BOOST_TEST(b == a);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(into_non_empty, Devector, all_devectors)
{
  const Devector a{1, 2, 3, 4, 5, 6, 7, 8};

  std::stringstream stream;

  {
    boost::archive::text_oarchive out(stream);
    out << a;
  }

  Devector b{11, 12, 13, 14};
  boost::archive::text_iarchive in(stream);
  in >> b;

  BOOST_TEST(b == a);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(with_enough_capacity, Devector, all_devectors)
{
  const Devector a{1, 2, 3, 4, 5, 6, 7, 8};

  std::stringstream stream;

  {
    boost::archive::text_oarchive out(stream);
    out << a;
  }

  Devector b{11, 12, 13, 14, 15, 16, 17, 18};
  b.pop_front();
  b.pop_front();
  b.pop_back();

  boost::archive::text_iarchive in(stream);
  in >> b;

  BOOST_TEST(b == a);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(with_enough_size, Devector, all_devectors)
{
  const Devector a{1, 2, 3, 4};

  std::stringstream stream;

  {
    boost::archive::text_oarchive out(stream);
    out << a;
  }

  Devector b{11, 12, 13, 14, 15, 16, 17, 18};
  b.pop_front();
  b.pop_back();
  b.pop_back();

  boost::archive::text_iarchive in(stream);
  in >> b;

  BOOST_TEST(b == a);
}
