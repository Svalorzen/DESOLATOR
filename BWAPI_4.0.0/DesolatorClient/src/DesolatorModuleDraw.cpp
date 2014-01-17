#include <Desolator/DesolatorModule.hpp>

#include <Desolator/Random.hpp>

using namespace BWAPI;
using namespace Filter;

namespace Desolator {
    void DesolatorModule::drawTargets() {
        for ( auto u : us_->getUnits() ) {
            auto & GS = unitStates_[u->getID()];
            if ( ! GS.draw ) continue;

            auto & target = GS.drawTargetPosition;

            if ( target.isPosition() )
                Broodwar->drawLineMap(u->getPosition(), target.getPosition(), BWAPI::Color(0,255,0));
            else
                Broodwar->drawLineMap(u->getPosition(), target.getUnit()->getPosition(), BWAPI::Color(0,255,0));
        }
    }

    void DesolatorModule::drawIDs() {
        for ( auto u : us_->getUnits() ) {
            auto p = u->getPosition(); p.y -= 30;
            Broodwar->drawTextMap(p, "%d", u->getID());
        }
    }
}
