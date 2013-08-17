#include "pam.h"
#include <pwd.h>
#include <signal.h>
#include <unistd.h>

//{{{ Signal handling --------------------------------------------------

#define S(s) (1u<<(s))
enum {
    sigset_Quit	= S(SIGINT)|S(SIGQUIT)|S(SIGTERM)|S(SIGPWR)|
		S(SIGILL)|S(SIGABRT)|S(SIGBUS)|S(SIGFPE)|
		S(SIGSYS)|S(SIGSEGV)|S(SIGALRM)|S(SIGXCPU),
    qc_ShellSignalQuitOffset = 128
};

static void OnSignal (int s)
{
    static bool s_DoubleSignal = false;
    if (!s_DoubleSignal) {
	s_DoubleSignal = true;
	psignal (s, "[S] Error");
	exit (s+qc_ShellSignalQuitOffset);
    }
    _exit (s+qc_ShellSignalQuitOffset);
}

static void InstallCleanupHandlers (void)
{
    for (unsigned i = 0; i < NSIG; ++i)
	if (S(i) & sigset_Quit)
	    signal (i, OnSignal);
}
#undef S

//}}}-------------------------------------------------------------------
//{{{ Shell

static void LaunchShell (const char* username)
{
    struct passwd* pwn = getpwnam (username);
    fprintf (stderr, "The user [%s,%hu,%hu] has been authenticated and `logged in'\n", username, pwn->pw_uid, pwn->pw_gid);

    setuid (pwn->pw_uid);
    setgid (pwn->pw_gid);
    setsid();
    setpgrp();

    /* this is always a really bad thing for security! */
    system("/bin/sh");
}

//}}}-------------------------------------------------------------------

int main (void)
{
    InstallCleanupHandlers();
    PamOpen();
    const char* username = PamLogin();
    LaunchShell (username);
    PamLogout();
    PamClose();
    return (0);
}
