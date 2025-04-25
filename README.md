Project's assignment can be found [here](https://git.fit.vutbr.cz/NESFIT/IPK-Projects/src/branch/master/Project_2).

Documentation of Project 2 IPK 2024/2025<br>
Name and surname: Andrii Klymenko<br>
Login: xklyme00

## Theory

In this chapter there is presented the basic information that was necessary to study before implementing the project.

###     TCP protocol

TCP (transmission control protocol) is the most common transport layer protocol. It distinguishes computer applications using ports (just like UDP).
TCP must establish a connection between the client and the server before sending the data, which is done by means of the so-called three-way handshake. Thanks
to that, it provides a reliable way to send and receive data over a network. If a packet is lost or damaged (data
received does not match the data sent) on the way, it is retransmitted. This is to make sure that all packets arrive to the destination.
Applications that run over TCP, unlike those one that run over UDP, don't need to implement messages acknowledgment and retransmission.

TCP is stream-oriented protocol. Its API (using functions like _send()_/_recv()_) treats data as a continuous byte stream rather than separate messages.
There are no message boundaries: TCP may combine data from multiple _send()_ calls into a single network segment or split data from one _send()_
across multiple segments. However, this process is hidden from the application. When you call _recv()_, you simply get a chunk
of bytes, with no direct correlation to how many _send()_ calls originally produced that data. This protocol feature
introduces a challenge of processing incoming messages from the server: for example, the program may need multiple
_recv()_ calls to recognize one IPK25CHAT protocol message from the server. Also, one _recv()_ call might return multiple IPK25CHAT protocol messages from the server.
It is important to correctly detect the end of each server message.

###     UDP protocol

UDP (user datagram protocol) is also transport layer protocol. However, is opposite to TCP: id does not care if the packet get damaged (use of checksum is not guaranteed) or
lost which increases the communication speed. No handshake is done and communication uses a
simple request-response method: does not acknowledge delivery of UDP datagrams and data may be delivered in the wrong order.
It introduces a challenge for applications that run over UDP: they need to take into account that data may be lost while
delivering so the program may need to implement messages acknowledgment and retransmission.

UDP is message-oriented protocol. Its API allows you to send and receive individual datagrams.
Each _send()_ call transmits exactly one datagram, and each _recv()_ call retrieves exactly one datagram.
The boundaries between messages are preserved—what you send is exactly what the receiver gets. Given that, we don't need
to worry about one message with multiple _recv()_ calls or multiples messages with one _recv()_ calls as in TCP variant: 
in both these cases the message from the server is considered to be malformed.

##     Implementation details

This program is written in the C++ programming language and is specific for Linux-based systems, because it
utilizes the epoll I/O notification facility.

Processing and storage of all program arguments is implemented in the [**Args**](https://git.fit.vutbr.cz/xklyme00/ipk-project1-2024-vut-fit/src/branch/main/include/args.h) class, which instance is the member of
the abstract [**Client**](https://git.fit.vutbr.cz/xklyme00/ipk-project1-2024-vut-fit/src/branch/main/include/client.h) class. Both [**Tcp_client**](https://git.fit.vutbr.cz/xklyme00/ipk-project1-2024-vut-fit/src/branch/main/include/tcp-client.h) and [**Udp_client**](https://git.fit.vutbr.cz/xklyme00/ipk-project1-2024-vut-fit/src/branch/main/include/udp-client.h) inherit from this base class. **Client** class
contains a factory method for creating a Client instance based on the program provided argument. In its constructor it
sets a _SIGINT_ signal callback, creates a client socket, epoll and timer file descriptors, and then adds these file descriptors
to the corresponding epoll events. Below you can find a bit simplified version of the main client loop:
```c++
bool Client::run()
{
    while(true)
    {
        // Wait for events
        m_epoll_event_count = epoll_wait(m_epoll_fd, &m_actual_event, s_MAX_EPOLL_EVENT_NUMBER, -1);

        if(m_epoll_event_count == 1)
        {
            if(m_actual_event.data.fd == STDIN_FILENO)
            {
                processStdinEvent();
            }
            else if(m_actual_event.data.fd == m_client_socket)
            {
                uint8_t result = processSocketEvent();

                if(result == 0 || result == 1)
                {
                    return static_cast<bool> (result);
                }
            }
            else // timer event
            {
                processTimerEvent();
            }
        }
    }
}
```
Both client versions have the same set of commands, so **Client** class implements user input parsing (from _stdin_). It also
contains a functions _disableStdinEvents()_ and _enableStdinEvents()_. First one is used in the TCP version when user
sends a message to the server that requires a REPLY message from the server, and it blocks _stdin_ events, because according to the project's assignment:
> _"The client must simultaneously process only a single user input (chat or request message/local command invocation). Processing of additional user input is deferred until after the previous action has been completed."_
>
and when reply is received, _stdin_ events are enabled (unblocked) using _enableStdinEvents()_ again. In this case we don't need to buffer user's commands.
It works similarly in the UDP version.

Furthermore, it implements functions _startTimer()_ and _stopTimer()_. In the UDP version, for example,
the timer is started bye _startTimer()_ every time client sends some message to the server and waits for its confirmation. Timer length depends on the program
arguments (default value is 250 ms). If the message is confirmed by the server before timer event happens, the timer is stopped using _stopTimer()_.
Otherwise, the message is retransmitted and the timer is started again. This process repeats if message doesn't get confirmed and
UDP maximal retransmission (provided in the program's arguments, default value is 3) is not exhausted. These functions are also used in the TCP
variant when waiting for the server's REPLY message.

Both versions of the client implement their own processing functions for every type of the server message, and also
their own build messages to server functions, because TCP version of the IPK25CHAT protocol is text-based, while UDP
version is binary, and UDP version has some additional messages that TCP version doesn't have.

## Testing

All testing was done under the reference developer environment specified in the project's assignment.

While implementing the project, I created both TCP and UDP pseudo-servers: [**tcp-serv.c**](https://git.fit.vutbr.cz/xklyme00/ipk-project1-2024-vut-fit/src/branch/main/pseudo-servers/tcp-serv.c) and [**udp-serv.cpp**](https://git.fit.vutbr.cz/xklyme00/ipk-project1-2024-vut-fit/src/branch/main/pseudo-servers/udp-serv.cpp).
I call them pseudo-servers because in order to receive and send messages from and to the client user needs to manually
'ask' program to do so while it is running by entering appropriate input to the _stdin_. I have tried to cover all transitions
in the project assignment finite state machine, to cover every single line of code. Every time I run
my program and pseudo-servers in parallel I did it using _valgrind_ in order to make sure there are no memory leaks and any
other related memory issues.

Here is an example of how I tested that program's TCP version processes messages of type MSG from the server while being in the state
JOIN:

Client's output:
```text
==53510== Memcheck, a memory error detector
==53510== Copyright (C) 2002-2024, and GNU GPL'd, by Julian Seward et al.
==53510== Using Valgrind-3.23.0 and LibVEX; rerun with -h for copyright info
==53510== Command: ./ipk25chat-client -t tcp -s localhost -p 8080
==53510==
/auth a a a
Action Success: you are welcome here!
/join abc1
user123: message while waiting for join's reply.
Action Failure: you are not welcome here :((
^C==53510==
==53510== HEAP SUMMARY:
==53510==     in use at exit: 0 bytes in 0 blocks
==53510==   total heap usage: 2,589 allocs, 2,589 frees, 184,555 bytes allocated
==53510==
==53510== All heap blocks were freed -- no leaks are possible
==53510==
==53510== For lists of detected and suppressed errors, rerun with: -s
==53510== ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)
```

Pseudo-server's output:
```text
Socket successfully created..
Socket successfully binded..
Server listening..
Server accepted the client...

Choose an option:
1. Send a message to the client
2. Receive a message from the client
3. Exit
Enter your choice: 2
Message from client: AUTH a AS a USING a

Choose an option:
1. Send a message to the client
2. Receive a message from the client
3. Exit
Enter your choice: 1
Enter the message to send: RePLy Ok iS you are welcome here!
Message sent to client: RePLy Ok iS you are welcome here!

Choose an option:
1. Send a message to the client
2. Receive a message from the client
3. Exit
Enter your choice: 2
Message from client: JOIN abc1 AS a

Choose an option:
1. Send a message to the client
2. Receive a message from the client
3. Exit
Enter your choice: 1
Enter the message to send: MsG FRoM user123 Is message while waiting for join's reply.
Message sent to client: MsG FRoM user123 Is message while waiting for join's reply.

Choose an option:
1. Send a message to the client
2. Receive a message from the client
3. Exit
Enter your choice: 1
Enter the message to send: RePLy NoK iS you are not welcome here :((
Message sent to client: RePLy NoK iS you are not welcome here :((

Choose an option:
1. Send a message to the client
2. Receive a message from the client
3. Exit
Enter your choice: 3
Server Exit...
```

Part of _tcpdump_ output:
```text
12:05:34.394849 IP localhost.48486 > localhost.http-alt: Flags [S], seq 20733456, win 65495, options [mss 65495,sackOK,TS val 1941565237 ecr 0,nop,wscale 7], length 0
        0x0000:  4500 003c 5a73 4000 4006 e246 7f00 0001  E..<Zs@.@..F....
        0x0010:  7f00 0001 bd66 1f90 013c 5e10 0000 0000  .....f...<^.....
        0x0020:  a002 ffd7 fe30 0000 0204 ffd7 0402 080a  .....0..........
        0x0030:  73b9 ef35 0000 0000 0103 0307            s..5........
12:05:34.394856 IP localhost.http-alt > localhost.48486: Flags [S.], seq 756199921, ack 20733457, win 65483, options [mss 65495,sackOK,TS val 1941565237 ecr 1941565237,nop,wscale 7], length 0
        0x0000:  4500 003c 0000 4000 4006 3cba 7f00 0001  E..<..@.@.<.....
        0x0010:  7f00 0001 1f90 bd66 2d12 b1f1 013c 5e11  .......f-....<^.
        0x0020:  a012 ffcb fe30 0000 0204 ffd7 0402 080a  .....0..........
        0x0030:  73b9 ef35 73b9 ef35 0103 0307            s..5s..5....
12:05:34.394863 IP localhost.48486 > localhost.http-alt: Flags [.], ack 1, win 512, options [nop,nop,TS val 1941565237 ecr 1941565237], length 0
        0x0000:  4500 0034 5a74 4000 4006 e24d 7f00 0001  E..4Zt@.@..M....
        0x0010:  7f00 0001 bd66 1f90 013c 5e11 2d12 b1f2  .....f...<^.-...
        0x0020:  8010 0200 fe28 0000 0101 080a 73b9 ef35  .....(......s..5
        0x0030:  73b9 ef35                                s..5
12:05:36.808721 IP localhost.48486 > localhost.http-alt: Flags [P.], seq 1:22, ack 1, win 512, options [nop,nop,TS val 1941567650 ecr 1941565237], length 21: HTTP
        0x0000:  4500 0049 5a75 4000 4006 e237 7f00 0001  E..IZu@.@..7....
        0x0010:  7f00 0001 bd66 1f90 013c 5e11 2d12 b1f2  .....f...<^.-...
        0x0020:  8018 0200 fe3d 0000 0101 080a 73b9 f8a2  .....=......s...
        0x0030:  73b9 ef35 4155 5448 2061 2041 5320 6120  s..5AUTH.a.AS.a.
        0x0040:  5553 494e 4720 610d 0a                   USING.a..
12:05:36.808729 IP localhost.http-alt > localhost.48486: Flags [.], ack 22, win 512, options [nop,nop,TS val 1941567650 ecr 1941567650], length 0
        0x0000:  4500 0034 73ab 4000 4006 c916 7f00 0001  E..4s.@.@.......
        0x0010:  7f00 0001 1f90 bd66 2d12 b1f2 013c 5e26  .......f-....<^&
        0x0020:  8010 0200 fe28 0000 0101 080a 73b9 f8a2  .....(......s...
        0x0030:  73b9 f8a2                                s...
12:05:49.572237 IP localhost.http-alt > localhost.48486: Flags [P.], seq 1:36, ack 22, win 512, options [nop,nop,TS val 1941580414 ecr 1941567650], length 35: HTTP
        0x0000:  4500 0057 73ac 4000 4006 c8f2 7f00 0001  E..Ws.@.@.......
        0x0010:  7f00 0001 1f90 bd66 2d12 b1f2 013c 5e26  .......f-....<^&
        0x0020:  8018 0200 fe4b 0000 0101 080a 73ba 2a7e  .....K......s.*~
        0x0030:  73b9 f8a2 5265 504c 7920 4f6b 2069 5320  s...RePLy.Ok.iS.
        0x0040:  796f 7520 6172 6520 7765 6c63 6f6d 6520  you.are.welcome.
        0x0050:  6865 7265 210d 0a                        here!..
12:05:49.572272 IP localhost.48486 > localhost.http-alt: Flags [.], ack 36, win 512, options [nop,nop,TS val 1941580414 ecr 1941580414], length 0
        0x0000:  4500 0034 5a76 4000 4006 e24b 7f00 0001  E..4Zv@.@..K....
        0x0010:  7f00 0001 bd66 1f90 013c 5e26 2d12 b215  .....f...<^&-...
        0x0020:  8010 0200 fe28 0000 0101 080a 73ba 2a7e  .....(......s.*~
        0x0030:  73ba 2a7e                                s.*~
12:05:59.694675 IP localhost.48486 > localhost.http-alt: Flags [P.], seq 22:38, ack 36, win 512, options [nop,nop,TS val 1941590536 ecr 1941580414], length 16: HTTP
        0x0000:  4500 0044 5a77 4000 4006 e23a 7f00 0001  E..DZw@.@..:....
        0x0010:  7f00 0001 bd66 1f90 013c 5e26 2d12 b215  .....f...<^&-...
        0x0020:  8018 0200 fe38 0000 0101 080a 73ba 5208  .....8......s.R.
        0x0030:  73ba 2a7e 4a4f 494e 2061 6263 3120 4153  s.*~JOIN.abc1.AS
        0x0040:  2061 0d0a                                .a..
12:05:59.694706 IP localhost.http-alt > localhost.48486: Flags [.], ack 38, win 512, options [nop,nop,TS val 1941590536 ecr 1941590536], length 0
        0x0000:  4500 0034 73ad 4000 4006 c914 7f00 0001  E..4s.@.@.......
        0x0010:  7f00 0001 1f90 bd66 2d12 b215 013c 5e36  .......f-....<^6
        0x0020:  8010 0200 fe28 0000 0101 080a 73ba 5208  .....(......s.R.
        0x0030:  73ba 5208                                s.R.
12:06:07.750361 IP localhost.http-alt > localhost.48486: Flags [P.], seq 36:97, ack 38, win 512, options [nop,nop,TS val 1941598592 ecr 1941590536], length 61: HTTP
        0x0000:  4500 0071 73ae 4000 4006 c8d6 7f00 0001  E..qs.@.@.......
        0x0010:  7f00 0001 1f90 bd66 2d12 b215 013c 5e36  .......f-....<^6
        0x0020:  8018 0200 fe65 0000 0101 080a 73ba 7180  .....e......s.q.
        0x0030:  73ba 5208 4d73 4720 4652 6f4d 2075 7365  s.R.MsG.FRoM.use
        0x0040:  7231 3233 2049 7320 6d65 7373 6167 6520  r123.Is.message.
        0x0050:  7768 696c 6520 7761 6974 696e 6720 666f  while.waiting.fo
        0x0060:  7220 6a6f 696e 2773 2072 6570 6c79 2e0d  r.join's.reply..
        0x0070:  0a
```

Here is an example of how I tested that program's UDP version properly retransmits unconfirmed messages:

Client's output:
```text
93551== Memcheck, a memory error detector
==93551== Copyright (C) 2002-2024, and GNU GPL'd, by Julian Seward et al.
==93551== Using Valgrind-3.23.0 and LibVEX; rerun with -h for copyright info
==93551== Command: ./ipk25chat-client -t udp -s 127.0.0.1 -p 8080 -r 5
==93551==
/auth a a a
==93551==
==93551== HEAP SUMMARY:
==93551==     in use at exit: 0 bytes in 0 blocks
==93551==   total heap usage: 1,469 allocs, 1,469 frees, 153,535 bytes allocated
==93551==
==93551== All heap blocks were freed -- no leaks are possible
==93551==
==93551== For lists of detected and suppressed errors, rerun with: -s
==93551== ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)
```

Pseudo-server's output:
```text
Socket successfully created..
Socket successfully binded..
aaaaaasc0sb0a
Waiting for client's message...
C: AUTH: 0 a AS a USING a
Waiting for client's message...
C: AUTH: 0 a AS a USING a
Waiting for client's message...
C: AUTH: 0 a AS a USING a
Waiting for client's message...
C: AUTH: 0 a AS a USING a
Waiting for client's message...
C: AUTH: 0 a AS a USING a
Waiting for client's message...
C: AUTH: 0 a AS a USING a
S: CONFIRM: 0
S: BYE: 0 FROM server_display_name
Waiting for client's message...
C: CONFIRM: 0
```

_Tcpdump_ output:
```text
12:29:24.050673 IP localhost.51735 > localhost.8080: UDP, length 9
        0x0000:  4500 0025 6bc9 4000 4011 d0fc 7f00 0001  E..%k.@.@.......
        0x0010:  7f00 0001 ca17 1f90 0011 fe24 0200 0061  ...........$...a
        0x0020:  0061 0061 00                             .a.a.
12:29:24.302395 IP localhost.51735 > localhost.8080: UDP, length 9
        0x0000:  4500 0025 6bdb 4000 4011 d0ea 7f00 0001  E..%k.@.@.......
        0x0010:  7f00 0001 ca17 1f90 0011 fe24 0200 0061  ...........$...a
        0x0020:  0061 0061 00                             .a.a.
12:29:24.552868 IP localhost.51735 > localhost.8080: UDP, length 9
        0x0000:  4500 0025 6bdd 4000 4011 d0e8 7f00 0001  E..%k.@.@.......
        0x0010:  7f00 0001 ca17 1f90 0011 fe24 0200 0061  ...........$...a
        0x0020:  0061 0061 00                             .a.a.
12:29:24.803075 IP localhost.51735 > localhost.8080: UDP, length 9
        0x0000:  4500 0025 6bea 4000 4011 d0db 7f00 0001  E..%k.@.@.......
        0x0010:  7f00 0001 ca17 1f90 0011 fe24 0200 0061  ...........$...a
        0x0020:  0061 0061 00                             .a.a.
12:29:25.053269 IP localhost.51735 > localhost.8080: UDP, length 9
        0x0000:  4500 0025 6c00 4000 4011 d0c5 7f00 0001  E..%l.@.@.......
        0x0010:  7f00 0001 ca17 1f90 0011 fe24 0200 0061  ...........$...a
        0x0020:  0061 0061 00                             .a.a.
12:29:25.303424 IP localhost.51735 > localhost.8080: UDP, length 9
        0x0000:  4500 0025 6c16 4000 4011 d0af 7f00 0001  E..%l.@.@.......
        0x0010:  7f00 0001 ca17 1f90 0011 fe24 0200 0061  ...........$...a
        0x0020:  0061 0061 00                             .a.a.
12:29:25.303862 IP localhost.8080 > localhost.51735: UDP, length 3
        0x0000:  4500 001f 6c17 4000 4011 d0b4 7f00 0001  E...l.@.@.......
        0x0010:  7f00 0001 1f90 ca17 000b fe1e 0000 00    ...............
12:29:25.303907 IP localhost.8080 > localhost.51735: UDP, length 23
        0x0000:  4500 0033 6c18 4000 4011 d09f 7f00 0001  E..3l.@.@.......
        0x0010:  7f00 0001 1f90 ca17 001f fe32 ff00 0073  ...........2...s
        0x0020:  6572 7665 725f 6469 7370 6c61 795f 6e61  erver_display_na
        0x0030:  6d65 00                                  me.
12:29:25.325416 IP localhost.51735 > localhost.8080: UDP, length 3
        0x0000:  4500 001f 6c19 4000 4011 d0b2 7f00 0001  E...l.@.@.......
        0x0010:  7f00 0001 ca17 1f90 000b fe1e 0000 00    ...............
```

All scenarios of testing UDP program's variant can be found [**here**](https://git.fit.vutbr.cz/xklyme00/ipk-project1-2024-vut-fit/src/branch/main/pseudo-servers/udp-test-cases).

I have also cloned a public repository [[3]](https://github.com/tomashobza/tcp-udp-chat-client-tester) containing testing script for the same project from the previous year and modified accordingly to the current
project's assignment because it differs from the assignment from the previous year. All tests pass.

Then I also cloned a public repository [[4]](https://github.com/Vlad6422/VUT_IPK_CLIENT_TESTS) containing testing script
for the current project's assignment and added some my own tests. All tests pass as well.

Testing of whether TCP variant of my program correctly processes case-insensitive grammar of IPK25CHAT protocol that runs
over TCP and that my program correctly detects end of each server message was done using [this python script](https://gist.github.com/okurka12/87460576c644f33f38551cb819cdc075).

I have run my program multiple times in a production-like environment that was specified in the project's assignment
against a reference server implementation that is compliant with the protocol specification. The application's behaviour met the expectations.

TCP version:
```text
Action Success: Authentication successful.
Server: dname has joined `discord.general` via TCP.
Server: test123 has joined `discord.general` via UDP.
hihihiha: fdd
Server: Honza_test has left `discord.general`.
Server: x has joined `discord.general` via UDP.
hhServer: hihihiha has switched from `discord.general` to `discord.testik`.
Server: masha has joined `discord.general` via UDP.
hello there!
Server: oaoao has joined `discord.general` via TCP.
oaoao: hi
Server: testik has switched from `discord.general` to `discord.test`.
Server: x has joined `discord.general` via UDP.
/jServer: arstryx has joined `discord.general` via TCP.
oServer: oaoao has left `discord.general`.
in abc
Action Success: Channel abc successfully joined.
Server: dname has joined `abc` via TCP.
Server: dname has switched from `discord.general` to `abc`.
/join discord.asdf
Action Failure: Cannot join channel discord.asdf: Failed to create channel discord.asdf.
/join discord.test
Server: dname has switched from `abc` to `discord.test`.
Action Success: Channel discord.test successfully joined.
Server: dname has joined `discord.test` via TCP.
??
==1900138==
==1900138== HEAP SUMMARY:
==1900138==     in use at exit: 0 bytes in 0 blocks
==1900138==   total heap usage: 3,428 allocs, 3,428 frees, 218,949 bytes allocated
==1900138==
==1900138== All heap blocks were freed -- no leaks are possible
==1900138==
==1900138== For lists of detected and suppressed errors, rerun with: -s
==1900138== ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)
```

Part of _tcpdump_ output:
```text
23:05:05.382135 IP anton5.fit.vutbr.cz.4567 > 172.25.254.131.56864: Flags [P.], seq 1:41, ack 68, win 509, options [nop,nop,TS val 3432279467 ecr 3082142358], length 40
0x0000:  4500 005c e35f 4000 3b06 14c6 93e5 08f4  E..\._@.;.......
0x0010:  ac19 fe83 11d7 de20 7bf1 3b58 372e ed42  ........{.;X7..B
0x0020:  8018 01fd cb27 0000 0101 080a cc94 6dab  .....'........m.
0x0030:  b7b5 c296 5245 504c 5920 4f4b 2049 5320  ....REPLY.OK.IS.
0x0040:  4175 7468 656e 7469 6361 7469 6f6e 2073  Authentication.s
0x0050:  7563 6365 7373 6675 6c2e 0d0a            uccessful...
23:05:05.382144 IP 172.25.254.131.56864 > anton5.fit.vutbr.cz.4567: Flags [.], ack 41, win 502, options [nop,nop,TS val 3082142363 ecr 3432279467], length 0
0x0000:  4500 0034 2891 4000 4006 cabc ac19 fe83  E..4(.@.@.......
0x0010:  93e5 08f4 de20 11d7 372e ed42 7bf1 3b80  ........7..B{.;.
0x0020:  8010 01f6 479d 0000 0101 080a b7b5 c29b  ....G...........
0x0030:  cc94 6dab                                ..m.
23:05:05.680441 IP anton5.fit.vutbr.cz.4567 > 172.25.254.131.56864: Flags [P.], seq 41:105, ack 68, win 509, options [nop,nop,TS val 3432279766 ecr 3082142363], length 64
0x0000:  4500 0074 e360 4000 3b06 14ad 93e5 08f4  E..t.`@.;.......
        0x0010:  ac19 fe83 11d7 de20 7bf1 3b80 372e ed42  ........{.;.7..B
        0x0020:  8018 01fd 05d3 0000 0101 080a cc94 6ed6  ..............n.
        0x0030:  b7b5 c29b 4d53 4720 4652 4f4d 2053 6572  ....MSG.FROM.Ser
        0x0040:  7665 7220 4953 2064 6e61 6d65 2068 6173  ver.IS.dname.has
        0x0050:  206a 6f69 6e65 6420 6064 6973 636f 7264  .joined.`discord
0x0060:  2e67 656e 6572 616c 6020 7669 6120 5443  .general`.via.TC
        0x0070:  502e 0d0a                                P...
23:05:05.680479 IP 172.25.254.131.56864 > anton5.fit.vutbr.cz.4567: Flags [.], ack 105, win 502, options [nop,nop,TS val 3082142661 ecr 3432279766], length 0
        0x0000:  4500 0034 2892 4000 4006 cabb ac19 fe83  E..4(.@.@.......
        0x0010:  93e5 08f4 de20 11d7 372e ed42 7bf1 3bc0  ........7..B{.;.
        0x0020:  8010 01f6 479d 0000 0101 080a b7b5 c3c5  ....G...........
        0x0030:  cc94 6ed6                                ..n.
23:05:05.775224 IP anton5.fit.vutbr.cz.4567 > 172.25.254.131.56864: Flags [P.], seq 105:171, ack 68, win 509, options [nop,nop,TS val 3432279861 ecr 3082142661], length 66
        0x0000:  4500 0076 e361 4000 3b06 14aa 93e5 08f4  E..v.a@.;.......
        0x0010:  ac19 fe83 11d7 de20 7bf1 3bc0 372e ed42  ........{.;.7..B
        0x0020:  8018 01fd d2e5 0000 0101 080a cc94 6f35  ..............o5
        0x0030:  b7b5 c3c5 4d53 4720 4652 4f4d 2053 6572  ....MSG.FROM.Ser
        0x0040:  7665 7220 4953 2074 6573 7431 3233 2068  ver.IS.test123.h
        0x0050:  6173 206a 6f69 6e65 6420 6064 6973 636f  as.joined.`disco
0x0060:  7264 2e67 656e 6572 616c 6020 7669 6120  rd.general`.via.
0x0070:  5544 502e 0d0a                           UDP...
23:05:05.775240 IP 172.25.254.131.56864 > anton5.fit.vutbr.cz.4567: Flags [.], ack 171, win 502, options [nop,nop,TS val 3082142756 ecr 3432279861], length 0
0x0000:  4500 0034 2893 4000 4006 caba ac19 fe83  E..4(.@.@.......
0x0010:  93e5 08f4 de20 11d7 372e ed42 7bf1 3c02  ........7..B{.<.
0x0020:  8010 01f6 479d 0000 0101 080a b7b5 c424  ....G..........$
0x0030:  cc94 6f35                                ..o5
23:05:07.066743 IP anton5.fit.vutbr.cz.4567 > 172.25.254.131.56864: Flags [P.], seq 171:197, ack 68, win 509, options [nop,nop,TS val 3432281152 ecr 3082142756], length 26
0x0000:  4500 004e e362 4000 3b06 14d1 93e5 08f4  E..N.b@.;.......
0x0010:  ac19 fe83 11d7 de20 7bf1 3c02 372e ed42  ........{.<.7..B
0x0020:  8018 01fd e11e 0000 0101 080a cc94 7440  ..............t@
0x0030:  b7b5 c424 4d53 4720 4652 4f4d 2068 6968  ...$MSG.FROM.hih
0x0040:  6968 6968 6120 4953 2066 6464 0d0a       ihiha.IS.fdd..
```

UDP version:
```text
Action Success: Authentication successful.
Server: dname has joined `discord.general` via UDP.
Server: jani has joined `discord.general` via TCP.
jani: Ahoj
hello there!
/join abc
Action Success: Channel abc successfully joined.
Server: dname has switched from `discord.general` to `abc`.
Server: dname has joined `abc` via UDP.
/join discord.asdf
Action Failure: Cannot join channel discord.asdf: Failed to create channel discord.asdf.
asdf
/join discrod.test
Action Failure: Cannot join channel discrod.test: No channel manager for service 'discrod' registered.
/join discord.general
Server: dname has switched from `abc` to `discord.general`.
Action Success: Channel discord.general successfully joined.
Server: dname has joined `discord.general` via UDP.
whaat
Server: krtek has joined `discord.general` via UDP.
Server: omen has joined `discord.general` via TCP.
^C==1910826==
==1910826== HEAP SUMMARY:
==1910826==     in use at exit: 0 bytes in 0 blocks
==1910826==   total heap usage: 3,395 allocs, 3,395 frees, 216,657 bytes allocated
==1910826==
==1910826== All heap blocks were freed -- no leaks are possible
==1910826==
==1910826== For lists of detected and suppressed errors, rerun with: -s
==1910826== ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)
```

Part of _tcpdump_ output:
```text
23:13:10.600458 IP anton5.fit.vutbr.cz.4567 > 172.25.254.131.54783: UDP, length 3
0x0000:  4500 001f bfa3 4000 3b11 38b4 93e5 08f4  E.....@.;.8.....
0x0010:  ac19 fe83 11d7 d5ff 000b d08a 0000 00    ...............
23:13:10.603991 IP anton5.fit.vutbr.cz.48681 > 172.25.254.131.54783: UDP, length 33
0x0000:  4500 003d bfa7 4000 3b11 3892 93e5 08f4  E..=..@.;.8.....
0x0010:  ac19 fe83 be29 d5ff 0029 0cb9 0100 0001  .....)...)......
0x0020:  0000 4175 7468 656e 7469 6361 7469 6f6e  ..Authentication
0x0030:  2073 7563 6365 7373 6675 6c2e 00         .successful..
23:13:10.631150 IP 172.25.254.131.54783 > anton5.fit.vutbr.cz.48681: UDP, length 3
0x0000:  4500 001f 73c7 4000 4011 7f90 ac19 fe83  E...s.@.@.......
0x0010:  93e5 08f4 d5ff be29 000b 4793 0000 00    .......)..G....
23:13:12.094354 IP anton5.fit.vutbr.cz.48681 > 172.25.254.131.54783: UDP, length 54
0x0000:  4500 0052 c1ec 4000 3b11 3638 93e5 08f4  E..R..@.;.68....
0x0010:  ac19 fe83 be29 d5ff 003e f40e 0400 0153  .....)...>.....S
0x0020:  6572 7665 7200 646e 616d 6520 6861 7320  erver.dname.has.
0x0030:  6a6f 696e 6564 2060 6469 7363 6f72 642e  joined.`discord.
        0x0040:  6765 6e65 7261 6c60 2076 6961 2055 4450  general`.via.UDP
0x0050:  2e00                                     ..
23:13:12.103563 IP 172.25.254.131.54783 > anton5.fit.vutbr.cz.48681: UDP, length 3
0x0000:  4500 001f 7413 4000 4011 7f44 ac19 fe83  E...t.@.@..D....
0x0010:  93e5 08f4 d5ff be29 000b 4793 0000 01    .......)..G....
23:13:12.514974 IP anton5.fit.vutbr.cz.48681 > 172.25.254.131.54783: UDP, length 53
0x0000:  4500 0051 c255 4000 3b11 35d0 93e5 08f4  E..Q.U@.;.5.....
0x0010:  ac19 fe83 be29 d5ff 003d 3c2d 0400 0253  .....)...=<-...S
0x0020:  6572 7665 7200 6a61 6e69 2068 6173 206a  erver.jani.has.j
0x0030:  6f69 6e65 6420 6064 6973 636f 7264 2e67  oined.`discord.g
        0x0040:  656e 6572 616c 6020 7669 6120 5443 502e  eneral`.via.TCP.
0x0050:  00                                       .
23:13:12.515374 IP 172.25.254.131.54783 > anton5.fit.vutbr.cz.48681: UDP, length 3
0x0000:  4500 001f 7420 4000 4011 7f37 ac19 fe83  E...t.@.@..7....
0x0010:  93e5 08f4 d5ff be29 000b 4793 0000 02    .......)..G....
23:13:16.305304 IP anton5.fit.vutbr.cz.48681 > 172.25.254.131.54783: UDP, length 13
0x0000:  4500 0029 c2f5 4000 3b11 3558 93e5 08f4  E..)..@.;.5X....
0x0010:  ac19 fe83 be29 d5ff 0015 a178 0400 036a  .....).....x...j
0x0020:  616e 6900 4168 6f6a 00                   ani.Ahoj.
23:13:16.305804 IP 172.25.254.131.54783 > anton5.fit.vutbr.cz.48681: UDP, length 3
0x0000:  4500 001f 748a 4000 4011 7ecd ac19 fe83  E...t.@.@.~.....
0x0010:  93e5 08f4 d5ff be29 000b 4793 0000 03    .......)..G....
23:13:18.877763 IP 172.25.254.131.54783 > anton5.fit.vutbr.cz.48681: UDP, length 22
0x0000:  4500 0032 7551 4000 4011 7df3 ac19 fe83  E..2uQ@.@.}.....
0x0010:  93e5 08f4 d5ff be29 001e 47a6 0400 0164  .......)..G....d
0x0020:  6e61 6d65 0068 656c 6c6f 2074 6865 7265  name.hello.there
0x0030:  2100                                     !.
23:13:18.878948 IP anton5.fit.vutbr.cz.48681 > 172.25.254.131.54783: UDP, length 3
0x0000:  4500 001f c6f5 4000 3b11 3162 93e5 08f4  E.....@.;.1b....
0x0010:  ac19 fe83 be29 d5ff 000b 2338 0000 01    .....)....#8...
```

## AI usage

I have used two large language models while solving this project: ChatGPT [1](https://chat.openai.com/) and DeepSeek Chat [2](https://chat.deepseek.com/). I used it for
code refactoring, commenting, and implementation of two pseudo-servers. The code that was generated by one of these
large language models and used in this project is marked by the comments. 

## Bibliography

[1] OpenAI. ChatGPT [online]. OpenAI, 2025 [cit. 2025-04-16]. Available at: https://chat.openai.com/

[2] DeepSeek. DeepSeek Chat [online]. DeepSeek, 2025 [cited 2025-04-16]. Available at: https://chat.deepseek.com/

[3] Tomáš HOBZA, et al. tcp-udp-chat-client-tester [online]. GitHub, 2025 [cit. 2025-04-16]. Available at: https://github.com/tomashobza/tcp-udp-chat-client-tester

[4] MALASHCHUK Vladyslav, Tomáš HOBZA, et al. VUT_IPK_CLIENT_TESTS [online]. GitHub, 2025 [cit. 2025-04-16]. Available at: https://github.com/Vlad6422/VUT_IPK_CLIENT_TESTS

[5] Vit Pavlik, test-tcp-stream.py [online]. GitHub, 2025 [cit. 2025-04-16]. Available at: https://gist.github.com/okurka12/87460576c644f33f38551cb819cdc075
