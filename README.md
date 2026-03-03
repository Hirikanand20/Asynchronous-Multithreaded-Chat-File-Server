
# 📡 Boost.Asio File + Chat Server (C++)

A multi-client **File Transfer + Chat Server** built using **C++17** and **Boost.Asio**, supporting authentication, real-time messaging, group communication, and secure file sharing between users.

This project demonstrates advanced **network programming**, **concurrency control**, **file streaming over TCP**, **session management**, and **client-server architecture** in modern C++.

---

## 🚀 Features

---

## 🔐 Authentication & User Management

* User Registration (`register:`)
* Login / Logout
* `whoami` command
* Online / Offline tracking
* Persistent user storage using SQLite3
* Thread-safe session handling

---

## 💬 Real-Time Messaging System

* Public chat (`public:`)
* Private messaging (`private:<user_id>`)
* Group messaging
* Real-time message delivery
* Concurrent multi-client support
* Message logging in database

---

## 📁 File Transfer System

* Send files to specific users
* Send files to groups
* File metadata exchange (name, size)
* Chunk-based file transmission over TCP
* Automatic file reconstruction on client side
* Concurrent file transfers supported
* Server-side file routing logic

---

## 👥 Group Chat + File Sharing

* Create groups (`group:create <group_id>`)
* Join / Leave groups
* List group members
* Group message broadcasting
* Multiple active groups simultaneously

---

## 🏗 Architecture

### 1️⃣ Server

* Accepts multiple TCP connections
* Manages sessions
* Routes messages and files
* Maintains active user list
* Handles concurrency using mutexes

### 2️⃣ Session

* One session per connected client
* Async read/write handling
* Auth state management
* File transfer state tracking

### 3️⃣ Client

* Command-line interface
* Supports chat + file upload/download
* Handles asynchronous responses
* Reconstructs received files

### 4️⃣ Database (SQLite3)

* User credentials
* Message logs
* Persistent storage

---

## 🛠 Technologies Used

* C++17
* Boost.Asio (Asynchronous TCP Networking)
* SQLite3
* Multithreading (`std::thread`, `std::mutex`)
* TCP/IP Socket Programming
* File I/O (Binary Mode)
* Git & GitHub

---

## 🖥 Supported Commands

### 🔑 Authentication

```
register: <username> <password>
login: <username> <password>
logout
whoami
list users
```

### 💬 Messaging

```
public: <message>
private:<user_id> <message>
```

### 👥 Groups

```
group:create <group_id>
group:join <group_id>
group:leave <group_id>
group:list <group_id>
```

### 📁 File Transfer

```
sendfile:<user_id> <filepath>
group:sendfile <group_id> <filepath>
```

### ❌ Exit

```
exit
```

---

## ⚙️ How to Build & Run

### 📌 Requirements

* Visual Studio 2022
* Boost Library (Asio)
* SQLite3
* Windows OS (Tested)

---

### 🔧 Steps

1. Clone the repository
2. Open `P11.sln`
3. Build **Server** and **Client**
4. Run the **Server** first
5. Run multiple clients in separate terminals
6. Login and start chatting or transferring files

---

## 🎥 Video Demonstration

## 📺 Project Demonstration

[![Boost.Asio File & Chat Server Demo](https://img.youtube.com/vi/cdryq1UTjgY/maxresdefault.jpg)](https://www.youtube.com/watch?v=cdryq1UTjgY)

*Click the image above to watch the full walkthrough on YouTube.*

## 📌 Learning Outcomes

* Advanced Client-Server Architecture Design
* Asynchronous Networking with Boost.Asio
* Concurrent Programming & Thread Safety
* File Streaming over TCP
* Session State Management
* Binary File Handling
* Database Integration (SQLite3)
* Command-based Network Protocol Design
* Real-world Distributed System Concepts

---

## 📈 Project Significance

This project simulates real-world messaging systems like:

* WhatsApp (chat logic)
* Telegram (group system)
* Basic cloud file routing architecture

It demonstrates strong backend engineering fundamentals in **modern C++ networking**.

