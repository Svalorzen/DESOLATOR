#ifndef AI_HEADER_FILE
#define AI_HEADER_FILE

#include <BWAPI.h>
#include "Logger.h"
#include "DesolatorModule.h"

class AI {
	public:
		static BWAPI::Position explore(int mapWidth, int mapHeight);
		static BWAPI::PositionOrUnit attack(BWAPI::Unit unit, const BWAPI::Unitset & allies, const BWAPI::Unitset & enemies, DesolatorModule::UnitStates & u);
		static BWAPI::Position flee(BWAPI::Unit unit, const BWAPI::Unitset & friends, const BWAPI::Unitset & enemies, DesolatorModule::UnitStates & u);

	private:
		static Logger log_;
};

#endif