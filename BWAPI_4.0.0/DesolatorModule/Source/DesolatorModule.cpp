#include "DesolatorModule.h"
#include <fstream>
#include <cmath>
#include <algorithm>

using namespace BWAPI;
using namespace Filter;

const int STANDARD_SPEED_FPS = 19;

void DesolatorModule::onStart()
{
  // COMMON VARIABLE INIT
  tableIsValid = false;

  /*** STANDARD IMPLEMENTATION ***/
  // Hello World!
  Broodwar->sendText("Desolator Module activated!");

  if ( !loadTable("transitions_numbers.data") )
    Broodwar->printf("###! COULD NOT LOAD TRANSITION NUMBERS !###");

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
    Broodwar->setLocalSpeed(50);

    BWAPI::Unitset myUnits = this->us->getUnits();
    BWAPI::Unitset enemyUnits = this->them->getUnits();

    // Initialize state and action for every unit
    for(BWAPI::Unitset::iterator u = myUnits.begin(); u != myUnits.end(); u++)
    {
      State state = this->getState(*u, myUnits, enemyUnits);
      this->states[u->getID()] = state;
      this->lastPositions[u->getID()] = u->getTilePosition();
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
  auto & myUnits = this->us->getUnits();
  auto & enemyUnits = this->them->getUnits();

  if(enemyUnits.empty())
  {
    // Explore together if no enemies are found and we are not moving.
    this->explore(myUnits);
  } else {
    for ( BWAPI::Unitset::iterator u = myUnits.begin(); u != myUnits.end(); ++u )
    {
      // Ignore the unit if it no longer exists
      // Make sure to include this block when handling any Unit pointer!
      if ( !u->exists() )
        continue;

      // Check when the units moved a tile
      BWAPI::TilePosition previous = this->lastPositions[u->getID()];
      BWAPI::TilePosition current = u->getTilePosition();

      bool moved = false;
      if ( this->lastPositions[u->getID()] != u->getTilePosition() ) {
        moved = true;
        this->lastPositions[u->getID()] = u->getTilePosition();
      }
 
      if ( !feedback ) {
          this->actions[u->getID()] = Flee;
          if ( this->states[u->getID()].enemyHeatMap == 2 ) {
            if ( moved || u->getOrder() != Orders::Move ) {
              u->move(flee(*u, myUnits, enemyUnits));
            }
          }
          else if ( u->getOrder() != Orders::AttackUnit ) {
            u->attack(*(enemyUnits.begin()));
          }
      }
    } // closure: unit iterator
  } // closure: else
}

void DesolatorModule::explore(const BWAPI::Unitset & units)
{
  /* Lets a group of units explore randomly */
  BWAPI::Unitset::iterator u = units.begin();

  if(!u->isMoving())
  {
    // Only move when idle.
    int x = rand() % (Broodwar->mapWidth());
    int y = rand() % (Broodwar->mapHeight());
    BWAPI::TilePosition pos(x, y);

    units.move(BWAPI::Position(pos));
    Broodwar->printf("Explore: (%d, %d)", x, y);
  }
}

void DesolatorModule::attack(BWAPI::Unit *unit, const BWAPI::Unitset & allies, const BWAPI::Unitset & enemies)
{
  /* This function implements the attack action */
  int realRange = getOptimizedWeaponRange(unit);
  BWAPI::Unitset &unitsInRange = unit->getUnitsInRadius(realRange);
  BWAPI::Unitset enemyUnitsInRange = BWAPI::Unitset();

  // Find enemies in range.
  for(BWAPI::Unitset::iterator u = unitsInRange.begin(); u != unitsInRange.end(); u++)
    if(enemies.exists(*u))
      enemyUnitsInRange.push_back(*u);

  if(enemyUnitsInRange.empty())
  {
    // No enemy in range move to closest ally that is targeted
    BWAPI::Unit *closestAlly = nullptr;
    for(BWAPI::Unitset::iterator ally = allies.begin(); ally != allies.end(); ally++)
    {
      State state = this->states[ally->getID()];
      if((closestAlly == nullptr || unit->getDistance(*ally) < unit->getDistance(closestAlly)) && unit->getID() != ally->getID() && state.enemyHeatMap >= 1)
      {
        closestAlly = *ally;
      }
    }

    if(closestAlly != nullptr)
    {
      // If we have a closest ally that is targeted move towards it.
      Broodwar->printf("Moving towards closest targeted ally.");
      unit->move(closestAlly->getPosition());
    } else {
      // If our allieds died or are not targeted kill closest enemy.
      Broodwar->printf("No allieds targeted, attacking closest enemy!");
      BWAPI::Unit *closestEnemy = nullptr;
      for(BWAPI::Unitset::iterator enemy = enemies.begin(); enemy != enemies.end(); enemy++)
      {
        if(closestEnemy == nullptr || unit->getDistance(*enemy) < unit->getDistance(closestEnemy))
          closestEnemy = *enemy;
      }
      if(closestEnemy != nullptr)
      {
        unit->attack(closestEnemy);
      } else {
        Broodwar->printf("ERROR: Something is wrong no enemy found in attack function");
      }
    }
    // If no closest ally
  } else {
    // Enemy in range
    Broodwar->printf("Attacking weakest unit in range");
    BWAPI::Unit *weakestEnemy = nullptr;
    for(BWAPI::Unitset::iterator enemy = enemyUnitsInRange.begin(); enemy != enemyUnitsInRange.end(); enemy++)
    {
      if(weakestEnemy == nullptr || enemy->getHitPoints() + enemy->getShields() < weakestEnemy->getHitPoints() + weakestEnemy->getShields())
      {
        weakestEnemy = *enemy;
      }
    }

    if(weakestEnemy != nullptr)
    {
      unit->attack(weakestEnemy);
    } else {
      Broodwar->printf("ERROR: Something went wrong no weakest unit found");
    }
  }
}

BWAPI::Position DesolatorModule::flee(BWAPI::Unit *unit, const BWAPI::Unitset & friends, const BWAPI::Unitset & enemies)
{
  double enemyForce = 5000.0;
  double enemyTargetedForce = 10000.0;
  // Friends attract
  double friendForceUncovered = -0.5; // This value is weird because it is not divided by distance
  double friendForceCovered = 3000.0;
  // Used to avoid infinite forces when units are near
  double minDistance = 40.0;

  Broodwar->drawCircleMap(unit->getPosition(), minDistance, BWAPI::Color(255,255,255));

  std::vector<std::pair<double, double>> fieldVectors;
  fieldVectors.reserve(friends.size() + enemies.size());

  auto unitPos = unit->getPosition();

  BWAPI::Unit* closestFriend = nullptr;
  for ( auto it = friends.begin(); it != friends.end(); it++ )
    if ( (closestFriend == nullptr || it->getDistance(unit) < closestFriend->getDistance(unit)) && (*it) != unit )
      closestFriend = *it;

  // We are alone, jump friend code
  if ( closestFriend != nullptr ) {
    auto friendPos = closestFriend->getPosition();
    // We are not covered
    if ( !states[unit->getID()].friendHeatMap ) {
      fieldVectors.emplace_back(std::make_pair(
        // They should go towards friends the more they are away
        (unitPos.x - friendPos.x)*friendForceUncovered,
        (unitPos.y - friendPos.y)*friendForceUncovered
      ));
    }
    else {
      auto distance = std::max(minDistance, unitPos.getDistance(friendPos));
      fieldVectors.emplace_back(std::make_pair(
        // They should go towards friends the more they are away
        (unitPos.x - friendPos.x)*friendForceCovered / (distance*distance),
        (unitPos.y - friendPos.y)*friendForceCovered / (distance*distance)
      ));
    }
  }
  else
    Broodwar->printf("No friends!");
  
  for ( auto it = enemies.begin(); it != enemies.end(); it++ ) {
    if ( *it != unit ) {
      auto enemyPos = (*it)->getPosition();
      auto distance = std::max(minDistance, unitPos.getDistance(enemyPos));
      auto force = (*it)->getOrderTarget() == unit ? enemyTargetedForce : enemyForce;

      fieldVectors.emplace_back(std::make_pair(
        (unitPos.x - enemyPos.x)*force / (distance*distance),
        (unitPos.y - enemyPos.y)*force / (distance*distance)
      ));
    }
  }

  // Create final vector
  std::pair<double,double> finalVector = std::make_pair(0.0, 0.0);
  for ( size_t i = 0; i < fieldVectors.size(); i++ ) {
    finalVector.first += fieldVectors[i].first;
    finalVector.second += fieldVectors[i].second;
  }

  BWAPI::Position placeIwouldLikeToGo = unit->getPosition();
  placeIwouldLikeToGo.x += static_cast<int>(finalVector.first);
  placeIwouldLikeToGo.y += static_cast<int>(finalVector.second);

  Broodwar->drawLineMap(unit->getPosition(), placeIwouldLikeToGo, BWAPI::Color(0,255,0));
  return placeIwouldLikeToGo;
}

State DesolatorModule::getState(BWAPI::Unit *unit, const BWAPI::Unitset & alliedUnits, const BWAPI::Unitset & enemyUnits)
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

  for (BWAPI::Unitset::iterator enemyUnit = enemyUnits.begin(); enemyUnit != enemyUnits.end(); enemyUnit++)
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
  for (BWAPI::Unitset::iterator alliedUnit = alliedUnits.begin(); alliedUnit != alliedUnits.end(); alliedUnit++)
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

// #############################################
// ############## TEXT FUNCTIONS ###############
// #############################################

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

// #############################################
// ########### AUXILLARY FUNCTIONS #############
// #############################################

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

// #############################################
// ############# TABLE FUNCTIONS ###############
// #############################################

void DesolatorModule::updateTable(BWAPI::Unit * u, const BWAPI::Unitset & myUnits, const BWAPI::Unitset & enemyUnits) {
    auto newState = this->getState(u, myUnits, enemyUnits);
   
    if ( this->actions[u->getID()] == Action::Attack ) 
      table[states[u->getID()]][newState].first++;
    else
      table[states[u->getID()]][newState].second++;

    this->states[u->getID()] = newState;
}

bool DesolatorModule::loadTable(const char * filename) {
  std::ifstream file(filename, std::ifstream::in);

  for ( size_t i = 0; i < State::statesNumber; i++ )
    for ( size_t j = 0; j < State::statesNumber; j++ )
      if ( !(file >> table[i][j].first >> table[i][j].second) ) {
        tableIsValid = false;
        return false;
      }
  // Should we verify the data in some way?
  file.close();
  return true;
}

bool DesolatorModule::saveTable(const char * filename) {
  if ( !tableIsValid ) return false;
  std::ofstream file(filename, std::ofstream::out);
  int counter = 0;
  for ( size_t i = 0; i < State::statesNumber; i++ ) {
    for ( size_t j = 0; j < State::statesNumber; j++ ) {
      file << table[i][j].first << " " << table[i][j].second << " ";
    }
    file << "\n";
  }

  file.close();
  return true;
}

// #############################################
// ########### DRAWING FUNCTIONS ###############
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
void DesolatorModule::onEnd(bool isWinner) {
  //saveTable("transitions_number.data");
}

