#include "DesolatorModule.h"
#include <cmath>
#include <algorithm>
#include <ctime>

#include "helpers.h"
#include "AI.h"

using namespace BWAPI;
using namespace Filter;

DesolatorModule::DesolatorModule() : log_("desolator.log", std::fstream::out | std::fstream::app) {}

void DesolatorModule::onStart() {
    // Init random number generator
    srand((unsigned)time(NULL));

    // Hello World!
    Broodwar->sendText("Desolator Module activated!");

    if ( !table_.load("transitions_numbers.data") )
        Broodwar->printf("###! COULD NOT LOAD TRANSITION NUMBERS !###");

    if ( !policy_.load("policy.data"))
        Broodwar->printf("###! COULD NOT LOAD POLICY !###");

    // Enable the UserInput flag, which allows us to control the bot and type messages.
    Broodwar->enableFlag(Flag::UserInput);

    // Set the command optimization level so that common commands can be grouped
    // and reduce the bot's APM (Actions Per Minute).
    Broodwar->setCommandOptimizationLevel(2);

	Broodwar->setLocalSpeed(50);

    // We setup the environment only if this is not a replay
    if ( !Broodwar->isReplay() ) {
        if ( Broodwar->enemy() ) // First make sure there is an enemy
            Broodwar << "The matchup is " << Broodwar->self()->getRace() << " vs " << Broodwar->enemy()->getRace() << std::endl;

        /*** DESOLATOR IMPLEMENTATION ***/
        this->us = Broodwar->self();
        this->them = Broodwar->enemy();

        auto & myUnits = us->getUnits();
        auto & enemyUnits = them->getUnits();

        // Initialize state and action for every unit
        for(BWAPI::Unitset::iterator u = myUnits.begin(); u != myUnits.end(); u++)
        {
            UnitState uState(*u);
            unitStates[u->getID()] = uState;
        }

        this->feedback = false;
    }
}

void DesolatorModule::onFrame() {
    auto & myUnits = us->getUnits();
    auto & enemyUnits = them->getUnits();

	/************************/
    /***** DRAWING CODE *****/
	/************************/

    // Display the game frame rate as text in the upper left area of the screen
    // Broodwar->drawTextScreen(5, 0,  "FPS: %d", Broodwar->getFPS() );
    // Broodwar->drawTextScreen(5, 10, "Average FPS: %f", Broodwar->getAverageFPS() );

    //auto & regions = Broodwar->getAllRegions();
    //for ( auto it = regions.begin(); it != regions.end(); it++ ) {
    ///    if ( !it->isAccessible() ){
    //        Broodwar->drawBoxMap(Position(it->getBoundsLeft(), it->getBoundsTop()), Position(it->getBoundsRight(), it->getBoundsBottom()), Color(255,255,255));
    //    }
    //}

    //drawHeatMap(this->us, this->them);
    //drawState(&this->unitStates);

    // Draws IDs on top of units
    for ( BWAPI::Unitset::iterator u = myUnits.begin(); u != myUnits.end(); ++u ) {
        auto p = u->getPosition(); p.y -= 30;
        Broodwar->drawTextMap(p, "%d", u->getID());
    }

	/************************/
	/*** SKIP FRAMES CODE ***/
	/************************/

    // Return if the game is a replay or is paused
    if ( Broodwar->isReplay() || Broodwar->isPaused() || !Broodwar->self() )
        return;

    // Prevent spamming by only running our onFrame once every number of latency frames.
    // Latency frames are the number of frames before commands are processed.
    if ( Broodwar->getFrameCount() % Broodwar->getLatencyFrames() != 0 )
        return;

	/************************/
    /**** DESOLATOR CODE ****/
	/************************/

	if ( enemyUnits.empty() ) {
		// Only move when idle.
		if(!myUnits.begin()->isMoving()) {
			myUnits.move(AI::explore(Broodwar->mapHeight(), Broodwar->mapWidth()));
		}
    } 
    else {
		// Broodwar->setLocalSpeed(150);

        // This updates the game state for each unit we have ( not the MDP state though )
        for ( BWAPI::Unitset::iterator u = myUnits.begin(); u != myUnits.end(); ++u )
            updateUnitState(*u, myUnits, enemyUnits);

        // Now that the observations of the units are correct, we select the actions that we want the units to perform.
        for ( BWAPI::Unitset::iterator u = myUnits.begin(); u != myUnits.end(); ++u ) {
            auto & GS = unitStates[u->getID()];

            // Check when the units moved a tile
            bool moved = false;
            {
                BWAPI::TilePosition currentPosition = u->getTilePosition();

                if ( GS.lastPosition != currentPosition ) {
                    moved = true;
                    GS.lastPosition = currentPosition;
                }
            }

            // Hack for avoiding units stopping while actually moving..
            if ( !moved && GS.actualAction == ActualAction::Move && GS.lastPerfectPosition == u->getPosition() )
                GS.notMovingTurns++;
            else {
                GS.lastPerfectPosition = u->getPosition();
                GS.notMovingTurns = 0;
            }

            // If our personal tick ended
            // FIXME: Sometimes it does not move..
            if ( u->isIdle() || moved || 
               ( GS.actualAction == ActualAction::Shoot && ! GS.isStartingAttack &&  ! u->isAttackFrame() ) ) {

                updateUnitState(*u, myUnits, enemyUnits, true);

                Action a;
                ActualAction actual;

                int selAction = 0;

                // We check if we follow the policy or we go full random
                if ( policy_.isValid() ) selAction = policy_[GS.state];
                else selAction = rand() % 2;
                
                // We flee
                if ( selAction == 1 ) {
                    auto position = AI::flee(*u, myUnits, enemyUnits, unitStates);

                    if ( convertToTile(position) != u->getTilePosition() ) {
                        u->move(position);
                        Broodwar->printf( "Unit %d has been ordered to move", u->getID() );
                    } 
                    
                    a = Action::Flee;
                    actual = ActualAction::Move;
                }
                // We attack
                else {
                    auto target = AI::attack(*u, myUnits, enemyUnits, unitStates);

                    actual = ActualAction::Shoot;
                    a = Action::Attack;

                    if ( target.isPosition() ) {
                        if ( convertToTile(target.getPosition()) != u->getTilePosition() ) {
                             u->move(target.getPosition());
                        }

                        actual = ActualAction::Move;
                    }
                    // This check is to avoid breaking Starcraft when we spam attack command
                    else if ( u->getOrder() != Orders::AttackUnit || unitStates[u->getID()].lastTarget != target.getUnit() ) {
                        u->attack(target.getUnit());

                        unitStates[u->getID()].isStartingAttack = true;
                        unitStates[u->getID()].lastTarget = target.getUnit();
                    }
                }
                // Update Action taken
                unitStates[u->getID()].lastAction = a;
                unitStates[u->getID()].actualAction = actual;

            } // End, personal tick
            // End of hack, here we stop the units so I guess internally it resets so we can move it again.
            else if ( GS.notMovingTurns == 3 ) {
                log_("HACK TRIGGERED");
				log_(GS.actualAction, " ", GS.lastAction);
				log_(u->getOrder(), " ", u->getPosition(), " ", u->getTargetPosition());
				log_("Diff = ", u->getPosition() - u->getTargetPosition());
				log_("Unit Tile: ", convertToTile(u->getPosition()), " ; Target Tile: ", convertToTile(u->getTargetPosition()));
                u->stop();
            }
        } // closure: unit iterator
    } // closure: We have enemies
}

void DesolatorModule::onEnd(bool isWinner) {
    if ( !table_.save("transitions_numbers.data") )
        log_("NOT FUCKING SAVED");
    Broodwar->restartGame();
}

void DesolatorModule::onUnitDestroy(BWAPI::Unit unit) {
    if( unit->getPlayer() == this->us ) {
        auto GS = unitStates[unit->getID()];
        int health =  unit->getType().maxShields() + unit->getType().maxHitPoints();
        double reward = - ( static_cast<double>(health)/2 );
        table_.update(GS.state, GS.lastAction, GS.state, reward);
    }
}

/* This function updates the current game situation for a particular unit. Additionally, if requested through the "alsoState" parameter,
   it updates the MDP state of the unit. This second update should only happen at each unit "tick", while the rest of the state should be
   updated as often as possible. */
void DesolatorModule::updateUnitState(BWAPI::Unit unit, const BWAPI::Unitset & alliedUnits, const BWAPI::Unitset & enemyUnits, bool alsoState) {
    auto & oldGameState = unitStates[unit->getID()];
    MDPState newState;

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
        oldGameState.shooted = true;
    }

    // Last Position - WE DON'T HAVE TO DO THIS
    // oldGameState.lastPosition = unit->getTilePosition();

    // Nearest Enemy/Attacker
    {
        BWAPI::Unit nearestEnemy = nullptr, nearestAttacker = nullptr;
        for ( auto it = enemyUnits.begin(); it != enemyUnits.end(); it++ ) {
            // Find nearest Enemy
            if ( nearestEnemy == nullptr || nearestEnemy->getDistance(unit) > it->getDistance(unit) )
                nearestEnemy = *it;

            if ( alsoState ) {
                // Get Enemy heatmap for this unit
                if ( newState.enemyHeatMap < 2 && unit == it->getOrderTarget() ) {
                    newState.enemyHeatMap = 2;
                    if ( nearestAttacker == nullptr || nearestAttacker->getDistance(unit) > it->getDistance(unit) )
                        nearestAttacker = *it;
                }
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
        oldGameState.nearestAttacker = nearestAttacker;
    }

    // Nearest Attacked Ally
    {
    BWAPI::Unit nearestAttackedAlly = nullptr, nearestAlly = nullptr;
        for ( auto it = alliedUnits.begin(); it != alliedUnits.end(); it++ ) {
            if ( *it == unit ) continue;

            // Find nearest Ally
            if ( nearestAlly == nullptr || nearestAlly->getDistance(unit) > it->getDistance(unit) )
                nearestAlly = *it;

            // Find nearest Attacked Ally
            if ( unitStates[it->getID()].state.enemyHeatMap == 2 && ( nearestAttackedAlly == nullptr || nearestAttackedAlly->getDistance(unit) > it->getDistance(unit) ) )
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
        // If our unit is melee it shouldn't care about dmg as much
        if ( isMelee(unit) )
            reward /= 4;

        // If we actually shoot (not only attacked), this should happen only 1 time
        if(oldGameState.shooted )
        {
            if ( isMelee(unit) )
                // Zealots do 16, BWAPI values are wrong
                reward += 16;
            else
                // Dragoons do 20
                reward += 20;
            oldGameState.shooted = false;
        }

        //if(this->feedback && reward != 0 )
        //    Broodwar->printf("ID: %d Reward: %f lastHealth: %d currentHealth: %d dmg: %d", unit->getID(), reward, oldGameState.lastHealth, currentHealth, unit->getType().groundWeapon().damageAmount());

        // Feedback print
        // Broodwar->printf("Last action of Unit %d: %s", unit->getID(), oldGameState.lastAction == ActualAction::Move ? "move" : "shoot");
        // We are updating the MDP state so we need to update the transition table.
        table_.update(oldGameState.state, oldGameState.lastAction, newState, reward);
        // Actual update
        oldGameState.lastHealth = currentHealth;
        oldGameState.state = newState;
    }
}