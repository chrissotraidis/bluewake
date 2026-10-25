// Only the local-disc primitives needed by the HD importer. No TLS/networking.
#pragma once
#define MBEDTLS_AES_C
#define MBEDTLS_AES_ROM_TABLES
#define MBEDTLS_CIPHER_MODE_CBC
#define MBEDTLS_SHA1_C
