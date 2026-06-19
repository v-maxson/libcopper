#ifndef CPR_RESULT_H
#define CPR_RESULT_H

#define CPR_RESULT_TABLE            \
	CODE(CPR_OK, 0)             \
	CODE(CPR_ERR_OOM, -1)       \
	CODE(CPR_ERR_INVALID, -2)   \
	CODE(CPR_ERR_OVERFLOW, -3)  \
	CODE(CPR_ERR_NOT_FOUND, -4) \
	CODE(CPR_ERR_IO, -5)        \
	CODE(CPR_ERR_EXHAUSTED, -6) \
	CODE(CPR_ERR_ALIGN, -7)     \
	CODE(CPR_ERR_BUSY, -8)

typedef int CprResult;

enum {
#define CODE(code, value) code = value,
	CPR_RESULT_TABLE
#undef CODE
};

// utility macros
#define cpr_ok(result) ((result) == CPR_OK)
#define cpr_err(result) ((result) != CPR_OK)

#ifdef __cplusplus
extern "C" {
#endif

/// Returns the name of `result` as a string (e.g. "CPR_OK").
static inline const char *cpr_result_str(CprResult result)
{
	switch (result) {
#define CODE(code, value) \
	case code:        \
		return #code;
		CPR_RESULT_TABLE
#undef CODE
	default:
		return "unknown result";
	}
}

#ifdef __cplusplus
}
#endif

#endif // CPR_RESULT_H
