#include <Desolator/BWAPIHelpers.hpp>

using namespace BWAPI;

namespace Desolator {
    // The range actually takes into consideration unit size, so we try to do that
    int getActualWeaponRange(const BWAPI::Unit & unit) {
        return unit->getType().groundWeapon().maxRange() + std::max(unit->getType().dimensionLeft(),unit->getType().dimensionUp());
    }

    // This function returns a range differently if the unit is considered
    // melee or ranged. If ranged the range is the normal range, if melee
    // is the range + the movement it can do in 1 second
    int getAppliedWeaponRange(const BWAPI::Unit & unit) {
        if ( isMelee(unit) )
            return getActualWeaponRange(unit) + static_cast<int>(unit->getType().topSpeed()) * STANDARD_SPEED_FPS;
        else
            return getActualWeaponRange(unit);
    }

    // An unit is considered melee if it can move more than it can shoot
    bool isMelee(const BWAPI::Unit & unit) {
        return static_cast<int>(unit->getType().topSpeed()) * STANDARD_SPEED_FPS > getActualWeaponRange(unit);
    }


    BWAPI::TilePosition convertToTile(const BWAPI::Position & point) {
        return TilePosition( Position(abs(point.x - 32 / 2),
                    abs(point.y - 32 / 2)) );
    }

    BWAPI::Position normalize(const BWAPI::Position & p, double length) {
        BWAPI::Position norm;

        if ( p.y != 0.0 ) {
            auto correlation = std::abs((double)p.x / (double)p.y);

            norm.y = length / std::sqrt((double)(correlation*correlation + 1));
            norm.x = norm.y * correlation;
        }
        else {
            norm.y = 0.0;
            norm.x = length;
        }

        norm = copysign(norm, p);

        return norm;
    }

    BWAPI::Position copysign(const BWAPI::Position& magn, const BWAPI::Position& sign) {
        BWAPI::Position result;
        result.y = std::copysign(magn.y, sign.y);
        result.x = std::copysign(magn.x, sign.x);
        return result;
    }
}
