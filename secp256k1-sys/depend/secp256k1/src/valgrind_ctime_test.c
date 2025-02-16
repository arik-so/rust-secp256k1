/***********************************************************************
 * Copyright (c) 2020 Gregory Maxwell                                  *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#include <valgrind/memcheck.h>
#include <stdio.h>

#include "../include/secp256k1.h"
#include "assumptions.h"
#include "util.h"

#ifdef ENABLE_MODULE_ECDH
# include "../include/rustsecp256k1_v0_8_0_ecdh.h"
#endif

#ifdef ENABLE_MODULE_RECOVERY
# include "../include/rustsecp256k1_v0_8_0_recovery.h"
#endif

#ifdef ENABLE_MODULE_EXTRAKEYS
# include "../include/rustsecp256k1_v0_8_0_extrakeys.h"
#endif

#ifdef ENABLE_MODULE_SCHNORRSIG
#include "../include/secp256k1_schnorrsig.h"
#endif

void run_tests(rustsecp256k1_v0_8_0_context *ctx, unsigned char *key);

int main(void) {
    rustsecp256k1_v0_8_0_context* ctx;
    unsigned char key[32];
    int ret, i;

    if (!RUNNING_ON_VALGRIND) {
        fprintf(stderr, "This test can only usefully be run inside valgrind.\n");
        fprintf(stderr, "Usage: libtool --mode=execute valgrind ./valgrind_ctime_test\n");
        return 1;
    }
    ctx = rustsecp256k1_v0_8_0_context_create(SECP256K1_CONTEXT_DECLASSIFY);
    /** In theory, testing with a single secret input should be sufficient:
     *  If control flow depended on secrets the tool would generate an error.
     */
    for (i = 0; i < 32; i++) {
        key[i] = i + 65;
    }

    run_tests(ctx, key);

    /* Test context randomisation. Do this last because it leaves the context
     * tainted. */
    VALGRIND_MAKE_MEM_UNDEFINED(key, 32);
    ret = rustsecp256k1_v0_8_0_context_randomize(ctx, key);
    VALGRIND_MAKE_MEM_DEFINED(&ret, sizeof(ret));
    CHECK(ret);

    rustsecp256k1_v0_8_0_context_destroy(ctx);
    return 0;
}

void run_tests(rustsecp256k1_v0_8_0_context *ctx, unsigned char *key) {
    rustsecp256k1_v0_8_0_ecdsa_signature signature;
    rustsecp256k1_v0_8_0_pubkey pubkey;
    size_t siglen = 74;
    size_t outputlen = 33;
    int i;
    int ret;
    unsigned char msg[32];
    unsigned char sig[74];
    unsigned char spubkey[33];
#ifdef ENABLE_MODULE_RECOVERY
    rustsecp256k1_v0_8_0_ecdsa_recoverable_signature recoverable_signature;
    int recid;
#endif
#ifdef ENABLE_MODULE_EXTRAKEYS
    rustsecp256k1_v0_8_0_keypair keypair;
#endif

    for (i = 0; i < 32; i++) {
        msg[i] = i + 1;
    }

    /* Test keygen. */
    VALGRIND_MAKE_MEM_UNDEFINED(key, 32);
    ret = rustsecp256k1_v0_8_0_ec_pubkey_create(ctx, &pubkey, key);
    VALGRIND_MAKE_MEM_DEFINED(&pubkey, sizeof(rustsecp256k1_v0_8_0_pubkey));
    VALGRIND_MAKE_MEM_DEFINED(&ret, sizeof(ret));
    CHECK(ret);
    CHECK(rustsecp256k1_v0_8_0_ec_pubkey_serialize(ctx, spubkey, &outputlen, &pubkey, SECP256K1_EC_COMPRESSED) == 1);

    /* Test signing. */
    VALGRIND_MAKE_MEM_UNDEFINED(key, 32);
    ret = rustsecp256k1_v0_8_0_ecdsa_sign(ctx, &signature, msg, key, NULL, NULL);
    VALGRIND_MAKE_MEM_DEFINED(&signature, sizeof(rustsecp256k1_v0_8_0_ecdsa_signature));
    VALGRIND_MAKE_MEM_DEFINED(&ret, sizeof(ret));
    CHECK(ret);
    CHECK(rustsecp256k1_v0_8_0_ecdsa_signature_serialize_der(ctx, sig, &siglen, &signature));

#ifdef ENABLE_MODULE_ECDH
    /* Test ECDH. */
    VALGRIND_MAKE_MEM_UNDEFINED(key, 32);
    ret = rustsecp256k1_v0_8_0_ecdh(ctx, msg, &pubkey, key, NULL, NULL);
    VALGRIND_MAKE_MEM_DEFINED(&ret, sizeof(ret));
    CHECK(ret == 1);
#endif

#ifdef ENABLE_MODULE_RECOVERY
    /* Test signing a recoverable signature. */
    VALGRIND_MAKE_MEM_UNDEFINED(key, 32);
    ret = rustsecp256k1_v0_8_0_ecdsa_sign_recoverable(ctx, &recoverable_signature, msg, key, NULL, NULL);
    VALGRIND_MAKE_MEM_DEFINED(&recoverable_signature, sizeof(recoverable_signature));
    VALGRIND_MAKE_MEM_DEFINED(&ret, sizeof(ret));
    CHECK(ret);
    CHECK(rustsecp256k1_v0_8_0_ecdsa_recoverable_signature_serialize_compact(ctx, sig, &recid, &recoverable_signature));
    CHECK(recid >= 0 && recid <= 3);
#endif

    VALGRIND_MAKE_MEM_UNDEFINED(key, 32);
    ret = rustsecp256k1_v0_8_0_ec_seckey_verify(ctx, key);
    VALGRIND_MAKE_MEM_DEFINED(&ret, sizeof(ret));
    CHECK(ret == 1);

    VALGRIND_MAKE_MEM_UNDEFINED(key, 32);
    ret = rustsecp256k1_v0_8_0_ec_seckey_negate(ctx, key);
    VALGRIND_MAKE_MEM_DEFINED(&ret, sizeof(ret));
    CHECK(ret == 1);

    VALGRIND_MAKE_MEM_UNDEFINED(key, 32);
    VALGRIND_MAKE_MEM_UNDEFINED(msg, 32);
    ret = rustsecp256k1_v0_8_0_ec_seckey_tweak_add(ctx, key, msg);
    VALGRIND_MAKE_MEM_DEFINED(&ret, sizeof(ret));
    CHECK(ret == 1);

    VALGRIND_MAKE_MEM_UNDEFINED(key, 32);
    VALGRIND_MAKE_MEM_UNDEFINED(msg, 32);
    ret = rustsecp256k1_v0_8_0_ec_seckey_tweak_mul(ctx, key, msg);
    VALGRIND_MAKE_MEM_DEFINED(&ret, sizeof(ret));
    CHECK(ret == 1);

    /* Test keypair_create and keypair_xonly_tweak_add. */
#ifdef ENABLE_MODULE_EXTRAKEYS
    VALGRIND_MAKE_MEM_UNDEFINED(key, 32);
    ret = rustsecp256k1_v0_8_0_keypair_create(ctx, &keypair, key);
    VALGRIND_MAKE_MEM_DEFINED(&ret, sizeof(ret));
    CHECK(ret == 1);

    /* The tweak is not treated as a secret in keypair_tweak_add */
    VALGRIND_MAKE_MEM_DEFINED(msg, 32);
    ret = rustsecp256k1_v0_8_0_keypair_xonly_tweak_add(ctx, &keypair, msg);
    VALGRIND_MAKE_MEM_DEFINED(&ret, sizeof(ret));
    CHECK(ret == 1);

    VALGRIND_MAKE_MEM_UNDEFINED(key, 32);
    VALGRIND_MAKE_MEM_UNDEFINED(&keypair, sizeof(keypair));
    ret = rustsecp256k1_v0_8_0_keypair_sec(ctx, key, &keypair);
    VALGRIND_MAKE_MEM_DEFINED(&ret, sizeof(ret));
    CHECK(ret == 1);
#endif

#ifdef ENABLE_MODULE_SCHNORRSIG
    VALGRIND_MAKE_MEM_UNDEFINED(key, 32);
    ret = rustsecp256k1_v0_8_0_keypair_create(ctx, &keypair, key);
    VALGRIND_MAKE_MEM_DEFINED(&ret, sizeof(ret));
    CHECK(ret == 1);
    ret = rustsecp256k1_v0_8_0_schnorrsig_sign32(ctx, sig, msg, &keypair, NULL);
    VALGRIND_MAKE_MEM_DEFINED(&ret, sizeof(ret));
    CHECK(ret == 1);
#endif
}
