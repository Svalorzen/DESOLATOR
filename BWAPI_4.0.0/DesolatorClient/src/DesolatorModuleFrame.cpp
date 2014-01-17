#include <Desolator/DesolatorModule.hpp>

#include <Desolator/AI.hpp>
#include <Desolator/BWAPIHelpers.hpp>
#include <Desolator/Random.hpp>

#include <iostream>

using std::cout;
using std::endl;

using namespace BWAPI;
using namespace Filter;

namespace Desolator {

    void DesolatorModule::onFrame() {
        auto & ourUnits    = us_->getUnits();
        auto & theirUnits  = them_->getUnits();

        // Return if the game is a replay or is paused
        if ( Broodwar->isReplay() || Broodwar->isPaused() || !Broodwar->self() )
            return;

        // Prevent spamming by only running our onFrame once every number of latency frames.
        // Latency frames are the number of frames before commands are processed.
        if ( Broodwar->getFrameCount() % Broodwar->getLatencyFrames() != 0 )
            return;

        /************************/
        /**** DESOLATOR CODE ****/
        /************************/

        if ( theirUnits.empty() ) {
            cout << "Exploring" << endl;
            for ( auto u : ourUnits )
                unitStates_[u->getID()].setNoDraw();

            // Only move when idle.
            if(!ourUnits.begin()->isMoving()) {
                ourUnits.move(AI::explore());
            }
        }
        else {
            cout << "New action frame" << endl;
            // This updates the game state for each unit we have ( not the MDP state though )
            for ( auto u : ourUnits )
                updateUnitState(u);
            cout << "Basic update completed" << endl;
            for ( auto u : ourUnits )
                shareKnowledge(u);
            cout << "Shares completed" << endl;
            // Now that the observations of the units are correct, we select the actions that we want the units to perform.
            for ( auto u : ourUnits ) {
                auto & GS = unitStates_[u->getID()];
                cout << "Executing unit " << u->getID() << endl;

                // Check when the units moved a tile
                bool moved = GS.updatePosition(u->getTilePosition());

                // Hack to avoid units stopping while actually moving..
                if ( !moved && GS.lastAction == Action::Move && GS.lastPerfectPosition == u->getPosition() )
                    GS.notMovingTurns++;
                else {
                    GS.lastPerfectPosition = u->getPosition();
                    GS.notMovingTurns = 0;
                }
                cout << "- unit movement updates done" << endl;
                // If our personal tick ended
                if ( u->isIdle() || moved || GS.lastAction == Action::None ||
                   ( GS.lastAction == Action::Attack && ! GS.isStartingAttack &&  ! u->isAttackFrame() ) ) {
                    cout << "- UNIT TICK" << endl;
                    updateUnitMDPState(u);
                    cout << "- UNIT MDP updated state" << endl;

                    Strategy strategy;
                    Action action;

                    int selectedAction = 0;
                    //cout << "We use policy? " << usingPolicy_ << endl;
                    // We check if we follow the policy or we go full random
                    if ( usingPolicy_ ) selectedAction = policy_.sampleAction(GS.state);
                    else selectedAction = RandomInt::get(0,1);
                    //selectedAction = 1;
                    cout << "Got action: " << selectedAction << endl;
                    switch ( selectedAction ) {
                        case 1: { // We flee
                            strategy = Strategy::Flee;
                            cout << "Flee!" << endl;
                            // Obtain where to flee
                            auto position = AI::flee(u, ourUnits, theirUnits, unitStates_);
                            cout << "Got Position." << endl;
                            if ( convertToTile(position) != u->getTilePosition() ) {
                                u->move   ( position );
                                GS.setDraw( position );
                                action = Action::Move;
                                cout << "MOVE: different so we draw." << endl;
                            }
                            else {
                                GS.setNoDraw();
                                action = Action::None;
                                cout << "NONE: same we don't draw." << endl;
                            }
                            break;
                        }
                        default: { // We attack
                            strategy = Strategy::Fight;
                            action   = Action::Attack;
                            cout << "Fight" << endl;
                            // Obtain who to attack or where to go
                            PositionOrUnit target = AI::attack(u, ourUnits, theirUnits, unitStates_);

                            if ( target.isPosition() ) {
                                if ( convertToTile(target.getPosition()) != u->getTilePosition() ) {
                                    u->move   ( target.getPosition() );
                                    GS.setDraw( target.getPosition() );
                                    action = Action::Move;
                                    cout << "Moving" << endl;
                                }
                                else {
                                    GS.setNoDraw();
                                    action = Action::None;
                                    cout << "Nothing.." << endl;
                                }
                            }
                            // This check is to avoid breaking Starcraft when we spam attack command
                            else {
                                if ( u->getOrder() != Orders::AttackUnit || unitStates_[u->getID()].lastTarget != target.getUnit() ) {
                                    u->attack(target.getUnit());

                                    GS.isStartingAttack = true;
                                    GS.lastTarget = target.getUnit();

                                    GS.setDraw(target);
                                    cout << "Attack" << endl;
                                }
                            }
                        }
                    }
                    //cout << "UNIT action selected" << strategy << endl;
                    // Update Action taken
                    GS.lastStrategy = strategy;
                    GS.lastAction = action;

                } // End, personal tick
                // End of hack, here we stop the units so I guess internally it resets so we can move it again.
                else if ( GS.notMovingTurns == 3 ) {
                    //Broodwar->printf("Unit %d triggered an hack", u->getID());

                    log_ << "HACK TRIGGERED" << "\n";
                    log_ << "Action: " << GS.lastAction << " -- Strategy: " << GS.lastStrategy << "\n";
                    log_ << "Order:  " << u->getOrder() << " -- Position: " << u->getPosition() << " -- Target Position: " << u->getTargetPosition() << "\n";
                    log_ << "Diff = "       << u->getPosition() - u->getTargetPosition() << "\n";
                    log_ << "Unit Tile: "   << convertToTile(u->getPosition())  << " -- Target Tile: " << convertToTile(u->getTargetPosition()) << "\n";

                    u->stop();
                    GS.setNoDraw();
                }
            } // closure: unit iterator
        } // closure: We have enemies
    }

}
