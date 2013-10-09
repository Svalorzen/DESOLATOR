#include "helpers.h"

using namespace BWAPI;

double copysign(double x, double y) {
	if ( x * y < 0 )
		x = -x;

	return x;
}

// #############################################
// ########### AUXILLARY FUNCTIONS #############
// #############################################

// The range actually takes into consideration unit size, so we try to do that
int getActualWeaponRange(BWAPI::Unit unit) {
    return unit->getType().groundWeapon().maxRange() + std::max(unit->getType().dimensionLeft(),unit->getType().dimensionUp());
}

// An unit is considered melee if it can move more than it can shoot
bool isMelee(BWAPI::Unit unit) {
    return static_cast<int>(unit->getType().topSpeed()) * STANDARD_SPEED_FPS > getActualWeaponRange(unit);
}

// This function returns a range differently if the unit is considered
// melee or ranged. If ranged the range is the normal range, if melee
// is the range + the movement it can do in 1 second
int getOptimizedWeaponRange(BWAPI::Unit unit) {
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
// ########### DRAWING FUNCTIONS ###############
// #############################################
void drawState(const DesolatorModule::UnitStates & states)
{
    /* Loops over the states mapping */
    int position = 20;
    for(auto i = states.begin(); i != states.end(); i++)
    {
        MDPState state = i->second.state;
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

void drawHeatMap(BWAPI::Player us, BWAPI::Player enemy)
{
    // Load visible units
    auto & ourUnits = us->getUnits();
    auto & theirUnits = enemy->getUnits();
    // DRAW HEATMAPS
    {
        auto printBaseRange = [&](decltype(ourUnits.begin()) unit, BWAPI::Color c){
            Broodwar->drawCircleMap(
                unit->getPosition(),
                getActualWeaponRange(*unit),
                c);
        };
        auto printLongRange = [&](decltype(ourUnits.begin()) unit, BWAPI::Color c){
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
