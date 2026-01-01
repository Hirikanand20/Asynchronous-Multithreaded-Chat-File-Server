📡 Boost.Asio Chat Server (C++)

A multi-client chat server built using C++ and Boost.Asio, supporting authentication, private messaging, public broadcast, group chats, and session management.

This project demonstrates network programming, concurrency, session handling, and server-client architecture in C++.

🚀 Features

🔐 Authentication & User Management

1.User registration (register:<username> <password>)

2.Login / Logout

3.whoami command

4.Online / Offline user tracking

5.Persistent storage using SQLite

<img width="1917" height="1078" alt="ss3" src="https://github.com/user-attachments/assets/a2dfc0de-c006-4286-93b7-e22d662add7f" />


💬 Messaging System

1.Public messages (public:<message>)

2.Private messages (private:<user_id> <message>)

3.Real-time message delivery

4.Thread-safe session handling

<img width="1918" height="1078" alt="ss4" src="https://github.com/user-attachments/assets/ee513ee8-4885-4e98-a9ba-dabb0792840e" />

👥 Group Chat Support

1.Create groups (group:create <id>)

2.Join / Leave groups

3.List group users

4.Group message broadcasting

5.Multiple groups active simultaneously


<img width="1918" height="1078" alt="ss2" src="https://github.com/user-attachments/assets/96e88474-74f6-4bf4-bbd7-9a31451aff41" />







<img width="1918" height="1078" alt="ss1" src="https://github.com/user-attachments/assets/1f33052e-2363-47d5-9061-ca8eae8fe55b" />






🏗 Architecture


1.Server: Accepts connections and manages sessions

2.Session: One per connected client

3.Client: Interactive command-line interface

4.Database: Stores users and message logs            





🛠 Technologies Used

1.C++17

2.Boost.Asio

3.SQLite3

4.Multithreading (std::thread, mutex)

5.TCP/IP Networking

6.Git & GitHub




🖥 Commands Supported

register:<username> <password>

login:<username> <password>

logout

whoami

public:<message>

private:<user_id> <message>

group:create <group_id>

group:join <group_id>

group:leave <group_id>

group:list <group_id>

list users

exit





⚙️ How to Build & Run



Requirements:

Visual Studio 2022

Boost (Asio)

SQLite3

Steps

Clone the repository

Open P11.sln

Build Server and Client

Run server first

Run multiple clients in separate terminals





📌 Learning Outcomes

1.Real-world client-server design

2.Concurrent programming with thread safety

3.TCP networking using Boost.Asio

4.Database integration in C++

5.Command-driven protocol design
