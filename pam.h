#pragma once
#include "config.h"

void PamOpen (void);
void PamClose (void);
const char* PamLogin (void);
void PamLogout (void);
