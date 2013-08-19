#include "pam.h"
#include <security/pam_appl.h>
#include <pwd.h>

static int xconv (int num_msg, const struct pam_message** msgm, struct pam_response** response, void* appdata_ptr);

static pam_handle_t* _pamh = NULL;
static const char* _username = NULL;

static void verify (int r, const char* fn)
{
    if (r == PAM_SUCCESS)
	return;
    fprintf(stderr,"Error: %s: %s\n", fn, pam_strerror(_pamh,r));
    exit (EXIT_FAILURE);
}

static void PamSetEnvironment (void)
{
    const char* user = getlogin();
    if (user)
	pam_set_item (_pamh, PAM_RUSER, user);
    pam_set_item (_pamh, PAM_RHOST, "localhost");
    const char* tty = ttyname (STDIN_FILENO);
    if (tty)
	pam_set_item (_pamh, PAM_TTY, tty);
    pam_set_item (_pamh, PAM_USER_PROMPT, "Username: ");
}

void PamOpen (void)
{
    static const struct pam_conv conv = { xconv, NULL };
    int r = pam_start (LOGIX_NAME, NULL, &conv, &_pamh);
    verify(r,"pam_start");
    PamSetEnvironment();
    atexit (PamClose);
}

void PamClose (void)
{
    if (!_pamh)
	return;
    if (_username)
	PamLogout();
    pam_end (_pamh, PAM_SUCCESS);
    _pamh = NULL;
}

const char* PamLogin (void)
{
    int r = pam_authenticate (_pamh, PAM_SILENT| PAM_DISALLOW_NULL_AUTHTOK);
    verify(r,"pam_authenticate");
    r = pam_acct_mgmt (_pamh, PAM_SILENT| PAM_DISALLOW_NULL_AUTHTOK);
    if (r == PAM_NEW_AUTHTOK_REQD) {
	fprintf(stderr,"Application must request new password...\n");
	r = pam_chauthtok(_pamh,PAM_CHANGE_EXPIRED_AUTHTOK);
	verify(r,"pam_chauthtok");
    }
    verify(r,"pam_acct_mgmt");
    r = pam_setcred(_pamh, PAM_SILENT| PAM_ESTABLISH_CRED);
    verify(r,"pam_setcred");
    r = pam_open_session (_pamh, PAM_SILENT);
    verify(r,"pam_open_session");
    pam_get_item (_pamh, PAM_USER, (const void**) &_username);
    return (_username);
}

void PamLogout (void)
{
    if (!_username)
	return;
    pam_close_session (_pamh, PAM_SILENT);
    pam_setcred (_pamh, PAM_SILENT| PAM_DELETE_CRED);
    _username = NULL;
}

int xconv (int num_msg, const struct pam_message** msgm, struct pam_response** response, void* appdata_ptr __attribute__((unused)))
{
    if (num_msg <= 0)
	return (PAM_CONV_ERR);

    struct pam_response* reply = (struct pam_response*) calloc (num_msg, sizeof(struct pam_response));
    if (!reply)
	return (PAM_CONV_ERR);

    for (int count = 0; count < num_msg; ++count) {
	switch (msgm[count]->msg_style) {
	    case PAM_PROMPT_ECHO_OFF:
	    case PAM_PROMPT_ECHO_ON: {
		char answer [32];
		printf ("%s", msgm[count]->msg);
		if (!fgets (answer, sizeof(answer), stdin))
		    goto failed_conversation;
		answer[strlen(answer)-1] = 0;
		reply[count].resp = strdup(answer);
		reply[count].resp_retcode = 0;
		break; }
	    case PAM_ERROR_MSG:
		if (puts (msgm[count]->msg) < 0)
		    goto failed_conversation;
		break;
	    case PAM_TEXT_INFO:
		if (puts (msgm[count]->msg) < 0)
		    goto failed_conversation;
		break;
	    default:
		printf ("erroneous conversation (%d)\n", msgm[count]->msg_style);
		goto failed_conversation;
	}
    }
    *response = reply;
    return (PAM_SUCCESS);

failed_conversation:
    if (reply) {
	for (int count = 0; count < num_msg; ++count) {
	    if (reply[count].resp) {
		memset (reply[count].resp, 0, strlen(reply[count].resp));
		free (reply[count].resp);
		reply[count].resp = NULL;
	    }
	}
	free(reply);
    }
    return (PAM_CONV_ERR);
}
