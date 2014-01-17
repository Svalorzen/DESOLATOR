#include <Desolator/DesolatorModule.hpp>

#include <Desolator/BWAPIHelpers.hpp>

#include <iostream>

using std::cout;
using std::endl;

using namespace BWAPI;
using namespace Filter;

namespace Desolator {
    /* This function updates the current game situation for a particular unit. Additionally, if requested through the "alsoState" parameter,
       it updates the MDP state of the unit. This second update should only happen at each unit "tick", while the rest of the state should be
       updated as often as possible. */
    void DesolatorModule::updateUnitState(BWAPI::Unit & unit, bool alsoState) {
        auto & GS = unitStates_[unit->getID()];
        MDPState newState; // Everything is 0

        //cout << "Updating State"; if (alsoState) cout << " and TICK!";
        //cout << endl;

        auto & ourUnits    = us_->getUnits();
        auto & theirUnits  = them_->getUnits();

        // STATE INITIAL STUFF
        if ( alsoState ) {
            // Update health
            size_t health = (unit->getHitPoints() + unit->getShields() - 1) * 100;
            health /= (unit->getType().maxHitPoints() + unit->getType().maxShields()) * 25;

            newState.setFeatureValue(MDPState::HEALTH, health);

            // Update weapon cooldown
            newState.setFeatureValue(MDPState::WEAPON_COOLDOWN, unit->getGroundWeaponCooldown() != 0);
        }

        // We can't know the Strategy yet
        // GS.lastStrategy = CAN'T FILL
        //cout << "Stage 1 completed" << endl;
   

        // Is the attack tick done?
        if ( GS.isStartingAttack && unit->isAttackFrame() ) {
            GS.isStartingAttack = false;
            GS.shooted = true;
        }

        // Last Position - WE DON'T HAVE TO DO THIS
        // GS.lastPosition = unit->getTilePosition();

        // Nearest Enemy/Attacker
        {
            BWAPI::Unit nearestEnemy = nullptr, nearestAttacker = nullptr;
            for ( auto u : theirUnits ) {
                // Find nearest Enemy
                if ( nearestEnemy == nullptr || nearestEnemy->getDistance(unit) > u->getDistance(unit) )
                    nearestEnemy = u;

                if ( alsoState ) {
                    // Get Enemy heatmap for this unit
                    if ( newState.getFeatureValue(MDPState::ENEMY_PROXIMITY) < 2 && unit == u->getOrderTarget() ) {
                        newState.setFeatureValue(MDPState::ENEMY_PROXIMITY, 2);
                        if ( nearestAttacker == nullptr || nearestAttacker->getDistance(unit) > u->getDistance(unit) )
                            nearestAttacker = u;
                    }
                    else if( newState.getFeatureValue(MDPState::ENEMY_PROXIMITY) < 1 ) {
                        int enemyRealRange = getAppliedWeaponRange(u);
                        BWAPI::Unitset unitsInEnemyRange = u->getUnitsInRadius(enemyRealRange);
                        // Checks if we are in enemy optimized range
                        if ( unitsInEnemyRange.find(unit) != unitsInEnemyRange.end() )
                            newState.setFeatureValue(MDPState::ENEMY_PROXIMITY, 1);
                    }

                    if ( !newState.getFeatureValue(MDPState::CAN_TARGET) && unit->isInWeaponRange(nearestEnemy) )
                        newState.setFeatureValue(MDPState::CAN_TARGET, 1);
                }
            }
            GS.nearestEnemy = nearestEnemy;
            GS.nearestAttacker = nearestAttacker;
        }
        //cout << "Stage 2 completed" << endl;
        // Nearest Attacked Ally
        {
            BWAPI::Unit nearestAttackedAlly = nullptr, nearestAlly = nullptr;
            for ( auto u : ourUnits ) {
                if ( u == unit ) continue;

                // Find nearest Ally
                if ( nearestAlly == nullptr || nearestAlly->getDistance(unit) > u->getDistance(unit) )
                    nearestAlly = u;

                // Find nearest Attacked Ally
                if ( GS.state.getFeatureValue(MDPState::ENEMY_PROXIMITY) == 2 && ( nearestAttackedAlly == nullptr || nearestAttackedAlly->getDistance(unit) > u->getDistance(unit) ) )
                    nearestAttackedAlly = u;

                if ( alsoState && !newState.getFeatureValue(MDPState::FRIEND_PROXIMITY) ) {
                    int allyRealRange = getAppliedWeaponRange(u);
                    auto unitsInFriendRange = u->getUnitsInRadius(allyRealRange);

                    if (unitsInFriendRange.find(unit) != unitsInFriendRange.end() )
                        newState.setFeatureValue(MDPState::FRIEND_PROXIMITY, 1);
                }
            }
            GS.nearestAttackedAlly = nearestAttackedAlly;
            GS.nearestAlly = nearestAlly;
        }
        // cout << "Stage 3 completed" << endl;
        // State and Table updating
        if ( alsoState ) {
            int currentHealth = unit->getHitPoints() + unit->getShields();
            double reward = currentHealth - GS.lastHealth;
            // If our unit is melee it shouldn't care about dmg as much
            if ( isMelee(unit) )
                reward /= 4;

            // If we actually shoot (not only attacked), this should happen only 1 time
            if ( GS.shooted ) {
                if ( isMelee(unit) )
                    reward += 16; // Zealots do 16, BWAPI values are wrong
                else
                    reward += 20; // Dragoons do 20
                GS.shooted = false;
            }

            //if(this->feedback && reward != 0 )
            //    Broodwar->printf("ID: %d Reward: %f lastHealth: %d currentHealth: %d dmg: %d", 
            //    unit->getID(), reward, GS.lastHealth, currentHealth, unit->getType().groundWeapon().damageAmount());
            //std::cout << "Old state: " << GS.state << " -- New state: " << newState << " -- Action: " << GS.lastStrategy << "Possibles: " << Strategy::Fight << " " << Strategy::Flee << endl;
            // We are updating the MDP state so we need to update the transition table.
            table_.record(GS.state, newState, GS.lastStrategy, reward);
            // Actual update
            GS.lastHealth = currentHealth;
            GS.state = newState;
            //std::cout << "Finished stage 4" << endl;
        }
        //std::cout << "Done." << endl;
    }
}
