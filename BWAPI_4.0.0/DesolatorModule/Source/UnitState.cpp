#include "UnitState.h"

// This is needed to be able to use operator[] in std::map.. we don't use it anyway
UnitState::UnitState() {}

UnitState::UnitState(const BWAPI::Unit & u) {
    // Saved states
    lastHealth = u->getHitPoints() + u->getShields();
    lastPosition = u->getTilePosition();
    lastPerfectPosition = u->getPosition();
    notMovingTurns = 0;
    isStartingAttack = false;
    shooted = false;
    // Saved units
    lastTarget = nullptr;
    nearestAttackedAlly = nullptr;
    nearestAttacker = nullptr;
    nearestAlly = nullptr;
    nearestEnemy = nullptr;
}