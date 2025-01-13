#include "InterpDelay.hpp"

float DSY_SDRAM_BSS sdramData[50][144000];
unsigned int count = 0;
bool triggerClear;
float clearPopCancelValue = 1.;
float hold = 0.;