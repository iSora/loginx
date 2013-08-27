// This file is part of the loginx project
//
// Copyright (c) 2013 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#pragma once
#include "pam.h"
#include "pw.h"

enum { MAX_PW_LEN = 64 };

unsigned LoginBox (acclist_t al, char* password);
