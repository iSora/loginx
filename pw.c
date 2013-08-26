// This file is part of the loginx project
//
// Copyright (c) 2013 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "pw.h"
#include <pwd.h>
#include <utmp.h>
#include <fcntl.h>
#include <sys/stat.h>

static struct account** _accts = NULL;
static unsigned _naccts = 0;

static void CleanupAccounts (void)
{
    if (!_accts)
	return;
    for (unsigned i = 0; i < _naccts; ++i) {
	xfree (_accts[i]->name);
	xfree (_accts[i]->dir);
	xfree (_accts[i]->shell);
	xfree (_accts[i]);
    }
    xfreenull (_accts);
}

static bool CanLogin (const struct passwd* pw)
{
    return (pw->pw_shell && strcmp(pw->pw_shell, "/bin/false") && strcmp(pw->pw_shell,"/sbin/nologin"));
}

acclist_t ReadAccounts (void)
{
    unsigned nac = 0;
    setpwent();
    for (struct passwd* pw; (pw = getpwent());)
	nac += CanLogin (pw);
    _accts = (struct account**) xmalloc ((nac+1)*sizeof(struct account*));
    atexit (CleanupAccounts);
    nac = 0;
    setpwent();
    for (struct passwd* pw; (pw = getpwent());) {
	if (!CanLogin (pw))
	    continue;
	_accts[nac] = (struct account*) xmalloc (sizeof(struct account));
	_accts[nac]->uid = pw->pw_uid;
	_accts[nac]->gid = pw->pw_gid;
	_accts[nac]->name = strdup (pw->pw_name);
	_accts[nac]->dir = strdup (pw->pw_dir);
	_accts[nac]->shell = strdup (pw->pw_shell);
	++nac;
    }
    _naccts = nac;
    return ((acclist_t) _accts);
}

unsigned NAccounts (void)
{
    return (_naccts);
}

void ReadLastlog (void)
{
    int fd = open ("/var/log/lastlog", O_RDONLY);
    if (fd < 0)
	return;
    struct stat st;
    if (fstat (fd, &st) == 0) {
	const unsigned maxuid = st.st_size / sizeof(struct lastlog);
	for (unsigned i = 0; i < _naccts; ++i)
	    if (_accts[i]->uid < maxuid)
		pread (fd, &_accts[i]->ltime, sizeof(_accts[i]->ltime), _accts[i]->uid * sizeof(struct lastlog));
    }
    close (fd);
}
