#include "DesolatorModule.h"

using namespace BWAPI;
using namespace Filter;

void DesolatorModule::onSendText(std::string text)
{
    // Send the text to the game if it is not being processed.
    Broodwar->sendText("%s", text.c_str());
    this->evaluateText(text);
    // Make sure to use %s and pass the text as a parameter,
    // otherwise you may run into problems when you use the %(percent) character!
}

void DesolatorModule::onReceiveText(BWAPI::Player player, std::string text)
{
    // Parse the received text
    Broodwar << player->getName() << " said \"" << text << "\"" << std::endl;
}

void DesolatorModule::evaluateText(std::string text)
{
    if(text == "r")
    {
        this->feedback = !feedback;

         if ( feedback )
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
    else if(this->feedback)
    {
        try
        {
            int speed = std::stoi(text);
			Broodwar->setLocalSpeed(speed);
            return;
        } catch (const std::invalid_argument& error)
        {
            Broodwar->sendText("Unable to parse text");
        }
    }
}
