/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2019 Aaron M. D. Jones <aaronmdjones@gmail.com>
 *
 * Helper utility for the SASL ECDH-X25519-CHALLENGE mechanism.
 */

#ifndef ATHEME_SRC_ECDH_X25519_TOOL_H
#define ATHEME_SRC_ECDH_X25519_TOOL_H 1

#include <atheme/attributes.h>

#define X25519TOOL_OPTFLAG_NONE           0x00000000U
#define X25519TOOL_OPTFLAG_SELFTEST_ONLY  0x00000001U
#define X25519TOOL_OPTFLAG_CREATE_KEYPAIR 0x00000002U
#define X25519TOOL_OPTFLAG_PRINT_PUBKEY   0x00000004U
#define X25519TOOL_OPTFLAG_PRINT_QRCODE   0x00000008U
#define X25519TOOL_OPTFLAG_RESPOND        0x00000010U
#define X25519TOOL_OPTFLAG_SERVER         0x00000020U

void print_stderr(const char *, ...) ATHEME_FATTR_PRINTF(1, 2);
void print_stdout(const char *, ...) ATHEME_FATTR_PRINTF(1, 2);
int ecdh_x25519_tool_print_qrcode(const char *);

#endif /* !ATHEME_SRC_ECDH_X25519_TOOL_H */
