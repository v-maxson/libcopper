#ifndef CPR_INT_ERROR_H
#define CPR_INT_ERROR_H

#include "copper/result.h"

#ifdef __cplusplus
extern "C" {
#endif

void cpr__set_error(CprResult code, const char *msg);

#ifdef __cplusplus
}
#endif

#endif // CPR_INT_ERROR_H
