#include <iostream>
#include <windows.h>

#include <BWAPI.h>
#include <BWAPI/Client.h>

#include <Desolator/DesolatorModule.hpp>

using namespace BWAPI;

void reconnect() {
    while( !BWAPIClient.connect() )
        Sleep(1000);
}

int main() {
    std::cout << "Connecting..." << std::endl;
    reconnect();

    Desolator::DesolatorModule module;

    while( true ) {
        std::cout << "Waiting to enter match.." << std::endl;
        while ( !Broodwar->isInGame() ) {
            BWAPI::BWAPIClient.update();
            if (!BWAPI::BWAPIClient.isConnected()) {
                std::cout << "Reconnecting..." << std::endl;
                reconnect();
            }
        }
        std::cout << "Starting match!" << std::endl;

        while( Broodwar->isInGame() ) {
            for( auto e = Broodwar->getEvents().begin(); e != Broodwar->getEvents().end(); ++e) {
                module.dispatch(*e);
            }

            // Debug draws
            //Broodwar->drawTextScreen(300,0,"FPS: %f",Broodwar->getAverageFPS());
            //module.drawTargets();
            //module.drawIDs();

            BWAPI::BWAPIClient.update();
            if (!BWAPI::BWAPIClient.isConnected()) {
                std::cout << "Reconnecting..." << std::endl;
                reconnect();
            }
        }
        std::cout << "Game ended" << std::endl;
    }
    std::cout << "Press ENTER to continue..." << std::endl;
    std::cin.ignore();
    return 0;
}
