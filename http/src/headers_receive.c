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

#include <avs_commons_config.h>
#include <avs_commons_posix_config.h>

#include <ctype.h>
#include <errno.h>
#include <string.h>

#include "body_receivers.h"
#include "client.h"
#include "headers.h"
#include "http_log.h"

VISIBILITY_SOURCE_BEGIN

typedef struct {
    http_stream_t *stream;
    http_transfer_encoding_t transfer_encoding;
    avs_http_content_encoding_t content_encoding;
    size_t content_length;
    avs_url_t *redirect_url;
} header_parser_state_t;

static int parse_size(size_t *out, const char *in) {
    char *endptr = NULL;
    while (*in && isspace(*in)) {
        ++in;
    }
    if (!(isdigit(*in) || *in == '+')) {
        return -1;
    }
    errno = 0;
    unsigned long long tmp = strtoull(in, &endptr, 0);
    if (errno || !endptr || *endptr || tmp > SIZE_MAX) {
        return -1;
    }
    *out = (size_t) tmp;
    return 0;
}

static int http_handle_header(const char *key, const char *value,
                              header_parser_state_t *state) {
    if (strcasecmp(key, "WWW-Authenticate") == 0) {
        _avs_http_auth_setup(&state->stream->auth, value);
    } else if (strcasecmp(key, "Set-Cookie") == 0) {
        if (_avs_http_set_cookie(state->stream->http, false, value) < 0) {
            return -1;
        }
    } else if (strcasecmp(key, "Set-Cookie2") == 0) {
        if (_avs_http_set_cookie(state->stream->http, true, value) < 0) {
            return -1;
        }
    } else if (strcasecmp(key, "Content-Length") == 0) {
        if (state->transfer_encoding != TRANSFER_IDENTITY
                || parse_size(&state->content_length, value)) {
            return -1;
        }
        state->transfer_encoding = TRANSFER_LENGTH;
    } else if (strcasecmp(key, "Transfer-Encoding") == 0) {
        if (strcasecmp(value, "identity") != 0) { /* see RFC 2616, sec. 4.4 */
            if (state->transfer_encoding != TRANSFER_IDENTITY) {
                return -1;
            }
            state->transfer_encoding = TRANSFER_CHUNKED;
        }
    } else if (strcasecmp(key, "Content-Encoding") == 0) {
        if (strcasecmp(value, "identity") != 0) {
            if (state->content_encoding != AVS_HTTP_CONTENT_IDENTITY) {
                return -1;
            }
            if (strcasecmp(value, "gzip") == 0
                    || strcasecmp(value, "x-gzip") == 0) {
                state->content_encoding = AVS_HTTP_CONTENT_GZIP;
            } else if (strcasecmp(value, "deflate") == 0) {
                state->content_encoding = AVS_HTTP_CONTENT_DEFLATE;
            }
        }
    } else if (strcasecmp(key, "Connection") == 0) {
        if (strcasecmp(value, "close") == 0) {
            state->stream->flags.keep_connection = 0;
        }
    } else if (state->stream->status / 100 == 3
            && strcasecmp(key, "Location") == 0) {
        avs_url_free(state->redirect_url);
        state->redirect_url = avs_url_parse(value);
    } else {
        LOG(DEBUG, "Unhandled HTTP header: %s: %s", key, value);
    }
    return 0;
}

static int discard_line(avs_stream_abstract_t *stream) {
    int c;

    do {
        c = avs_stream_getch(stream, NULL);
        if (c == EOF) {
            LOG(ERROR, "EOF found when discarding line");
            return -1;
        }
    } while (c != '\n');

    return 0;
}

static int get_http_header_line(avs_stream_abstract_t *stream,
                                char *line_buf,
                                size_t line_buf_size) {
    int result;

    do {
        result = avs_stream_getline(stream, NULL, NULL,
                                    line_buf, line_buf_size);

        if (result < 0) {
            LOG(ERROR, "Could not read header line");
            return -1;
        }

        if (result > 0) {
            LOG(WARNING, "HTTP header too long to handle: %s", line_buf);
            if (discard_line(stream)) {
                LOG(ERROR, "Could not discard header line");
                return -1;
            }
        }
    } while (result);

    return 0;
}

static const char *http_header_split(char *line) {
    char *value = strchr(line, ':');
    if (value && *value) {
        *value++ = '\0';
        while (*value && isspace((unsigned char) *value)) {
            *value++ = '\0';
        }
    }
    return value;
}

static int http_receive_headers_internal(header_parser_state_t *state) {
    char header[state->stream->http->buffer_sizes.header_line];
    header[0] = '\0';
    state->stream->flags.keep_connection = 1;
    /* read parse headline */
    if (avs_stream_getline(state->stream->backend, NULL, NULL,
                           header, sizeof(header))) {
        LOG(ERROR, "Could not receive HTTP headline");
        /* default to 100 Continue if nothing received */
        state->stream->status = 100;
        goto http_receive_headers_error;
    }
    if (sscanf(header, "HTTP/%*s %d", &state->stream->status) != 1) {
        /* discard HTTP version
         * some weird servers return HTTP/1.0 to HTTP/1.1 */
        LOG(ERROR, "Bad HTTP headline: %s", header);
        goto http_receive_headers_error;
    }
    LOG(TRACE, "Received HTTP headline, status == %d", stream->status);
    /* handle headers */
    while (1) {
        const char *value = NULL;
        if (get_http_header_line(state->stream->backend,
                                 header, sizeof(header))) {
            LOG(ERROR, "Error receiving headers");
            goto http_receive_headers_error;
        }

        if (header[0] == '\0') { /* empty line */
            break;
        }
        LOG(TRACE, "HTTP header: %s", header);
        if (!(value = http_header_split(header))
                || http_handle_header(header, value, state)) {
            LOG(ERROR, "Error parsing or handling headers");
            goto http_receive_headers_error;
        }
    }

    switch (state->stream->status / 100) {
    case 1: // 100 Continue - ignore and treat as success
        break;

    case 2: // 2xx - success
        state->stream->auth.state.flags.retried = 0;
        if (_avs_http_body_receiver_init(state->stream,
                                         state->transfer_encoding,
                                         state->content_encoding,
                                         state->content_length)) {
            goto http_receive_headers_error;
        }
        state->stream->redirect_count = 0;
        break;

    case 3: // 3xx - redirect
        state->stream->auth.state.flags.retried = 0;
        if (!state->redirect_url) {
            state->stream->status = EINVAL;
        } else {
            int result = _avs_http_redirect(state->stream,
                                            &state->redirect_url);
            if (!result) {
                /* redirect was a success;
                 * still, receiving headers for _this particular_ response
                 * didn't result in a success (i.e. a usable input stream),
                 * so this function returns failure - without clearing the
                 * keep connection flag, as we have already made the new
                 * connection */
                return -1;
            }
            state->stream->status = result;
        }
        goto http_receive_headers_error;

    default: // most likely 5xx - server error
        state->stream->auth.state.flags.retried = 0;
        // fall-through
    case 4: // 4xx - client error
        if (_avs_http_body_receiver_init(state->stream,
                                         state->transfer_encoding,
                                         state->content_encoding,
                                         state->content_length)) {
            goto http_receive_headers_error;
        }
        /* we MUST NOT close connection, as required by TR-069,
         * so we actually receive and discard the response */
        LOG(WARNING, "http_receive_headers: error response");
        if (avs_stream_ignore_to_end(state->stream->body_receiver) < 0) {
            LOG(WARNING, "http_receive_headers: response read error");
            state->stream->flags.keep_connection = 0;
        }
        LOG(TRACE, "http_receive_headers: clearing body receiver");
        avs_stream_cleanup(&state->stream->body_receiver);
        return -1; /* without clearing the keep connection flag */
    }

    LOG(TRACE, "http_receive_headers: success");
    return 0;

http_receive_headers_error:
    LOG(ERROR, "http_receive_headers: failure");
    state->stream->flags.keep_connection = 0;
    return -1;
}

static void update_flags_after_receiving_headers(http_stream_t *stream) {
    if (stream->status / 100 == 3) {
        /* redirect happened */
        stream->flags.should_retry = 1;
    } else if (stream->status == 401
            && (stream->auth.credentials.user
                    || stream->auth.credentials.password)
            && stream->auth.state.flags.type != HTTP_AUTH_TYPE_NONE
            && !stream->auth.state.flags.retried) {
        /* retry authentication */
        stream->auth.state.flags.retried = 1;
        stream->flags.should_retry = 1;
    } else if (stream->status == 417 && !stream->flags.no_expect) {
        /* retry without Expect: 100-continue */
        stream->flags.no_expect = 1;
        stream->flags.should_retry = 1;
    } else {
        stream->flags.should_retry = 0;
    }
}

int _avs_http_receive_headers(http_stream_t *stream) {
    int result;

    /* The only case where we don't want to ignore 100-Continue messages is
     * just after sending chunked message headers - in such case, we need to
     * return to the upper layer to prepare chunked message body. */
    bool skip_100_continue = !stream->flags.chunked_sending;

    LOG(TRACE, "receiving headers, %sskipping 100 Continue",
              skip_100_continue ? "" : "NOT ");
    do {
        header_parser_state_t parser_state = {
            .stream = stream
        };
        result = http_receive_headers_internal(&parser_state);
        avs_url_free(parser_state.redirect_url);
    } while (!result
             && skip_100_continue && stream->status == 100);

    update_flags_after_receiving_headers(stream);
    return result;
}
