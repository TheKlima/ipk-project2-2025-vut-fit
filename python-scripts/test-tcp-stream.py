# IPK25chat client TCP stream test
# author: vita v22.0
# date: 13. 4. 2025
# python 3.13.0

import socket

ADDRESS = "127.0.0.1"
PORT = 4567

def test(s: str) -> None:
    escaped = s.replace("\n", "\\n").replace("\r", "\\r")
    print(f"Press enter to send \"{escaped}\": ", flush=True, end="")
    input()
    csock.send(s.encode("ascii"))
    print("sent")

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.bind((ADDRESS, PORT))
sock.listen()
print("socket is bound and listening")
csock, _ = sock.accept()
print("connected")

# test("RePLy Ok iS oK\r\n")
test("RePLy Ok ")
test("iS oK\r\n")
test("MSg FROM 2 is 2\r\nmsg from 3 is 3\r\nmsg from 4 is 4\r\n")
test("msg fROm 5 is 5\r\nmsg from 6 is 6\r\nmsg from 7 is")
test(" 7\r\n")
test("msg from")
test(" 8 is 8\r\n")
test("bYE from xd\r\n")

print("Pres enter to shutdown the socket: ", flush=True, end="")
input()
csock.shutdown(socket.SHUT_RDWR)
csock.close()
sock.shutdown(socket.SHUT_RDWR)
sock.close()
print()