#include <Desolator/DesolatorModule.hpp>

#include <Desolator/Random.hpp>

using namespace BWAPI;
using namespace Filter;

namespace Desolator {
    void DesolatorModule::dispatch( const BWAPI::Event & e) {
        switch ( e.getType() ) {
            case EventType::MatchStart:
                onStart();
                break;
            case EventType::MatchFrame:
                onFrame();
                break;
            case EventType::MatchEnd:
                onEnd(e.isWinner());
                break;
            case EventType::UnitDestroy:
                onUnitDestroy(e.getUnit());
                break;
            case EventType::SendText:
                onSendText(e.getText());
                break;
            default:;
        }
    }
}
