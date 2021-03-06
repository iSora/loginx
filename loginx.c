// This file is part of the loginx project
//
// Copyright (c) 2013 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "ui.h"
#include "pw.h"
#include <pwd.h>
#include <signal.h>
#include <time.h>
#include <locale.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/kd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

//----------------------------------------------------------------------

static void ExitWithError (const char* fn) __attribute__((noreturn));
static void ExitWithMessage (const char* msg) __attribute__((noreturn));

static void InitEnvironment (void);
static int  OpenTTYFd (const char* ttypath);
static void OpenTTY (const char* ttypath);
static void ResetTerminal (void);

//----------------------------------------------------------------------

static pid_t _pgrp = 0;

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
	alarm(1);
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
//{{{ Utility functions

void* xmalloc (size_t n)
{
    void* p = calloc (1, n);
    if (!p) {
	puts ("Error: out of memory");
	exit (EXIT_FAILURE);
    }
    return (p);
}

void xfree (void* p)
{
    if (p)
	free (p);
}

static void ExitWithError (const char* fn)
{
    printf ("Error: %s: %s\n", fn, strerror(errno));
    exit (EXIT_FAILURE);
}

static void ExitWithMessage (const char* msg)
{
    printf ("Error: %s\n", msg);
    exit (EXIT_FAILURE);
}

#if 0
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
#endif

int main (void)
{
    char ttypath [16];
    ttyname_r (STDIN_FILENO, ttypath, sizeof(ttypath));
    InstallCleanupHandlers();
    InitEnvironment();
    OpenTTY (ttypath);
    ResetTerminal();
#if 0
    acclist_t al = ReadAccounts();
    ReadLastlog();
    char password [MAX_PW_LEN];
    unsigned ali = LoginBox (al, password);
    PamOpen();
    const char* username = PamLogin();
    LaunchShell (username);
    PamLogout();
    PamClose();
    time_t ltime = al[ali]->ltime;
    printf ("Logging in user '%s', password '%s', lastlog %s", al[ali]->name, password, ctime(&ltime));
#endif
    return (0);
}

static void InitEnvironment (void)
{
    if ((_pgrp = setsid()) < 0)
	ExitWithError ("setsid");

    for (unsigned f = STDERR_FILENO+1, fend = getdtablesize(); f < fend; ++f)
	close (f);
}

static int OpenTTYFd (const char* ttypath)
{
    // O_NOCTTY is needed to use TIOCSCTTY, which will steal the tty control from any other pgrp
    int fd = open (ttypath, O_RDWR| O_NOCTTY| O_NONBLOCK, 0);
    if (fd < 0)
	ExitWithError ("open");

    if (!isatty (fd))
	ExitWithMessage (LOGINX_NAME " must run on a tty");
    struct stat ttyst;
    if (fstat (fd, &ttyst))
	ExitWithError ("fstat");
    if (!S_ISCHR(ttyst.st_mode))
	ExitWithMessage ("the tty is not a character device");

    // Establish as session leader and tty owner
    //if (_pgrp != tcgetsid(fd))
	//if (ioctl (fd, TIOCSCTTY, 1) < 0)
	    //ExitWithError ("failed to take tty control");
    return (fd);
}

static void OpenTTY (const char* ttypath)
{
    // First time open tty to make it controlling terminal and vhangup
    int fd = OpenTTYFd (ttypath);
    // Now close it and vhangup to eliminate all other processes on this tty
    close (fd);
    for (unsigned f = STDIN_FILENO; f <= STDERR_FILENO; ++f)
	close (f);
    signal (SIGHUP, SIG_IGN);	// To not be killed by vhangup
    //vhangup();
    signal (SIGHUP, OnSignal);	// To be killed by init

    // Reopen the tty and establish standard fds
    fd = OpenTTYFd (ttypath);
    if (fd != STDIN_FILENO)	// All fds, except syslog, must be closed at this point
	ExitWithError ("open stdin");
    if (dup(fd) != STDOUT_FILENO || dup(fd) != STDERR_FILENO)
	ExitWithError ("open stdout");

    //if (_pgrp != tcgetsid(fd))
	//if (ioctl (fd, TIOCSCTTY, 1) < 0)
	    //ExitWithError ("failed to take tty control");

    //if (tcsetpgrp (STDIN_FILENO, _pgrp))
	//ExitWithError ("tcsetpgrp");
}

static void ResetTerminal (void)
{
    //
    // If keyboard is in raw state, unset it
    //
    int kbmode = K_XLATE;
    if (0 == ioctl (STDIN_FILENO, KDGKBMODE, &kbmode))
	if (kbmode != K_XLATE && kbmode != K_UNICODE)
	    ioctl (STDIN_FILENO, KDSKBMODE, kbmode = K_XLATE);
    setlocale (LC_ALL, kbmode == K_XLATE ? "C" : "C.UTF-8");

    //
    // Reset normal terminal settings.
    //
    struct termios ti;
    if (0 > tcgetattr (STDIN_FILENO, &ti))
	ExitWithError ("tcgetattr");
    // Overwrite known fields with sane defaults
    ti.c_iflag = ICRNL| IXON| BRKINT| IUTF8;
    ti.c_oflag = ONLCR| OPOST;
    ti.c_cflag = HUPCL| CREAD| CS8| B38400;
    ti.c_lflag = ISIG| ICANON| ECHO| ECHOE| ECHOK| ECHOCTL| ECHOKE| IEXTEN;
    #define TCTRL(k) k-('A'-1)
    static const cc_t c_CtrlChar[NCCS] = {
	[VINTR]		= TCTRL('C'),
	[VQUIT]		= TCTRL('\\'),
	[VERASE]	= 127,
	[VKILL]		= TCTRL('U'),
	[VEOF]		= TCTRL('D'),
	[VMIN]		= 1,
	[VSUSP]		= TCTRL('Z'),
	[VREPRINT]	= TCTRL('R'),
	[VDISCARD]	= TCTRL('O'),
	[VWERASE]	= TCTRL('W'),
	[VLNEXT]	= TCTRL('V'),
    };

    memcpy (ti.c_cc, c_CtrlChar, sizeof(ti.c_cc));

    // Remove this when non-vc terminals are not supported
    const char* termname = getenv("TERM");
    if (!termname || 0 != strcmp (termname, "linux"))
	ti.c_cc[VERASE] = TCTRL('H');
    #undef TCTRL

    if (0 > tcsetattr (STDIN_FILENO, TCSANOW, &ti))
	ExitWithError ("tcsetattr");

    // Clear the screen; [r resets scroll region, [H homes cursor, [J erases
    printf ("\e[r\e[H\e[J");
}

