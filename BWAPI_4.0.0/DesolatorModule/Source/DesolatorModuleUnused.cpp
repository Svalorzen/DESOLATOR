#include "DesolatorModule.h"

using namespace BWAPI;
using namespace Filter;

void DesolatorModule::onNukeDetect(BWAPI::Position target){}
void DesolatorModule::onUnitDiscover(BWAPI::Unit unit){}
void DesolatorModule::onUnitEvade(BWAPI::Unit unit){}
void DesolatorModule::onUnitShow(BWAPI::Unit unit){}
void DesolatorModule::onUnitHide(BWAPI::Unit unit){}
void DesolatorModule::onUnitCreate(BWAPI::Unit unit){}
void DesolatorModule::onUnitMorph(BWAPI::Unit unit){}
void DesolatorModule::onUnitRenegade(BWAPI::Unit unit){}
void DesolatorModule::onUnitComplete(BWAPI::Unit unit){}
void DesolatorModule::onPlayerLeft(BWAPI::Player player){}

void DesolatorModule::onSaveGame(std::string gameName) {
    Broodwar << "The game was saved to \"" << gameName << "\"" << std::endl;
}