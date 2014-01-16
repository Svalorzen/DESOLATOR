#ifndef DESOLATOR_AI_HEADER_FILE
#define DESOLATOR_AI_HEADER_FILE

#include <BWAPI.h>
#include <Desolator/DesolatorModule.hpp>

namespace Desolator {
    namespace AI {
		BWAPI::Position explore();
		BWAPI::PositionOrUnit attack(BWAPI::Unit unit, const BWAPI::Unitset & allies, const BWAPI::Unitset & enemies, DesolatorModule::UnitStates & u);
		BWAPI::Position flee(BWAPI::Unit unit, const BWAPI::Unitset & friends, const BWAPI::Unitset & enemies, DesolatorModule::UnitStates & u);
    }
}

#endif
