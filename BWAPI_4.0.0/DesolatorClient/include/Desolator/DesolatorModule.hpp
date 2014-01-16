#ifndef DESOLATOR_DESOLATOR_MODULE_HEADER_FILE
#define DESOLATOR_DESOLATOR_MODULE_HEADER_FILE

#include <BWAPI.h>

#include <map>
#include <fstream>

#include <Desolator/UnitState.hpp>
#include <AIToolbox/Experience.hpp>
#include <AIToolbox/Policy.hpp>

// Remember not to use "Broodwar" in any global class constructor!
namespace Desolator {

    class DesolatorModule {
        public:
            typedef std::map<int, UnitState> UnitStates;

            DesolatorModule();

            void dispatch(BWAPI::Event);

            void onStart        ();
            void onEnd          (bool isWinner);
            void onFrame        ();
            void onSendText     (std::string text);
            void onUnitDestroy  (BWAPI::Unit unit);

            void drawTargets();
            void drawIDs();

        private:
            BWAPI::Player us_;
            BWAPI::Player them_;
            
            BWAPI::Position explore();

            // STATE VARIABLES
            UnitStates unitStates_;

            // FEEDBACK STUFF
            bool feedback_;

            std::ofstream log_;

            bool usingPolicy_;
            AIToolbox::Experience table_;
            AIToolbox::Policy policy_;

            // STATE METHODS
            void updateUnitState(BWAPI::Unit unit, const BWAPI::Unitset & alliedUnits, const BWAPI::Unitset & enemyUnits, bool alsoState = false);
    };

}

#endif
