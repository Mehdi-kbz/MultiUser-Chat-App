### Milestone 1 - TCP Multi-Client Server

In this first stage, we designed a client/server communication model based on IPv4, where a server responds to client requests by echoing back the character string sent by the client. This milestone focuses on establishing a TCP connection, enabling data transfer between a client and a server using sockets.

### Developed Features

1. **TCP Client/Server Connection**: The client connects to a server specified by its domain name and port. On the server side, it listens on a specified port, accepts incoming connections, and exchanges data with the client.

2. **Managing Multiple Connections**: The server can handle multiple clients simultaneously using a single process, thanks to the `poll()` function. Client information is stored in a linked list, which simplifies managing various connections.

3. **Sending and Receiving Character Strings**: Once connected, the client can send messages to the server, which echoes them back. This echo interaction ensures the client sees their own input confirmed by the server.

4. **Using `poll()` for Concurrency**: On both the client and server sides, the `poll()` function enables asynchronous input and output management. This allows the client to type text while smoothly receiving responses from the server.

5. **Connection Termination and Resource Cleanup**: The connection ends when the client sends the `/quit` command. All resources are properly released at the end, with sockets closed and data structures cleaned up.

### Requirements Met

The functionalities implemented in this milestone adhere to the following requirements:
- **Req1.1** to **Req1.8**: Creation and management of TCP connections, character string handling, simultaneous client management, and appropriate resource cleanup.

With this first stage, we establish a robust and extensible communication system that will serve as a foundation for future developments.

### How to Use the IPv4 Client/Server Echo Model

This guide provides steps to set up and run the client/server echo system, which allows multiple clients to connect to a server, send messages, and receive the same message back as confirmation. Follow these instructions to run the application:

#### 1. Compile the Client and Server Programs

First, compile the client and server code. Make sure both source files are in the same directory, and then run the following command:

```bash
make
```

#### 2. Start the Server

To run the server, specify the port number as an argument. The server will listen on this port for incoming client connections:

```bash
./server <server_port>
```

Replace `<server_port>` with a valid port number, such as `12345`. Ensure that the chosen port is not in use by another application.

#### 3. Connect the Client

Next, run the client program by specifying the server’s domain name (or IP address) and the same port number used by the server:

```bash
./client <server_name> <server_port>
```

Replace `<server_name>` with the server’s IP address (e.g., `127.0.0.1` for local connections) and `<server_port>` with the port specified when starting the server.

#### 4. Send Messages and Receive Echoes

Once connected, you can type messages into the client’s terminal. Each message will be sent to the server, and the server will echo it back to the client. The echoed message will be displayed in the client’s terminal.

To quit the session, type `/quit` and press Enter. This will terminate the connection, release all resources, and close the client.

#### 5. Close the Server

The server will continue to run until you manually stop it. To shut down the server, press `Ctrl+C` in the server’s terminal.

### Example Usage

Here’s a quick example assuming the server is running locally on port `12345`:

1. Start the server:
   ```bash
   ./server 12345
   ```

2. Connect a client:
   ```bash
   ./client 127.0.0.1 12345
   ```

3. Type and send a message in the client’s terminal:
   ```plaintext
   Hello, Server!
   ```

4. The server echoes the message back, which displays in the client terminal:
   ```plaintext
   Message received: Hello, Server!
   ```

5. Type `/quit` to exit:
   ```plaintext
   /quit
   ```
After you are done running the client and server programs, you can clean up the directory by removing the compiled binaries and any other temporary files:

6. Clean Up Compiled Files
      ```plaintext
      make clean
      ```

This step is optional but recommended, especially if you plan to recompile the programs or if you’re sharing the project files with others. It ensures that only the source code and essential files remain in the project directory.

### Milestone 2 - User Management and Messaging

### Overview

In this second milestone, we transform the server from a basic echo model to a fully functional intermediary supporting multiple users and interactions. The system allows users to log in with unique nicknames, view the list of connected users, send private and broadcast messages, and retrieve detailed user information. Additionally, users can change their nickname, view their current nickname, and gracefully disconnect from the server. All communication between the client and server is managed through structured commands.

### Developed Features

1. **Structured Message Format**:
   - A structured message format has been implemented to handle various message types. The format includes fields for the sender's nickname, message type (e.g., `NICKNAME_NEW`, `UNICAST_SEND`, `BROADCAST_SEND`), and additional information. This structure supports flexible and robust communication between the server and clients.

2. **Server Support for Multiple Clients**:
   - The server is capable of handling multiple clients simultaneously. It keeps track of users and their connection details using a linked list, dynamically updating the user list as clients connect, disconnect, or change their nicknames.

3. **Nickname Management**:
   - **/nick <nickname>**: Users are required to log in or update their nickname with the `/nick` command. The server ensures that nicknames are unique and handles edge cases such as excessively long nicknames or invalid characters (spaces or special characters). Feedback is provided if a nickname is invalid or already in use.






<img width="490" alt="Screenshot 2024-10-23 at 18 49 24" src="https://github.com/user-attachments/assets/d74d12a2-be9b-4459-9656-9d3f7d66982e">







<img width="490" alt="Screenshot 2024-10-23 at 19 01 36" src="https://github.com/user-attachments/assets/e54662dc-2bca-4b0a-bb78-c6ccee6e3bc7">









<img width="490" alt="Screenshot 2024-10-23 at 19 03 56" src="https://github.com/user-attachments/assets/6d5f4e51-48a2-4a67-8db2-81e61c06fc4f">


4. **View All Connected Users**:
   - **/who**: The `/who` command allows users to view a list of all connected users. This list is updated in real-time as users join, leave, or change their nickname.


<img width="490" alt="Screenshot 2024-10-23 at 19 09 27" src="https://github.com/user-attachments/assets/944aee30-57bb-4e62-911e-775593183851">





5. **View User Details**:
   - **/whois <user>**: The `/whois` command allows users to retrieve detailed information about a specific connected user. It checks if the specified user is online and, if so, returns the following details:

- **IP Address**: The user's network address.

- **Port Number**: The port through which the user is connected.

- **Connection Time**: The timestamp of when the user connected to the server.

> If the user is not connected/existant, the command will inform the requester that the user is non-existant. This functionality enhances user interaction within the messaging service.




<img width="837" alt="Screenshot 2024-10-23 at 19 11 05" src="https://github.com/user-attachments/assets/2cd1ac8c-0e66-45ce-8896-133c67013a9e">



6. **View Current Nickname**:
   - **/whoami**: Users can use the `/whoami` command to retrieve and display their own information including their IP address, port number, and connection time.


<img width="837" alt="Screenshot 2024-10-23 at 19 20 24" src="https://github.com/user-attachments/assets/811a1798-95bf-41ad-a777-30d99f40d7b5">



7. **Private Messaging**:
   - **/msg <user> <message>**: Users can send private messages to other connected users with the `/msg` command. 

In Mehdi's Terminal:
<img height="20" width="837" alt="Screenshot 2024-10-23 at 19 32 32" src="https://github.com/user-attachments/assets/f29fc2cf-c93e-4958-9613-87269b79c097">


In Deanarys's Terminal:
<img width="837" alt="Screenshot 2024-10-23 at 19 28 35" src="https://github.com/user-attachments/assets/5f215e7e-7d8e-4a08-a88f-a4b0a319332f">

> If the target user does not exist, the sender is notified. This ensures users only send messages to valid recipients.
<img width="837" alt="Screenshot 2024-10-23 at 19 35 08" src="https://github.com/user-attachments/assets/3a30b109-42e4-48ed-8ff3-bfd339a46fcb">




8. **Broadcast Messaging**:
   - **/msgall <message>**: The `/msgall` command allows users to send a message to all connected users simultaneously. The server ensures that the sender does not receive their own broadcast message.

In Mehdi's Terminal:

<img width="837" alt="Screenshot 2024-10-23 at 19 38 00" src="https://github.com/user-attachments/assets/2d4ebb17-17ca-4b72-829f-a7f7540d6cb0">

In other Terminals:
<img width="837" alt="Screenshot 2024-10-23 at 19 38 35" src="https://github.com/user-attachments/assets/49050f65-17a2-404e-96ba-f76dfe73c876">



9. **Exit**:

   - **/quit**: The `/quit` command allows users to disconnect from the server gracefully. Upon exiting, the server updates the user list and informs the user of the disconnection.

<img width="837" alt="Screenshot 2024-10-23 at 19 45 07" src="https://github.com/user-attachments/assets/9d5cede7-aad8-42a5-8b2b-773b3e7c6d6c">




This release introduces a complete user management system, enabling seamless interactions between multiple clients and the server. Users can manage nicknames, send messages, and view information, laying the groundwork for more advanced features in future milestones.


### MILESTONE 3 - Chat Rooms implementation

### Overview
In this third milestone, the server evolves from a basic messaging tool into a full-fledged chat application supporting discussion rooms. This new feature enables users to create and join chat rooms, where they can send messages that only other room members will receive. Users can switch between rooms, view available rooms, and receive real-time notifications when other users enter or exit a shared room. This milestone expands the application's utility, creating a collaborative environment for group conversations.

### Developed Features

1. **Creating Chat Rooms:**

-  Users can create a new chat room with the /create command: `/create <channel_name>` (Type: MULTICAST_CREATE)

- Automatic Joining: Users are automatically added to any chat room they create, leaving their previous room if they were already in one.

<img width="800" alt="Screenshot 2024-11-10 at 4 37 03 PM" src="https://github.com/user-attachments/assets/3b6312e5-f6f9-44fc-a57f-7ac34512b664">

> If a room already exists with the specified name or the name includes invalid characters (e.g., spaces or special characters), the server returns an error:

<img width="800" alt="Screenshot 2024-11-10 at 4 33 44 PM" src="https://github.com/user-attachments/assets/3cfbe6e7-7f3e-4967-aac9-7d8174a4fbe4">



2. **Listing Available Rooms:**

- The `/channel_list` command (Type: MULTICAST_LIST) allows users to view a list of active chat rooms. The server responds with an up-to-date list, helping users choose a room to join.
<img width="800" alt="Screenshot 2024-11-10 at 4 40 09 PM" src="https://github.com/user-attachments/assets/21174cc7-2636-4b9e-810d-9efc25dbdfaa">

>  if no channels are available,The server responds with :

<img width="800" alt="Screenshot 2024-11-10 at 4 34 06 PM" src="https://github.com/user-attachments/assets/35fca499-190f-4c48-b244-64c9f42b393b">

4. **Joining Chat Rooms:**

- The`/join <channel_name>` command (Type: MULTICAST_JOIN) allows users to join a specific chat room using this command. Upon joining, they automatically exit their previous room.

<img width="800" alt="Screenshot 2024-11-10 at 4 50 51 PM" src="https://github.com/user-attachments/assets/95949f63-0380-4387-9b97-cc090536a5ff">


>  if there is no channel with the specified name, The server responds with:

<img width="800" alt="Screenshot 2024-11-10 at 4 51 26 PM" src="https://github.com/user-attachments/assets/80beb473-4d73-449d-a8e8-dd65561097e0">


5. **Leaving Chat Rooms:**

- The `/quit` command (Type: MULTICAST_QUIT)enables users to leave a chat room. 
<img width="800" alt="Screenshot 2024-11-10 at 4 57 23 PM" src="https://github.com/user-attachments/assets/1d19022c-0d1d-43e2-b82a-9f3f1eac3ed0">


> If a user is the last participant in a room, the server closes the room and informs him of its closure.
<img width="800" alt="Screenshot 2024-11-10 at 5 02 48 PM" src="https://github.com/user-attachments/assets/65f885e0-116f-44c2-8b6b-0a899274cfb9">



5. **Sending Messages within Rooms:**

- Multicast Messaging: Users in a chat room can send messages by simply typing their message, which the server then delivers to all members of the room only (Type: MULTICAST_SEND). This ensures privacy and a focused conversation among room participants.

**sender's terminal:**
<img width="800" alt="Screenshot 2024-11-10 at 5 08 14 PM" src="https://github.com/user-attachments/assets/9ea05720-f3ab-40bc-872c-c2974a43c1a2">

**All channel members terminals (except the sender):**
<img width="800" alt="Screenshot 2024-11-10 at 5 07 49 PM" src="https://github.com/user-attachments/assets/be6fe051-adc6-4629-8470-3d879f9d9be1">



6. **Real-Time Room Notifications:**

- The server sends notifications to all room members whenever a user joins or leaves. This keeps all participants informed of activity in the room, adding a layer of interaction and community awareness.
<img width="800" alt="Screenshot 2024-11-10 at 5 11 42 PM" src="https://github.com/user-attachments/assets/9fac769f-50fb-41c0-8cc0-6aa138f24582">


 This release establishes the foundation for group chat functionality, allowing users to communicate within specialized rooms. By providing structured room management commands and robust notification features, this milestone enhances user experience and interaction within the application, paving the way for additional collaborative functionalities in future updates.

 ### MILESTONE 4 - P2P File Transfers

 ### Overview

In this fourth milestone, we introduce peer-to-peer (P2P) file transfers, enabling users to send files directly to each other without routing data through the server. The server functions as an intermediary, only responsible for facilitating the connection between users who want to exchange files. This approach allows for efficient data transfer by minimizing server load and network latency.

### Developed Features

1. File Transfer Request:

 - The sender initiates a file transfer by issuing the `/send <user> <file_name>` command, specifying the recipient's username and the file name. This command sends a FILE_REQUEST message to the server, which forwards it to the specified recipient.

In the following example, Mehdi (sender) requests a file transfer to MehdiToo (recipient)
**sender's terminal:**
<img width="800" alt="Screenshot 2024-11-10 at 5 54 08 PM" src="https://github.com/user-attachments/assets/04b7317a-53c7-4524-8d64-819173d0e621">


**recipient's terminal:**
<img width="800" alt="Screenshot 2024-11-10 at 5 54 48 PM" src="https://github.com/user-attachments/assets/85118b86-079e-4297-8884-a7972681086e">

> If the specified user or file does not exist, the server responds with:
<img width="800" alt="Screenshot 2024-11-10 at 6 00 41 PM" src="https://github.com/user-attachments/assets/0bb3afb9-08fe-454f-ba51-32ae2d8d77ab">

Or:

<img width="800" alt="Screenshot 2024-11-10 at 5 59 47 PM" src="https://github.com/user-attachments/assets/d1917015-5adf-4d5e-9153-22eaeff4f0f2">



2. Recipient Acceptance or Rejection and Direct P2P Connection:

 - Upon receiving the FILE_REQUEST, the recipient is prompted to either accept or reject the file transfer. The recipient can respond with a simple "Y/y" or "N/n" to accept (FILE_ACCEPT) or reject (FILE_REJECT) the transfer. 
  - If the recipient accepts, their response includes their IP address and listening port for direct P2P connection. The sender connects directly to this address, bypassing the server, and begins sending the file (FILE_SEND). This direct approach supports efficient, high-speed file transfers.
  - Once the file transfer is complete, the recipient sends a FILE_ACK message back to the sender, confirming successful file receipt. Both users receive a final confirmation message indicating the transfer's successful completion.
 

https://github.com/user-attachments/assets/87776415-4de8-415a-a519-5e5865fbd8f6


  
 > If rejected, the sender is notified, and no further action is taken:
<img width="800" alt="Screenshot 2024-11-10 at 6 04 37 PM" src="https://github.com/user-attachments/assets/2047b435-869a-42fc-a79c-4ad57a984615">







