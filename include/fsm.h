/**
 * @file fsm.h
 * @author Andrii Klymenko
 * @brief Finite State Machine (FSM) states used in the client protocol flow.
 */

#ifndef FSM_H
#define FSM_H

/**
 * @brief Represents the states of the client's finite state machine.
 */
enum class FSM_state
{
    S_START, ///< Initial state before authentication.
    S_AUTH,  ///< Client is authenticating with the server.
    S_OPEN,  ///< Communication channel is open.
    S_JOIN,  ///< Client is joining the chat.
};

#endif // FSM_H
