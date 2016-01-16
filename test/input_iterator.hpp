//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Benedek Thaler 2015-2016. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://erenon.hu/double_ended for documentation.
//
//////////////////////////////////////////////////////////////////////////////

/**
 * Emulates input iterator, validates algorithm
 * does a single pass only, removes visited elements.
 *
 * Container::erase(front_iterator) must not invalidate other iterators.
 *
 * The hack around `_erase_on_destroy` is required to make `*it++` work.
 */
template <typename Container>
class input_iterator : public std::iterator<std::input_iterator_tag, typename Container::value_type>
{
  typedef typename Container::iterator iterator;

  struct erase_on_destroy {};

public:
  typedef typename Container::value_type value_type;

  input_iterator() = default;

  input_iterator(Container& c, iterator it)
    :_container(&c),
     _it(it)
  {}

  input_iterator(const input_iterator& rhs)
    :_container(rhs._container),
     _it(rhs._it),
     _erase_on_destroy(rhs._erase_on_destroy)
  {
    rhs._erase_on_destroy = false;
  }

  input_iterator(const input_iterator& rhs, erase_on_destroy)
    :_container(rhs._container),
     _it(rhs._it),
     _erase_on_destroy(true)
  {}

  ~input_iterator()
  {
    if (_erase_on_destroy)
    {
      _container->erase(_it); // must not invalidate other iterators
    }
  }

  const value_type& operator*()
  {
    return *_it;
  }

  input_iterator operator++()
  {
    _container->erase(_it);
    ++_it;
    return *this;
  }

  input_iterator operator++(int)
  {
    input_iterator old(*this, erase_on_destroy{});
    ++_it;
    return old;
  }

  friend bool operator==(const input_iterator a, const input_iterator b)
  {
    return a._it == b._it;
  }

  friend bool operator!=(const input_iterator a, const input_iterator b)
  {
    return !(a == b);
  }

private:
  Container* _container;
  iterator _it;
  mutable bool _erase_on_destroy = false;
};

template <typename Container>
input_iterator<Container> make_input_iterator(Container& c, typename Container::iterator it)
{
  return input_iterator<Container>(c, it);
}
