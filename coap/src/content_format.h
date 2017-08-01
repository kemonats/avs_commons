/*
 * Copyright 2017 AVSystem <avsystem@avsystem.com>
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

#ifndef ANJAY_COAP_CONTENT_FORMAT_H
#define ANJAY_COAP_CONTENT_FORMAT_H

#pragma GCC visibility push(hidden)

/** Auxiliary constants for common Content-Format Option values */

#define ANJAY_COAP_FORMAT_APPLICATION_LINK 40

#define ANJAY_COAP_FORMAT_PLAINTEXT 0
#define ANJAY_COAP_FORMAT_OPAQUE 42
#define ANJAY_COAP_FORMAT_TLV 11542
#define ANJAY_COAP_FORMAT_JSON 11543

#ifdef WITH_LEGACY_CONTENT_FORMAT_SUPPORT
#define ANJAY_COAP_FORMAT_LEGACY_PLAINTEXT 1541
#define ANJAY_COAP_FORMAT_LEGACY_TLV 1542
#define ANJAY_COAP_FORMAT_LEGACY_JSON 1543
#define ANJAY_COAP_FORMAT_LEGACY_OPAQUE 1544
#endif // WITH_LEGACY_CONTENT_FORMAT_SUPPORT

/**
 * A magic value used to indicate the absence of the Content-Format option.
 * Mainly used during CoAP message parsing, passing it to the info object does
 * nothing.
 * */
#define ANJAY_COAP_FORMAT_NONE 65535

#pragma GCC visibility pop

#endif // ANJAY_COAP_CONTENT_FORMAT_H