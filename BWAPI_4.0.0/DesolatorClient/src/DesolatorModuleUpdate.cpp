#include <Desolator/DesolatorModule.hpp>

#include <Desolator/BWAPIHelpers.hpp>

using namespace BWAPI;
using namespace Filter;

namespace Desolator {
    /* This function updates the current game situation for a particular unit. Additionally, if requested through the "alsoState" parameter,
       it updates the MDP state of the unit. This second update should only happen at each unit "tick", while the rest of the state should be
       updated as often as possible. */
    void DesolatorModule::updateUnitState(BWAPI::Unit & unit) {
        auto & GS = unitStates_[unit->getID()];

        auto & theirUnits  = them_->getUnits();

        // We can't know the Strategy yet
        // GS.lastStrategy = CAN'T FILL

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

                if ( unit == u->getOrderTarget() && ( nearestAttacker == nullptr || nearestAttacker->getDistance(unit) > u->getDistance(unit) ))
                    nearestAttacker = u;
            }
            GS.nearestEnemy = nearestEnemy;
            GS.nearestAttacker = nearestAttacker;
        }
    }

    void DesolatorModule::shareKnowledge(BWAPI::Unit & unit) {
        auto & GS = unitStates_[unit->getID()];

        // Nearest Attacked Ally
        BWAPI::Unit nearestAttackedAlly = nullptr, nearestAlly = nullptr;
        for ( auto u : us_->getUnits() ) {
            if ( u == unit ) continue;

            // Find nearest Ally
            if ( nearestAlly == nullptr || nearestAlly->getDistance(unit) > u->getDistance(unit) )
                nearestAlly = u;

            // Find nearest Attacked Ally
            if ( GS.state.getFeatureValue(MDPState::ENEMY_PROXIMITY) == 2 && ( nearestAttackedAlly == nullptr || nearestAttackedAlly->getDistance(unit) > u->getDistance(unit) ) )
                nearestAttackedAlly = u;
        }
        GS.nearestAttackedAlly = nearestAttackedAlly;
        GS.nearestAlly = nearestAlly;
    }

    void DesolatorModule::updateUnitMDPState(BWAPI::Unit & unit) {
        auto & GS = unitStates_[unit->getID()];
        auto & theirUnits   = them_->getUnits();
        auto & ourUnits     = us_->getUnits();
        MDPState newState; // Everything is 0

        // Update weapon cooldown
        newState.setFeatureValue(MDPState::WEAPON_COOLDOWN, unit->getGroundWeaponCooldown() != 0);

        if ( GS.nearestAttacker )
            newState.setFeatureValue(MDPState::ENEMY_PROXIMITY, 2);
        else { // We loop over everything because nearest enemy may have bad range 
            for ( auto & e : theirUnits ) {
                int enemyRealRange = getAppliedWeaponRange(e);
                BWAPI::Unitset unitsInEnemyRange = e->getUnitsInRadius(enemyRealRange);
                // Checks if we are in enemy optimized range
                if ( unitsInEnemyRange.find(unit) != unitsInEnemyRange.end() ) {
                    newState.setFeatureValue(MDPState::ENEMY_PROXIMITY, 1);
                    break;
                }
            }
        }

        if ( GS.nearestEnemy && unit->isInWeaponRange(GS.nearestEnemy) )
            newState.setFeatureValue(MDPState::CAN_TARGET, 1);

        for ( auto & u : ourUnits ) {
            int allyRealRange = getAppliedWeaponRange(u);
            auto unitsInFriendRange = u->getUnitsInRadius(allyRealRange);

            if (unitsInFriendRange.find(unit) != unitsInFriendRange.end() ) {
                newState.setFeatureValue(MDPState::FRIEND_PROXIMITY, 1);
                break;
            }
        }


        int currentHealth = unit->getHitPoints() + unit->getShields();
        // Update health
        {
            size_t tickHealth = (currentHealth - 1) * 100;
            tickHealth /= (unit->getType().maxHitPoints() + unit->getType().maxShields()) * 25;

            newState.setFeatureValue(MDPState::HEALTH, tickHealth);
        }
        // Record reward
        {
            // Punishment based on life lost
            double reward = currentHealth - GS.lastHealth;
            // If our unit is melee it shouldn't care about dmg as much
            if ( isMelee(unit) ) reward /= 4;

            // If we actually shoot (not only attacked), this should happen only 1 time
            if ( GS.shooted ) {
                if ( isMelee(unit) )    reward += 16; // Zealots do 16, BWAPI values are wrong
                else                    reward += 20; // Dragoons do 20
                GS.shooted = false;
            }
            
            //if(this->feedback && reward != 0 )
            //    Broodwar->printf("ID: %d Reward: %f lastHealth: %d currentHealth: %d dmg: %d", 
            //    unit->getID(), reward, GS.lastHealth, currentHealth, unit->getType().groundWeapon().damageAmount());

            // We are updating the MDP state so we need to update the transition table.
            table_.record(GS.state, newState, GS.lastStrategy, reward);
        }
        // Actual update
        GS.lastHealth = currentHealth;
        GS.state = newState;
    }
}
