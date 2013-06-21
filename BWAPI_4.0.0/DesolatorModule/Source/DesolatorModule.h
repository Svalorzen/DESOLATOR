#pragma once
#include <BWAPI.h>
#include <map>
#include <array>
#include <time.h>
#include <fstream>

#include "gameState.h"

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

  // STATE VARIABLES
  std::map<int, GameState> gameStates;
  // STATE METHODS
  void updateGameState(BWAPI::Unit *unit, const BWAPI::Unitset & alliedUnits, const BWAPI::Unitset & enemyUnits, bool alsoState = false);
  
  // ACTIONS
  void explore(const BWAPI::Unitset & units);
  BWAPI::PositionOrUnit attack(BWAPI::Unit *unit, const BWAPI::Unitset & allies, const BWAPI::Unitset & enemies);
  BWAPI::Position flee(BWAPI::Unit *unit, const BWAPI::Unitset & friends, const BWAPI::Unitset & enemies);

  // FEEDBACK STUFF
  void evaluateText(std::string text);
  bool feedback;

  // TABLE VARIABLES
  bool tableIsValid;
  std::array<std::array<std::pair<long unsigned, long unsigned>, State::statesNumber>, State::statesNumber> table;
  // TABLE METHODS
  void updateTable(State, Action, State, double reward = 0.0);
  bool loadTable(const char * filename);
  bool saveTable(const char * filename);

  std::ofstream log;
};

/* AUXILLARY FUNCTIONS */
// BWAPI BOOSTS
int getActualWeaponRange(BWAPI::Unit *unit);
bool isMelee(BWAPI::Unit *unit);
int getOptimizedWeaponRange(BWAPI::Unit *unit);
BWAPI::TilePosition convertToTile(BWAPI::Position);
// DRAWING FUNCTIONS
void drawHeatMap(BWAPI::Player *us, BWAPI::Player *enemy);
void drawState(std::map<int, GameState> *states);