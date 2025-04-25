/**
 * @file args.h
 * @author Andrii Klymenko
 * @brief Parses and stores command-line arguments for the IPK25-CHAT application.
 */

#ifndef ARGS_H
#define ARGS_H

#include <netinet/in.h> // struct sockaddr_in
#include <array>
#include <string>

/**
 * @class Args
 * @brief Handles command-line argument parsing and storage for the client application.
 */
class Args {
public:
    /**
     * @brief Constructs an Args object and parses the input arguments.
     * @param argc Number of arguments.
     * @param argv Array of argument strings.
     */
    Args(int argc, char** argv);

    // 'getters'

    /// @return True if TCP is selected; false for UDP.
    bool getIsTcp() const;

    /// @return Maximum number of UDP retransmission attempts.
    uint8_t getUdpMaxRetransCount() const;

    /// @return Pointer to internal sockaddr_in structure used for the server address.
    struct sockaddr_in* getServerAddrStructAddress();

    /// @return Size of the sockaddr_in structure.
    int getSizeofServerAddrStruct() const;

    /// @return Timeout in milliseconds used to confirm UDP delivery.
    uint16_t getUdpConfirmTimeout() const;

    /// @return True if the help flag (-h) was used.
    bool getIsHelpUsed() const;

    // end of 'getters'

    // bool getIsConstructorErr() const;

    /// @brief Prints usage information to stdout.
    static void printHelp();
    
private:
    uint16_t m_server_port{4567};                                        ///< Server port number.
    uint16_t m_udp_confirm_timeout{250};                                 ///< Timeout for UDP confirmation (ms).
    uint8_t m_udp_max_retrans_count{3};                                  ///< Max UDP retransmission attempts.
    bool m_is_help_used{false};                                          ///< Indicates if help was requested.
    const std::array<char, 6> m_arg_flags{'t', 's', 'p', 'd', 'r', 'h'}; ///< Valid argument flags.
    bool m_is_tcp{};                                                     ///< Protocol flag: true for TCP, false for UDP.
    struct sockaddr_in m_server_addr{};                                  ///< Parsed server address.

    // void checkNextArgument(int current_arg, int argc) const;

    /**
     * @brief Converts and validates a given server address (IP or hostname).
     * @param server_addr Server address string (IP or hostname).
     */
    void processServerAddress(const char* server_addr);

    /**
     * @brief Converts a hostname to an IP address and stores it in m_server_addr.
     * @param hostname Server hostname.
     */
    void hostnameToIpAddress(const char* hostname);
};

#endif // ARGS_H
