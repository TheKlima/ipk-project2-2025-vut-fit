/**
 * @file protocol_msg_type.h
 * @author Andrii Klymenko
 * @brief Enumerates all possible message types defined in the IPK25-CHAT protocol.
 */

#ifndef PROTOCOL_MSG_TYPE_H
#define PROTOCOL_MSG_TYPE_H

/**
 * @brief Represents different message types used in the IPK25-CHAT protocol.
 */
enum class Protocol_msg_type
{
    M_CONFIRM = 0x00, ///< Confirmation message.
    M_REPLY   = 0x01, ///< Server reply to AUTH or JOIN.
    M_AUTH    = 0x02, ///< Authentication request from client.
    M_JOIN    = 0x03, ///< Join request from client.
    M_MSG     = 0x04, ///< Chat message.
    M_PING    = 0xFD, ///< Ping message from server.
    M_ERR     = 0xFE, ///< Error message.
    M_BYE     = 0xFF, ///< Termination message.
    M_UNKNOWN         ///< Unknown or unrecognized message type.
};

#endif // PROTOCOL_MSG_TYPE_H
