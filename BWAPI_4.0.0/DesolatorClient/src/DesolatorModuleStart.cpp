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
                                solver_(model_, 0.95, 0.05, 200),
                                policy_(solver_.getQFunction())
    {
        feedback_ = false; explfeedback_ = true;
        currentSpeed_ = 0;
        completedMatches_ = 0;
        exploration_ = 0.9;

        std::ifstream tFile("transitions_numbers.data"), pFile("policy.data");

        if (!tFile || !(tFile >> table_))
            Broodwar->printf("###! COULD NOT LOAD TRANSITION NUMBERS !###");
        else {
            model_.sync();
            // solver_.sync();
        }

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

        episodeSteps_ = 0;
        episodeReward_ = 0.0;
    }

    void DesolatorModule::onEnd(bool isWinner ) {
        ++completedMatches_;

        solver_.batchUpdateQ();
        std::cout << "Steps done: " << episodeSteps_ << "\n";

        std::cout << ( isWinner ? "#### WON  ####\n" : "#### LOST ####\n" );
        std::cout << "Completed matches: " << completedMatches_ << "\n";
        {
            std::ofstream tFile("reward.data", std::ios::app);
            tFile << isWinner << " " << episodeReward_ << '\n';
        }
        if (!(completedMatches_ % 10)) {
            // Solve policy with value iteration.
            auto solution = AIToolbox::MDP::ValueIteration(0.95)(model_);
            AIToolbox::MDP::QGreedyPolicy pp(std::get<2>(solution));
            // Check that policies are the same
            bool same = true;
            for (size_t s = 0; s < S; s++) {
                for (size_t a = 0; a < A; a++) {
                    if (pp.getActionProbability(s, a) != policy_.getActionProbability(s, a)) {
                        same = false;
                        goto out;
                    }
                }
            }
        out:
            if (same) { std::cout << "Policies are the same, continuing..\n"; }
            else {
                std::cout << "Policies are not the same! -- Saving policy...\n";
                {
                    std::string filename = "policy_";
                    filename += std::to_string(completedMatches_);

                    std::ofstream pFile(filename);
                    pFile << policy_;
                }
                std::cout << "-- Saving policy solved with ValueIteration...\n";
                {
                    std::string vi = "VI_policy_";
                    vi += std::to_string(completedMatches_);

                    std::ofstream ppfile(vi); ppfile << pp;
                }
                std::cout << "-- Done.\n";
            }
        }
        std::cout << "#######################\n";
        Broodwar->restartGame();
    }    
}
