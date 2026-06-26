#include "copper/error.h"

#include "copper/result.h"
#include "copper/thread.h"
#include <stddef.h>

static CPR_THREAD_LOCAL CprError S_ERROR = { 0, "" };

// INTERNAL ONLY (defined in internal/int_error.h)
void cpr__set_error(CprResult code, const char *msg)
{
	S_ERROR.code = code;
	S_ERROR.message = msg;
}

CprError cpr_get_error(void)
{
	return S_ERROR;
}

void cpr_clear_error(void)
{
	S_ERROR.code = CPR_OK;
	S_ERROR.message = "";
}
