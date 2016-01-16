//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Benedek Thaler 2015-2016. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://erenon.hu/double_ended for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#include <sstream>

//[doc_serialize_basics
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

#include <boost/double_ended/serialize_devector.hpp>
#include <boost/double_ended/serialize_batch_deque.hpp>

namespace de = boost::double_ended;

int main()
{
  const de::batch_deque<int> bd{1, 2, 3, 4, 5, 6, 7, 8};
  const de::devector<int> dv{9, 10, 11, 12, 13, 14, 15, 16};

  std::stringstream stream;

  {
    boost::archive::text_oarchive out(stream);
    out << bd;
    out << dv;
  }

  de::batch_deque<int> new_bd;
  de::devector<int> new_dv;

  {
    boost::archive::text_iarchive in(stream);
    in >> new_bd;
    in >> new_dv;
  }

  BOOST_ASSERT(bd == new_bd);
  BOOST_ASSERT(dv == new_dv);

  return 0;
}
//]
