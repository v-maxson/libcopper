#include "copper/copper.h"

#include "copper/internal/int_version.h"

int cpr_version_major(void)
{
	return CPR_VERSION_MAJOR;
}

int cpr_version_minor(void)
{
	return CPR_VERSION_MINOR;
}

int cpr_version_patch(void)
{
	return CPR_VERSION_PATCH;
}

const char *cpr_version_string(void)
{
	return CPR_VERSION_STRING;
}
