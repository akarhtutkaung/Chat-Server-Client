Overview
--------
Systems programming is often associated with communication as this invariably requires coordinating multiple entities that are related only based on their desire to share information. This project focuses on developing a simple chat server and client called blather. The basic usage is in two parts.

Server,
  Some user starts bl_server which manages the chat "room". The server is non-interactive and will likely only print debugging output as it runs.
Client,
  Any user who wishes to chat runs bl_client which takes input typed on the keyboard and sends it to the server. The server broadcasts the input to all other clients who can respond by typing their own input.

Note# Unlike standard internet chat services, blather will be restricted to a single Unix machine and to users with permission to read related files.

System implementation
----------------------
Multiple communicating processes: clients to servers
Communication through FIFOs
Signal handling for graceful server shutdown
Alarm signals for periodic behavior
Input multiplexing with poll()
Multiple threads in the client to handle typed input versus info from the server

By,
- Name  : Ace Kaung
- Email : kaung006@umn.edu

- Name  : Vy Vu
- Email : vuxxx291@umn.edu
