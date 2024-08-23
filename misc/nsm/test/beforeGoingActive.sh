#!/bin/bash
#
# This script is called before an instance transitions
# to goingActive in order to determine the numeric range
# within which to generate a random number.
#
# By default the range is 1000000-2000000.
#
# By returning "1000000-2000000", this script effectively sets
# the range to the default.
#
# However, the range can be changed with a different return
# string.  For example: "3000000-3500000" will result
# in a random number between 3 million and 3.5 million which
# sets the instance apart from any other instance using the
# defaults of 1000000-2000000 and, therefore, ensure it
# winning the election assuming other instances are not
# using similar ranges.
#
# This is useful for those instances where a version of
# this script analyzes the environment to determine any
# external factors that should be dynamically taken into 
# account for voting; such as network reachability, 
# bandwidth cost, roundtrip times, and so on.

echo "1000000-2000000"