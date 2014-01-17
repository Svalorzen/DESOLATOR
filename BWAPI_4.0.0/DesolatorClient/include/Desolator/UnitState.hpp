#ifndef DESOLATOR_UNIT_STATE_HEADER_FILE
#define DESOLATOR_UNIT_STATE_HEADER_FILE

#include <BWAPI.h>

#include <Desolator/MDPState.hpp>

namespace Desolator {

    enum Strategy {
        Fight = 0,
        Flee
    };

    enum Action{
        Attack = 0,
        Move,
        None
    };

    /* This keeps track of the MDP state of a unit and a bunch of other stats that are useful to know to speed up computing. */
    class UnitState {
        public:
            UnitState();
            UnitState(const BWAPI::Unit &);

            bool updatePosition(const BWAPI::TilePosition &);
            void setDraw(const BWAPI::PositionOrUnit &);
            void setNoDraw();

            MDPState state;

            Strategy lastStrategy;
            Action   lastAction;

            bool isStartingAttack;
            bool shooted;

            int                 lastHealth;
            BWAPI::TilePosition lastPosition;
            BWAPI::Position     lastPerfectPosition;
            BWAPI::Unit         lastTarget;

            BWAPI::Unit nearestEnemy;
            BWAPI::Unit nearestAttacker;

            BWAPI::Unit nearestAttackedAlly;
            BWAPI::Unit nearestAlly;

            int notMovingTurns;

            bool draw;
            BWAPI::Position drawTargetPosition;
    };

}

#endif
