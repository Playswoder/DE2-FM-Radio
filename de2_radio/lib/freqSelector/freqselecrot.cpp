#include "FreqSelector.h"

FreqSelector* FreqSelector::instance = nullptr;

// ISR for PORTD pin change (PCINT2 group)
ISR(PCINT2_vect)
{
    FreqSelector::handleInterrupt();
}
