#include <Desolator/AI.hpp>

#include <fstream>
#include <cmath>

#include <BWAPI.h>

#include <Desolator/BWAPIHelpers.hpp>
#include <Desolator/Random.hpp>

#include <iostream>

using std::cout;
using std::endl;

namespace Desolator {
    namespace AI {
      //  static std::fstream log("ai.log", std::fstream::out | std::fstream::app);

        BWAPI::Position explore() {
            int x = RandomInt::get(0, BWAPI::Broodwar->mapWidth()-1);
            int y = RandomInt::get(0, BWAPI::Broodwar->mapHeight()-1);

            return BWAPI::Position(x*32, y*32);
        }

        BWAPI::PositionOrUnit attack(BWAPI::Unit unit, const BWAPI::Unitset & /*allies*/, const BWAPI::Unitset & enemies, DesolatorModule::UnitStates & unitStates ) {
            auto & GS = unitStates[unit->getID()];
            //cout << "## Attack AI" << endl;
            if ( !GS.state.getFeatureValue(MDPState::CAN_TARGET) ) {
                //    cout << "Can't target anyone" << endl;
                // No enemy in range move to closest ally that is targeted
                if ( GS.nearestAttackedAlly != nullptr ) {
                    // If we have a closest ally that is targeted move towards it.
                    // Broodwar->printf("Attack method for unit %d: ATTACK TO NEAREST FRIEND", unit->getID());
                    //  cout << "Finding attacker of nearest ally" << endl;
                    auto & attacker = unitStates[GS.nearestAttackedAlly->getID()].nearestAttacker;
                    if (!attacker) cout << "WTFWTFWTF" << endl;
                    //cout << "   done." << endl;
                    return attacker;
                }
                else {
                    // If our allieds died or are not targeted kill closest enemy.
                    if ( GS.nearestEnemy != nullptr ) {
                        //  cout << "Returning closest enemy" << endl;
                        // Broodwar->printf("Attack method for unit %d: MOVE TO NEAREST ENEMY", unit->getID());
                        return GS.nearestEnemy;
                    }
                    // Nothing to do...
                    else {
                        //cout << "### WAT -- no closest enemy? where is everyone? ###" << endl;
        //                log << "AI-Attack: " << __LINE__ << " -- ERROR: Called attack with no enemies in sight\n";
                        return unit->getPosition();
                    }
                }
            }
            else {
                //   cout << "Setting target for shooting " << endl;
                BWAPI::Unit weakestEnemy = nullptr;
                BWAPI::Unitset unitsInRange = unit->getUnitsInRadius(unit->getType().groundWeapon().maxRange());

                // Get weakest in range
                for ( auto & u : unitsInRange )
                    if ( enemies.exists(u) && ( weakestEnemy == nullptr || ( u->getHitPoints() + u->getShields() < weakestEnemy->getHitPoints() + weakestEnemy->getShields() )) )
                        weakestEnemy = u;
                    // cout << "Got weakest in range " << endl;
                if ( weakestEnemy != nullptr ) {
                    // Broodwar->printf("Attack method for unit %d: ATTACK WEAKEST ENEMY", unit->getID());
                    return weakestEnemy;
                }
                else {
                    //cout << "### WTF: NO ENEMY IN RANGE EVEN THOUGH I CAN TARGET" << endl;
                    // Broodwar->printf("Attack method for unit %d: ERROR NO ENEMY IN RANGE", unit->getID());
          //          log << "AI-Attack: " << __LINE__ << " -- ERROR: No enemy in range with canTarget true\n";
                    return GS.nearestEnemy;
                }
            }
        }

        //#######################################
        //############### FLEE ##################
        //#######################################

        BWAPI::Position flee(BWAPI::Unit unit, const BWAPI::Unitset & friends, const BWAPI::Unitset & enemies, DesolatorModule::UnitStates & unitStates) {
            auto & GS = unitStates[unit->getID()];
            //cout << "AI: we be fleeing yo." << endl;
            double enemyForce = 5000.0;
            double enemyTargetedForce = 10000.0;
            // Friends attract
            double friendForceUncovered = -0.5; // This value is weird because it is not divided by distance
            double friendForceCovered = 3000.0;
            // Used to avoid infinite forces when units are near
            double minDistance = 40.0;
            BWAPI::Position finalVector(0, 0);

            auto unitPos = unit->getPosition();

            // If we are alone, jump friend code
            if ( GS.nearestAlly != nullptr ) {
                auto friendPos = GS.nearestAlly->getPosition();
                // We are not covered
                if ( !GS.state.getFeatureValue(MDPState::FRIEND_PROXIMITY) ) {
                    // They should go towards friends the more they are away
                    finalVector.x += (unitPos.x - friendPos.x)*friendForceUncovered;
                    finalVector.y += (unitPos.y - friendPos.y)*friendForceUncovered;
                }
                else {
                    auto distance = std::max(minDistance, unitPos.getDistance(friendPos));
                    finalVector.x += (unitPos.x - friendPos.x)*friendForceCovered / (distance*distance);
                    finalVector.y += (unitPos.y - friendPos.y)*friendForceCovered / (distance*distance);
                }
            }
            //cout << "Stage 1 done" << endl;
            // Enemies code
            for ( auto & e : enemies ) {
                auto enemyPos = e->getPosition();
                auto distance = std::max(minDistance, unitPos.getDistance(enemyPos));
                auto force = ( e->getOrderTarget() == unit ) ? enemyTargetedForce : enemyForce;

                finalVector.x += (unitPos.x - enemyPos.x)*force / (distance*distance);
                finalVector.y += (unitPos.y - enemyPos.y)*force / (distance*distance);
            }
            // Go towards center of the map.
            {
                BWAPI::Position mapCenter;
                mapCenter.x = BWAPI::Broodwar->mapWidth()*32 / 2;
                mapCenter.y = BWAPI::Broodwar->mapHeight()*32 / 2;
                auto distance = std::max(minDistance, unitPos.getDistance(mapCenter));
                finalVector.x += - (unitPos.x - mapCenter.x) / 6.0;
                finalVector.y += - (unitPos.y - mapCenter.y) / 6.0;
            }

            BWAPI::Position placeIwouldLikeToGo = unit->getPosition() + finalVector;
            placeIwouldLikeToGo.makeValid(); // Make sure that the place is within the bounds of the map
            // We get again the true movement vector
            finalVector = placeIwouldLikeToGo - unit->getPosition();

            // Here we normalize our vector to length 320 just because.
            double normalizedLength = 32.0 * 10;
            BWAPI::Position normalizedFinalVector = normalize(finalVector, normalizedLength);

            placeIwouldLikeToGo = unit->getPosition() + normalizedFinalVector;
            return placeIwouldLikeToGo;
        }

            // Get repulsed by inaccessible regions
            /*
            auto & regions = Broodwar->getAllRegions();
            for ( auto & r : regions ) {
                if ( !r->isAccessible() ){
                    auto regionPos = r->getCenter();
                    auto distance = std::max(minDistance, regionPos.getDistance(unit->getPosition()));

                    fieldVectors.emplace_back(
                                                (unitPos.x - regionPos.x)*enemyForce / (distance*distance),
                                                (unitPos.y - regionPos.y)*enemyForce / (distance*distance)
                                            );
                }
            } */

    }
}


