// This file is part of the loginx project
//
// Copyright (c) 2013 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#pragma once
#include "config.h"

void PamOpen (void);
void PamClose (void);
const char* PamLogin (void);
void PamLogout (void);
