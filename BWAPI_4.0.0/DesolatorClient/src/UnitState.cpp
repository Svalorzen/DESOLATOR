#include <Desolator/UnitState.hpp>

namespace Desolator {
    // This is needed to be able to use operator[] in std::map.. we don't use it anyway
    UnitState::UnitState() {}

    UnitState::UnitState(const BWAPI::Unit & u) {
        lastStrategy = Strategy::Flee;
        lastAction   = Action::None;

        isStartingAttack = false;
        shooted = false;

        lastHealth = u->getHitPoints() + u->getShields();
        lastPosition = u->getTilePosition();
        lastPerfectPosition = u->getPosition();
        lastTarget = nullptr;

        nearestEnemy = nullptr;
        nearestAttacker = nullptr;

        nearestAttackedAlly = nullptr;
        nearestAlly = nullptr;

        notMovingTurns = 0;

        draw = false;
        drawTargetPosition = nullptr;
    }

    void UnitState::setDraw(const BWAPI::PositionOrUnit & p) {
        drawTargetPosition = p;
        draw = true;
    }

    void UnitState::setNoDraw() {
        draw = false;
    }

    bool UnitState::updatePosition(const BWAPI::TilePosition & p) {
        if ( lastPosition != p ) {
            lastPosition = p;
            return true;
        }
        return false;
    }
}
