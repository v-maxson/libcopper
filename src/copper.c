#include "copper/copper.h"

#define CPR_VERSION_MAJOR 0
#define CPR_VERSION_MINOR 1
#define CPR_VERSION_PATCH 0
#define CPR_VERSION_STRING "0.1.0"

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
