# DoubleEndend containers
[![Build Status](https://travis-ci.org/erenon/double_ended.svg?branch=master)](https://travis-ci.org/erenon/double_ended)
[![Build Status](https://ci.appveyor.com/api/projects/status/github/erenon/double_ended?svg=true&branch=master)](https://ci.appveyor.com/project/erenon/double-ended)

This library provides two double ended containers, `devector` and `batch_deque`
similar to the `vector` and `deque` in the C++ Standard Library,
but with additional features geared towards high performance, and unsafe constructs, giving more
control to the user.

## Overview

`devector` is a hybrid of the standard vector and deque containers, as well as the `small_vector` of Boost.Container.
It offers cheap (amortized constant time) insertion at both the front and back ends,
while also providing the regular features of `std::vector`, in particular the contiguous underlying memory.
In contrast to the standard vector, however, `devector` offers greater control over its internals.
Like `small_vector`, it can store elements internally without dynamically allocating memory.

`batch_deque` is very similar to the standard `deque`, with a slight twist. It lets you specify its /segment size/:
The size of the contiguous memory segments it uses to store its elements. This provides a consistent layout across platforms,
and lets you iterate over the segments with batch operations.

Read more in the [documentation][0]

> This is not an official Boost library

[0]: http://erenon.hu/double_ended/
