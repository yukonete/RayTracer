#include "AABB.h"
#include "Interval.h"

const auto AABB::empty = AABB{Interval::empty, Interval::empty, Interval::empty};
const auto AABB::universe = AABB{Interval::universe, Interval::universe, Interval::universe};