#include <Desolator/DesolatorModule.hpp>

#include <Desolator/Random.hpp>

#include <iostream>

using std::cout;
using std::endl;

using namespace BWAPI;
using namespace Filter;

namespace Desolator {

    DesolatorModule::DesolatorModule() :
                                log_("desolator.log", std::fstream::out | std::fstream::app),
                                table_(MDPState::getNumberOfStates(), 2),
                                policy_(MDPState::getNumberOfStates(),2) {}

    void DesolatorModule::onStart() {
        // We setup the environment only if this is not a replay
        if ( Broodwar->isReplay() ) return;

        cout << "Reading files..." << endl;
        {
            std::ifstream tFile("transitions_numbers.data"), pFile("policy.data");

            if ( !tFile || !( tFile >> table_ ))
                Broodwar->printf("###! COULD NOT LOAD TRANSITION NUMBERS !###");

            if ( !pFile || !( pFile >> policy_ )) {
                usingPolicy_ = false; 
                Broodwar->printf("###! COULD NOT LOAD POLICY !###");
            }
            else usingPolicy_ = true;               
        }
        cout << "Files read." << endl;

        // Enable the UserInput flag, which allows us to control the bot and type messages.
        Broodwar->enableFlag(Flag::UserInput);

        // Set the command optimization level so that common commands can be grouped
        // and reduce the bot's APM (Actions Per Minute).
        Broodwar->setCommandOptimizationLevel(2);

        Broodwar->setLocalSpeed(50);

        if ( Broodwar->enemy() ) // First make sure there is an enemy
            Broodwar << "The matchup is " << Broodwar->self()->getRace() << " vs " << Broodwar->enemy()->getRace() << std::endl;

        us_   = Broodwar->self();
        them_ = Broodwar->enemy();

        // Initialize state and action for every unit
        for(auto & u : us_->getUnits()) {
            UnitState uState(u);
            unitStates_[u->getID()] = uState;
            unitStates_[u->getID()].setNoDraw();
        }

        feedback_ = false;
        cout << "Start function completed." << endl;
    }


    void DesolatorModule::onEnd(bool /* isWinner */ ) {
        std::ofstream tFile("transitions_numbers.data");

        if ( tFile ) tFile << table_;
        else         log_ << "We could not save the new experience to file.\n";

        Broodwar->restartGame();
    }

    void DesolatorModule::onUnitDestroy(BWAPI::Unit unit) {
        if( unit->getPlayer() == us_ ) {
            auto & GS = unitStates_[unit->getID()];
            int maxHealth =  unit->getType().maxShields() + unit->getType().maxHitPoints();
            double penalty = - ( static_cast<double>(maxHealth)/2 );

            table_.record(GS.state, GS.state, GS.lastStrategy, penalty);
        }
    }
}
