#include <Desolator/DesolatorModule.hpp>

#include <Desolator/Random.hpp>
#include <AIToolbox\MDP\RLModel.hpp>
#include <AIToolbox\MDP\ValueIteration.hpp>
#include <AIToolbox\MDP\QGreedyPolicy.hpp>

#include <iostream>

using namespace BWAPI;
using namespace Filter;

namespace Desolator {

    DesolatorModule::DesolatorModule() :
                                log_("desolator.log", std::fstream::out | std::fstream::app),
                                table_ (MDPState::getNumberOfStates(), 2),
                                policy_(MDPState::getNumberOfStates(), 2)
    {
        feedback_ = false;
        startup_ = true;
        currentSpeed_ = 50;
        completedMatches_ = 0;
    }

    void DesolatorModule::onStart() {
        // We setup the environment only if this is not a replay
        if ( Broodwar->isReplay() ) return;

        if ( startup_ )
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

        // Enable the UserInput flag, which allows us to control the bot and type messages.
        Broodwar->enableFlag(Flag::UserInput);

        // Set the command optimization level so that common commands can be grouped
        // and reduce the bot's APM (Actions Per Minute).
        Broodwar->setCommandOptimizationLevel(2);

        if ( !startup_ )
            Broodwar->setLocalSpeed(currentSpeed_);

        if ( Broodwar->enemy() ) // First make sure there is an enemy
            Broodwar << "The matchup is " << Broodwar->self()->getRace() << " vs " << Broodwar->enemy()->getRace() << "\n";

        us_   = Broodwar->self();
        them_ = Broodwar->enemy();

        // Initialize state and action for every unit
        for(auto & u : us_->getUnits()) {
            UnitState uState(u);
            unitStates_[u->getID()] = uState;
            unitStates_[u->getID()].setNoDraw();
        }

        startup_ = false;
    }


    void DesolatorModule::onEnd(bool /* isWinner */ ) {
        ++completedMatches_;
        std::cout << "Completed matches: " << completedMatches_ << "\n";
        {
            std::ofstream tFile("transitions_numbers.data");

            if (tFile) tFile << table_;
            else         log_ << "We could not save the new experience to file.\n";
        }
        if (!(completedMatches_ % 50)) {
            std::cout << "-- Creating MDP model...\n";
            AIToolbox::MDP::RLModel mdp(table_, true);
            std::cout << "-- Solving data with Value Iteration...\n";
            AIToolbox::MDP::ValueIteration solver;
            auto solution = solver(mdp);
            std::cout << "-- Creating policy...\n";
            AIToolbox::MDP::QGreedyPolicy p(std::get<2>(solution));
            std::cout << "-- Saving policy...\n";
            std::string filename = "policy_";
            filename += std::to_string(completedMatches_);
        
            std::ofstream pFile(filename);
            pFile << p;
            std::cout << "-- Saving respective experience for policy...\n";
            filename = "experience_";
            filename += std::to_string(completedMatches_);

            std::ofstream tFile(filename);
            tFile << table_;

            std::cout << "-- Done.\n";
        }

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
