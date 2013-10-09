#pragma once
#include <BWAPI.h>
#include <map>
#include <array>
#include <time.h>
#include <fstream>

#include "UnitState.h"
#include "Logger.h"
#include "Table.h"
#include "Policy.h"

// Remember not to use "Broodwar" in any global class constructor!

class DesolatorModule : public BWAPI::AIModule
{
public:
    typedef std::map<int, UnitState> UnitStates;

    DesolatorModule();

    // Virtual functions for callbacks, leave these as they are.
    virtual void onStart();
    virtual void onEnd(bool isWinner);
    virtual void onFrame();
    virtual void onSendText(std::string text);
    virtual void onReceiveText(BWAPI::Player player, std::string text);
    virtual void onPlayerLeft(BWAPI::Player player);
    virtual void onNukeDetect(BWAPI::Position target);
    virtual void onUnitDiscover(BWAPI::Unit unit);
    virtual void onUnitEvade(BWAPI::Unit unit);
    virtual void onUnitShow(BWAPI::Unit unit);
    virtual void onUnitHide(BWAPI::Unit unit);
    virtual void onUnitCreate(BWAPI::Unit unit);
    virtual void onUnitDestroy(BWAPI::Unit unit);
    virtual void onUnitMorph(BWAPI::Unit unit);
    virtual void onUnitRenegade(BWAPI::Unit unit);
    virtual void onSaveGame(std::string gameName);
    virtual void onUnitComplete(BWAPI::Unit unit);
    // Everything below this line is safe to modify.
private:
    BWAPI::Player us;
    BWAPI::Player them;

    // STATE VARIABLES
    UnitStates unitStates;

    // STATE METHODS
    void updateUnitState(BWAPI::Unit unit, const BWAPI::Unitset & alliedUnits, const BWAPI::Unitset & enemyUnits, bool alsoState = false);

    // FEEDBACK STUFF
    void evaluateText(std::string text);
    bool feedback;

    Logger log_;
    Table table_;
    Policy policy_;
};