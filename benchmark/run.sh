#!/bin/bash

# (C) Copyright Benedek Thaler 2015-2016
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

if [ $# -lt 1 ]; then
  echo "Usage $0 <benchmark_binary>"
  exit 1
fi

TMP_DIR=/tmp/double_ended_bench
BINARY=$1

mkdir -p $TMP_DIR

echo "Don't forget to turn off CPU scaling on CPU0, and boot with isolcpus=1"
echo "Run tests..."

$BINARY -- $TMP_DIR

function plot3
{
  title=$1
  output=$TMP_DIR/$2.png
  src1=$TMP_DIR/$3.dat
  src2=$TMP_DIR/$4.dat
  src3=$TMP_DIR/$5.dat

  gnuplot \
    -e "set title \"$title\"" \
    -e "set term pngcairo" \
    -e "set output \"$output\"" \
    -e "set key left" \
    -e "set key autotitle columnheader" \
    -e "set autoscale xfix" \
    -e "set format x \"%.0s%c\"" \
    -e "set xlabel 'Container size'" \
    -e "set ylabel 'Time Stamp Counter'" \
    -e "plot \"$src1\" u 1:2 smooth csplines, \"$src2\" u 1:2 smooth csplines, \"$src3\" u 1:2 smooth csplines" &&

  echo "Plot done: $output"
}

echo "Plot..."

plot3 "Vector push_back() to preallocated" vector_push_back push_back_vector push_back_bcvector push_back_devector
plot3 "Deque push_back()" deque_push_back push_back_deque push_back_bcdeque push_back_batch_deque
