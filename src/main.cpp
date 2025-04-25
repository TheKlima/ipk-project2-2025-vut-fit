/**
 * @file main.cpp
 * @author Andrii Klymenko
 * @brief Entry point for the IPK25CHAT client.
 *
 * This file contains the main function which sets up command-line arguments,
 * initializes the client, and handles execution including exception management.
 */

#include "args.h"
#include "client.h"
#include "exception.h"
#include "error.h"

/**
 * @brief Main function for the application.
 *
 * Parses command-line arguments, initializes the client, and starts the protocol.
 * Manages exceptions gracefully.
 *
 * @param argc Number of command-line arguments.
 * @param argv Array of command-line argument strings.
 * @return int EXIT_SUCCESS on successful execution, EXIT_FAILURE otherwise.
 */
int main(const int argc, char* argv[]) try
{
    // Parse and validate command-line arguments
    const Args args{argc, argv};

    // Check if help was requested and print usage
    if(args.getIsHelpUsed())
    {
        Args::printHelp();
        return EXIT_SUCCESS;
    }

    // Create and initialize the client using parsed arguments
    const std::unique_ptr<Client> client{Client::create(args)};

    // Start the client logic (e.g., connect to server, handle communication)
    return client->run();
}
catch (const std::bad_alloc& e) {
    // Handle memory allocation failures
    printErrMsg(std::string{"operator 'new' has failed: "} + e.what());
    return EXIT_FAILURE;
}
catch (const Exception& e) {
    // Gracefully handle known application-specific exceptions
    if(e.isSigintOrEofReceived())
    {
        return EXIT_SUCCESS;
    }

    printErrMsg(e.what());
    return EXIT_FAILURE;
}
catch (const std::exception& e) {
    // Handle all other exceptions derived from std::exception
    printErrMsg(e.what());
    return EXIT_FAILURE;
}
catch (...) {
    // Handle all other exceptions (e.g., non-standard exceptions)
    printErrMsg("Unknown exception occurred.");
    return EXIT_FAILURE;
}
