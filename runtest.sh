#!/bin/bash

rm -rf /tmp/sampledata
/usr/bin/cp -avf testsuite/sampledata /tmp
runtest -v  -v  --tool ferrisstreams EXBASE=`pwd`/ SDATA=`pwd`/testsuite/sampledata --srcdir `pwd`/testsuite

