/*
 * Copyright 2021 AVSystem <avsystem@avsystem.com>
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

#define AVS_UNIT_ENABLE_SHORT_ASSERTS
#include <avsystem/commons/avs_unit_test.h>

#include <avsystem/commons/avs_aead.h>
#include <avsystem/commons/avs_memory.h>

#include <string.h>

static void test_impl(const unsigned char *key,
                      size_t key_len,
                      const unsigned char *iv,
                      size_t iv_len,
                      const unsigned char *aad,
                      size_t aad_len,
                      const unsigned char *input,
                      size_t input_len,
                      const unsigned char *ciphertext,
                      size_t ciphertext_len) {
    unsigned char *encrypted = NULL;
    unsigned char *decrypted = NULL;
    if (input_len) {
        encrypted =
                (unsigned char *) avs_calloc(input_len, sizeof(unsigned char));
        decrypted =
                (unsigned char *) avs_calloc(input_len, sizeof(unsigned char));
    }

    // Ciphertext = concatenated encrypted data and tag.
    // Encrypted data size is equal to input data len.
    size_t tag_len = ciphertext_len - input_len;
    unsigned char *tag =
            (unsigned char *) avs_calloc(tag_len, sizeof(unsigned char));

    ASSERT_OK(avs_crypto_aead_aes_ccm_encrypt(key, key_len, iv, iv_len, aad,
                                              aad_len, input, input_len, tag,
                                              tag_len, encrypted));
    ASSERT_EQ_BYTES_SIZED(encrypted, ciphertext, input_len);
    ASSERT_EQ_BYTES_SIZED(tag, ciphertext + input_len, tag_len);

    ASSERT_OK(avs_crypto_aead_aes_ccm_decrypt(key, key_len, iv, iv_len, aad,
                                              aad_len, encrypted, input_len,
                                              tag, tag_len, decrypted));
    ASSERT_EQ_BYTES_SIZED(decrypted, input, input_len);

    avs_free(encrypted);
    avs_free(tag);
    avs_free(decrypted);
}

// Test vectors from draft-ietf-core-object-security-16
// https://tools.ietf.org/html/draft-ietf-core-object-security-16

AVS_UNIT_TEST(avs_crypto_aead, test_vector_4) {
    const unsigned char plaintext[] = { 0x01, 0xb3, 0x74, 0x76, 0x31 };

    const unsigned char encryption_key[] = { 0xf0, 0x91, 0x0e, 0xd7, 0x29, 0x5e,
                                             0x6a, 0xd4, 0xb5, 0x4f, 0xc7, 0x93,
                                             0x15, 0x43, 0x02, 0xff };

    const unsigned char nonce[] = { 0x46, 0x22, 0xd4, 0xdd, 0x6d, 0x94, 0x41,
                                    0x68, 0xee, 0xfb, 0x54, 0x98, 0x68 };

    const unsigned char aad[] = { 0x83, 0x68, 0x45, 0x6e, 0x63, 0x72, 0x79,
                                  0x70, 0x74, 0x30, 0x40, 0x48, 0x85, 0x01,
                                  0x81, 0x0a, 0x40, 0x41, 0x14, 0x40 };

    const unsigned char ciphertext[] = { 0x61, 0x2f, 0x10, 0x92, 0xf1,
                                         0x77, 0x6f, 0x1c, 0x16, 0x68,
                                         0xb3, 0x82, 0x5e };

    test_impl(encryption_key, sizeof(encryption_key), nonce, sizeof(nonce), aad,
              sizeof(aad), plaintext, sizeof(plaintext), ciphertext,
              sizeof(ciphertext));
}

AVS_UNIT_TEST(avs_crypto_aead, test_vector_5) {
    const unsigned char plaintext[] = { 0x01, 0xb3, 0x74, 0x76, 0x31 };

    const unsigned char encryption_key[] = { 0x32, 0x1b, 0x26, 0x94, 0x32, 0x53,
                                             0xc7, 0xff, 0xb6, 0x00, 0x3b, 0x0b,
                                             0x64, 0xd7, 0x40, 0x41 };

    const unsigned char nonce[] = { 0xbf, 0x35, 0xae, 0x29, 0x7d, 0x2d, 0xac,
                                    0xe9, 0x10, 0xc5, 0x2e, 0x99, 0xed };

    const unsigned char aad[] = { 0x83, 0x68, 0x45, 0x6e, 0x63, 0x72, 0x79,
                                  0x70, 0x74, 0x30, 0x40, 0x49, 0x85, 0x01,
                                  0x81, 0x0a, 0x41, 0x00, 0x41, 0x14, 0x40 };

    const unsigned char ciphertext[] = { 0x4e, 0xd3, 0x39, 0xa5, 0xa3,
                                         0x79, 0xb0, 0xb8, 0xbc, 0x73,
                                         0x1f, 0xff, 0xb0 };

    test_impl(encryption_key, sizeof(encryption_key), nonce, sizeof(nonce), aad,
              sizeof(aad), plaintext, sizeof(plaintext), ciphertext,
              sizeof(ciphertext));
}

AVS_UNIT_TEST(avs_crypto_aead, test_vector_6) {
    const unsigned char plaintext[] = { 0x01, 0xb3, 0x74, 0x76, 0x31 };

    const unsigned char encryption_key[] = { 0xaf, 0x2a, 0x13, 0x00, 0xa5, 0xe9,
                                             0x57, 0x88, 0xb3, 0x56, 0x33, 0x6e,
                                             0xee, 0xcd, 0x2b, 0x92 };

    const unsigned char nonce[] = { 0x2c, 0xa5, 0x8f, 0xb8, 0x5f, 0xf1, 0xb8,
                                    0x1c, 0x0b, 0x71, 0x81, 0xb8, 0x4a };

    const unsigned char aad[] = { 0x83, 0x68, 0x45, 0x6e, 0x63, 0x72, 0x79,
                                  0x70, 0x74, 0x30, 0x40, 0x48, 0x85, 0x01,
                                  0x81, 0x0a, 0x40, 0x41, 0x14, 0x40 };

    const unsigned char ciphertext[] = { 0x72, 0xcd, 0x72, 0x73, 0xfd,
                                         0x33, 0x1a, 0xc4, 0x5c, 0xff,
                                         0xbe, 0x55, 0xc3 };

    test_impl(encryption_key, sizeof(encryption_key), nonce, sizeof(nonce), aad,
              sizeof(aad), plaintext, sizeof(plaintext), ciphertext,
              sizeof(ciphertext));
}

AVS_UNIT_TEST(avs_crypto_aead, test_vector_7) {
    const unsigned char plaintext[] = { 0x45, 0xff, 0x48, 0x65, 0x6c,
                                        0x6c, 0x6f, 0x20, 0x57, 0x6f,
                                        0x72, 0x6c, 0x64, 0x21 };

    const unsigned char encryption_key[] = { 0xff, 0xb1, 0x4e, 0x09, 0x3c, 0x94,
                                             0xc9, 0xca, 0xc9, 0x47, 0x16, 0x48,
                                             0xb4, 0xf9, 0x87, 0x10 };

    const unsigned char nonce[] = { 0x46, 0x22, 0xd4, 0xdd, 0x6d, 0x94, 0x41,
                                    0x68, 0xee, 0xfb, 0x54, 0x98, 0x68 };

    const unsigned char aad[] = { 0x83, 0x68, 0x45, 0x6e, 0x63, 0x72, 0x79,
                                  0x70, 0x74, 0x30, 0x40, 0x48, 0x85, 0x01,
                                  0x81, 0x0a, 0x40, 0x41, 0x14, 0x40 };

    const unsigned char ciphertext[] = { 0xdb, 0xaa, 0xd1, 0xe9, 0xa7, 0xe7,
                                         0xb2, 0xa8, 0x13, 0xd3, 0xc3, 0x15,
                                         0x24, 0x37, 0x83, 0x03, 0xcd, 0xaf,
                                         0xae, 0x11, 0x91, 0x06 };

    test_impl(encryption_key, sizeof(encryption_key), nonce, sizeof(nonce), aad,
              sizeof(aad), plaintext, sizeof(plaintext), ciphertext,
              sizeof(ciphertext));
}

AVS_UNIT_TEST(avs_crypto_aead, test_vector_8) {
    const unsigned char plaintext[] = { 0x45, 0xff, 0x48, 0x65, 0x6c,
                                        0x6c, 0x6f, 0x20, 0x57, 0x6f,
                                        0x72, 0x6c, 0x64, 0x21 };

    const unsigned char encryption_key[] = { 0xff, 0xb1, 0x4e, 0x09, 0x3c, 0x94,
                                             0xc9, 0xca, 0xc9, 0x47, 0x16, 0x48,
                                             0xb4, 0xf9, 0x87, 0x10 };

    const unsigned char nonce[] = { 0x47, 0x22, 0xd4, 0xdd, 0x6d, 0x94, 0x41,
                                    0x69, 0xee, 0xfb, 0x54, 0x98, 0x7c };

    const unsigned char aad[] = { 0x83, 0x68, 0x45, 0x6e, 0x63, 0x72, 0x79,
                                  0x70, 0x74, 0x30, 0x40, 0x48, 0x85, 0x01,
                                  0x81, 0x0a, 0x40, 0x41, 0x14, 0x40 };

    const unsigned char ciphertext[] = { 0x4d, 0x4c, 0x13, 0x66, 0x93, 0x84,
                                         0xb6, 0x73, 0x54, 0xb2, 0xb6, 0x17,
                                         0x5f, 0xf4, 0xb8, 0x65, 0x8c, 0x66,
                                         0x6a, 0x6c, 0xf8, 0x8e };

    test_impl(encryption_key, sizeof(encryption_key), nonce, sizeof(nonce), aad,
              sizeof(aad), plaintext, sizeof(plaintext), ciphertext,
              sizeof(ciphertext));
}

// Custom test vectors

AVS_UNIT_TEST(avs_crypto_aead, no_aad) {
    const char *plaintext = "test";
    const char *encryption_key = "ptkilatajaklczem";
    const char *nonce = "nonceee";

    // From PyCryptodome
    const unsigned char ciphertext[] = { 0xa5, 0xdb, 0xea, 0x4f, 0x18,
                                         0x68, 0x5b, 0xb1, 0x2b, 0x3d,
                                         0x70, 0xf1, 0xde, 0xc0, 0x6e,
                                         0x9a, 0x92, 0xca, 0x75, 0x04 };

    test_impl((const unsigned char *) encryption_key, strlen(encryption_key),
              (const unsigned char *) nonce, strlen(nonce), NULL, 0,
              (const unsigned char *) plaintext, strlen(plaintext), ciphertext,
              sizeof(ciphertext));
}

AVS_UNIT_TEST(avs_crypto_aead, long_message) {
    const char *plaintext =
            "Bacon ipsum dolor amet t-bone capicola chuck meatloaf ham. "
            "Leberkas "
            "chuck ham pancetta. Shankle beef capicola meatloaf strip steak, "
            "drumstick swine jerky buffalo chuck turducken rump tri-tip cupim "
            "bresaola. Beef ham hock short ribs pork meatball ribeye hamburger "
            "kielbasa.";
    const char *encryption_key = "ptkilatajaklczem";
    const char *nonce = "nonceee";
    const char *aad = "aad";

    // From PyCryptodome
    const unsigned char ciphertext[] = {
        0x93, 0xdf, 0xfa, 0x54, 0x1f, 0x04, 0xc9, 0x76, 0x2a, 0xba, 0x82, 0xb3,
        0x61, 0x15, 0x45, 0x0c, 0xa1, 0x18, 0x9b, 0xde, 0x57, 0x99, 0xd3, 0x91,
        0x34, 0x66, 0xa5, 0x0b, 0x9d, 0x09, 0xbd, 0x56, 0x61, 0x61, 0xa6, 0x8e,
        0x97, 0xaf, 0xf1, 0x4a, 0xa2, 0x7b, 0x83, 0x9a, 0x74, 0xa6, 0xd8, 0x07,
        0x63, 0x87, 0xdf, 0x86, 0x66, 0x13, 0xea, 0x21, 0x46, 0x3e, 0x0c, 0x59,
        0x5b, 0x14, 0xd1, 0xdd, 0x44, 0x3d, 0x8c, 0xb2, 0x52, 0xad, 0x4f, 0x66,
        0xa8, 0xc7, 0x63, 0x8a, 0xe1, 0x9a, 0xf5, 0x95, 0xe4, 0xf7, 0x90, 0x4c,
        0x83, 0x29, 0xd8, 0xfc, 0x95, 0x66, 0xcf, 0x83, 0x17, 0x75, 0xf7, 0x20,
        0x45, 0x8b, 0xfc, 0xd2, 0xae, 0x83, 0x9e, 0x56, 0x9a, 0x26, 0xa9, 0xd1,
        0xd9, 0x0e, 0x12, 0x94, 0xc9, 0x5d, 0x31, 0xc6, 0x90, 0xb6, 0x37, 0x4f,
        0x66, 0x5a, 0x68, 0x4e, 0xd7, 0xb3, 0x9e, 0xbe, 0xcd, 0x62, 0x73, 0x7a,
        0x45, 0x4e, 0xde, 0x87, 0x77, 0xb8, 0x0f, 0xc5, 0xaf, 0xc7, 0x47, 0xfa,
        0x93, 0x60, 0xbc, 0x87, 0x83, 0x60, 0x0a, 0xe2, 0xf1, 0x75, 0xc5, 0x4c,
        0x9a, 0x61, 0xb6, 0xd9, 0x09, 0xd8, 0xe0, 0x37, 0x4f, 0xa1, 0xd3, 0x47,
        0x3f, 0xbd, 0xe9, 0xfa, 0xb9, 0x4d, 0xc7, 0x68, 0x33, 0x9c, 0x94, 0xda,
        0x32, 0xf0, 0x21, 0xc5, 0x11, 0xbe, 0xad, 0x99, 0x46, 0xdf, 0x51, 0x5a,
        0x82, 0x65, 0xbd, 0x2b, 0x39, 0x9a, 0x6a, 0xc6, 0xc0, 0x81, 0xf0, 0xc8,
        0xd6, 0xdf, 0x90, 0x09, 0x3e, 0x49, 0x02, 0xc4, 0x8f, 0xf5, 0xa0, 0x04,
        0xb1, 0xdb, 0x1d, 0xbe, 0xf7, 0xdc, 0xcf, 0xd7, 0x4d, 0x5f, 0xcf, 0x47,
        0xe6, 0xf6, 0x1c, 0x1c, 0x40, 0x25, 0xf6, 0xf7, 0xec, 0x53, 0x69, 0xd5,
        0x84, 0x95, 0xda, 0xe8, 0x41, 0xf3, 0x9a, 0x57, 0xb5, 0x14, 0xad, 0xb5,
        0x3d, 0x59, 0x9d, 0xcf, 0x28, 0xfe, 0xf7, 0x25, 0xde, 0x1e, 0x52, 0x4f,
        0xb7, 0x7f, 0x04, 0x29, 0xa6, 0x0d, 0xcf, 0xa3, 0xdc, 0x55, 0xa8, 0x8c,
        0x4e, 0x3f, 0xba, 0xfd, 0xda, 0x85, 0xfd, 0xaf, 0x5f, 0xd0, 0xa8, 0xf6
    };

    test_impl((const unsigned char *) encryption_key, strlen(encryption_key),
              (const unsigned char *) nonce, strlen(nonce),
              (const unsigned char *) aad, strlen(aad),
              (const unsigned char *) plaintext, strlen(plaintext), ciphertext,
              sizeof(ciphertext));
}

AVS_UNIT_TEST(avs_crypto_aead, authenticate_only) {
    const char *encryption_key = "ptkilatajaklczem";
    const char *nonce = "nonceee";
    const char *aad = "aad";

    // From PyCryptodome
    const unsigned char ciphertext[] = { 0x18, 0x3a, 0xc3, 0x4a, 0x05, 0xbe,
                                         0x03, 0x78, 0x10, 0x7d, 0x17, 0x2c,
                                         0xa4, 0x27, 0x3a, 0x86 };

    test_impl((const unsigned char *) encryption_key, strlen(encryption_key),
              (const unsigned char *) nonce, strlen(nonce),
              (const unsigned char *) aad, strlen(aad), NULL, 0, ciphertext,
              sizeof(ciphertext));
}
