#include <Desolator/DesolatorModule.hpp>

using namespace BWAPI;
using namespace Filter;

namespace Desolator {
    void DesolatorModule::onSendText(std::string text) {
        // Send the text to the game if it is not being processed.
        //Broodwar->sendText("%s", text.c_str());

        if (text == "f") {
            feedback_ = !feedback_;
            if (feedback_) explfeedback_ = false;

            Broodwar->printf("%s", feedback_ ? "Feedback ACTIVE" : "Feedback DISABLED");
        }
        else if (text == "e") {
            explfeedback_ = !explfeedback_;
            if (explfeedback_) feedback_ = false;
        }
        else if ( text == "q" ) {
            Broodwar->leaveGame();
        }
        else if ( text == "r" ) {
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
                Broodwar->printf("Unable to parse text");
            }
        }
        else if (explfeedback_) {
            try {
                double exp = std::stod(text);
                exploration_ = std::min(1.0, exp);
                Broodwar->printf("New exploration value: %f", exploration_);
            }
            catch (const std::invalid_argument& error) {
                Broodwar->printf("Unable to parse text");
            }
        }
    }
}
