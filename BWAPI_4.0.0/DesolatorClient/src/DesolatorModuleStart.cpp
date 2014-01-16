#include <Desolator/DesolatorModule.hpp>

#include <Desolator/Random.hpp>

using namespace BWAPI;
using namespace Filter;

namespace Desolator {

    DesolatorModule::DesolatorModule() :
                                log_("desolator.log", std::fstream::out | std::fstream::app),
                                table_(MDPState::getNumberOfStates(), 2),
                                policy_(MDPState::getNumberOfStates(),2) {}

    void DesolatorModule::onStart() {
        // We setup the environment only if this is not a replay
        if ( Broodwar->isReplay() ) return;

        {
            std::ifstream tFile("transitions_numbers.data"), pFile("policy.data");

            if ( !tFile || !( tFile >> table_ ))
                Broodwar->printf("###! COULD NOT LOAD TRANSITION NUMBERS !###");

            if ( !pFile || !( pFile >> policy_ )) {
                usingPolicy_ = false; 
                Broodwar->printf("###! COULD NOT LOAD POLICY !###");
            }
            else usingPolicy_ = true;               
        }

        // Enable the UserInput flag, which allows us to control the bot and type messages.
        Broodwar->enableFlag(Flag::UserInput);

        // Set the command optimization level so that common commands can be grouped
        // and reduce the bot's APM (Actions Per Minute).
        Broodwar->setCommandOptimizationLevel(2);

        Broodwar->setLocalSpeed(50);

        if ( Broodwar->enemy() ) // First make sure there is an enemy
            Broodwar << "The matchup is " << Broodwar->self()->getRace() << " vs " << Broodwar->enemy()->getRace() << std::endl;

        us_   = Broodwar->self();
        them_ = Broodwar->enemy();

        // Initialize state and action for every unit
        for(auto & u : us_->getUnits()) {
            UnitState uState(u);
            unitStates_[u->getID()] = uState;
        }

        feedback_ = false;
    }


    void DesolatorModule::onEnd(bool /* isWinner */ ) {
        std::ofstream tFile("transitions_numbers.data");

        if ( tFile ) tFile << table_;
        else         log_ << "We could not save the new experience to file.\n";

        Broodwar->restartGame();
    }

    void DesolatorModule::onUnitDestroy(BWAPI::Unit unit) {
        if( unit->getPlayer() == us_ ) {
            auto & GS = unitStates_[unit->getID()];
            int maxHealth =  unit->getType().maxShields() + unit->getType().maxHitPoints();
            double penalty = - ( static_cast<double>(maxHealth)/2 );

            table_.record(GS.state, GS.state, GS.lastStrategy, penalty);
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
}
