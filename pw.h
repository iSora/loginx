// This file is part of the loginx project
//
// Copyright (c) 2013 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#pragma once
#include "config.h"

struct account {
    uid_t	uid;
    gid_t	gid;
    unsigned	ltime;
    char*	name;
    char*	dir;
    char*	shell;
};

typedef const struct account* const*	acclist_t;

acclist_t ReadAccounts (void);
void ReadLastlog (void);
unsigned NAccounts (void);
