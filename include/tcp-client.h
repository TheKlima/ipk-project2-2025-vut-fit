/**
 * @file tcp-client.h
 * @author Andrii Klymenko
 * @brief Class of TCP version of IPK25-CHAT client.
 */

#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H

#include "client.h"
#include <cstring>

/**
 * @brief Represents a TCP-based chat client implementing the IPK25-CHAT protocol.
 *
 * This class handles building and sending protocol messages over a TCP socket, processing
 * server responses, and managing input/output events.
 */
class Tcp_client : public Client {
public:
    /**
     * @brief Constructs a TCP client with the given command-line arguments.
     * @param args Command-line arguments specifying configuration such as address and port.
     */
    Tcp_client(const Args& args);

    /**
     * @brief Sends a BYE message to the server and terminates a connection
     */
    void sendByeMsgToServer() override;

    static constexpr const char* s_END_OF_MESSAGE{"\r\n"};
    static constexpr std::size_t s_BYTES_IN_END_OF_MESSAGE{2};

    /// @brief Maximum message size for outgoing incoming/outgoing messages
    static constexpr int s_MAX_MSG_SIZE{static_cast<int> (strlen("MSG FROM")) + static_cast<int> (strlen(" ")) +
        s_DISPLAY_NAME_MAX_LENGTH + static_cast<int> (strlen(" IS ")) + s_MSG_CONTENT_MAX_LENGTH + static_cast<int> (s_BYTES_IN_END_OF_MESSAGE)};

private:
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
     * @brief Sends the built message to the server.
     */
    void sendMsgToServer() override;

    /**
     * @brief Sends an error message and terminates the client.
     * @param err_msg The message to display before terminating.
     */
    void sendErrMsgAndTerminate(const char* err_msg);

    /**
     * @brief Handles a timeout event from the timer file descriptor.
     */
    void processTimerEvent() override;

    /**
     * @brief Processes a read event on the socket file descriptor.
     * @return uint8_t 0 if EXIT_SUCCESS needs to be returned in main(), 1 if EXIT_FAILURE needs to be returned in main(),
     * 2 if client's loop needs to be continued
     */
    uint8_t processSocketEvent() override;

    /**
     * @brief Processes a read event on standard input (user input).
     */
    void processStdinEvent() override;

    /**
     * @brief Handles SIGINT (Ctrl+C) by sending BYE message and successfully terminating.
     */
    void sigintHandler() override;

    /**
     * @brief Processes a server BYE message.
     * @param bye_msg_from_server The full BYE server message string.
     */
    void processServerByeMsg(const std::string& bye_msg_from_server);

    /**
     * @brief Processes a server ERR message.
     * @param err_msg_from_server The full ERR server message string.
     */
    void processServerErrMsg(const std::string& err_msg_from_server);

    /**
     * @brief Processes a server MSG message.
     * @param msg_msg_from_server The full MSG server message string.
     */
    void processServerMsgMsg(const std::string& msg_msg_from_server);

    /**
     * @brief Processes a server MSG message.
     * @param msg_msg_from_server The full MSG server message string.
     */
    void processServerReplyMsg(const std::string& reply_msg_from_server);

    /**
     * @brief Processes an incoming server message.
     * @param msg_from_server The full message received from the server.
     * @return uint8_t 0 if EXIT_SUCCESS needs to be returned from main(), 1 if EXIT_FAILURE needs to be returned from main(),
     * 2 if client's loop needs to be continued
     */
    uint8_t processMessageFromServer(const std::string& msg_from_server);

    /**
     * @brief Extracts the protocol message type from a server message.
     * @param msg_from_server The message received from the server.
     * @return The detected protocol message type.
     */
    Protocol_msg_type getServerMsgType(const std::string& msg_from_server) const;

    /**
     * @brief Gets the regex used to match REPLY messages.
     */
    static const std::regex& getReplyMsgRegex();

    /**
     * @brief Gets the regex used to match MSG messages.
     */
    static const std::regex& getMsgMsgRegex();

    /**
     * @brief Gets the regex used to match ERR messages.
     */
    static const std::regex& getErrMsgRegex();

    /**
     * @brief Gets the regex used to match BYE messages.
     */
    static const std::regex& getByeMsgRegex();

    /// @brief Internal buffer for a message received from the server.
    std::string m_msg_from_server{};
};

#endif // TCP_CLIENT_H
