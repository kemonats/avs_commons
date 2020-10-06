/*
 * Copyright 2017-2020 AVSystem <avsystem@avsystem.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define AVS_STREAM_STREAM_FILE_C
#define _GNU_SOURCE // for mkstemps()

#include <avs_commons_posix_init.h>

#include <avsystem/commons/avs_unit_test.h>
#include <avsystem/commons/avs_utils.h>

#include <openssl/conf.h>
#include <openssl/engine.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../pki.h"

#include "src/crypto/avs_global.h"
#include "src/crypto/openssl/avs_openssl_common.h"
#include "src/crypto/openssl/avs_openssl_data_loader.h"
#include "src/crypto/openssl/avs_openssl_global.h"

#define MODULE_NAME openssl_engine_test
#include <avs_x_log_config.h>

static char TOKEN[] = "XXXXXX";
static const char PIN[] = "0001password";
static char PUBLIC_KEY_PATH[] = "/tmp/public_key_XXXXXX.der";
static const char *KEY_PAIR_LABEL = "my_key";
static char OPENSSL_ENGINE_CONF_FILE[] = "/tmp/openssl_engine_XXXXXX.conf";
static const char *OPENSSL_ENGINE_CONF_STR =
        "openssl_conf = openssl_init\n"
        ".include /etc/ssl/openssl.cnf\n\n"
        "[openssl_init]\n"
        "engines=engine_section\n\n"
        "[engine_section]\n"
        "pkcs11 = pkcs11_section\n\n"
        "[pkcs11_section]\n"
        "engine_id = pkcs11\n"
        "MODULE_PATH = /usr/lib/softhsm/libsofthsm2.so\n"
        "init = 0;\n";

static void system_cleanup(void) {
    char delete_token_command[50];
    AVS_UNIT_ASSERT_TRUE(
            snprintf(delete_token_command, sizeof(delete_token_command),
                     "softhsm2-util --delete-token --token %s", TOKEN)
            > 0);
    AVS_UNIT_ASSERT_SUCCESS(system(delete_token_command));

    AVS_UNIT_ASSERT_SUCCESS(unlink(PUBLIC_KEY_PATH));
    AVS_UNIT_ASSERT_SUCCESS(unlink(OPENSSL_ENGINE_CONF_FILE));
}

AVS_UNIT_SUITE_INIT(backend_openssl_engine, verbose) {
    (void) verbose;

    AVS_UNIT_ASSERT_SUCCESS(_avs_crypto_ensure_global_state());

    AVS_UNIT_ASSERT_NOT_EQUAL(mkstemps(OPENSSL_ENGINE_CONF_FILE, 5), -1);
    FILE *f = fopen(OPENSSL_ENGINE_CONF_FILE, "w");
    fwrite(OPENSSL_ENGINE_CONF_STR, sizeof(char),
           strlen(OPENSSL_ENGINE_CONF_STR), f);
    fclose(f);

    AVS_UNIT_ASSERT_NOT_EQUAL(mkstemps(PUBLIC_KEY_PATH, 4), -1);

    char soft_hsm_command[200];
    char pkcs11_command_1[200];
    char pkcs11_command_2[200];

    AVS_UNIT_ASSERT_TRUE(mkstemp(TOKEN) >= 0);
    unlink(TOKEN);

    printf("%s\n", TOKEN);
    fflush(NULL);

    AVS_UNIT_ASSERT_TRUE(
            avs_simple_snprintf(soft_hsm_command, sizeof(soft_hsm_command),
                                "softhsm2-util --init-token --free --label %s "
                                "--pin %s --so-pin %s ",
                                TOKEN, PIN, PIN)
            > 0);
    AVS_UNIT_ASSERT_TRUE(
            avs_simple_snprintf(pkcs11_command_1, sizeof(pkcs11_command_1),
                                "pkcs11-tool --module "
                                "/usr/lib/softhsm/libsofthsm2.so --token %s "
                                "--login --pin %s --keypairgen "
                                "--key-type rsa:2048 --label %s",
                                TOKEN, PIN, KEY_PAIR_LABEL)
            > 0);
    AVS_UNIT_ASSERT_TRUE(
            avs_simple_snprintf(pkcs11_command_2, sizeof(pkcs11_command_2),
                                "pkcs11-tool --module "
                                "/usr/lib/softhsm/libsofthsm2.so -r --type "
                                "pubkey --token %s --label %s -o %s",
                                TOKEN, KEY_PAIR_LABEL, PUBLIC_KEY_PATH)
            > 0);

    AVS_UNIT_ASSERT_SUCCESS(system(soft_hsm_command));
    AVS_UNIT_ASSERT_SUCCESS(system(pkcs11_command_1));
    AVS_UNIT_ASSERT_SUCCESS(system(pkcs11_command_2));

    AVS_UNIT_ASSERT_SUCCESS(atexit(system_cleanup));
}

void assert_trust_store_loadable(
        const avs_crypto_certificate_chain_info_t *certs,
        const avs_crypto_cert_revocation_list_info_t *crls) {
    X509_STORE *store = X509_STORE_new();
    AVS_UNIT_ASSERT_NOT_NULL(store);
    AVS_UNIT_ASSERT_SUCCESS(_avs_crypto_openssl_load_ca_certs(store, certs));
    AVS_UNIT_ASSERT_SUCCESS(_avs_crypto_openssl_load_crls(store, crls));
    X509_STORE_free(store);
}

static EVP_PKEY *load_pubkey() {
    FILE *public_key_file = fopen(PUBLIC_KEY_PATH, "rb");
    AVS_UNIT_ASSERT_NOT_NULL(public_key_file);
    EVP_PKEY *public_key = d2i_PUBKEY_fp(public_key_file, NULL);
    AVS_UNIT_ASSERT_NOT_NULL(public_key);
    AVS_UNIT_ASSERT_SUCCESS(fclose(public_key_file));
    return public_key;
}

AVS_UNIT_TEST(backend_openssl_engine, key_loading_from_pkcs11) {
    // Text preparation
    unsigned char original_text[256] = "Text to be encrypted.";
    int original_text_len = (int) strlen((const char *) original_text);
    memset((void *) (original_text + original_text_len), 'X',
           255 - original_text_len);
    original_text[255] = 0;
    original_text_len = 256;

    // Encryption
    EVP_PKEY *public_key = load_pubkey();
    EVP_PKEY_CTX *encrypt_ctx = EVP_PKEY_CTX_new(public_key, NULL);
    AVS_UNIT_ASSERT_NOT_NULL(encrypt_ctx);
    AVS_UNIT_ASSERT_EQUAL(EVP_PKEY_encrypt_init(encrypt_ctx), 1);
    AVS_UNIT_ASSERT_EQUAL(
            EVP_PKEY_CTX_set_rsa_padding(encrypt_ctx, RSA_NO_PADDING), 1);
    size_t encrypted_text_len;
    AVS_UNIT_ASSERT_EQUAL(EVP_PKEY_encrypt(encrypt_ctx, NULL,
                                           &encrypted_text_len, original_text,
                                           original_text_len),
                          1);
    unsigned char *encrypted_text =
            (unsigned char *) OPENSSL_malloc(encrypted_text_len);
    AVS_UNIT_ASSERT_EQUAL(EVP_PKEY_encrypt(encrypt_ctx, encrypted_text,
                                           &encrypted_text_len, original_text,
                                           original_text_len),
                          1);
    EVP_PKEY_CTX_free(encrypt_ctx);
    EVP_PKEY_free(public_key);

    // Loading private key from engine
    const char *query_template = "pkcs11:token=%s;object=%s;pin-value=%s";
    size_t query_buffer_size = strlen(query_template) + strlen(KEY_PAIR_LABEL)
                               + strlen(PIN) + strlen(TOKEN)
                               - (3 * strlen("%s")) + 1;
    char *query = (char *) avs_malloc(query_buffer_size);
    AVS_UNIT_ASSERT_NOT_NULL(query);

    AVS_UNIT_ASSERT_TRUE(avs_simple_snprintf(query, query_buffer_size,
                                             query_template, TOKEN,
                                             KEY_PAIR_LABEL, PIN)
                         >= 0);
    const avs_crypto_private_key_info_t private_key_info =
            avs_crypto_private_key_info_from_engine(query);
    EVP_PKEY *private_key = NULL;
    AVS_UNIT_ASSERT_SUCCESS(_avs_crypto_openssl_load_private_key(
            &private_key, &private_key_info));
    avs_free(query);
    AVS_UNIT_ASSERT_NOT_NULL(private_key);

    // Decryption
    //
    // Without this, a memory leak is triggered in the pkcs11 engine's key URL
    // parsing function...
    AVS_UNIT_ASSERT_EQUAL(ENGINE_ctrl_cmd(_avs_global_engine, "FORCE_LOGIN", 0,
                                          NULL, NULL, 0),
                          1);
    AVS_UNIT_ASSERT_EQUAL(ENGINE_init(_avs_global_engine), 1);
    EVP_PKEY_CTX *decrypt_ctx =
            EVP_PKEY_CTX_new(private_key, _avs_global_engine);
    AVS_UNIT_ASSERT_NOT_NULL(decrypt_ctx);
    AVS_UNIT_ASSERT_EQUAL(EVP_PKEY_decrypt_init(decrypt_ctx), 1);
    AVS_UNIT_ASSERT_EQUAL(
            EVP_PKEY_CTX_set_rsa_padding(decrypt_ctx, RSA_NO_PADDING), 1);

    unsigned char decrypted_text[256];
    size_t decrypted_text_len = original_text_len;
    AVS_UNIT_ASSERT_EQUAL(EVP_PKEY_decrypt(decrypt_ctx, decrypted_text,
                                           &decrypted_text_len, encrypted_text,
                                           encrypted_text_len),
                          1);
    EVP_PKEY_CTX_free(decrypt_ctx);
    EVP_PKEY_free(private_key);
    OPENSSL_free(encrypted_text);
    AVS_UNIT_ASSERT_EQUAL(ENGINE_finish(_avs_global_engine), 1);

    // Check
    AVS_UNIT_ASSERT_EQUAL(decrypted_text_len, original_text_len);
    AVS_UNIT_ASSERT_EQUAL(strncmp((const char *) original_text,
                                  (const char *) decrypted_text,
                                  original_text_len),
                          0);
}

static avs_error_t load_first_cert(void *cert_, void *out_cert_ptr_) {
    X509 *cert = (X509 *) cert_;
    X509 **out_cert_ptr = (X509 **) out_cert_ptr_;
    if (!*out_cert_ptr) {
        if (!X509_up_ref(cert)) {
            log_openssl_error();
            return avs_errno(AVS_ENOMEM);
        }
        *out_cert_ptr = cert;
    }
    return AVS_OK;
}

AVS_UNIT_TEST(backend_openssl_engine, cert_loading_from_pkcs11) {
    // System preparation
    char openssl_cli_command[300];
    char pkcs11_command[200];

    char cert_path[] = "/tmp/cert_XXXXXX.der";
    const char *cert_label = "my_cert";

    AVS_UNIT_ASSERT_NOT_EQUAL(mkstemps(cert_path, 4), -1);

    AVS_UNIT_ASSERT_TRUE(
            avs_simple_snprintf(
                    openssl_cli_command, sizeof(openssl_cli_command),
                    "OPENSSL_CONF=%s openssl req -new -x509 "
                    "-days 365 -subj '/CN=%s' -sha256 -engine pkcs11 "
                    "-keyform engine -key 'pkcs11:token=%s;object=%s"
                    ";pin-value=%s' -outform der -out %s",
                    OPENSSL_ENGINE_CONF_FILE, KEY_PAIR_LABEL, TOKEN,
                    KEY_PAIR_LABEL, PIN, cert_path)
            > 0);
    AVS_UNIT_ASSERT_TRUE(
            avs_simple_snprintf(
                    pkcs11_command, sizeof(pkcs11_command),
                    "pkcs11-tool --module /usr/lib/softhsm/libsofthsm2.so "
                    "--pin %s -w %s --type cert --label %s --token %s",
                    PIN, cert_path, cert_label, TOKEN)
            > 0);

    AVS_UNIT_ASSERT_SUCCESS(system(openssl_cli_command));
    AVS_UNIT_ASSERT_SUCCESS(system(pkcs11_command));

    // Loading certificate
    const char *query_template =
            "pkcs11:type=cert;token=%s;object=%s;pin-value=%s";
    size_t query_buffer_size = strlen(query_template) + strlen(cert_label)
                               + strlen(PIN) + strlen(TOKEN)
                               - (3 * strlen("%s")) + 1;
    char *query = (char *) avs_malloc(query_buffer_size);
    AVS_UNIT_ASSERT_NOT_NULL(query);
    AVS_UNIT_ASSERT_TRUE(avs_simple_snprintf(query, query_buffer_size,
                                             query_template, TOKEN, cert_label,
                                             PIN)
                         >= 0);
    const avs_crypto_certificate_chain_info_t cert_info =
            avs_crypto_certificate_chain_info_from_engine(query);
    X509 *cert = NULL;
    AVS_UNIT_ASSERT_SUCCESS(_avs_crypto_openssl_load_client_certs(
            &cert_info, load_first_cert, &cert));
    AVS_UNIT_ASSERT_NOT_NULL(cert);

    // Verifying certificate
    EVP_PKEY *public_key = load_pubkey();
    AVS_UNIT_ASSERT_TRUE(X509_verify(cert, public_key));

    // Memory cleanup
    X509_free(cert);
    EVP_PKEY_free(public_key);
    avs_free(query);

    // System cleanup
    AVS_UNIT_ASSERT_SUCCESS(unlink(cert_path));
}
