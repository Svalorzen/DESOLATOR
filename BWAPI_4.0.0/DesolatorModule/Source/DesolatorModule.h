#pragma once
#include <BWAPI.h>
#include <map>
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
public:
  time_t timestamp;
  time_t lastAttack;
  int health;
  double distanceToClosestEnemy;
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
  std::map<int, Observation> observations;
  std::map<int, State> states;
  std::map<int, Action> actions;
  void explore(BWAPI::Unit *unit);
  void flee(BWAPI::Unit *unit);
  void findEnemies(BWAPI::Unitset *enemies);
  void evaluateText(std::string text);
  BWAPI::Unit * findClosestEnemy(BWAPI::Unit *unit);
  bool feedback;
};
