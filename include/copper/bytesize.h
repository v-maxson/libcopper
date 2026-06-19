#ifndef CPR_BYTESIZE_H
#define CPR_BYTESIZE_H

//* This module provides easy-to-use macros to convert between byte sizes.

// --- Decimal (SI) unit conversions ---
#define cpr_kb_to_b(x) ((x) * 1000ULL)
#define cpr_mb_to_b(x) ((x) * 1000000ULL)
#define cpr_gb_to_b(x) ((x) * 1000000000ULL)
#define cpr_tb_to_b(x) ((x) * 1000000000000ULL)

#define cpr_b_to_kb(x) ((x) / 1000ULL)
#define cpr_b_to_mb(x) ((x) / 1000000ULL)
#define cpr_b_to_gb(x) ((x) / 1000000000ULL)
#define cpr_b_to_tb(x) ((x) / 1000000000000ULL)

#define cpr_kb_to_mb(x) ((x) / 1000ULL)
#define cpr_kb_to_gb(x) ((x) / 1000000ULL)
#define cpr_mb_to_kb(x) ((x) * 1000ULL)
#define cpr_mb_to_gb(x) ((x) / 1000ULL)
#define cpr_gb_to_kb(x) ((x) * 1000000ULL)
#define cpr_gb_to_mb(x) ((x) * 1000ULL)

// --- Binary (IEC) unit conversions ---
#define cpr_kib_to_b(x) ((x) * 1024ULL)
#define cpr_mib_to_b(x) ((x) * 1048576ULL)
#define cpr_gib_to_b(x) ((x) * 1073741824ULL)
#define cpr_tib_to_b(x) ((x) * 1099511627776ULL)

#define cpr_b_to_kib(x) ((x) / 1024ULL)
#define cpr_b_to_mib(x) ((x) / 1048576ULL)
#define cpr_b_to_gib(x) ((x) / 1073741824ULL)
#define cpr_b_to_tib(x) ((x) / 1099511627776ULL)

#define cpr_kib_to_mib(x) ((x) / 1024ULL)
#define cpr_kib_to_gib(x) ((x) / 1048576ULL)
#define cpr_mib_to_kib(x) ((x) * 1024ULL)
#define cpr_mib_to_gib(x) ((x) / 1024ULL)
#define cpr_gib_to_kib(x) ((x) * 1048576ULL)
#define cpr_gib_to_mib(x) ((x) * 1024ULL)

// --- Literal size macros (produce values in bytes) ---
#define cpr_kb(x) cpr_kb_to_b(x)
#define cpr_mb(x) cpr_mb_to_b(x)
#define cpr_gb(x) cpr_gb_to_b(x)
#define cpr_tb(x) cpr_tb_to_b(x)
#define cpr_kib(x) cpr_kib_to_b(x)
#define cpr_mib(x) cpr_mib_to_b(x)
#define cpr_gib(x) cpr_gib_to_b(x)
#define cpr_tib(x) cpr_tib_to_b(x)

#endif // CPR_BYTESIZE_H
