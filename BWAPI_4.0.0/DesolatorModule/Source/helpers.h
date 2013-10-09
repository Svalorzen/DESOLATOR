#include <BWAPI.h>
#include "DesolatorModule.h"

const int STANDARD_SPEED_FPS = 19;

double copysign(double a, double b);

/* AUXILLARY FUNCTIONS */
// BWAPI BOOSTS
int getActualWeaponRange(BWAPI::Unit unit);
bool isMelee(BWAPI::Unit unit);
int getOptimizedWeaponRange(BWAPI::Unit unit);
BWAPI::TilePosition convertToTile(BWAPI::Position);

// DRAWING FUNCTIONS
void drawHeatMap(BWAPI::Player us, BWAPI::Player enemy);
void drawState(const DesolatorModule::UnitStates & states);