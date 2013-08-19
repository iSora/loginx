#pragma once
#include "pam.h"
#include "pw.h"

enum { MAX_PW_LEN = 64 };

void LoginBox (acclist_t al, unsigned* pali, char* password);
