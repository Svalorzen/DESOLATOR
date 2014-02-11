#include <Desolator/DesolatorModule.hpp>

#include <Desolator/Random.hpp>

using namespace BWAPI;
using namespace Filter;

namespace Desolator {
    void DesolatorModule::drawTargets() {
        for ( auto u : us_->getUnits() ) {
            auto & GS = unitStates_[u->getID()];
            if ( ! GS.draw ) continue;

            Broodwar->drawLineMap(u->getPosition(), GS.drawTargetPosition, BWAPI::Color(0,255,0));
        }
    }

    void DesolatorModule::drawIDs() {
        for ( auto u : us_->getUnits() ) {
            auto p = u->getPosition(); p.y -= 30;
            auto & GS = unitStates_[u->getID()];
            Broodwar->drawTextMap(p, "%s", 
                GS.idtext.c_str());
        }
    }
}
