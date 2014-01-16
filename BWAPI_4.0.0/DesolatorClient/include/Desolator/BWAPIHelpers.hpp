#ifndef DESOLATOR_BWAPI_HELPERS_HEADER_FILE
#define DESOLATOR_BWAPI_HELPERS_HEADER_FILE

#include <BWAPI.h>

namespace Desolator {
    constexpr int STANDARD_SPEED_FPS = 19;

    // The range actually takes into consideration unit size, so we try to do that
    int getActualWeaponRange(const BWAPI::Unit & unit);

    // This function returns a range differently if the unit is considered
    // melee or ranged. If ranged the range is the normal range, if melee
    // is the range + the movement it can do in 1 second
    int getAppliedWeaponRange(const BWAPI::Unit & unit);

    // An unit is considered melee if it can move more than it can shoot
    bool isMelee(const BWAPI::Unit & unit);

    BWAPI::TilePosition convertToTile(const BWAPI::Position &);

    BWAPI::Position normalize(const BWAPI::Position &, double length);
}

#endif
