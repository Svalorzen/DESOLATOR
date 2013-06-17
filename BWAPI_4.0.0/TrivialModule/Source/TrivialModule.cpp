#include "TrivialModule.h"

using namespace BWAPI;
using namespace Filter;

void TrivialModule::onStart()
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
    /*** TRIVIAL IMPLEMENTATION ***/
    this->us = Broodwar->self();
    this->feedback = false;
  }
}

void TrivialModule::onEnd(bool isWinner)
{
  // Called when the game ends
  if ( isWinner )
  {
    // Log your win here!
  }
}

void TrivialModule::onFrame()
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

  /*** TRIVIAL IMPLEMENTATION ***/
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
}

void TrivialModule::onSendText(std::string text)
{
  // Send the text to the game if it is not being processed.
  Broodwar->sendText("%s", text.c_str());
  this->evaluateText(text);
  // Make sure to use %s and pass the text as a parameter,
  // otherwise you may run into problems when you use the %(percent) character!
}

void TrivialModule::onReceiveText(BWAPI::Player* player, std::string text)
{
  // Parse the received text
  Broodwar << player->getName() << " said \"" << text << "\"" << std::endl;
  this->evaluateText(text);
}

void TrivialModule::evaluateText(std::string text)
{
  if(text == "r")
  {
    Broodwar << "Game paused for feedback.";
    this->feedback = true;
    return;
  }
  else if(text == "cont")
  {
    this->feedback = false;
  }
}

void TrivialModule::onPlayerLeft(BWAPI::Player* player)
{
  // Interact verbally with the other players in the game by
  // announcing that the other player has left.
  Broodwar->sendText("Goodbye %s!", player->getName().c_str());
}

void TrivialModule::onNukeDetect(BWAPI::Position target)
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

void TrivialModule::onUnitDiscover(BWAPI::Unit* unit)
{
}

void TrivialModule::onUnitEvade(BWAPI::Unit* unit)
{
}

void TrivialModule::onUnitShow(BWAPI::Unit* unit)
{
}

void TrivialModule::onUnitHide(BWAPI::Unit* unit)
{
}

void TrivialModule::onUnitCreate(BWAPI::Unit* unit)
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

void TrivialModule::onUnitDestroy(BWAPI::Unit* unit)
{
}

void TrivialModule::onUnitMorph(BWAPI::Unit* unit)
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

void TrivialModule::onUnitRenegade(BWAPI::Unit* unit)
{
}

void TrivialModule::onSaveGame(std::string gameName)
{
  Broodwar << "The game was saved to \"" << gameName << "\"" << std::endl;
}

void TrivialModule::onUnitComplete(BWAPI::Unit *unit)
{
}
