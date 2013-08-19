#include "pw.h"
#include <pwd.h>

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
