#include "DesolatorModule.h"
#include <fstream>
#include <cmath>
#include <algorithm>

using namespace BWAPI;
using namespace Filter;

const int STANDARD_SPEED_FPS = 19;

void DesolatorModule::onStart()
{
  /*** STANDARD IMPLEMENTATION ***/
  // Hello World!
  Broodwar->sendText("Desolator Module activated!");

  loadTable("test.data");
  saveTable("test2.data");

  // Print the map name.
  // BWAPI returns std::string when retrieving a string, don't forget to add .c_str() when printing!
  Broodwar << "The map is " << Broodwar->mapName() << "!" << std::endl;

  // Enable the UserInput flag, which allows us to control the bot and type messages.
  Broodwar->enableFlag(Flag::UserInput);

  // Uncomment the following line and the bot will know about everything through the fog of war (cheat).
  //Broodwar->enableFlag(Flag::CompleteMapInformation);

  // Set the command optimization level so that common commands can be grouped
  // and reduce the bot's APM (Actions Per Minute).
  Broodwar->setCommandOptimizationLevel(2);

  // Check if this is a replay
  if ( Broodwar->isReplay() )
  {
    // Announce the players in the replay
    Broodwar << "The following players are in this replay:" << std::endl;
    
    // Iterate all the players in the game using a std:: iterator
    Playerset players = Broodwar->getPlayers();
    for(auto p = players.begin(); p != players.end(); ++p)
    {
      // Only print the player if they are not an observer
      if ( !p->isObserver() )
        Broodwar << p->getName() << ", playing as " << p->getRace() << std::endl;
    }

  }
  else // if this is not a replay
  {
    // Retrieve you and your enemy's races. enemy() will just return the first enemy.
    // If you wish to deal with multiple enemies then you must use enemies().
    if ( Broodwar->enemy() ) // First make sure there is an enemy
      Broodwar << "The matchup is " << Broodwar->self()->getRace() << " vs " << Broodwar->enemy()->getRace() << std::endl;

    /*** DESOLATOR IMPLEMENTATION ***/
    this->us = Broodwar->self();
    this->them = Broodwar->enemy();

    BWAPI::Unitset myUnits = this->us->getUnits();
    BWAPI::Unitset enemyUnits = this->them->getUnits();

    // Initialize state and action for every unit
    for(BWAPI::Unitset::iterator u = myUnits.begin(); u != myUnits.end(); u++)
    {
      State state = this->getState(*u, &myUnits, &enemyUnits);
      this->states[u->getID()] = state;
    }
    this->feedback = false;
  }
}

void DesolatorModule::onFrame()
{
  /*** STANDARD IMPLEMENTATION ***/
  // Display the game frame rate as text in the upper left area of the screen
  Broodwar->drawTextScreen(5, 0,  "FPS: %d", Broodwar->getFPS() );
  Broodwar->drawTextScreen(5, 10, "Average FPS: %f", Broodwar->getAverageFPS() );

  drawHeatMap(this->us, this->them);
  drawState(&this->states);

  // Return if the game is a replay or is paused
  if ( Broodwar->isReplay() || Broodwar->isPaused() || !Broodwar->self() )
    return;

  // Prevent spamming by only running our onFrame once every number of latency frames.
  // Latency frames are the number of frames before commands are processed.
  if ( Broodwar->getFrameCount() % Broodwar->getLatencyFrames() != 0 )
    return;

  /*** DESOLATOR IMPLEMENTATION ***/
  // For feedback
  if(this->feedback)
  {
    // Stop all units.
    BWAPI::Unitset units = this->us->getUnits();
    for(BWAPI::Unitset::iterator unit = units.begin(); unit != units.end(); unit++)
    {
      unit->stop();
    }
    return;
  }

  // FOR CAMIEL:

  // This is your change
  auto & myUnits = this->us->getUnits();
  auto & enemyUnits = this->them->getUnits();

  // Please leave auto 

  // Observe new state
  for(Unitset::iterator u = myUnits.begin(); u != myUnits.end(); ++u)
  {
    this->states[u->getID()] = this->getState(*u, &myUnits, &enemyUnits);
  }

  if(enemyUnits.empty())
  {
    // Explore together if no enemies are found and we are not moving.
    BWAPI::Unitset::iterator u = myUnits.begin();

    if(!u->isMoving())
    {
      int x = rand() % (Broodwar->mapWidth());
      int y = rand() % (Broodwar->mapHeight());
      BWAPI::TilePosition pos(x, y);

      for (; u != myUnits.end(); u++)
      {
        u->move(BWAPI::Position(pos));
        Broodwar->printf("Explore: (%d, %d)", x, y);
      }
    }
  } else {
    // Otherwise coordinate units.
    for ( BWAPI::Unitset::iterator u = myUnits.begin(); u != myUnits.end(); ++u )
    {
      // Ignore the unit if it no longer exists
      // Make sure to include this block when handling any Unit pointer!
      if ( !u->exists() )
        continue;

      // Check when the units moved a tile
      BWAPI::TilePosition previous = this->lastPositions[u->getID()];
      BWAPI::TilePosition current = u->getTilePosition();
      if (previous != current)
        Broodwar->printf("Moved from (%d, %d) to (%d, %d)",
          previous.x,
          previous.y,
          current.x,
          current.y);

      BWAPI::Unit *enemy = this->findClosestEnemy(*u);
      if(u->isUnderAttack())
      {
        if(this->actions[u->getID()] != Flee)
        {
          this->flee(*u);
          this->actions[u->getID()] = Flee;
        }
      }
      else if(this->actions[u->getID()] != Attack)
      {
        u->attack(enemy->getPosition());
        this->actions[u->getID()] = Attack;
        Broodwar->printf("Attacking enemy at: %d %d", enemy->getTilePosition().x, enemy->getTilePosition().y);
      }

      // Update last positions
      this->lastPositions[u->getID()] = u->getTilePosition();
    } // closure: unit iterator
  } // closure: else
}

void DesolatorModule::flee(BWAPI::Unit *unit)
{
  BWAPI::Unit *enemy = this->findClosestEnemy(unit);
  if(enemy == NULL)
    return;

  BWAPI::TilePosition enemyPos = enemy->getTilePosition();
  BWAPI::TilePosition unitPos = unit->getTilePosition();
  int dx = unitPos.x - enemyPos.x;
  int dy = unitPos.y - enemyPos.y;
  int x = std::max(std::min(unitPos.x + dx * 5, Broodwar->mapWidth()), 0);
  int y = std::max(std::min(unitPos.y + dy * 5, Broodwar->mapHeight()), 0);
  TilePosition pos(x, y);
  unit->move(Position(pos));
  Broodwar->printf("Fleeing: (%d, %d) ", x, y);
}

void DesolatorModule::findEnemies(BWAPI::Unitset *enemies)
{
  /* Finds all visible enemies */
  BWAPI::Playerset players = Broodwar->getPlayers();

  for(BWAPI::Playerset::iterator player = players.begin(); player != players.end(); player++)
  {
    if(player->isEnemy(this->us))
    {
      BWAPI::Unitset units = player->getUnits();
      enemies->insert(units);
    }
  }
}

BWAPI::Unit * DesolatorModule::findClosestEnemy(BWAPI::Unit *unit)
{
  BWAPI::Unitset enemies = BWAPI::Unitset();
  this->findEnemies(&enemies);
  if(enemies.empty())
    return NULL;

  BWAPI::Unit *enemy = NULL;
  double shortestDistance = -1;
  for(BWAPI::Unitset::iterator eUnit = enemies.begin(); eUnit != enemies.end(); eUnit++)
  {    
    double distance = unit->getDistance(*eUnit);
    if(shortestDistance == -1 || distance < shortestDistance)
    {
      shortestDistance = distance;
      enemy = *eUnit;
    }  
  }
  return enemy;
}

void DesolatorModule::onSendText(std::string text)
{
  // Send the text to the game if it is not being processed.
  Broodwar->sendText("%s", text.c_str());
  this->evaluateText(text);
  // Make sure to use %s and pass the text as a parameter,
  // otherwise you may run into problems when you use the %(percent) character!
}

void DesolatorModule::onReceiveText(BWAPI::Player* player, std::string text)
{
  // Parse the received text
  Broodwar << player->getName() << " said \"" << text << "\"" << std::endl;
  this->evaluateText(text);
}

void DesolatorModule::evaluateText(std::string text)
{
  if(text == "r")
  {
    Broodwar << "Game paused for feedback.";
    this->feedback = true;
    return;
  }
  else if(this->feedback)
  {
    try
    {
      int reward = std::stoi(text);
      this->feedback = false;
      Broodwar->printf("Reward: %d", reward);
      Broodwar->sendText("cont");
      return;
    } catch (const std::invalid_argument& error)
    {
      Broodwar->sendText("Unable to parse text");
    }
  }
}

State DesolatorModule::getState(BWAPI::Unit *unit, const BWAPI::Unitset *alliedUnits, const BWAPI::Unitset *enemyUnits)
{
  /* Returns the current state of the unit */
  State state = State();

  // Update health
  state.health =  (unit->getHitPoints()           + unit->getShields() - 1      ) * 100;
  state.health /= (unit->getType().maxHitPoints() + unit->getType().maxShields()) * 25;
  
  // Update weapon cooldown
  state.weaponCooldown = unit->getGroundWeaponCooldown() != 0;

  // Update enemy heat map
  int realRange = getOptimizedWeaponRange(unit);
  BWAPI::Unitset &unitsInMyRange = unit->getUnitsInRadius(realRange);

  for (BWAPI::Unitset::iterator enemyUnit = enemyUnits->begin(); enemyUnit != enemyUnits->end(); enemyUnit++)
  {
    if (state.enemyHeatMap < 2 && unit == enemyUnit->getOrderTarget())
    {
      state.enemyHeatMap = 2;
    }
    else if(state.enemyHeatMap < 1)
    {
      int enemyRealRange = getOptimizedWeaponRange(*enemyUnit);
      BWAPI::Unitset &unitsInEnemyRange = enemyUnit->getUnitsInRadius(enemyRealRange);
      // Checks if we are in enemy optimized range
      if ( unitsInEnemyRange.find(unit) != unitsInEnemyRange.end() )
        state.enemyHeatMap = 1;
    }

    if ( !state.canTarget && unitsInMyRange.find(*enemyUnit) != unitsInMyRange.end() )
      state.canTarget = 1;
  }

  // Update friendly heat map
  for (BWAPI::Unitset::iterator alliedUnit = alliedUnits->begin(); alliedUnit != alliedUnits->end(); alliedUnit++)
  {
    if (*alliedUnit != unit) {
      int allyRealRange = getOptimizedWeaponRange(*alliedUnit);
      auto & unitsInFriendRange = alliedUnit->getUnitsInRadius(allyRealRange);
      if (unitsInFriendRange.find(unit) != unitsInFriendRange.end() ) {
        state.friendHeatMap = 1;
        break;
      }
    }
  }

  return state;
}


/* AUXILLARY FUNCTIONS */
// Utils functions
// The range actually takes into consideration unit size, so we try to do that
int getActualWeaponRange(BWAPI::Unit* unit) {
  return unit->getType().groundWeapon().maxRange() + std::max(unit->getType().dimensionLeft(),unit->getType().dimensionUp());
}

// An unit is considered melee if it can move more than it can shoot
bool isMelee(BWAPI::Unit*unit) {
  return static_cast<int>(unit->getType().topSpeed()) * STANDARD_SPEED_FPS > getActualWeaponRange(unit);
}

// This function returns a range differently if the unit is considered
// melee or ranged. If ranged the range is the normal range, if melee
// is the range + the movement it can do in 1 second
int getOptimizedWeaponRange(BWAPI::Unit* unit) {
  if ( isMelee(unit) )
    return getActualWeaponRange(unit) + static_cast<int>(unit->getType().topSpeed()) * STANDARD_SPEED_FPS;
  else
    return getActualWeaponRange(unit);
}

void drawHeatMap(BWAPI::Player *us, BWAPI::Player *enemy)
{
  // Load visible units
  auto & ourUnits = us->getUnits();
  auto & theirUnits = enemy->getUnits();
  // DRAW HEATMAPS
  {
    auto printBaseRange = [](decltype(ourUnits.begin()) unit, BWAPI::Color c){
      Broodwar->drawCircleMap(
          unit->getPosition(),
          getActualWeaponRange(*unit),
          c);
    };
    auto printLongRange = [](decltype(ourUnits.begin()) unit, BWAPI::Color c){
      Broodwar->drawCircleMap(
          unit->getPosition(),
          getOptimizedWeaponRange(*unit),
          c);
    };
    // Green
    BWAPI::Color c(0,255,0);
    for ( auto unit = ourUnits.begin(); unit != ourUnits.end(); unit++ ) {
      printLongRange(unit, c);
    }
    // Red
    c = BWAPI::Color(0,0,255);
    for ( auto unit = theirUnits.begin(); unit != theirUnits.end(); unit++ ) {
      printBaseRange(unit, c);
      printLongRange(unit, c);
    }
  }
}

void DesolatorModule::loadTable(const char * filename) {
  std::ifstream file(filename, std::ifstream::in);

  for ( size_t i = 0; i < State::statesNumber; i++ )
    for ( size_t j = 0; j < State::statesNumber; j++ )
      if ( !(file >> table[i][j]) ) throw std::runtime_error("Couldn't load selected file");;
  // Should we verify the data in some way?
  file.close();
}

void DesolatorModule::saveTable(const char * filename) {
  std::ofstream file(filename, std::ofstream::out);
  int counter = 0;
  for ( size_t i = 0; i < State::statesNumber; i++ ) {
    for ( size_t j = 0; j < State::statesNumber; j++ ) {
      file << table[i][j] << " ";
    }
    file << "\n";
  }

  file.close();
}

// #############################################
// ################ STATE ######################
// #############################################
void drawState(std::map<int, State> *states)
{
  /* Loops over the states mapping */
  int position = 20;
  for(std::map<int, State>::iterator i = states->begin(); i != states->end(); i++)
  {
    State state = i->second;
    Broodwar->drawTextScreen(5,
      position,
      "Id = %d | Health = %d | Cooldown = %d | EnemyHeat = %d | Covered = %d | canTarget = %d",
      i->first,
      state.health,
      state.weaponCooldown,
      state.enemyHeatMap,
      state.friendHeatMap,
      state.canTarget);
    position += 10;
  }
}







// #############################################
// ############# USELESS STUFF #################
// #############################################


void DesolatorModule::onPlayerLeft(BWAPI::Player* player)
{
  // Interact verbally with the other players in the game by
  // announcing that the other player has left.
  Broodwar->sendText("Goodbye %s!", player->getName().c_str());
}


void DesolatorModule::onSaveGame(std::string gameName) {
  Broodwar << "The game was saved to \"" << gameName << "\"" << std::endl;
}

void DesolatorModule::onNukeDetect(BWAPI::Position target){}
void DesolatorModule::onUnitDiscover(BWAPI::Unit* unit){}
void DesolatorModule::onUnitEvade(BWAPI::Unit* unit){}
void DesolatorModule::onUnitShow(BWAPI::Unit* unit){}
void DesolatorModule::onUnitHide(BWAPI::Unit* unit){}
void DesolatorModule::onUnitCreate(BWAPI::Unit* unit){}
void DesolatorModule::onUnitDestroy(BWAPI::Unit* unit){}
void DesolatorModule::onUnitMorph(BWAPI::Unit* unit){}
void DesolatorModule::onUnitRenegade(BWAPI::Unit* unit){}
void DesolatorModule::onUnitComplete(BWAPI::Unit *unit){}
void DesolatorModule::onEnd(bool isWinner) {}