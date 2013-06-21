#include "DesolatorModule.h"
#include <cmath>
#include <algorithm>

using namespace BWAPI;
using namespace Filter;

const int STANDARD_SPEED_FPS = 19;

void DesolatorModule::onStart()
{
    log.open("log.txt");
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
    Broodwar->setLocalSpeed(0);

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
            GameState gameState;
            // Saved states
            gameState.lastHealth = u->getHitPoints() + u->getShields();
            gameState.lastPosition = u->getTilePosition();
            gameState.lastPerfectPosition = u->getPosition();
            gameState.notMovingTurns = 0;
            gameState.isStartingAttack = false;
            // Saved units
            gameState.lastTarget = nullptr;
            gameState.nearestAttackedAlly = nullptr;
            gameState.nearestAlly = nullptr;
            gameState.nearestEnemy = nullptr;
            // Commit
            gameStates[u->getID()] = gameState;
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

    auto & regions = Broodwar->getAllRegions();
    for ( auto it = regions.begin(); it != regions.end(); it++ ) {
        if ( !it->isAccessible() ){
            Broodwar->drawBoxMap(Position(it->getBoundsLeft(), it->getBoundsTop()), Position(it->getBoundsRight(), it->getBoundsBottom()), Color(255,255,255));
        }
    }

    drawHeatMap(this->us, this->them);
    drawState(&this->gameStates);

    auto & myUnits = this->us->getUnits();
    auto & enemyUnits = this->them->getUnits();

    for ( BWAPI::Unitset::iterator u = myUnits.begin(); u != myUnits.end(); ++u ) {
        auto p = u->getPosition(); p.y -= 30;
        Broodwar->drawTextMap(p, "%d", u->getID());
    }

    // Return if the game is a replay or is paused
    if ( Broodwar->isReplay() || Broodwar->isPaused() || !Broodwar->self() )
        return;

    // Prevent spamming by only running our onFrame once every number of latency frames.
    // Latency frames are the number of frames before commands are processed.
    if ( Broodwar->getFrameCount() % Broodwar->getLatencyFrames() != 0 )
        return;

    /*** DESOLATOR IMPLEMENTATION ***/
    if(enemyUnits.empty())
    {
        if ( ! feedback )
            // Explore together if no enemies are found and we are not moving.
            this->explore(myUnits);
    } else {
        for ( BWAPI::Unitset::iterator u = myUnits.begin(); u != myUnits.end(); ++u )
            updateGameState(*u, myUnits, enemyUnits);

        for ( BWAPI::Unitset::iterator u = myUnits.begin(); u != myUnits.end(); ++u )
            flee(*u, myUnits, enemyUnits);

        for ( BWAPI::Unitset::iterator u = myUnits.begin(); u != myUnits.end(); ++u )
        {
            auto & GS = gameStates[u->getID()];

            // Check when the units moved a tile
            bool moved = false;
            {
                BWAPI::TilePosition & previous = GS.lastPosition;
                BWAPI::TilePosition current = u->getTilePosition();

                if ( previous != current ) {
                    moved = true;
                    previous = current;
                }
            }
            // Hack for avoiding units stopping while actually moving..
            if ( !moved && GS.actualAction == ActualAction::Move && GS.lastPerfectPosition == u->getPosition() )
                GS.notMovingTurns++;
            else {
                GS.lastPerfectPosition = u->getPosition();
                GS.notMovingTurns = 0;
            }

            // If no feedback, we can act
            if ( !feedback ) {
                // If our personal tick ended
                // FIXME: Sometimes it does not move..
                if ( u->isIdle() || moved || 
                    ( GS.actualAction == ActualAction::Shoot && ! GS.isStartingAttack &&  ! u->isAttackFrame() ) ) {

                    updateGameState(*u, myUnits, enemyUnits, true);

                    Action a;
                    ActualAction actual;

                    //int random = rand() % 2;

                    // Game logic....
                    
                    if ( gameStates[u->getID()].state.enemyHeatMap == 2 ) {
                    //if ( random ) {
                        auto position = flee(*u, myUnits, enemyUnits);
                        if ( convertToTile(position) != u->getTilePosition() )
                            if ( ! u->move(position) ) Broodwar->printf("CANT FLEE!!!!!!!!");
                        
                        a = Action::Flee;
                        actual = ActualAction::Move;

                        // DEBUG
                        Broodwar->printf("%d - Flee", u->getID());
                        log << u->getID() << " - Flee\n";
                    }
                    else {
                        auto target = attack(*u, myUnits, enemyUnits);

                        actual = ActualAction::Shoot;
                        a = Action::Attack;

                        if ( target.isPosition() ) {
                            if ( convertToTile(target.getPosition()) != u->getTilePosition() )
                                if ( ! u->move(target.getPosition())) Broodwar->printf("CANT MOVE TO ATTACK!!!!!!!!");

                            actual = ActualAction::Move;

                            // DEBUG
                            Broodwar->printf("%d - Moving to Attack", u->getID());
                            log << u->getID() << " - Moving to Attack\n";
                            if ( convertToTile(target.getPosition()) == u->getTilePosition() ) log << "#### Moving to same spot...\n";
                        }
                        // This check is to avoid breaking Starcraft when we spam attack coomand
                        else if ( u->getOrder() != Orders::AttackUnit || gameStates[u->getID()].lastTarget != target.getUnit() ) {
                            if ( ! u->attack(target.getUnit())) Broodwar->printf("CANT ATTACK!!!!!!!!");

                            gameStates[u->getID()].isStartingAttack = true;
                            gameStates[u->getID()].lastTarget = target.getUnit();

                            // DEBUG
                            Broodwar->printf("%d - Attacking Target", u->getID());
                            log << u->getID() << " Attacking Target\n";
                        }
                    }
                    // Update Action taken
                    gameStates[u->getID()].lastAction = a;
                    gameStates[u->getID()].actualAction = actual;

                } // End, personal tick
                // End of hack, here we stop the units so I guess internally it resets so we can move it again.
                else if ( GS.notMovingTurns == 3 && ( log << "#### USING TRICK TO CONTINUE\n" || true  ) )
                    u->stop();
            } // End, feedback
            else if ( u->isAttacking() )
                u->stop();
        } // closure: unit iterator
    } // closure: We have enemies
}

void DesolatorModule::explore(const BWAPI::Unitset & units)
{
    /* Lets a group of units explore randomly */
    BWAPI::Unitset::iterator u = units.begin();

    if(!u->isMoving())
    {
        // Only move when idle.
        int x = ( rand() % (Broodwar->mapWidth()) ) * 32;
        int y = ( rand() % (Broodwar->mapHeight()) ) * 32;
        BWAPI::Position pos(x, y);

        units.move(pos);

        Broodwar->printf("Explore: (%d, %d)", x, y);
    }
}

BWAPI::PositionOrUnit DesolatorModule::attack(BWAPI::Unit *unit, const BWAPI::Unitset & allies, const BWAPI::Unitset & enemies)
{
    auto & GS = gameStates[unit->getID()];


    if ( !GS.state.canTarget ) {
        // No enemy in range move to closest ally that is targeted
        if ( GS.nearestAttackedAlly != nullptr )
        {
            Broodwar->printf("There is an attacked ally");
            // If we have a closest ally that is targeted move towards it.
            return GS.nearestAttackedAlly->getPosition();
        } else {
            // If our allieds died or are not targeted kill closest enemy.
            if ( GS.nearestEnemy != nullptr )
                return GS.nearestEnemy;
            else {
                Broodwar->printf("ERROR: No enemy to attack in attack function");
                log << "ERROR: No enemy in range with canTarget true\n";
                return unit->getPosition();
            }
        }
    } else {  
        BWAPI::Unit *weakestEnemy = nullptr;
        BWAPI::Unitset unitsInRange = unit->getUnitsInRadius(unit->getType().groundWeapon().maxRange());
     
        // Enemy in range
        for ( BWAPI::Unitset::iterator it = unitsInRange.begin(); it != unitsInRange.end(); it++ )
        {
            if ( enemies.exists(*it) && ( weakestEnemy == nullptr || ( it->getHitPoints() + it->getShields() < weakestEnemy->getHitPoints() + weakestEnemy->getShields() )) )
            {
                weakestEnemy = *it;
            }
        }
     
        if ( weakestEnemy != nullptr )
            return weakestEnemy;
        else {
            Broodwar->printf("ERROR: No enemy in range with canTarget true");
            log << "ERROR: No enemy in range with canTarget true\n";
            return GS.nearestEnemy;
        }
    }
}

BWAPI::Position DesolatorModule::flee(BWAPI::Unit *unit, const BWAPI::Unitset & friends, const BWAPI::Unitset & enemies)
{
    auto & GS = gameStates[unit->getID()];

    double enemyForce = 5000.0;
    double enemyTargetedForce = 10000.0;
    // Friends attract
    double friendForceUncovered = -0.5; // This value is weird because it is not divided by distance
    double friendForceCovered = 3000.0;
    // Used to avoid infinite forces when units are near
    double minDistance = 40.0;

    std::vector<std::pair<double, double>> fieldVectors;
    fieldVectors.reserve(friends.size() + enemies.size());

    auto unitPos = unit->getPosition();

    // If we are alone, jump friend code
    if ( GS.nearestAlly != nullptr ) {
        auto friendPos = GS.nearestAlly->getPosition();
        // We are not covered
        if ( !gameStates[unit->getID()].state.friendHeatMap ) {
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

    // Enemies code
    for ( auto it = enemies.begin(); it != enemies.end(); it++ ) {
        if ( *it != unit ) {
            auto enemyPos = it->getPosition();
            auto distance = std::max(minDistance, unitPos.getDistance(enemyPos));
            auto force = it->getOrderTarget() == unit ? enemyTargetedForce : enemyForce;

            fieldVectors.emplace_back(std::make_pair(
                (unitPos.x - enemyPos.x)*force / (distance*distance),
                (unitPos.y - enemyPos.y)*force / (distance*distance)
                ));
        }
    }

    // Get repulsed by inaccessible regions
    auto & regions = Broodwar->getAllRegions();
    for ( auto it = regions.begin(); it != regions.end(); it++ ) {
        if ( !it->isAccessible() ){
            auto regionPos = it->getCenter();
            auto distance = std::max(minDistance, regionPos.getDistance(unit->getPosition()));

            fieldVectors.emplace_back(std::make_pair(
                (unitPos.x - regionPos.x)*enemyForce / (distance*distance),
                (unitPos.y - regionPos.y)*enemyForce / (distance*distance)
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

    // Now we check that were we want to go is in map boundaries, otherwise we fix it ( not perfect but meh )
    if ( placeIwouldLikeToGo.x < 0 ) placeIwouldLikeToGo.x = 0;
    else if ( placeIwouldLikeToGo.x > Broodwar->mapWidth() * 32 ) placeIwouldLikeToGo.x = Broodwar->mapWidth() * 32;

    if ( placeIwouldLikeToGo.y < 0 ) placeIwouldLikeToGo.y = 0;
    else if ( placeIwouldLikeToGo.y > Broodwar->mapHeight() * 32 ) placeIwouldLikeToGo.y = Broodwar->mapHeight() * 32;

    Broodwar->drawLineMap(unit->getPosition(), placeIwouldLikeToGo, BWAPI::Color(0,255,0));

    return placeIwouldLikeToGo;
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
    else if ( text == "q" ) {
        Broodwar->leaveGame();
    }
    else if ( text == "d" ) {
        Broodwar->restartGame();
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

BWAPI::TilePosition convertToTile(BWAPI::Position point) {
    return TilePosition( Position(abs(point.x - 32 / 2),
                                  abs(point.y - 32 / 2)) );
  }

// #############################################
// ############# TABLE FUNCTIONS ###############
// #############################################

void DesolatorModule::updateTable(State before, Action a, State after, double reward) {
    if ( a == Action::Attack ) {
        std::get<0>(table[before][after])++;
        std::get<2>(table[before][after]) += reward;
    }
    else {
        std::get<1>(table[before][after])++;
        std::get<3>(table[before][after]) += reward;
    }
}
bool DesolatorModule::loadTable(const char * filename) {
    std::ifstream file(filename, std::ifstream::in);

    for ( size_t i = 0; i < State::statesNumber; i++ )
        for ( size_t j = 0; j < State::statesNumber; j++ )
            if ( !(file >> std::get<0>(table[i][j]) >> std::get<1>(table[i][j]) >> std::get<2>(table[i][j]) >> std::get<3>(table[i][j]) ) ) {
                tableIsValid = false;
                return false;
            }
    // Should we verify the data in some way?
    file.close();
    return true;
}

bool DesolatorModule::saveTable(const char * filename) {
    //if ( !tableIsValid ) return false;
    std::ofstream file(filename, std::ofstream::out);
    int counter = 0;
    for ( size_t i = 0; i < State::statesNumber; i++ ) {
        for ( size_t j = 0; j < State::statesNumber; j++ ) {
            file << std::get<0>(table[i][j]) << " " << std::get<1>(table[i][j]) << " " << std::get<2>(table[i][j]) << " " << std::get<3>(table[i][j]) << " ";
        }
        file << "\n";
    }

    file.close();
    return true;
}

// #############################################
// ########### DRAWING FUNCTIONS ###############
// #############################################
void drawState(std::map<int, GameState> *states)
{
    /* Loops over the states mapping */
    int position = 20;
    for(std::map<int, GameState>::iterator i = states->begin(); i != states->end(); i++)
    {
        State state = i->second.state;
        Broodwar->drawTextScreen(5,
            position,
            "Id = %d | Health = %d | Cooldown = %d | EnemyHeat = %d | Covered = %d | canTarget = %d | isStartingAttack = %d |",
            i->first,
            state.health,
            state.weaponCooldown,
            state.enemyHeatMap,
            state.friendHeatMap,
            state.canTarget,
            i->second.isStartingAttack );
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
void DesolatorModule::onUnitMorph(BWAPI::Unit* unit){}
void DesolatorModule::onUnitRenegade(BWAPI::Unit* unit){}
void DesolatorModule::onUnitComplete(BWAPI::Unit *unit){}
void DesolatorModule::onEnd(bool isWinner) {
    saveTable("transitions_numbers.data");
    log.close();
}

void DesolatorModule::onUnitDestroy(BWAPI::Unit* unit) {
    if(unit->getPlayer() == this->us) {
        auto GS = this->gameStates[unit->getID()];
        int health =  unit->getType().maxShields() + unit->getType().maxHitPoints();
        double reward = - ( static_cast<double>(health)/ 4 );
        updateTable(GS.state, GS.lastAction, GS.state, reward);
    }
}

void DesolatorModule::updateGameState(BWAPI::Unit *unit, const BWAPI::Unitset & alliedUnits, const BWAPI::Unitset & enemyUnits, bool alsoState) {
    auto & oldGameState = gameStates[unit->getID()];
    State newState;

    // STATE INITIAL STUFF
    if ( alsoState ) {
        // Update health
        newState.health =  (unit->getHitPoints()           + unit->getShields() - 1      ) * 100;
        newState.health /= (unit->getType().maxHitPoints() + unit->getType().maxShields()) * 25;

        // Update weapon cooldown
        newState.weaponCooldown = unit->getGroundWeaponCooldown() != 0;
    }
    // We can't know the Action yet
    // oldGameState.lastAction = CAN'T FILL

    // Is the attack tick done?
    if ( oldGameState.isStartingAttack && unit->isAttackFrame() ) {
        oldGameState.isStartingAttack = false;
    }

    // Last Position - WE DON'T HAVE TO DO THIS
    // oldGameState.lastPosition = unit->getTilePosition();

    // Nearest Attacker
    {
        BWAPI::Unit * nearestEnemy = nullptr;
        for ( auto it = enemyUnits.begin(); it != enemyUnits.end(); it++ ) {
            // Find nearest Attacker
            if ( nearestEnemy == nullptr || nearestEnemy->getDistance(unit) > it->getDistance(unit) )
                nearestEnemy = *it;

            if ( alsoState ) {
                // Get Enemy heatmap for this unit
                if ( newState.enemyHeatMap < 2 && unit == it->getOrderTarget() )
                    newState.enemyHeatMap = 2;
                else if( newState.enemyHeatMap < 1 ) {
                    int enemyRealRange = getOptimizedWeaponRange(*it);
                    BWAPI::Unitset &unitsInEnemyRange = it->getUnitsInRadius(enemyRealRange);
                    // Checks if we are in enemy optimized range
                    if ( unitsInEnemyRange.find(unit) != unitsInEnemyRange.end() )
                        newState.enemyHeatMap = 1;
                }

                if ( !newState.canTarget && unit->isInWeaponRange(nearestEnemy) )
                    newState.canTarget = 1;
            }
        }
        oldGameState.nearestEnemy = nearestEnemy;
    }

    // Nearest Attacked Ally
    {
    BWAPI::Unit * nearestAttackedAlly = nullptr, * nearestAlly = nullptr;
        for ( auto it = alliedUnits.begin(); it != alliedUnits.end(); it++ ) {
            if ( *it == unit ) continue;

            // Find nearest Ally
            if ( nearestAlly == nullptr || nearestAlly->getDistance(unit) > it->getDistance(unit) )
                nearestAlly = *it;

            // Find nearest Attacked Ally
            if ( gameStates[it->getID()].state.enemyHeatMap == 2 && ( nearestAttackedAlly == nullptr || nearestAttackedAlly->getDistance(unit) > it->getDistance(unit) ) )
                nearestAttackedAlly = *it;

            if ( alsoState && !newState.friendHeatMap ) {
                int allyRealRange = getOptimizedWeaponRange(*it);
                auto & unitsInFriendRange = it->getUnitsInRadius(allyRealRange);
                
                if (unitsInFriendRange.find(unit) != unitsInFriendRange.end() ) {
                    newState.friendHeatMap = 1;
                    continue;
                }
            }
        }
        oldGameState.nearestAttackedAlly = nearestAttackedAlly;
        oldGameState.nearestAlly = nearestAlly;
    }

    // State and Table updating
    if ( alsoState ) {
        int currentHealth = unit->getHitPoints() + unit->getShields();
        double reward = currentHealth - oldGameState.lastHealth;
        if(oldGameState.actualAction == ActualAction::Shoot && ! oldGameState.isStartingAttack &&  ! unit->isAttackFrame())
        {
            reward += unit->getType().groundWeapon().damageAmount();
        }
//        if(currentHealth - oldGameState.lastHealth < 0)
//            Broodwar->printf("ID: %d Reward: %f lastHealth: %d currentHealth: %d", unit->getID(), reward, oldGameState.lastHealth, currentHealth);

        updateTable(oldGameState.state, oldGameState.lastAction, newState, reward);
        oldGameState.lastHealth = currentHealth;
        oldGameState.state = newState;
    }
}
