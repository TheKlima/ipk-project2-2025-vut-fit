/**
 * @file tcp-client.cpp
 * @author Andrii Klymenko
 * @brief Implementation of the class of TCP version of IPK25-CHAT client.
 */

#include "tcp-client.h"
#include "exception.h"
#include "error.h"
#include <iostream>
#include <csignal>

Tcp_client::Tcp_client(const Args& args)
    :
    Client::Client{args}
{
    if(connect(m_client_socket, reinterpret_cast<struct sockaddr*>(m_args.getServerAddrStructAddress()), sizeof(*(m_args.getServerAddrStructAddress()))) < 0)
    {
        throw Exception{"couldn't connect to the server."};
    }
}

void Tcp_client::processStdinEvent()
{
    // Check if stdin was closed
    if(m_actual_event.events & EPOLLHUP)
    {
        sendByeMsgToServer();
        throw Exception{""};
    }

    if(m_actual_event.events & EPOLLERR)
    {
        throw Exception{"stdin error occurred."};
    }

    // Handle stdin event
    std::vector<std::string> user_input{parseUserInput()};

    if(user_input.empty() || processNonMsgToServer(user_input))
    {
        return; // Skip empty input
    }

    if(user_input[0] == m_user_commands[0])
    {
        m_user_display_name = user_input[3];
    }

    if(user_input[0] == m_user_commands[2])
    {
        m_current_state = FSM_state::S_JOIN;
    }

    buildUserMsgToServer(user_input);
    sendMsgToServer();

    if(user_input[0] == m_user_commands[0] && m_current_state == FSM_state::S_START)
    {
        m_current_state = FSM_state::S_AUTH;
    }

    if(user_input[0] == m_user_commands[0] || user_input[0] == m_user_commands[2])
    {
        disableStdinEvents();
        startTimer(s_MAX_REPLY_WAIT_TIME * 1000); // Time in ms
        m_is_waiting_for_reply = true;
    }
}

void Tcp_client::processServerReplyMsg(const std::string& reply_msg_from_server)
{
    if(m_current_state != FSM_state::S_AUTH && m_current_state != FSM_state::S_JOIN)
    {
        sendErrMsgAndTerminate("received a REPLY message in unexpected state.");
        return;
    }

    if(m_is_waiting_for_reply)
    {
        std::smatch matches{};
        if(std::regex_match(reply_msg_from_server, matches, getReplyMsgRegex()) && isValidMsgContentLength(matches[2].length()))
        {
            stopTimer();
            bool is_positive_reply{strcasecmp(matches[1].str().c_str(), "OK") == 0};
            outputIncomingReply(is_positive_reply, matches[2]);
            if(m_current_state == FSM_state::S_JOIN || is_positive_reply)
            {
                m_current_state = FSM_state::S_OPEN;
            }
            m_is_waiting_for_reply = false;
            enableStdinEvents();
            return;
        }

        sendErrMsgAndTerminate("received a malformed REPLY message from the server.");
    }

    sendErrMsgAndTerminate("didn't expect any reply message from the server.");
}


uint8_t Tcp_client::processMessageFromServer(const std::string& msg_from_server)
{
    Protocol_msg_type type_of_msg_from_server{getServerMsgType(m_msg_from_server)};

    if(type_of_msg_from_server == Protocol_msg_type::M_UNKNOWN)
    {
        sendErrMsgAndTerminate("only messages of types BYE, ERR, MSG and REPLY are expected to be received"
                        " from the server.");
    }

    if(type_of_msg_from_server == Protocol_msg_type::M_BYE)
    {
        processServerByeMsg(msg_from_server);
        return 0;
    }

    if(type_of_msg_from_server == Protocol_msg_type::M_ERR)
    {
        processServerErrMsg(msg_from_server);
        return 1;
    }

    switch(m_current_state)
    {
        case FSM_state::S_START:
            sendErrMsgAndTerminate("only messages of types BYE and ERR are expected to be received"
                        " from the server in the client's START state.");
            break;

        case FSM_state::S_AUTH:
            if(type_of_msg_from_server == Protocol_msg_type::M_REPLY)
            {
                processServerReplyMsg(msg_from_server);
                break;
            }

            sendErrMsgAndTerminate("only messages of types BYE, ERR and REPLY are expected to be received"
                " from the server in the client's AUTH state.");
            break;

        case FSM_state::S_OPEN:
            if(type_of_msg_from_server == Protocol_msg_type::M_MSG)
            {
                processServerMsgMsg(msg_from_server);
            }
            else
            {
                sendErrMsgAndTerminate("only messages of types BYE, ERR and MSG are expected to be received"
                    " from the server in the client's OPEN state.");
            }
            break;

        case FSM_state::S_JOIN:
            switch(type_of_msg_from_server)
            {
                case Protocol_msg_type::M_REPLY:
                    processServerReplyMsg(msg_from_server);
                    break;

                case Protocol_msg_type::M_MSG:
                    processServerMsgMsg(msg_from_server);
                    break;

                default:
                    sendErrMsgAndTerminate("only messages of types BYE, ERR, MSG and REPLY are expected to be received"
                        " from the server in the client's JOIN state.");
            }
            break;

        default:
            throw Exception{"Invalid client's FSM state."};
    }

    return 2;
}

// this function was generated by AI
uint8_t Tcp_client::processSocketEvent()
{
    const long server_msg_length{recv(m_client_socket, m_server_msg.get(), s_MAX_MSG_SIZE + 1, 0)};

    if(server_msg_length < 0)
    {
        sendErrMsgAndTerminate("couldn't receive a message from the server: recv() has failed.");
    }

    if(server_msg_length == 0) // Connection closed by the server
    {
        return 0;
    }

    // Append received data to the buffer
    m_msg_from_server += std::string{m_server_msg.get(), static_cast<size_t>(server_msg_length)};
    size_t end_of_msg_position;

    // Keep processing as long as we have complete messages
    while((end_of_msg_position = m_msg_from_server.find(s_END_OF_MESSAGE)) != std::string::npos)
    {
        std::string single_msg = m_msg_from_server.substr(0, end_of_msg_position + s_BYTES_IN_END_OF_MESSAGE);

        // Validate length
        if(single_msg.size() > s_MAX_MSG_SIZE)
        {
            sendErrMsgAndTerminate("too long message from server.");
        }

        // Process the single complete message
        uint8_t result{processMessageFromServer(single_msg)};

        if(result == 0 || result == 1)
        {
            return result;
        }

        // Remove processed message from the buffer
        m_msg_from_server.erase(0, end_of_msg_position + s_BYTES_IN_END_OF_MESSAGE);
    }

    // Validate length
    if(m_msg_from_server.size() >= s_MAX_MSG_SIZE)
    {
        sendErrMsgAndTerminate("too long message from server.");
    }

    return 2;
}

void Tcp_client::sigintHandler()
{
    sendByeMsgToServer();
    throw Exception{""};
}

void Tcp_client::processTimerEvent()
{
    if(m_is_waiting_for_reply)
    {
        sendErrMsgAndTerminate("waited too long for the server's reply.");
    }

    throw Exception{"timer event, but waiting_for_reply is false."};
}

void Tcp_client::sendErrMsgAndTerminate(const char* err_msg)
{
    buildErrMsg(err_msg);
    sendMsgToServer();
    throw Exception{err_msg};
}

void Tcp_client::processServerMsgMsg(const std::string& msg_msg_from_server)
{
    std::smatch matches{};
    if(std::regex_match(msg_msg_from_server, matches, getMsgMsgRegex()) && isValidDisplayNameLength(matches[1].length())
        && isValidMsgContentLength(matches[2].length()))
    {
        outputIncomingMsg(matches[1], matches[2]);
        return;
    }

    sendErrMsgAndTerminate("received a malformed MSG message from the server.");
}

void Tcp_client::processServerErrMsg(const std::string& err_msg_from_server)
{
    std::smatch matches{};
    if(std::regex_match(err_msg_from_server, matches, getErrMsgRegex()) && isValidDisplayNameLength(matches[1].length())
        && isValidMsgContentLength(matches[2].length()))
    {
        printErrFromServer(matches[1], matches[2]);
        return;
    }

    sendErrMsgAndTerminate("received a malformed ERR message from the server.");
}

void Tcp_client::processServerByeMsg(const std::string& bye_msg_from_server)
{
    std::smatch matches{};
    if(!std::regex_match(bye_msg_from_server, matches, getByeMsgRegex()) || !isValidDisplayNameLength(matches[1].length()))
    {
        sendErrMsgAndTerminate("received a malformed BYE message from the server.");
    }
}

void Tcp_client::sendByeMsgToServer()
{
    buildByeMsg();
    sendMsgToServer();
}

void Tcp_client::sendMsgToServer()
{
    if(send(m_client_socket, m_msg_to_server.data(), m_msg_to_server.size(), 0) == -1)
    {
        throw Exception{"couldn't send a message to the server: send() has failed."};
    }
}

// this function was generated by AI
Protocol_msg_type Tcp_client::getServerMsgType(const std::string& msg_from_server) const
{
    if(msg_from_server.size() >= 3)
    {
        std::string prefix = msg_from_server.substr(0, strlen("REPLY")); // longest is "REPLY"
        std::transform(prefix.begin(), prefix.end(), prefix.begin(),
                       [](unsigned char c) { return std::toupper(c); });

        if (prefix.starts_with("MSG")) return Protocol_msg_type::M_MSG;
        if (prefix.starts_with("ERR")) return Protocol_msg_type::M_ERR;
        if (prefix.starts_with("BYE")) return Protocol_msg_type::M_BYE;
        if (prefix.starts_with("REPLY")) return Protocol_msg_type::M_REPLY;
    }

    return Protocol_msg_type::M_UNKNOWN;
}

void Tcp_client::buildJoinMsg(const std::string& channel_id)
{
    m_msg_to_server = "JOIN " + channel_id + " AS " + m_user_display_name + s_END_OF_MESSAGE;
}

void Tcp_client::buildMsgMsg(const std::string& user_msg)
{
    m_msg_to_server = "MSG FROM " + m_user_display_name + " IS " + user_msg + s_END_OF_MESSAGE;
}

void Tcp_client::buildAuthMsg(const std::string& username, const std::string& secret)
{
    m_msg_to_server = "AUTH " + username + " AS " + m_user_display_name + " USING " + secret + s_END_OF_MESSAGE;
}

void Tcp_client::buildErrMsg(std::string content)
{
    m_msg_to_server = "ERR FROM " + m_user_display_name + " IS " + content + s_END_OF_MESSAGE;
}

void Tcp_client::buildByeMsg()
{
    m_msg_to_server = "BYE FROM " + m_user_display_name + s_END_OF_MESSAGE;
}

const std::regex& Tcp_client::getReplyMsgRegex()
{
    static const std::regex value{"REPLY (OK|NOK) IS " + getPrintableCharsSpaceLf() + s_END_OF_MESSAGE, std::regex_constants::icase};
    return value;
}

const std::regex& Tcp_client::getMsgMsgRegex()
{
    static const std::regex value{"MSG FROM " + getPrintableChars() + " IS " + getPrintableCharsSpaceLf() + s_END_OF_MESSAGE, std::regex_constants::icase};
    return value;
}

const std::regex& Tcp_client::getErrMsgRegex()
{
    static const std::regex value{"ERR FROM " + getPrintableChars() + " IS " + getPrintableCharsSpaceLf() + s_END_OF_MESSAGE, std::regex_constants::icase};
    return value;
}

const std::regex& Tcp_client::getByeMsgRegex()
{
    static const std::regex value{"BYE FROM " + getPrintableChars() + s_END_OF_MESSAGE, std::regex_constants::icase};
    return value;
}
