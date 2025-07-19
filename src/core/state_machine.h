#ifndef CORE_STATE_MACHINE_H
#define CORE_STATE_MACHINE_H

#include <Arduino.h>
#include "logger.h"

template<typename StateEnum>
class StateMachine {
public:
    StateMachine(StateEnum initialState) 
        : currentState(initialState), previousState(initialState), 
          stateStartTime(millis()), lastUpdateTime(0) {}
    
    virtual ~StateMachine() = default;
    
    virtual void update() {
        unsigned long now = millis();
        lastUpdateTime = now;
        handleState();
    }
    
    StateEnum getCurrentState() const {
        return currentState;
    }
    
    StateEnum getPreviousState() const {
        return previousState;
    }
    
    unsigned long getTimeInCurrentState() const {
        return millis() - stateStartTime;
    }
    
protected:
    virtual void handleState() = 0;
    virtual const char* getStateName(StateEnum state) = 0;
    
    void transition(StateEnum newState) {
        if (newState != currentState) {
            LOG_DEBUGF("%s: %s -> %s", 
                      getStateMachineName(),
                      getStateName(currentState), 
                      getStateName(newState));
            
            onExitState(currentState);
            previousState = currentState;
            currentState = newState;
            stateStartTime = millis();
            onEnterState(newState);
        }
    }
    
    virtual void onEnterState(StateEnum state) {}
    virtual void onExitState(StateEnum state) {}
    virtual const char* getStateMachineName() = 0;
    
    bool hasBeenInStateFor(unsigned long duration) const {
        return getTimeInCurrentState() >= duration;
    }
    
    StateEnum currentState;
    StateEnum previousState;
    unsigned long stateStartTime;
    unsigned long lastUpdateTime;
};

#endif