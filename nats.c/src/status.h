// Copyright 2015-2019 The NATS Authors
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef STATUS_H_
#define STATUS_H_

#ifdef __cplusplus
extern "C" {
#endif


/// The connection state
typedef enum
{
#if defined(NATS_CONN_STATUS_NO_PREFIX)
    // This is deprecated and applications referencing connection
    // status should be updated to use the values prefixed with NATS_CONN_STATUS_.

    DISCONNECTED = 0, ///< The connection has been disconnected
    CONNECTING,       ///< The connection is in the process or connecting
    CONNECTED,        ///< The connection is connected
    CLOSED,           ///< The connection is closed
    RECONNECTING,     ///< The connection is in the process or reconnecting
    DRAINING_SUBS,    ///< The connection is draining subscriptions
    DRAINING_PUBS,    ///< The connection is draining publishers
#else
    NATS_CONN_STATUS_DISCONNECTED = 0, ///< The connection has been disconnected
    NATS_CONN_STATUS_CONNECTING,       ///< The connection is in the process or connecting
    NATS_CONN_STATUS_CONNECTED,        ///< The connection is connected
    NATS_CONN_STATUS_CLOSED,           ///< The connection is closed
    NATS_CONN_STATUS_RECONNECTING,     ///< The connection is in the process or reconnecting
    NATS_CONN_STATUS_DRAINING_SUBS,    ///< The connection is draining subscriptions
    NATS_CONN_STATUS_DRAINING_PUBS,    ///< The connection is draining publishers
#endif

} natsConnStatus;

/// Status returned by most of the APIs
typedef enum
{
    NATS_OK         = 0,                ///< Success

    NATS_ERR,                           ///< Generic error
    NATS_PROTOCOL_ERROR,                ///< Error when parsing a protocol message,
                                        ///  or not getting the expected message.
    NATS_IO_ERROR,                      ///< IO Error (network communication).
    NATS_LINE_TOO_LONG,                 ///< The protocol message read from the socket
                                        ///  does not fit in the read buffer.

    NATS_CONNECTION_CLOSED,             ///< Operation on this connection failed because
                                        ///  the connection is closed.
    NATS_NO_SERVER,                     ///< Unable to connect, the server could not be
                                        ///  reached or is not running.
    NATS_STALE_CONNECTION,              ///< The server closed our connection because it
                                        ///  did not receive PINGs at the expected interval.
    NATS_SECURE_CONNECTION_WANTED,      ///< The client is configured to use TLS, but the
                                        ///  server is not.
    NATS_SECURE_CONNECTION_REQUIRED,    ///< The server expects a TLS connection.
    NATS_CONNECTION_DISCONNECTED,       ///< The connection was disconnected. Depending on
                                        ///  the configuration, the connection may reconnect.

    NATS_CONNECTION_AUTH_FAILED,        ///< The connection failed due to authentication error.
    NATS_NOT_PERMITTED,                 ///< The action is not permitted.
    NATS_NOT_FOUND,                     ///< An action could not complete because something
                                        ///  was not found. So far, this is an internal error.

    NATS_ADDRESS_MISSING,               ///< Incorrect URL. For instance no host specified in
                                        ///  the URL.

    NATS_INVALID_SUBJECT,               ///< Invalid subject, for instance NULL or empty string.
    NATS_INVALID_ARG,                   ///< An invalid argument is passed to a function. For
                                        ///  instance passing NULL to an API that does not
                                        ///  accept this value.
    NATS_INVALID_SUBSCRIPTION,          ///< The call to a subscription function fails because
                                        ///  the subscription has previously been closed.
    NATS_INVALID_TIMEOUT,               ///< Timeout must be positive numbers.

    NATS_ILLEGAL_STATE,                 ///< An unexpected state, for instance calling
                                        ///  #natsSubscription_NextMsg() on an asynchronous
                                        ///  subscriber.

    NATS_SLOW_CONSUMER,                 ///< The maximum number of messages waiting to be
                                        ///  delivered has been reached. Messages are dropped.

    NATS_MAX_PAYLOAD,                   ///< Attempt to send a payload larger than the maximum
                                        ///  allowed by the NATS Server.
    NATS_MAX_DELIVERED_MSGS,            ///< Attempt to receive more messages than allowed, for
                                        ///  instance because of #natsSubscription_AutoUnsubscribe().

    NATS_INSUFFICIENT_BUFFER,           ///< A buffer is not large enough to accommodate the data.

    NATS_NO_MEMORY,                     ///< An operation could not complete because of insufficient
                                        ///  memory.

    NATS_SYS_ERROR,                     ///< Some system function returned an error.

    NATS_TIMEOUT,                       ///< An operation timed-out. For instance
                                        ///  #natsSubscription_NextMsg().

    NATS_FAILED_TO_INITIALIZE,          ///< The library failed to initialize.
    NATS_NOT_INITIALIZED,               ///< The library is not yet initialized.

    NATS_SSL_ERROR,                     ///< An SSL error occurred when trying to establish a
                                        ///  connection.

    NATS_NO_SERVER_SUPPORT,             ///< The server does not support this action.

    NATS_NOT_YET_CONNECTED,             ///< A connection could not be immediately established and
                                        ///  #natsOptions_SetRetryOnFailedConnect() specified
                                        ///  a connected callback. The connect is retried asynchronously.

    NATS_DRAINING,                      ///< A connection and/or subscription entered the draining mode.
                                        ///  Some operations will fail when in that mode.

    NATS_INVALID_QUEUE_NAME,            ///< An invalid queue name was passed when creating a queue subscription.

} natsStatus;

#ifdef __cplusplus
}
#endif

#endif /* STATUS_H_ */
