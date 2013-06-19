#pragma once
#include <BWAPI.h>
#include <map>
#include <array>
#include <time.h>

enum Action
{
  Attack,
  Flee,
  Explore,
  Init
};

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

  static const int statesNumber;
};

class Observation
{
public:
  Action previousAction;
  State previousState;
};

// Remember not to use "Broodwar" in any global class constructor!

class DesolatorModule : public BWAPI::AIModule
{
public:
  // Virtual functions for callbacks, leave these as they are.
  virtual void onStart();
  virtual void onEnd(bool isWinner);
  virtual void onFrame();
  virtual void onSendText(std::string text);
  virtual void onReceiveText(BWAPI::Player* player, std::string text);
  virtual void onPlayerLeft(BWAPI::Player* player);
  virtual void onNukeDetect(BWAPI::Position target);
  virtual void onUnitDiscover(BWAPI::Unit* unit);
  virtual void onUnitEvade(BWAPI::Unit* unit);
  virtual void onUnitShow(BWAPI::Unit* unit);
  virtual void onUnitHide(BWAPI::Unit* unit);
  virtual void onUnitCreate(BWAPI::Unit* unit);
  virtual void onUnitDestroy(BWAPI::Unit* unit);
  virtual void onUnitMorph(BWAPI::Unit* unit);
  virtual void onUnitRenegade(BWAPI::Unit* unit);
  virtual void onSaveGame(std::string gameName);
  virtual void onUnitComplete(BWAPI::Unit *unit);
  // Everything below this line is safe to modify.
private:
  BWAPI::Player *us;
  BWAPI::Player *them;

  std::map<int, Observation> observations;
  std::map<int, State> states;
  std::map<int, Action> actions;

  std::array<std::array<double, 96>, 96> table;
  void loadTable(const char * filename);
  void saveTable(const char * filename);

  State getState(BWAPI::Unit *unit, BWAPI::Unitset *alliedUnits, BWAPI::Unitset *enemyUnits);
  void flee(BWAPI::Unit *unit);
  void findEnemies(BWAPI::Unitset *enemies);
  void evaluateText(std::string text);
  BWAPI::Unit * findClosestEnemy(BWAPI::Unit *unit);
  bool seeEnemies();
  bool feedback;
};

/* AUXILLARY FUNCTIONS */
void drawHeatMap(BWAPI::Player *us, BWAPI::Player *enemy);
int getActualWeaponRange(BWAPI::Unit *unit);
bool isMelee(BWAPI::Unit *unit);
int getOptimizedWeaponRange(BWAPI::Unit *unit);