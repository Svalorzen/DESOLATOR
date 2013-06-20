#pragma once
class State
{
  /* Represents the state */
public:
  State();
  State(int);

  int enemyHeatMap;   // Either value (0: free, 1: enemy range, 2: targeted)
  int friendHeatMap;  // Either value (0: uncovered, 1: covered)
  int weaponCooldown; // Either value (0: false, 1: true)
  int canTarget;      // Either value (0: false, 1: true)
  int health;         // Either value (0: 25%, 1: 50%, 2: 75%, 3: 100%)

  // Converts the state into a number from 0 to 95 ( so we can use them as unique indexes for arrays and stuff )
  operator int();

  static const int statesNumber = 96;
};