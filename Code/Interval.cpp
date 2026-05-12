#include "RtWeekend.h"
#include "Interval.h"

const auto Interval::empty = Interval{+infinity, -infinity};
const auto Interval::universe = Interval{-infinity, +infinity};