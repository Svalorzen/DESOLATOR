#include <Desolator/DesolatorModule.hpp>

#include <Desolator/Random.hpp>
#include <AIToolbox/MDP/RLModel.hpp>
#include <AIToolbox/MDP/ValueIteration.hpp>
#include <AIToolbox/MDP/QGreedyPolicy.hpp>
#include <AIToolbox/MDP/Utils.hpp>

#include <iostream>

using namespace BWAPI;
using namespace Filter;

namespace Desolator {

    DesolatorModule::DesolatorModule() :
                                log_("desolator.log", std::fstream::out | std::fstream::app),
                                S(MDPState::getNumberOfStates()), A(2),
                                table_ (S, A),
                                loadedPolicy_(S, A),
                                model_(table_, false),
                                solver_(model_, 0.9, 0.05, 200),
                                policy_(solver_.getQFunction())
    {
        feedback_ = true;
        currentSpeed_ = 50;
        completedMatches_ = 0;

        std::ifstream tFile("transitions_numbers.data"), pFile("policy.data");

        if ( !tFile || !( tFile >> table_ ))
            Broodwar->printf("###! COULD NOT LOAD TRANSITION NUMBERS !###");

        if ( !pFile || !( pFile >> loadedPolicy_ )) {
            usingPolicy_ = false; 
            Broodwar->printf("###! COULD NOT LOAD POLICY !###");
        }
        else usingPolicy_ = true;               
    }

    void DesolatorModule::onStart() {
        // We setup the environment only if this is not a replay
        if ( Broodwar->isReplay() ) return;

        // Enable the UserInput flag, which allows us to control the bot and type messages.
        Broodwar->enableFlag(Flag::UserInput);

        // Set the command optimization level so that common commands can be grouped
        // and reduce the bot's APM (Actions Per Minute).
        Broodwar->setCommandOptimizationLevel(2);

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
    }


    void DesolatorModule::onEnd(bool isWinner ) {
        ++completedMatches_;
        std::cout << ( isWinner ? "#### WON  ####\n" : "#### LOST ####\n";
        std::cout << "Completed matches: " << completedMatches_ << "\n";
        {
          //  std::ofstream tFile("transitions_numbers.data");

         //   if (tFile) tFile << table_;
         //   else         log_ << "We could not save the new experience to file.\n";
        }
        if (!(completedMatches_ % 10)) {
            std::cout << "-- Saving policy...\n";
            {
                std::string filename = "policy_";
                filename += std::to_string(completedMatches_);
            
                std::ofstream pFile(filename);
                pFile << policy_;
            }
            std::cout << "-- Saving respective experience for policy...\n";
            {
                std::string filename = "experience_";
                filename += std::to_string(completedMatches_);

                std::ofstream tFile(filename);
                tFile << table_;
            }
            std::cout << "-- Saving policy solved with ValueIteration...\n";
            {
                std::string vi = "VI_policy_";
                vi += std::to_string(completedMatches_);
                
                auto solution = AIToolbox::MDP::ValueIteration()(model_);
                AIToolbox::MDP::QGreedyPolicy pp(std::get<2>(solution));
                std::ofstream ppfile(vi); ppfile << pp;
            }
            std::cout << "-- Done.\n";
        }
        std::cout << "#######################\n";
        Broodwar->restartGame();
    }

    void DesolatorModule::onUnitDestroy(BWAPI::Unit unit) {
        if( unit->getPlayer() == us_ ) {
            auto & GS = unitStates_[unit->getID()];
            int maxHealth =  unit->getType().maxShields() + unit->getType().maxHitPoints();
            double penalty = - ( static_cast<double>(maxHealth) )*1;

            table_.record(GS.state, GS.state, GS.lastStrategy, penalty);
            model_.sync(GS.state, GS.lastStrategy);
            solver_.stepUpdateQ(GS.state, GS.lastStrategy);
        }
    }
}
