/**
 * @file udp-client.cpp
 * @author Andrii Klymenko
 * @brief Implementation of the class of UDP version of IPK25-CHAT client.
 */

#include "udp-client.h"
#include "exception.h"
#include "error.h"
#include <iostream>
#include <regex>
#include <csignal>
#include <iomanip>

Udp_client::Udp_client(const Args& args)
    :
    Client::Client{args},
    m_msg_to_server_id{0}
{

}

uint8_t Udp_client::processMessageFromServer(const std::string& msg_from_server, unsigned msg_from_server_length, sockaddr_in& server_addr)
{
    if(static_cast<Protocol_msg_type> (static_cast<unsigned char> (msg_from_server[0])) == Protocol_msg_type::M_BYE)
    {
        return processServerByeMsg(msg_from_server, msg_from_server_length);
    }

    if(static_cast<Protocol_msg_type> (static_cast<unsigned char> (msg_from_server[0])) == Protocol_msg_type::M_ERR)
    {
        return processServerErrMsg(msg_from_server, msg_from_server_length);
    }

    switch(m_current_state)
    {
        case FSM_state::S_START:
            switch(static_cast<Protocol_msg_type> (static_cast<unsigned char> (msg_from_server[0])))
            {
                case Protocol_msg_type::M_CONFIRM:
                    return processServerConfirmMsg(msg_from_server, msg_from_server_length);

                default:
                    sendErrMsg("ERROR: only messages of types BYE, ERR or CONFIRM are expected to be received"
                        " from the server in the client's START state.");
                    break;
            }
            break;
        case FSM_state::S_AUTH:
            switch(static_cast<Protocol_msg_type> (static_cast<unsigned char> (msg_from_server[0])))
            {
                case Protocol_msg_type::M_CONFIRM:
                    return processServerConfirmMsg(msg_from_server, msg_from_server_length);

                case Protocol_msg_type::M_REPLY:
                    processServerReplyMsg(msg_from_server, msg_from_server_length, server_addr);
                    break;

                case Protocol_msg_type::M_PING:
                    processServerPingMsg(msg_from_server, msg_from_server_length);
                    break;

                default:
                    sendErrMsg("ERROR: only messages of types BYE, ERR, CONFIRM, PING and REPLY are expected to be received"
                        " from the server in the client's AUTH state.");
                    break;
            }
            break;
        case FSM_state::S_OPEN:
            switch(static_cast<Protocol_msg_type> (static_cast<unsigned char> (msg_from_server[0])))
            {
                case Protocol_msg_type::M_CONFIRM:
                    return processServerConfirmMsg(msg_from_server, msg_from_server_length);

                case Protocol_msg_type::M_MSG:
                    processServerMsgMsg(msg_from_server, msg_from_server_length);
                    break;

                case Protocol_msg_type::M_PING:
                    processServerPingMsg(msg_from_server, msg_from_server_length);
                    break;

                default:
                    sendErrMsg("ERROR: only messages of types BYE, ERR, CONFIRM, PING, JOIN and MSG are expected to be received"
                        " from the server in the client's OPEN state.");
                    break;
            }
            break;

        case FSM_state::S_JOIN:
            switch(static_cast<Protocol_msg_type> (static_cast<unsigned char> (msg_from_server[0])))
            {
                case Protocol_msg_type::M_MSG:
                    processServerMsgMsg(msg_from_server, msg_from_server_length);
                    break;

                case Protocol_msg_type::M_PING:
                    processServerPingMsg(msg_from_server, msg_from_server_length);
                    break;

                case Protocol_msg_type::M_REPLY:
                    processServerReplyMsg(msg_from_server, msg_from_server_length, server_addr);
                    break;

                case Protocol_msg_type::M_CONFIRM:
                    return processServerConfirmMsg(msg_from_server, msg_from_server_length);

                default:
                    sendErrMsg("ERROR: only messages of types BYE, ERR, CONFIRM, PING, REPLY and MSG are expected to be received"
                        " from the server in the client's JOIN state.");
                    break;
            }
            break;

        default:
            sendErrMsg("invalid client's FSM state.");
            break;
    }

    return 2;
}

uint8_t Udp_client::processSocketEvent()
{
    sockaddr_in server_addr{};
    socklen_t server_addr_len{sizeof(server_addr)};

    const long server_msg_length{recvfrom(m_client_socket, m_server_msg.get(), s_MAX_MSG_SIZE + 1, 0,
        reinterpret_cast<sockaddr*>(&server_addr), &server_addr_len)};

    if(server_msg_length < 0)
    {
        sendErrMsg("ERROR: couldn't receive a message from the server: recv() has failed.");
        return 2;
    }

    if(server_msg_length > s_MAX_MSG_SIZE)
    {

        sendErrMsg("ERROR: too long message from server.");
        return 2;
    }

    std::string msg_from_server{m_server_msg.get(), static_cast<unsigned long> (server_msg_length)};
    return processMessageFromServer(msg_from_server, server_msg_length, server_addr);
}

bool Udp_client::isValidConfirmMsg(const std::string& confirm_msg, unsigned confirm_msg_length) const
{
    return static_cast<Protocol_msg_type> (static_cast<unsigned char> (confirm_msg[0])) == Protocol_msg_type::M_CONFIRM &&
        isValidConfirmMsgLength(confirm_msg_length);
}

uint8_t Udp_client::processServerConfirmMsg(const std::string& confirm_msg, unsigned confirm_msg_length)
{
    switch(m_current_state)
    {
        case FSM_state::S_START:
        case FSM_state::S_AUTH:
        case FSM_state::S_OPEN:
        case FSM_state::S_JOIN:
            break;
        default:
            sendErrMsg("ERROR: received a CONFIRM message in unexpected state.");
            return 2;
    }

    if(isValidConfirmMsg(confirm_msg, confirm_msg_length))
    {
        if(getMsgId(confirm_msg) == m_msg_to_server_id)
        {
            if(static_cast<unsigned char> (m_msg_to_server[0]) == static_cast<unsigned char> (Protocol_msg_type::M_BYE))
            {
                return 0;
            }

            if(static_cast<unsigned char> (m_msg_to_server[0]) == static_cast<unsigned char> (Protocol_msg_type::M_ERR))
            {
                return 1;
            }

            switch(m_current_state)
            {
                case FSM_state::S_START:
                    // sendErrMsg("ERROR: confirm to unexpected message in the START state.");
                    break;

                case FSM_state::S_AUTH:
                    switch(static_cast<unsigned char> (m_msg_to_server[0]))
                    {
                        case static_cast<unsigned char> (Protocol_msg_type::M_AUTH):
                            m_is_waiting_for_confirm = false;
                            m_is_waiting_for_reply = true;
                            m_allowed_retransmissions = m_args.getUdpMaxRetransCount();
                            ++m_msg_to_server_id;
                            startTimer(s_MAX_REPLY_WAIT_TIME * 1000); // Time in ms
                            break;
                        default:
                            // sendErrMsg("ERROR: confirm to unexpected message in AUTH state.");
                            break;
                    }
                    break;


                case FSM_state::S_OPEN:
                    switch(static_cast<unsigned char> (m_msg_to_server[0]))
                    {
                        case static_cast<unsigned char> (Protocol_msg_type::M_MSG):
                            stopTimer();
                            m_is_waiting_for_confirm = false;
                            m_allowed_retransmissions = m_args.getUdpMaxRetransCount();
                            ++m_msg_to_server_id;
                            enableStdinEvents();
                            break;

                        case static_cast<unsigned char> (Protocol_msg_type::M_JOIN):
                            m_is_waiting_for_confirm = false;
                            m_is_waiting_for_reply = true;
                            m_allowed_retransmissions = m_args.getUdpMaxRetransCount();
                            ++m_msg_to_server_id;
                            m_current_state = FSM_state::S_JOIN;
                            startTimer(s_MAX_REPLY_WAIT_TIME * 1000); // Time in ms
                            break;

                        default:
                            // sendErrMsg("ERROR: confirm to unexpected message in OPEN state.");
                            break;
                    }
                    break;

                case FSM_state::S_JOIN:
                    // sendErrMsg("ERROR: confirm to unexpected message in the START state.");
                    break;

                default:
                    sendErrMsg("ERROR: received a CONFIRM message in unexpected state.");
                    break;
            }
        }

        return 2;
    }

    sendErrMsg("ERROR: received a malformed CONFIRM message from the server.");
    return 2;
}

void Udp_client::sendErrMsg(const char* err_msg)
{
    buildErrMsg(err_msg);
    printErrMsg(m_msg_to_server.substr(s_BYTES_IN_MSG_HEADER + m_user_display_name.size()
        + sizeof(s_VARIABLE_LENGTH_DATA_TERMINATOR) + strlen("ERROR: ")));
    sendMsgToServer();
    disableStdinEvents();
    startTimer(m_args.getUdpConfirmTimeout()); // Time in ms
    m_is_waiting_for_confirm = true;
    m_is_waiting_for_reply = false;
}

bool Udp_client::isValidConfirmMsgLength(unsigned confirm_msg_length) const
{
    return confirm_msg_length == s_BYTES_IN_MSG_HEADER;
}

void Udp_client::sigintHandler()
{
    sendByeMsgToServer();
}

void Udp_client::processTimerEvent()
{
    uint64_t expirations{};
    ssize_t bytes_read = read(m_timer_fd, &expirations, sizeof(expirations));
    (void) bytes_read;

    if(m_is_waiting_for_confirm || m_is_waiting_for_bye_confirm)
    {
        if(m_allowed_retransmissions == 0)
        {
            throw Exception{"exceeded udp max retransmission number."};
        }

        sendMsgToServer();
        startTimer(m_args.getUdpConfirmTimeout()); // Time in ms
        --m_allowed_retransmissions;
    }
    else if(m_is_waiting_for_reply)
    {
        sendErrMsg("ERROR: waited too long for the server's reply.");
    }
    else
    {
        throw Exception{"timer event, but m_is_waiting_for_confirm and m_is_waiting_for_reply are false."};
    }
}

bool Udp_client::isValidMsgMsg(const std::string& msg_msg, unsigned msg_msg_length) const
{
    if(!isValidMsgMsgLength(msg_msg_length) || static_cast<Protocol_msg_type> (static_cast<unsigned char> (msg_msg[0])) != Protocol_msg_type::M_MSG)
    {
        return false;
    }

    std::smatch matches{};

    std::string skipped_msg_header{msg_msg.substr(s_BYTES_IN_MSG_HEADER)};
    if(std::regex_match(skipped_msg_header, matches, getMsgMsgRegex()) && isValidDisplayNameLength(matches[1].length())
            && isValidMsgContentLength(matches[2].length()))
    {
        if(!m_confirmed_server_messages.test(getMsgId(msg_msg)))
        {
            outputIncomingMsg(matches[1], matches[2]);
        }

        return true;
    }

    return false;
}

void Udp_client::processServerMsgMsg(const std::string& msg_msg, unsigned msg_msg_length)
{
    if(isValidMsgMsg(msg_msg, msg_msg_length))
    {
        sendConfirmMsg(getMsgId(msg_msg));
        return;
    }

    sendErrMsg("ERROR: received a malformed MSG message from the server.");
}

bool Udp_client::isValidMsgMsgLength(unsigned msg_msg_length) const
{
    return msg_msg_length >= s_BYTES_IN_MSG_HEADER + s_MIN_VARIABLE_DATA_LENGTH +
        sizeof(s_VARIABLE_LENGTH_DATA_TERMINATOR) + s_MIN_VARIABLE_DATA_LENGTH + sizeof(s_VARIABLE_LENGTH_DATA_TERMINATOR) &&
            msg_msg_length <= s_BYTES_IN_MSG_HEADER + s_DISPLAY_NAME_MAX_LENGTH +
                sizeof(s_VARIABLE_LENGTH_DATA_TERMINATOR) + s_MSG_CONTENT_MAX_LENGTH + sizeof(s_VARIABLE_LENGTH_DATA_TERMINATOR);
}

void Udp_client::processServerPingMsg(const std::string& ping_msg, unsigned ping_msg_length)
{
    if(isValidPingMsgLength(ping_msg_length))
    {
        sendConfirmMsg(getMsgId(ping_msg));
        return;
    }

    sendErrMsg("ERROR: received a malformed PING message from the server.");
}

bool Udp_client::isValidPingMsgLength(unsigned ping_msg_length) const
{
    return ping_msg_length == s_BYTES_IN_MSG_HEADER;
}

bool Udp_client::isValidByeMsg(const std::string& bye_msg, unsigned bye_msg_length) const
{
    if(!isValidByeMsgLength(bye_msg_length) || static_cast<Protocol_msg_type> (static_cast<unsigned char> (bye_msg[0])) != Protocol_msg_type::M_BYE)
    {
        return false;
    }

    std::string skipped_msg_header{bye_msg.substr(s_BYTES_IN_MSG_HEADER)};
    return std::regex_match(skipped_msg_header, getByeMsgRegex());
}

uint8_t Udp_client::processServerByeMsg(const std::string& bye_msg, unsigned bye_msg_length)
{
    if(isValidByeMsg(bye_msg, bye_msg_length))
    {
        sendConfirmMsg(getMsgId(bye_msg));
        return 0;
    }

    sendErrMsg("ERROR: received a malformed BYE message from the server.");
    return 2;
}

bool Udp_client::isValidByeMsgLength(unsigned bye_msg_length) const
{
    return bye_msg_length >= s_BYTES_IN_MSG_HEADER + s_MIN_VARIABLE_DATA_LENGTH + sizeof(s_VARIABLE_LENGTH_DATA_TERMINATOR) &&
        bye_msg_length <= s_BYTES_IN_MSG_HEADER + s_DISPLAY_NAME_MAX_LENGTH + sizeof(s_VARIABLE_LENGTH_DATA_TERMINATOR);
}

bool Udp_client::isValidErrMsg(const std::string& err_msg, unsigned err_msg_length) const
{
    if(!isValidErrMsgLength(err_msg_length) || static_cast<Protocol_msg_type> (static_cast<unsigned char> (err_msg[0])) != Protocol_msg_type::M_ERR)
    {
        return false;
    }

    std::smatch matches{};

    std::string skipped_msg_header{err_msg.substr(s_BYTES_IN_MSG_HEADER)};
    if(std::regex_match(skipped_msg_header, matches, getErrMsgRegex()) && isValidDisplayNameLength(matches[1].length())
            && isValidMsgContentLength(matches[2].length()))
    {
        printErrFromServer(matches[1], matches[2]);
        return true;
    }

    return false;
}

uint8_t Udp_client::processServerErrMsg(const std::string& err_msg, unsigned err_msg_length)
{
    if(isValidErrMsg(err_msg, err_msg_length))
    {
        sendConfirmMsg(getMsgId(err_msg));
        return 1;
    }

    sendErrMsg("ERROR: received a malformed ERR message from the server.");
    return 2;
}

bool Udp_client::isValidErrMsgLength(unsigned err_msg_length) const
{
    return err_msg_length >= s_BYTES_IN_MSG_HEADER + s_MIN_VARIABLE_DATA_LENGTH +
        sizeof(s_VARIABLE_LENGTH_DATA_TERMINATOR) + s_MIN_VARIABLE_DATA_LENGTH + sizeof(s_VARIABLE_LENGTH_DATA_TERMINATOR) &&
            err_msg_length <= s_BYTES_IN_MSG_HEADER + s_DISPLAY_NAME_MAX_LENGTH +
                sizeof(s_VARIABLE_LENGTH_DATA_TERMINATOR) + s_MSG_CONTENT_MAX_LENGTH + sizeof(s_VARIABLE_LENGTH_DATA_TERMINATOR);
}

void Udp_client::processStdinEvent()
{
    // Check if stdin was closed
    if(m_actual_event.events & EPOLLHUP)
    {
        sendByeMsgToServer();
        return;
    }

    if(m_actual_event.events & EPOLLERR)
    {
        throw Exception{"stdin error occurred."};
    }

    // Handle stdin event
    std::vector<std::string> user_input{parseUserInput()};

    if(user_input.empty() || processNonMsgToServer(user_input))
    {
        return;
    }

    if(user_input[0] == m_user_commands[0])
    {
        m_user_display_name = user_input[3];
    }

    buildUserMsgToServer(user_input);
    sendMsgToServer();

    if(user_input[0] == m_user_commands[0] && m_current_state == FSM_state::S_START)
    {
        m_current_state = FSM_state::S_AUTH;
    }

    disableStdinEvents();
    m_is_waiting_for_confirm = true;
    m_is_waiting_for_reply = user_input[0] == m_user_commands[0] || user_input[0] == m_user_commands[2];
    startTimer(m_args.getUdpConfirmTimeout());
}

void Udp_client::sendConfirmMsg(uint16_t ref_msg_id)
{
    std::string confirm_msg{std::string{static_cast<char> (Protocol_msg_type::M_CONFIRM)}};
    uint16_t net_msg_id = htons(ref_msg_id);
    confirm_msg.append(reinterpret_cast<const char*>(&net_msg_id), sizeof(net_msg_id));
    if(sendto(m_client_socket, confirm_msg.data(), confirm_msg.size(), 0,
        reinterpret_cast<struct sockaddr*>(m_args.getServerAddrStructAddress()), sizeof(*(m_args.getServerAddrStructAddress()))) == -1)
    {
        throw Exception{"couldn't send a message to the server: send() has failed."};
    }
    m_confirmed_server_messages.set(ref_msg_id);
}

void Udp_client::sendByeMsgToServer()
{
    buildByeMsg();
    sendMsgToServer();
    disableStdinEvents();
    startTimer(m_args.getUdpConfirmTimeout()); // Time in ms
    m_is_waiting_for_confirm = false;
    m_is_waiting_for_reply = false;
    m_is_waiting_for_bye_confirm = true;
    m_allowed_retransmissions = m_args.getUdpMaxRetransCount();
}

// this function was generated by AI
uint16_t Udp_client::getMsgId(std::string_view msg) const
{
    uint16_t msg_id{};
    std::memcpy(&msg_id, msg.data() + s_BYTES_IN_PROTOCOL_MSG_TYPE, s_BYTES_IN_MSG_ID);
    return ntohs(msg_id);
}

// this function was generated by AI
uint16_t Udp_client::getRefMsgId(std::string_view reply_msg) const
{
    uint16_t ref_msg_id{};
    std::memcpy(&ref_msg_id, reply_msg.data() + s_BYTES_IN_MSG_HEADER + s_BYTES_IN_REPLY_RESULT, s_BYTES_IN_MSG_ID);
    return ntohs(ref_msg_id);
}

void Udp_client::sendMsgToServer()
{
    if(sendto(m_client_socket, m_msg_to_server.data(), m_msg_to_server.size(), 0,
        reinterpret_cast<struct sockaddr*>(m_args.getServerAddrStructAddress()), sizeof(*(m_args.getServerAddrStructAddress()))) == -1)
    {
        throw Exception{"couldn't send a message to the server: send() has failed."};
    }
}

void Udp_client::addMsgIdToMsgToServer(uint16_t msg_id)
{
    uint16_t net_id = htons(msg_id);
    m_msg_to_server.append(reinterpret_cast<const char*>(&net_id), sizeof(net_id));
}

void Udp_client::buildErrMsg(std::string content)
{
    m_msg_to_server = std::string{static_cast<char> (Protocol_msg_type::M_ERR)};
    ++m_msg_to_server_id;
    addMsgIdToMsgToServer(m_msg_to_server_id);
    m_msg_to_server += m_user_display_name + s_VARIABLE_LENGTH_DATA_TERMINATOR + content + s_VARIABLE_LENGTH_DATA_TERMINATOR;
}

void Udp_client::buildAuthMsg(const std::string& username, const std::string& secret)
{
    m_msg_to_server = std::string{static_cast<char> (Protocol_msg_type::M_AUTH)};
    addMsgIdToMsgToServer(m_msg_to_server_id);
    m_msg_to_server += username + s_VARIABLE_LENGTH_DATA_TERMINATOR + m_user_display_name + s_VARIABLE_LENGTH_DATA_TERMINATOR
        + secret + s_VARIABLE_LENGTH_DATA_TERMINATOR;
}

void Udp_client::buildJoinMsg(const std::string& channel_id)
{
    m_msg_to_server = std::string{static_cast<char> (Protocol_msg_type::M_JOIN)};
    addMsgIdToMsgToServer(m_msg_to_server_id);
    m_msg_to_server += channel_id + s_VARIABLE_LENGTH_DATA_TERMINATOR + m_user_display_name + s_VARIABLE_LENGTH_DATA_TERMINATOR;
}

void Udp_client::buildMsgMsg(const std::string& user_msg)
{
    m_msg_to_server = std::string{static_cast<char> (Protocol_msg_type::M_MSG)};
    addMsgIdToMsgToServer(m_msg_to_server_id);
    m_msg_to_server += m_user_display_name + s_VARIABLE_LENGTH_DATA_TERMINATOR + user_msg + s_VARIABLE_LENGTH_DATA_TERMINATOR;
}

void Udp_client::buildByeMsg()
{
    m_msg_to_server = std::string{static_cast<char> (Protocol_msg_type::M_BYE)};
    ++m_msg_to_server_id;
    addMsgIdToMsgToServer(m_msg_to_server_id);
    m_msg_to_server += m_user_display_name + s_VARIABLE_LENGTH_DATA_TERMINATOR;
}

void Udp_client::processServerReplyMsg(const std::string& reply_msg, unsigned reply_msg_length, sockaddr_in& server_addr)
{
    if(m_current_state != FSM_state::S_AUTH && m_current_state != FSM_state::S_JOIN)
    {
        sendErrMsg("ERROR: received a REPLY message in unexpected state.");
        return;
    }

    if(isValidReplyMsg(reply_msg, reply_msg_length))
    {
        if(m_current_state == FSM_state::S_AUTH)
        {
            *(m_args.getServerAddrStructAddress()) = server_addr;
        }

        if(m_is_waiting_for_reply && m_msg_to_server_id - 1 == getRefMsgId(reply_msg))
        {
            stopTimer();
            uint16_t reply_msg_id{getMsgId(reply_msg)};
            if(!m_confirmed_server_messages.test(reply_msg_id))
            {
                outputIncomingReply(reply_msg[s_BYTES_IN_MSG_HEADER], reply_msg.c_str() +
                    s_BYTES_IN_MSG_HEADER + s_BYTES_IN_REPLY_RESULT + s_BYTES_IN_MSG_ID);
            }

            sendConfirmMsg(reply_msg_id);

            if(m_current_state == FSM_state::S_JOIN || reply_msg[s_BYTES_IN_MSG_HEADER])
            {
                m_current_state = FSM_state::S_OPEN;
            }

            m_is_waiting_for_reply = false;
            enableStdinEvents();
        }

        return;
    }

    sendErrMsg("ERROR: received a malformed REPLY message from the server.");
}


bool Udp_client::isValidReplyMsgLength(unsigned reply_msg_length) const
{
    return reply_msg_length >= s_BYTES_IN_MSG_HEADER + s_BYTES_IN_REPLY_RESULT + s_BYTES_IN_MSG_ID +
        s_MIN_VARIABLE_DATA_LENGTH + sizeof(s_VARIABLE_LENGTH_DATA_TERMINATOR) && reply_msg_length <= s_BYTES_IN_PROTOCOL_MSG_TYPE
            + s_BYTES_IN_MSG_ID + s_BYTES_IN_REPLY_RESULT + s_BYTES_IN_MSG_ID + s_MSG_CONTENT_MAX_LENGTH + sizeof(s_VARIABLE_LENGTH_DATA_TERMINATOR);
}

bool Udp_client::isValidReplyMsgResult(char reply_msg_result) const
{
    return reply_msg_result == 0 || reply_msg_result == 1;
}

bool Udp_client::isValidReplyMsg(const std::string& reply_msg, unsigned reply_msg_length) const
{
    if(!isValidReplyMsgLength(reply_msg_length) || static_cast<Protocol_msg_type> (static_cast<unsigned char> (reply_msg[0])) != Protocol_msg_type::M_REPLY ||
        !isValidReplyMsgResult(reply_msg[s_BYTES_IN_MSG_HEADER]))
    {
        return false;
    }

    std::smatch matches{};

    std::string reply_msg_content{reply_msg.substr(s_BYTES_IN_MSG_HEADER + s_BYTES_IN_REPLY_RESULT + s_BYTES_IN_MSG_ID)};
    return std::regex_match(reply_msg_content, matches, getPrintableCharsSpaceLfAndTerminatorRegex());
}

const std::regex& Udp_client::getErrMsgRegex()
{
    static const std::regex value{
        getPrintableChars()
        + getEscapedVariableLengthTerminator()
        + getPrintableCharsSpaceLf()
        + getEscapedVariableLengthTerminator()
    };

    return value;
}

const std::regex& Udp_client::getMsgMsgRegex()
{
    return getErrMsgRegex();
}

const std::regex& Udp_client::getByeMsgRegex()
{
    static const std::regex value{getPrintableChars()+ getEscapedVariableLengthTerminator()};
    return value;
}

const std::regex& Udp_client::getPrintableCharsAndTerminatorRegex()
{
    static const std::regex value{Client::getPrintableChars() + getEscapedVariableLengthTerminator()};
    return value;
}

const std::regex& Udp_client::getPrintableCharsSpaceLfAndTerminatorRegex()
{
    static const std::regex value{Client::getPrintableCharsSpaceLf() + getEscapedVariableLengthTerminator()};
    return value;
}

// this function was generated by AI
const std::string& Udp_client::getEscapedVariableLengthTerminator()
{
    static const std::string value = [] {
        std::ostringstream oss;
        oss << "\\x" << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<int>(static_cast<unsigned char>(s_VARIABLE_LENGTH_DATA_TERMINATOR));
        return oss.str();
    }();

    return value;
}
