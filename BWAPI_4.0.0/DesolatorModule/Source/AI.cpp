#include "AI.h"

#include "helpers.h"
#include <cmath>

Logger AI::log_("ai.log", std::fstream::out | std::fstream::app);

using BWAPI::Broodwar;

BWAPI::Position AI::explore(int mapWidth, int mapHeight) {
    int x = ( rand() % (mapWidth) ) * 32;
    int y = ( rand() % (mapHeight) ) * 32;
    BWAPI::Position pos(x, y);

    return pos;
}

BWAPI::PositionOrUnit AI::attack(BWAPI::Unit unit, const BWAPI::Unitset & allies, const BWAPI::Unitset & enemies, DesolatorModule::UnitStates & unitStates ) {
    auto & GS = unitStates[unit->getID()];

    if ( !GS.state.canTarget ) {
        // No enemy in range move to closest ally that is targeted
        if ( GS.nearestAttackedAlly != nullptr ) {
            // If we have a closest ally that is targeted move towards it.
            Broodwar->printf("Attack method for unit %d: ATTACK TO NEAREST FRIEND", unit->getID());

            auto & attacker = unitStates[GS.nearestAttackedAlly->getID()].nearestAttacker;

            return attacker;
        }
        else {
            // If our allieds died or are not targeted kill closest enemy.
            if ( GS.nearestEnemy != nullptr ) {
                Broodwar->printf("Attack method for unit %d: MOVE TO NEAREST ENEMY", unit->getID());
                return GS.nearestEnemy;
            }
            // Nothing to do...
            else {
                log_("ERROR: Called attack with no enemies in sight");
                return unit->getPosition();
            }
        }
    } 
    else {  
        BWAPI::Unit weakestEnemy = nullptr;
        BWAPI::Unitset unitsInRange = unit->getUnitsInRadius(unit->getType().groundWeapon().maxRange());
     
        // Get weakest in range
        for ( BWAPI::Unitset::iterator it = unitsInRange.begin(); it != unitsInRange.end(); it++ )
            if ( enemies.exists(*it) && ( weakestEnemy == nullptr || ( it->getHitPoints() + it->getShields() < weakestEnemy->getHitPoints() + weakestEnemy->getShields() )) )
                weakestEnemy = *it;
     
        if ( weakestEnemy != nullptr ) {
            Broodwar->printf("Attack method for unit %d: ATTACK WEAKEST ENEMY", unit->getID());
            return weakestEnemy;
        }
        else {
            Broodwar->printf("Attack method for unit %d: ERROR NO ENEMY IN RANGE", unit->getID());
            log_("ERROR: No enemy in range with canTarget true");
            return GS.nearestEnemy;
        }
    }
}

BWAPI::Position AI::flee(BWAPI::Unit unit, const BWAPI::Unitset & friends, const BWAPI::Unitset & enemies, DesolatorModule::UnitStates & unitStates)
{
    auto & GS = unitStates[unit->getID()];

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
        if ( !unitStates[unit->getID()].state.friendHeatMap ) {
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

	// Here we normalize our vector to length 32 just because.
	double x, y;
	auto realx = (placeIwouldLikeToGo.x - unit->getPosition().x);
	auto realy = (placeIwouldLikeToGo.y - unit->getPosition().y);

	if ( realy != 0.0 ) {
		auto correlation = std::abs((double)realx / (double)realy);

		y = 32.0 / std::sqrt((double)(correlation*correlation + 1));
		x = y * correlation;
	}
	else {
		y = 0.0;
		x = 32.0;
	}

	x = copysign(x, realx);
	y = copysign(y, realy);

	placeIwouldLikeToGo = unit->getPosition();
	placeIwouldLikeToGo.x += x;
	placeIwouldLikeToGo.y += y;

   // Broodwar->drawLineMap(unit->getPosition(), placeIwouldLikeToGo, BWAPI::Color(0,255,0));

    return placeIwouldLikeToGo;
}