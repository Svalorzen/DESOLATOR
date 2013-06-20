#pragma once
#include <BWAPI.h>
#include <map>
#include <array>
#include <time.h>

#include "state.h"

enum Action
{
  Attack,
  Flee
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

  std::map<int, State> states;
  std::map<int, Action> actions;
  std::map<int, BWAPI::TilePosition> lastPositions;

  std::array<std::array<std::pair<long unsigned, long unsigned>, State::statesNumber>, State::statesNumber> table;
  bool tableIsValid;
  void DesolatorModule::updateTable(BWAPI::Unit * unit, const BWAPI::Unitset & myUnits, const BWAPI::Unitset & enemyUnits);
  bool loadTable(const char * filename);
  bool saveTable(const char * filename);

  State getState(BWAPI::Unit *unit, const BWAPI::Unitset & alliedUnits, const BWAPI::Unitset & enemyUnits);
  BWAPI::Position flee(BWAPI::Unit *unit, const BWAPI::Unitset & friends, const BWAPI::Unitset & enemies);
  void evaluateText(std::string text);
  BWAPI::Unit * findClosestEnemy(const BWAPI::Unit *unit, const BWAPI::Unitset & enemies);
  bool feedback;
};

/* AUXILLARY FUNCTIONS */
void drawHeatMap(BWAPI::Player *us, BWAPI::Player *enemy);
void drawState(std::map<int, State> *states);
int getActualWeaponRange(BWAPI::Unit *unit);
bool isMelee(BWAPI::Unit *unit);
int getOptimizedWeaponRange(BWAPI::Unit *unit);