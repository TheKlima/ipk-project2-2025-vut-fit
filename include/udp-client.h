/**
* @file udp-client.h
 * @author Andrii Klymenko
 * @brief Class of UDP version of IPK25-CHAT client.
 */

#ifndef UDP_CLIENT_H
#define UDP_CLIENT_H

#include "client.h"
#include <cstring>

/**
 * @class Udp_client
 * @brief UDP implementation of the Client interface for handling communication with the chat server.
 */
class Udp_client : public Client {
private:
    /// Number of bytes used to encode a message ID
    static constexpr uint8_t s_BYTES_IN_MSG_ID{sizeof(uint16_t)};

    /// Character used to terminate variable-length data
    static constexpr char s_VARIABLE_LENGTH_DATA_TERMINATOR{'\0'};

    /// Number of bytes in protocol message type field
    static constexpr uint8_t s_BYTES_IN_PROTOCOL_MSG_TYPE{1};

    /// Number of bytes in reply result field
    static constexpr uint8_t s_BYTES_IN_REPLY_RESULT{1};

    /// Total header size of a protocol message
    static constexpr uint8_t s_BYTES_IN_MSG_HEADER{s_BYTES_IN_PROTOCOL_MSG_TYPE + s_BYTES_IN_MSG_ID};

public:
    /**
     * @brief Constructs a new Udp_client object.
     * @param args Parsed command-line arguments.
     */
    Udp_client(const Args& args);

    /**
     * @brief Sends a BYE message to the server.
     */
    void sendByeMsgToServer() override;

    /// Maximum size of a message (header + payload + terminator)
    static constexpr int s_MAX_MSG_SIZE{s_BYTES_IN_MSG_HEADER + s_BYTES_IN_REPLY_RESULT +
        s_BYTES_IN_MSG_ID + s_MSG_CONTENT_MAX_LENGTH + sizeof(s_VARIABLE_LENGTH_DATA_TERMINATOR)};

private:
    /// Set of confirmed message IDs
    std::bitset<UINT16_MAX + 1> m_confirmed_server_messages{};

    /**
     * @brief Builds and stores an error message to send to the server.
     * @param content Content of the error message.
     */
    void buildErrMsg(std::string content) override;

    /**
     * @brief Builds an AUTH message using user's username and his secret.
     * @param username user's username.
     * @param secret user's secret.
     */
    void buildAuthMsg(const std::string& username, const std::string& secret) override;

    /**
     * @brief Builds a JOIN message using channel id.
     * @param channel_id id of the channel the user wants to join.
     */
    void buildJoinMsg(const std::string& channel_id) override;

    /**
     * @brief Builds a MSG message from user input.
     * @param user_msg user's message.
     */
    void buildMsgMsg(const std::string& user_msg) override;

    /**
     * @brief Builds a BYE message for clean disconnection.
     */
    void buildByeMsg() override;

    /**
     * @brief Adds a message ID to the message being sent to the server.
     * @param msg_id message's id
     */
    void addMsgIdToMsgToServer(uint16_t msg_id);

    /**
     * @brief Sends the constructed message to the server.
     */
    void sendMsgToServer() override;

    /**
     * @brief Extracts the message ID from a UDP message.
     * @param msg The message as a string view.
     * @return The extracted message ID as a 16-bit unsigned integer.
     */
    uint16_t getMsgId(std::string_view msg) const;

    /**
     * @brief Extracts the reference message ID from a REPLY message.
     * @param reply_msg The REPLY message as a string view.
     * @return The reference message ID contained in the REPLY.
     */
    uint16_t getRefMsgId(std::string_view reply_msg) const;

    /**
     * @brief Sends a CONFIRM message to the server acknowledging a received message.
     * @param ref_msg_id The ID of the message being confirmed.
     */
    void sendConfirmMsg(uint16_t ref_msg_id);

    /**
     * @brief Handles input from standard input (stdin).
     *        Called when stdin becomes readable and when it is enabled.
     */
    void processStdinEvent() override;

    /**
     * @brief Handles timer expiration event for retransmissions or timeouts.
     */
    void processTimerEvent() override;

    /**
     * @brief Handles an event from the UDP socket.
     * @return uint8_t 0 if EXIT_SUCCESS needs to be returned in main(), 1 if EXIT_FAILURE needs to be returned in main(),
     * 2 if client's loop needs to be continued
     */
    uint8_t processSocketEvent() override;

    /**
     * @brief Handles SIGINT signal (e.g., Ctrl+C) bye sending BYE message to the server.
     */
    void sigintHandler() override;

    /**
     * @brief Processes an incoming PING message from the server.
     * @param ping_msg The message string.
     * @param ping_msg_length The length of the message.
     */
    void processServerPingMsg(const std::string& ping_msg, unsigned ping_msg_length);

    /**
     * @brief Checks if the length of a PING message is valid.
     * @param ping_msg_length The length of the message.
     * @return True if valid, false otherwise.
     */
    bool isValidPingMsgLength(unsigned ping_msg_length) const;

    /**
     * @brief Processes a BYE message from the server.
     * @param bye_msg The message string.
     * @param bye_msg_length The length of the message.
     * @return 2 if malformed BYE message was received and ERR_EXIT must be returned from main(), 0 otherwise
     * (ERR_SUCCESS must be returned from main())
     */
    uint8_t processServerByeMsg(const std::string& bye_msg, unsigned bye_msg_length);

    /**
     * @brief Validates a BYE message.
     * @param bye_msg The message string.
     * @param bye_msg_length The length of the message.
     * @return True if valid, false otherwise.
     */
    bool isValidByeMsg(const std::string& bye_msg, unsigned bye_msg_length) const;

    /**
     * @brief Validates the length of a BYE message.
     * @param bye_msg_length The length of the message.
     * @return True if valid, false otherwise.
     */
    bool isValidByeMsgLength(unsigned bye_msg_length) const;

    /**
     * @brief Processes an ERR message from the server.
     * @param err_msg The message string.
     * @param err_msg_length The length of the message.
     * @return Status code indicating the result of processing.
     */
    uint8_t processServerErrMsg(const std::string& err_msg, unsigned err_msg_length);

    /**
     * @brief Validates an ERR message.
     * @param err_msg The message string.
     * @param err_msg_length The length of the message.
     * @return True if valid, false otherwise.
     */
    bool isValidErrMsg(const std::string& err_msg, unsigned err_msg_length) const;

    /**
     * @brief Validates the length of an ERR message.
     * @param err_msg_length The length of the message.
     * @return True if valid, false otherwise.
     */
    bool isValidErrMsgLength(unsigned err_msg_length) const;

    /**
     * @brief Processes a MSG message from the server.
     * @param msg_msg The message string.
     * @param msg_msg_length The length of the message.
     */
    void processServerMsgMsg(const std::string& msg_msg, unsigned msg_msg_length);

    /**
     * @brief Validates a MSG message.
     * @param msg_msg The message string.
     * @param msg_msg_length The length of the message.
     * @return True if valid, false otherwise.
     */
    bool isValidMsgMsg(const std::string& msg_msg, unsigned msg_msg_length) const;

    /**
     * @brief Validates the length of a MSG message.
     * @param msg_msg_length The length of the message.
     * @return True if valid, false otherwise.
     */
    bool isValidMsgMsgLength(unsigned msg_msg_length) const;

    /**
     * @brief Processes a REPLY message from the server.
     * @param reply_msg The REPLY message string.
     * @param reply_msg_length The length of the message.
     * @param server_addr The address of the server.
     */
    void processServerReplyMsg(const std::string& reply_msg, unsigned reply_msg_length, sockaddr_in& server_addr);

    /**
     * @brief Validates a REPLY message.
     * @param reply_msg The message string.
     * @param reply_msg_length The length of the message.
     * @return True if valid, false otherwise.
     */
    bool isValidReplyMsg(const std::string& reply_msg, unsigned reply_msg_length) const;

    /**
     * @brief Validates the length of a REPLY message.
     * @param reply_msg_length The length of the message.
     * @return True if valid, false otherwise.
     */
    bool isValidReplyMsgLength(unsigned reply_msg_length) const;

    /**
     * @brief Validates the result character of a REPLY message.
     * @param reply_msg_result The result character (1 or 0 is expected).
     * @return True if valid, false otherwise.
     */
    bool isValidReplyMsgResult(char reply_msg_result) const;

    /**
     * @brief Processes a CONFIRM message from the server.
     * @param confirm_msg The message string.
     * @param confirm_msg_length The length of the message.
     * @return uint8_t 0 if EXIT_SUCCESS needs to be returned in main(), 1 if EXIT_FAILURE needs to be returned in main(),
     * 2 if client's loop needs to be continued
     */
    uint8_t processServerConfirmMsg(const std::string& confirm_msg, unsigned confirm_msg_length);

    /**
     * @brief Validates a CONFIRM message.
     * @param confirm_msg The message string.
     * @param confirm_msg_length The length of the message.
     * @return True if valid, false otherwise.
     */
    bool isValidConfirmMsg(const std::string& confirm_msg, unsigned confirm_msg_length) const;

    /**
     * @brief Validates the length of a CONFIRM message.
     * @param confirm_msg_length The length of the message.
     * @return True if valid, false otherwise.
     */
    bool isValidConfirmMsgLength(unsigned confirm_msg_length) const;

    /**
     * @brief Processes any incoming message from the server.
     * @param msg_from_server The message string.
     * @param msg_from_server_length The length of the message.
     * @param server_addr The address of the server.
     * @return uint8_t 0 if EXIT_SUCCESS needs to be returned in main(), 1 if EXIT_FAILURE needs to be returned in main(),
     * 2 if client's loop needs to be continued
     */
    uint8_t processMessageFromServer(const std::string& msg_from_server, unsigned msg_from_server_length, sockaddr_in& server_addr);

    /**
     * @brief Sends an error message to the server.
     * @param err_msg A null-terminated C-string containing the error message.
     */
    void sendErrMsg(const char* err_msg);

    /**
     * @brief Gets the compiled regex for parsing MSG messages.
     * @return A reference to the regex object.
     */
    static const std::regex& getMsgMsgRegex();

    /**
     * @brief Gets the compiled regex for parsing ERR messages.
     * @return A reference to the regex object.
     */
    static const std::regex& getErrMsgRegex();

    /**
     * @brief Gets the compiled regex for parsing BYE messages.
     * @return A reference to the regex object.
     */
    static const std::regex& getByeMsgRegex();

    /**
     * @brief Gets a regex that matches printable characters and the null terminator.
     * @return A reference to the regex object.
     */
    static const std::regex& getPrintableCharsAndTerminatorRegex();

    /**
     * @brief Gets a regex that matches printable characters, space, line feed, and the null terminator.
     * @return A reference to the regex object.
     */
    static const std::regex& getPrintableCharsSpaceLfAndTerminatorRegex();

    /**
     * @brief Gets the string representation of the escaped null terminator.
     * @return A reference to the string containing the escaped null character.
     */
    static const std::string& getEscapedVariableLengthTerminator();

    uint8_t m_allowed_retransmissions{m_args.getUdpMaxRetransCount()};
    uint16_t m_msg_to_server_id{0};
    bool m_is_waiting_for_confirm{false};
    bool m_is_waiting_for_bye_confirm{false};

    static constexpr uint8_t s_MIN_VARIABLE_DATA_LENGTH{1};
};

#endif // UDP_CLIENT_H
