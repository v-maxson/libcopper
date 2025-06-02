#ifndef __COPPER_ERROR_H__
#define __COPPER_ERROR_H__

#ifndef CPR_SUCCESS
#define CPR_SUCCESS 0
#endif

#ifndef CPR_ERROR
#define CPR_ERROR -1
#endif

#ifndef CPR_ERROR_MSG_SIZE
#define CPR_ERROR_MSG_SIZE 256
#endif

/**
 * ! The below functions are thread-local. And may be called by 
 * ! this library internally to set error messages, it can, however
 * ! also be used by the user to set error messages for the current thread.
 */

/**
 * Sets the error message for this thread.
 * 
 * If the message is too long, it will be truncated to `CPR_ERROR_MSG_SIZE - 1` characters.
 */
void cpr_set_error(const char *fmt, ...);

/**
 * Gets the error message for this thread.
 */
const char *cpr_get_error(void);

#endif
