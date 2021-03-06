#include <Desolator/AI.hpp>

#include <fstream>
#include <cmath>

#include <Desolator/BWAPIHelpers.hpp>
#include <Desolator/Random.hpp>

#include <iostream>

using std::cout;
using std::endl;

namespace Desolator {
    namespace AI {
        static std::fstream log("ai.log", std::fstream::out | std::fstream::app);

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
                        log << "AI-Attack: " << __LINE__ << " -- ERROR: Called attack with no enemies in sight\n";
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
                    log << "AI-Attack: " << __LINE__ << " -- ERROR: No enemy in range with canTarget true\n";
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

            std::vector<BWAPI::Position> fieldVectors;
            fieldVectors.reserve(friends.size() + enemies.size());

            auto unitPos = unit->getPosition();

            // If we are alone, jump friend code
            if ( GS.nearestAlly != nullptr ) {
                auto friendPos = GS.nearestAlly->getPosition();
                // We are not covered
                if ( !GS.state.getFeatureValue(MDPState::FRIEND_PROXIMITY) ) {
                    fieldVectors.emplace_back(
                                                // They should go towards friends the more they are away
                                                (unitPos.x - friendPos.x)*friendForceUncovered,
                                                (unitPos.y - friendPos.y)*friendForceUncovered
                                            );
                }
                else {
                    auto distance = std::max(minDistance, unitPos.getDistance(friendPos));
                    fieldVectors.emplace_back(
                                                // They should go towards friends the more they are away
                                                (unitPos.x - friendPos.x)*friendForceCovered / (distance*distance),
                                                (unitPos.y - friendPos.y)*friendForceCovered / (distance*distance)
                                            );
                }
            }
            //cout << "Stage 1 done" << endl;
            // Enemies code
            for ( auto & e : enemies ) {
                auto enemyPos = e->getPosition();
                auto distance = std::max(minDistance, unitPos.getDistance(enemyPos));
                auto force = ( e->getOrderTarget() == unit ) ? enemyTargetedForce : enemyForce;

                fieldVectors.emplace_back(
                                            (unitPos.x - enemyPos.x)*force / (distance*distance),
                                            (unitPos.y - enemyPos.y)*force / (distance*distance)
                                        );
            }
            //cout << "Stage 2 done" << endl;
            // Create final vector
            BWAPI::Position finalVector(0,0);
            for ( auto & v : fieldVectors )
                finalVector += v;
            //cout << "Added up." << endl;
            BWAPI::Position placeIwouldLikeToGo = unit->getPosition() + finalVector;
            placeIwouldLikeToGo.makeValid(); // Make sure that the place is within the bounds of the map
            //cout << "Made valid. " << endl;
            // We get again the true movement vector
            finalVector = placeIwouldLikeToGo - unit->getPosition();
            // Here we normalize our vector to length 320 just because.
            double normalizedLength = 32.0 * 10;
            BWAPI::Position normalizedFinalVector = normalize(finalVector, normalizedLength);
            //cout << "Normalized" << endl;
            placeIwouldLikeToGo = unit->getPosition() + normalizedFinalVector;
            //cout << "Done, final place = " << placeIwouldLikeToGo << endl;
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


