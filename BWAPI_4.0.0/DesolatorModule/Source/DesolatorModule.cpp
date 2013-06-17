#include "DesolatorModule.h"
#include <iostream>
#include <cmath>
#include <algorithm>

using namespace BWAPI;
using namespace Filter;

void DesolatorModule::onStart()
{
  /*** STANDARD IMPLEMENTATION ***/
  // Hello World!
  Broodwar->sendText("Desolator Module activated!");

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
    BWAPI::Unitset myUnits = this->us->getUnits();

    // Initialize state and action
    for(BWAPI::Unitset::iterator u = myUnits.begin(); u != myUnits.end(); u++)
    {
      Action action = Init;
      State state = State();
      state.health = u->getHitPoints() + u->getShields();
      state.underAttack = u->isUnderAttack();
      state.distanceToClosestEnemy = u->getDistance(this->findClosestEnemy(*u));
      
      this->actions[u->getID()] = action;
      this->states[u->getID()] = state;
    }
    this->feedback = false;
  }
}

void DesolatorModule::onEnd(bool isWinner)
{
  // Called when the game ends
  if ( isWinner )
  {
    // Log your win here!
  }
}

void DesolatorModule::onFrame()
{
  /*** STANDARD IMPLEMENTATION ***/
  // Display the game frame rate as text in the upper left area of the screen
  Broodwar->drawTextScreen(5, 0,  "FPS: %d", Broodwar->getFPS() );
  Broodwar->drawTextScreen(5, 10, "Average FPS: %f", Broodwar->getAverageFPS() );

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

  Unitset myUnits = this->us->getUnits();
  /* Switched off for testing
  int i = 0;
  for(Unitset::iterator u = myUnits.begin(); u != myUnits.end(); ++u)
  {
    // Update observation
    this->observations[i] = Observation();
    this->observations[i].previousAction = this->actions[i];
    this->observations[i].previousState = this->states[i];

    // Observe new state
    State state = State();
    state.health = u->getHitPoints() + u->getShields();
    state.underAttack = u->isUnderAttack();
    state.distanceToClosestEnemy = this->findClosestEnemy(*u);
    this->states[i] = state;
    Broodwar->printf("State = {Health: %d, Under attack: %d, Closest enemy: %f}",
      state.health,
      state.underAttack,
      state.distanceToClosestEnemy);
  }
  */

  for ( Unitset::iterator u = myUnits.begin(); u != myUnits.end(); ++u )
  {
    // Ignore the unit if it no longer exists
    // Make sure to include this block when handling any Unit pointer!
    if ( !u->exists() )
      continue;

    BWAPI::Unit *enemy = this->findClosestEnemy(*u);
    if(enemy == NULL)
    {
      if(!u->isMoving())
      {
        this->explore(*u);
        this->actions[u->getID()] = Explore;
      }
    } else {
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
    }
  } // closure: unit iterator
}

void DesolatorModule::explore(BWAPI::Unit *unit)
{
  /* Simple explore module move randomly to explore the map */
  int x = rand() % (Broodwar->mapWidth());
  int y = rand() % (Broodwar->mapHeight());

  TilePosition pos(x, y);
  unit->move(Position(pos));
  Broodwar->printf("Exploring: (%d, %d) ", x, y);
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

void DesolatorModule::onPlayerLeft(BWAPI::Player* player)
{
  // Interact verbally with the other players in the game by
  // announcing that the other player has left.
  Broodwar->sendText("Goodbye %s!", player->getName().c_str());
}

void DesolatorModule::onNukeDetect(BWAPI::Position target)
{

  // Check if the target is a valid position
  if ( target )
  {
    // if so, print the location of the nuclear strike target
    Broodwar << "Nuclear Launch Detected at " << target << std::endl;
  }
  else 
  {
    // Otherwise, ask other players where the nuke is!
    Broodwar->sendText("Where's the nuke?");
  }

  // You can also retrieve all the nuclear missile targets using Broodwar->getNukeDots()!
}

void DesolatorModule::onUnitDiscover(BWAPI::Unit* unit)
{
}

void DesolatorModule::onUnitEvade(BWAPI::Unit* unit)
{
}

void DesolatorModule::onUnitShow(BWAPI::Unit* unit)
{
}

void DesolatorModule::onUnitHide(BWAPI::Unit* unit)
{
}

void DesolatorModule::onUnitCreate(BWAPI::Unit* unit)
{
  if ( Broodwar->isReplay() )
  {
    // if we are in a replay, then we will print out the build order of the structures
    if ( unit->getType().isBuilding() && !unit->getPlayer()->isNeutral() )
    {
      int seconds = Broodwar->getFrameCount()/24;
      int minutes = seconds/60;
      seconds %= 60;
      Broodwar->sendText("%.2d:%.2d: %s creates a %s", minutes, seconds, unit->getPlayer()->getName().c_str(), unit->getType().c_str());
    }
  }
}

void DesolatorModule::onUnitDestroy(BWAPI::Unit* unit)
{
}

void DesolatorModule::onUnitMorph(BWAPI::Unit* unit)
{
  if ( Broodwar->isReplay() )
  {
    // if we are in a replay, then we will print out the build order of the structures
    if ( unit->getType().isBuilding() && !unit->getPlayer()->isNeutral() )
    {
      int seconds = Broodwar->getFrameCount()/24;
      int minutes = seconds/60;
      seconds %= 60;
      Broodwar->sendText("%.2d:%.2d: %s morphs a %s", minutes, seconds, unit->getPlayer()->getName().c_str(), unit->getType().c_str());
    }
  }
}

void DesolatorModule::onUnitRenegade(BWAPI::Unit* unit)
{
}

void DesolatorModule::onSaveGame(std::string gameName)
{
  Broodwar << "The game was saved to \"" << gameName << "\"" << std::endl;
}

void DesolatorModule::onUnitComplete(BWAPI::Unit *unit)
{
}
