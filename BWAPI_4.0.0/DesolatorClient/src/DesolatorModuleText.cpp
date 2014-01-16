#include <Desolator/DesolatorModule.hpp>

using namespace BWAPI;
using namespace Filter;

namespace Desolator {
    void DesolatorModule::onSendText(std::string text) {
        // Send the text to the game if it is not being processed.
        Broodwar->sendText("%s", text.c_str());

        if (text == "r") {
            feedback_ = !feedback_;

            if ( feedback_ )
                Broodwar->setLocalSpeed(50);
            else
                Broodwar->setLocalSpeed(0);
            return;
        }
        else if ( text == "q" ) {
            Broodwar->leaveGame();
        }
        else if ( text == "d" ) {
            Broodwar->restartGame();
        }
        else if(feedback_) {
            try {
                int speed = std::stoi(text);
                Broodwar->setLocalSpeed(speed);
                return;
            }
            catch (const std::invalid_argument& error) {
                Broodwar->sendText("Unable to parse text");
            }
        }
    }
}