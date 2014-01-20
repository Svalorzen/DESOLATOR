#include <Desolator/DesolatorModule.hpp>

using namespace BWAPI;
using namespace Filter;

namespace Desolator {
    void DesolatorModule::onSendText(std::string text) {
        // Send the text to the game if it is not being processed.

        if (text == "f") {
            feedback_ = !feedback_;
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
                currentSpeed_ = speed;
                return;
            }
            catch (const std::invalid_argument& error) {
                Broodwar->sendText("Unable to parse text");
            }
        }
    }
}
