#pragma once
#include "config.h"

struct account {
    uid_t	uid;
    gid_t	gid;
    char*	name;
    char*	dir;
    char*	shell;
};

typedef const struct account* const*	acclist_t;

acclist_t ReadAccounts (void);
unsigned NAccounts (void);
