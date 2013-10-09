#pragma once

#include <BWAPI.h>

#include "MDPState.h"

enum Action {
    Attack,
    Flee
};

enum ActualAction{
    Shoot, // Can't redefine attack...
    Move
};

class UnitState {
    /* This keeps track of the MDP state of a unit and a bunch of other stats that are useful to know to speed up computing. */
    public:
        UnitState();
        UnitState(const BWAPI::Unit &);

        MDPState state;

        bool isStartingAttack;
        Action lastAction;
        ActualAction actualAction;
        bool shooted;
  
        int lastHealth;
        BWAPI::TilePosition lastPosition;
        BWAPI::Unit lastTarget;

        BWAPI::Unit nearestEnemy;
        BWAPI::Unit nearestAttacker;

        BWAPI::Unit nearestAttackedAlly;
        BWAPI::Unit nearestAlly;

        BWAPI::Position lastPerfectPosition;
        int notMovingTurns;
};

