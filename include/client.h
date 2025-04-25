/**
 * @file client.h
 * @author Andrii Klymenko
 * @brief Base class for the TCP and UDP versions of IPK25-CHAT client.
 */

#ifndef CLIENT_H
#define CLIENT_H

#include "args.h"
#include "protocol-msg-type.h"
#include "fsm.h"
#include <regex>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <csignal>
#include <functional>

/**
 * @class Client
 * @brief Abstract base class for implementing a chat client using FSM and epoll.
 */
class Client {
public:

    /**
     * @brief Constructs a Client object with parsed arguments.
     * @param args Command line arguments.
     */
    Client(const Args& args);

    /**
     * @brief Factory method for creating a Client instance.
     * @param args Command line arguments.
     * @return A unique pointer to a new Client instance.
     */
    static std::unique_ptr<Client> create(const Args& args);
    // virtual bool run() = 0;

    /**
     * @brief Runs the main client loop.
     * @return True on success, false on failure.
     */
    bool run();

    /**
     * @brief Virtual destructor.
     */
    virtual ~Client();

    /**
     * @brief Sends a BYE message to the server. Must be implemented by derived class.
     */
    virtual void sendByeMsgToServer() = 0;

    /// Maximum number of seconds to wait for a REPLY message.
    static constexpr uint8_t s_MAX_REPLY_WAIT_TIME{5};

protected:
    Args m_args; ///< Parsed arguments.
    FSM_state m_current_state{FSM_state::S_START}; ///< Current state of the FSM.

    std::string m_user_display_name{"unknown"}; ///< Display name of the user.

    /// Supported user commands.
    const std::array<std::string_view, 4> m_user_commands{"/auth", "/help", "/join", "/rename"};

    bool m_is_waiting_for_reply{false}; ///< True if waiting for server REPLY message.

    // File descriptors
    int m_client_socket{}; ///< Socket file descriptor.
    int m_epoll_fd{};      ///< Epoll file descriptor.
    int m_timer_fd{};      ///< Timer file descriptor.

    // Epoll event structures
    struct epoll_event m_socket_event{.events = EPOLLIN, .data = {} };
    struct epoll_event m_stdin_event{.events = EPOLLIN, .data = {} };
    struct epoll_event m_timer_event{.events = EPOLLIN, .data = {} };
    struct epoll_event m_actual_event{}; ///< Used in epoll_wait().
    short m_epoll_event_count{};         ///< Number of ready epoll events.
    static constexpr uint8_t s_MAX_EPOLL_EVENT_NUMBER{1}; ///< Max number of events to process at once.

    std::string m_msg_to_server{};          ///< Message prepared to be sent to the server.
    std::unique_ptr<char[]> m_server_msg{}; ///< Raw buffer to receive message from server.

    /**
     * @brief Handles non-MSG user commands.
     * @param user_input Parsed user input.
     * @return True if handled successfully, false otherwise.
     */
    bool processNonMsgToServer(const std::vector<std::string>& user_input);

    /**
     * @brief Reads and parses user input from stdin.
     * @return Vector of parsed tokens.
     */
    std::vector<std::string> parseUserInput();

    /**
     * @brief Starts the confirm/reply timer.
     * @param time Timeout in milliseconds.
     */
    void startTimer(uint16_t time);

    /**
     * @brief Stops the reply timer.
     */
    void stopTimer();

    /**
     * @brief Disables stdin events from epoll.
     */
    void disableStdinEvents();

    /**
     * @brief Re-enables stdin events in epoll.
     */
    void enableStdinEvents();

    /**
     * @brief Gets the message type associated with a command string.
     * @param command Command as a string_view.
     * @return Corresponding Protocol_msg_type.
     */
    Protocol_msg_type getUserMsgType(std::string_view command) const;

    /**
     * @brief Builds a message from user input to send to the server.
     * @param user_input Parsed input from user.
     */
    void buildUserMsgToServer(const std::vector<std::string>& user_input);

    /**
     * @brief Prints an error message received from the server.
     */
    void printErrFromServer(std::string display_name, std::string message_content) const;

    /**
     * @brief Prints a received chat message.
     */
    void outputIncomingMsg(std::string display_name, std::string content) const;

    /**
     * @brief Prints a received reply message.
     */
    void outputIncomingReply(bool is_positive, std::string content) const;

    /**
     * @brief Validates display name length.
     */
    bool isValidDisplayNameLength(unsigned display_name_length) const;

    /**
     * @brief Validates message content length.
     */
    bool isValidMsgContentLength(unsigned msg_content_length) const;

    /// Max limits
    static constexpr uint16_t s_MSG_CONTENT_MAX_LENGTH{60000};
    static constexpr uint8_t s_DISPLAY_NAME_MAX_LENGTH{20};
    static constexpr uint8_t s_USERNAME_MAX_LENGTH{20};
    static constexpr uint8_t s_CHANEL_ID_MAX_LENGTH{20};
    static constexpr uint8_t s_USER_SECRET_MAX_LENGTH{128};

    /**
     * @brief Gets a regex-compatible set of allowed characters: alphanumerics, underscore, and dash.
     */
    static const std::string& getAlphaNumericUnderlineDash();

    /**
     * @brief Gets printable ASCII characters (excluding space and LF).
     */
    static const std::string& getPrintableChars();

    /**
     * @brief Gets printable ASCII characters including space and LF.
     */
    static const std::string& getPrintableCharsSpaceLf();

private:
    /**
     * @brief Builds an ERR message.
     */
    virtual void buildErrMsg(std::string content) = 0;

    /**
     * @brief Builds an AUTH message.
     */
    virtual void buildAuthMsg(const std::string& username, const std::string& secret) = 0;

    /**
     * @brief Builds a JOIN message.
     */
    virtual void buildJoinMsg(const std::string& channel_id) = 0;

    /**
     * @brief Builds a MSG message.
     */
    virtual void buildMsgMsg(const std::string& user_msg) = 0;

    /**
     * @brief Builds a BYE message.
     */
    virtual void buildByeMsg() = 0;

    /**
     * @brief Sends the current message to the server.
     */
    virtual void sendMsgToServer() = 0;

    /**
     * @brief Processes timer expiration events.
     */
    virtual void processTimerEvent() = 0;


    /**
     * @brief Processes socket events.
     * @return uint8_t 0 if EXIT_SUCCESS needs to be returned from main(), 1 if EXIT_FAILURE needs to be returned from main(),
     * 2 if client's loop needs to be continued
     */
    virtual uint8_t processSocketEvent() = 0;

    /**
     * @brief Handles stdin client's input.
     */
    virtual void processStdinEvent() = 0;

    /**
     * @brief Handles SIGINT (Ctrl+C).
     */
    virtual void sigintHandler() = 0;


    /**
     * @brief Creates the timer file descriptor.
     */
    void createTimerFd();

    /**
     * @brief Checks if the client can currently send a message of the given type.
     */
    bool canSendMessageType(Protocol_msg_type msg_type) const;

    /**
     * @brief Creates the socket used for communication.
     */
    void createClientSocket();

    /**
     * @brief Prints supported user commands to stdout.
     */
    void printSupportedCommands() const;

    /**
     * @brief Adds socket, stdin, and timer to the epoll instance.
     */
    void addEntriesToEpollInstance();

    /**
     * @brief Creates the epoll instance.
     */
    void createEpollFd();

    /**
     * @brief Adds a file descriptor to epoll with the given event configuration.
     */
    void addFileDescriptorToEpollEvent(struct epoll_event& event, int file_descriptor);

    /**
     * @brief Extracts and tokenizes user input using regex match results.
     */
    std::vector<std::string> getUserInput(const std::smatch& user_input_matches) const;

    /// Regex matchers for user commands
    static const std::regex& getAuthCommandRegex();
    static const std::regex& getJoinCommandRegex();
    static const std::regex& getRenameCommandRegex();
    static const std::regex& getHelpCommandRegex();
    static const std::regex& getUserMsgRegex();

    /// SIGINT callback handler (used with signal())
    static std::function<void()> m_sigint_callback;
};

#endif
