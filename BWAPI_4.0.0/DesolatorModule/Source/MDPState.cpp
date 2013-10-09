#include "MDPState.h"

#include <stdexcept>

MDPState::MDPState() : enemyHeatMap(0), friendHeatMap(0), weaponCooldown(0), canTarget(0), health(0) {}

MDPState::MDPState(int i) {
  if ( i > 95 ) throw std::runtime_error("Index creating MDPState not valid");

  health          = i % 4;
  i >>= 2;
  canTarget       = i % 2;
  i >>= 1;
  weaponCooldown  = i % 2;
  i >>= 1;
  friendHeatMap   = i % 2;
  i >>= 1;
  enemyHeatMap    = i;
}

MDPState::operator int() const {
  int index =
    ( enemyHeatMap    << 5 ) +  // (0, 1, 2) -> (00, 01, 10)
    ( friendHeatMap   << 4 ) +  // (0, 1)    -> (0, 1)
    ( weaponCooldown  << 3 ) +  // (0, 1)    -> (0, 1)
    ( canTarget       << 2 ) +  // (0, 1)    -> (0, 1)
    ( health               );   // (0, 1, 2, 3)    -> (00, 01, 10, 11)

  // Final : (0, 95) -> (000 0000, 101 1111)
  return index;
}