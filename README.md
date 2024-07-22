# Client Application Documentation
## Denys Chernenko xchern08 IPK-1
### Overview
This client application is designed to communicate with a server using both UDP and TCP protocols. It provides functionality for sending messages, joining channels, authenticating with the server, and handling various error conditions. The application is implemented in C and consists of multiple source files including: `UDP.c`, 
`UDP.h`, 
`TCP.c`, 
`TCP.h`, 
`errorproc.c`.
`errorproc.h`.
`supportTCP.c`.
`supportTCP.h`.
`main.c`, 

## Describing File Structure
- **TCP.c** - Handles all the logic for connecting to the server using the TCP protocol, sending messages to the server, and receiving messages from the server. It utilizes two threads, including the main thread. The first thread is responsible for sending messages to the server, while the second thread is responsible for receiving messages from the server.

- **TCP.h** - Includes all the information about functions related to `TCP.c`.

- **UDP.c** - Manages all the logic for sending messages to the server using the UDP protocol. It employs three threads: the first one constructs messages to prepare them for sending to the server, the second one listens to the server and receives messages from it, and the third one is responsible for sending messages. Notably, UDP.c is designed with the `PendingMessage` struct, which serves as a queue for messages before sending, ensuring that each message is sent correctly.

- **UDP.h**: Contains all the information about functions related to `UDP.c`.
- **errorproc.c** - designed to wrap functions such as: `Socket` `Connect` or `Inet_Pton`, with their errors codes.
- **errorproc.h** - Includes all the information about functions related to `errorproc.c`.
- **SupportTCP.c** - designed to include functions, which are not related to speak with server, but helping in Grammar Contexts. Such as: `is_valid_secret`, `is_valid_dname`, `is_valid_id`, `is_valid_content`
- **SupportTCP.h** -  Includes all the information about functions related to `SupportTCP.h`.



## Usage
To use the client application, follow these steps:
1. **Main Logic**

- **-t**: Specifies the transport protocol used for the connection.
  - **Value**: User-provided.
  - **Possible values**: "tcp" or "udp".
  - **Meaning or expected program behavior**: Determines whether the client will use TCP or UDP protocol for the connection.

- **-s**: Specifies the IP address or hostname of the server.
  - **Value**: User-provided.
  - **Meaning or expected program behavior**: Indicates the server's IP address or hostname that the client will connect to.

- **-p**: Specifies the server port.
  - **Value**: Default value is 4567 (uint16).
  - **Meaning or expected program behavior**: Sets the port number that the client will use to connect to the server.

- **-d**: Specifies the UDP confirmation timeout.
  - **Value**: Default value is 250 (uint16).
  - **Meaning or expected program behavior**: Sets the timeout duration (in milliseconds) for receiving confirmation in UDP communication.

- **-r**: Specifies the maximum number of UDP retransmissions.
  - **Value**: Default value is 3 (uint8).
  - **Meaning or expected program behavior**: Sets the maximum number of retransmissions allowed in UDP communication.

- **-h**: Displays the program help output and exits.
  - **Meaning or expected program behavior**: Prints the usage instructions for the client program and terminates the program execution.

2. **Communication with Server** 
#### /auth
- **Usage**: `/auth {Username} {Secret} {DisplayName}`
- **Description**: Sends an AUTH message with the provided data (Username, Secret, DisplayName) to the server. It handles the Reply message from the server and locally sets the DisplayName value for the user, which will be used in subsequent messages or commands.

#### /join
- **Usage**: `/join {ChannelID}`
- **Description**: Sends a JOIN message with the specified ChannelID to the server. It handles the Reply message from the server.

#### /rename
- **Usage**: `/rename {DisplayName}`
- **Description**: Locally changes the display name of the user to the provided DisplayName. The new display name will be used in subsequent messages or selected commands.

#### /help
- **Usage**: `/help`
- **Description**: Prints out the supported local commands along with their parameters and descriptions. It provides users with information about the available commands and their usage.

3. **Sending a Message**
- **Description**: Sends a message to the server. If using UDP as the transport protocol the message is added to the PendingMessage queue until confirmation is received. If confirmation is not required or when using TCP, the message is immediately sent to the server.

## Executive summary

### TCP Implementation
- **Description**: TCP communication is implemented using two threads, including the main thread. The main thread is responsible for sending messages to the server, handling commands and messages received from the user, and managing signals for program termination (e.g., CTRL+Z, CTRL+C). Meanwhile, the receiving thread continuously checks for messages sent from the server. It validates the format of received messages; if the format is incorrect, it sends an error message to the server and terminates the program immediately by sending a bye message.
### TCP Simple Representation in Diagram
[![](https://mermaid.ink/img/pako:eNqNVduO2yAQ_RXEk9N6fyCvW1XtQ6RKad8iIRYmMV3MWDCJZK3y78XA5kKcJpZsMHPOYW7YH1yhBr7kysoQvhm587LfOBavlTTul8dpgb28sDWqd6A_ZKwhA2EW87vzIPX3vVNk0M1jXrHvpdM_4m3BF0xFTPuBP4BfQQhyB9foSmJCF9wanDZuV3kwg_-P-h2piZWjHyv5_EwJvAr2IxsY-6rQOVAkCEVIGwtSQ1OmA3pqWXmRWvu4_4IZRyc6iWB2cUF0aDX4ZsEOaHQ2H299qCp14Ue2NFvZGzu2jMYBWjZ4JFRor_d8zS43IVK2OvuXvGvZ9LTgLr2IhJ8OSAyE7iQfvGqZDvTA3br2Z3etCQTunLVG-l0W-3JXra71Wa1LS0JlQFPGKooCknvqnkN6cLJ_UvUvGvccsgM7nJCPyj3XyrdxlwbrM64p4wPte2fhrB6iKafrU9mkblEeYlfrKTlViImRUvHJUNFFB1ZMTG3CYOUo7hEr_x8T3kY4kebs4D3GcxdnymKYKpkaP4bZxHNXuj-DnstZ_ZG4SJYJ4iCt0SLnp8nDgr0h2ltQSl5TUjgPMTqm-9J45C3vwffS6PhRTztvOHXQw4Yv41TDVu4tbfjGTdBYOFyPTvEl-T20fD9oSVB-A3y5lTbEVdCG0K_Kj2Iajv8AJvEbOw?type=png)](https://mermaid.live/edit#pako:eNqNVduO2yAQ_RXEk9N6fyCvW1XtQ6RKad8iIRYmMV3MWDCJZK3y78XA5kKcJpZsMHPOYW7YH1yhBr7kysoQvhm587LfOBavlTTul8dpgb28sDWqd6A_ZKwhA2EW87vzIPX3vVNk0M1jXrHvpdM_4m3BF0xFTPuBP4BfQQhyB9foSmJCF9wanDZuV3kwg_-P-h2piZWjHyv5_EwJvAr2IxsY-6rQOVAkCEVIGwtSQ1OmA3pqWXmRWvu4_4IZRyc6iWB2cUF0aDX4ZsEOaHQ2H299qCp14Ue2NFvZGzu2jMYBWjZ4JFRor_d8zS43IVK2OvuXvGvZ9LTgLr2IhJ8OSAyE7iQfvGqZDvTA3br2Z3etCQTunLVG-l0W-3JXra71Wa1LS0JlQFPGKooCknvqnkN6cLJ_UvUvGvccsgM7nJCPyj3XyrdxlwbrM64p4wPte2fhrB6iKafrU9mkblEeYlfrKTlViImRUvHJUNFFB1ZMTG3CYOUo7hEr_x8T3kY4kebs4D3GcxdnymKYKpkaP4bZxHNXuj-DnstZ_ZG4SJYJ4iCt0SLnp8nDgr0h2ltQSl5TUjgPMTqm-9J45C3vwffS6PhRTztvOHXQw4Yv41TDVu4tbfjGTdBYOFyPTvEl-T20fD9oSVB-A3y5lTbEVdCG0K_Kj2Iajv8AJvEbOw)

### UDP Implementation
- **Description**: The UDP implementation consists of three threads:
  1. **process_commands thread**: This thread is responsible for constructing messages based on user commands and adding them to the message queue. Messages are only added to the queue when they are fully constructed and ready to be sent.
  2. **send_message_thread**: This thread manages the sending of messages to the server. It handles retransmissions with using the UDP timeout mechanism. Messages can only be sent if a confirmation for the previous message has been received. If no confirmation is received, the thread sends a BYE message to the server and terminates the program.
  3. **receive_messages thread**: This thread continuously receives messages from the server. It checks the type of each received message and acts accordingly. Additionally, it removes messages from the queue after receiving confirmation from the server.
- **PendingMessage Struct**: The PendingMessage struct represents the message queue. It contains the message content, length of the message, and message ID. This struct ensures the correct sending of messages and handles confirmations for each message sent by the client.  **process_commands thread** Is adding messages in Queue
**receive_messages thread** Is deleting messages from Queue

### UDP Simple Representation in Diagram
[![](https://mermaid.ink/img/pako:eNqFVMtuGzEM_BVBp3XrHHrpwdcERXsIUCDtzYBASPRahR4LimvUDfzvZfbheB9JfBG9nOVQM-Q-a5sd6p22AUp58FATxH1S8uueqEfwST33T5T6bHNKaNlwNgXphGRa11RD2GTirRr-gHOEpWyVAAz7iLmVZIS_hpAJUom-FJ9T2Sif-ErgU2EIwRRfJwjmCMkFpFJt1Cl718Mu_XHb5lMH_z6gbxqWOlJ-rDMpI1nMh9XUCsOvIyG4b22y_NL1DQWhRX9CE-W6UOPY66croKFsJWdsjlHIloCCyY2vG-6IppiVfh57-M--tk_1TUcj4WCF-NBIy1iNwVaNkQmYxDKynXkzdbq2xPGDpwgvtx57rIbzx8PaG0iU6Qp1vjQBziZBFN7xklKVMXH5QPTf7IPn85rq8WwONXKpCpPcwP-T6hIixI2yR6BXdWtMSMBXg4x_UbeVsfjy1fCb3Pe9W8NM3TC3w8iMflbDOdOiNZ0a0PLxKsa7yD_ZvyosV5BNCwuFB-zMiPtezgW2oW74MTSLDeqDbr3v7mb7M83NJn-aXIzhND03cJqdStzn5nv2DsdiA94knHm5XlNvdUSZdO_ke9jZvdd8xIh7vZPQ4QHawHu9TxeBiq_56Zys3jG1uNVt42TGhi-o3h0gFLz8B0gb0UM?type=png)](https://mermaid.live/edit#pako:eNqFVMtuGzEM_BVBp3XrHHrpwdcERXsIUCDtzYBASPRahR4LimvUDfzvZfbheB9JfBG9nOVQM-Q-a5sd6p22AUp58FATxH1S8uueqEfwST33T5T6bHNKaNlwNgXphGRa11RD2GTirRr-gHOEpWyVAAz7iLmVZIS_hpAJUom-FJ9T2Sif-ErgU2EIwRRfJwjmCMkFpFJt1Cl718Mu_XHb5lMH_z6gbxqWOlJ-rDMpI1nMh9XUCsOvIyG4b22y_NL1DQWhRX9CE-W6UOPY66croKFsJWdsjlHIloCCyY2vG-6IppiVfh57-M--tk_1TUcj4WCF-NBIy1iNwVaNkQmYxDKynXkzdbq2xPGDpwgvtx57rIbzx8PaG0iU6Qp1vjQBziZBFN7xklKVMXH5QPTf7IPn85rq8WwONXKpCpPcwP-T6hIixI2yR6BXdWtMSMBXg4x_UbeVsfjy1fCb3Pe9W8NM3TC3w8iMflbDOdOiNZ0a0PLxKsa7yD_ZvyosV5BNCwuFB-zMiPtezgW2oW74MTSLDeqDbr3v7mb7M83NJn-aXIzhND03cJqdStzn5nv2DsdiA94knHm5XlNvdUSZdO_ke9jZvdd8xIh7vZPQ4QHawHu9TxeBiq_56Zys3jG1uNVt42TGhi-o3h0gFLz8B0gb0UM)


## Testing
### Manual Testing: Local Server Connection

#### Overview
The initial testing phase involved connecting to a local server and manually interacting with the client application via the terminal. This testing approach helped identify basic issues with the program and assess its functionality in handling different scenarios.

#### Testing Environment
- **Operating System:** Ubuntu
- **Terminal:** Visual Studio Code

#### Test Cases and Results

1. **Authentication Command (/auth)**
   - **Input:** `/auth a a a`
   - **Server Output:** `AUTH a AS a USING a`
   - **Description:** The client successfully sends an authentication command to the server with the provided username, secret, and display name. The server responds with an authentication message.

2. **General Message Sending**
   - **Input:** `hi!`
   - **Server Output:** `MSG FROM a IS hi!`
   - **Client Output:** `a: hi!`
   - **Description:** The client sends a general message to the server. The server acknowledges receipt of the message and echoes it back to the client. The client correctly displays the sender's username along with the received message.

#### Conclusion
The manual testing process demonstrated the client's ability to connect to a local server, send commands, and receive responses. Basic functionality such as authentication and message sending was successfully validated, providing confidence in the client's core features. Further testing will explore additional scenarios and edge cases to ensure comprehensive coverage.

### File Input Testing: Local Server Connection

#### Overview
In addition to manual testing, the client application was tested using file input to simulate user interaction. This approach involved providing input commands and messages via a file, enabling automated testing of the client's functionality.

#### Testing Environment
- **Operating System:** Ubuntu
- **Terminal:** Visual Studio Code

#### Test Setup
- **Input File:** `test_input.txt`
- **Contents:**:    
`/auth b b b`
`hi`
`/rename peremoga`
`hi!`




#### Test Cases and Results

1. **Authentication Command (/auth)**
 - **Input:** `/auth b b b`
 - **Server Output:** `AUTH b AS b USING b`
 - **Description:** The client reads the `/auth` command and its parameters from the input file. It successfully sends the authentication command to the server with the provided username, secret, and display name. The server responds with an authentication message.

2. **General Message Sending**
 - **Input:** `hi`
 - **Server Output:** `MSG FROM b IS hi`
 - **Client Output:** `b: hi`
 - **Description:** The client reads the message content from the input file and sends it to the server. The server acknowledges receipt of the message and echoes it back to the client. The client correctly displays the sender's username along with the received message.

3. **Renaming Command (/rename)**
   - **Input:** `/rename peremoga`
   - **Client Output:** 
     - The client attempts to read the `/rename` command and change the display name of the user to `peremoga`.

4. **General Message Sending**
   - **Input:** `hi!`
   - **Server Output:** `MSG FROM peremoga IS hi`
   - **Client Output:** `peremoga: hi!`
   - **Description:** The client sends a message to the server with a different display name, which was successfully changed.

#### Conclusion
File input testing provided a means to automate client interaction and validate its behavior under predefined scenarios. The client successfully processed commands and messages from the input file, demonstrating its ability to communicate with the server effectively. This testing approach enhances repeatability and scalability, enabling more extensive testing of the client's functionality across various use cases.

### Automated Testing: Edge Cases

#### Overview
In addition to manual and file input testing, the client application underwent automated testing to verify its behavior under various edge cases. This testing focused on scenarios such as display names exceeding the maximum length and message contents surpassing the maximum allowed length.

#### Testing Environment
- **Operating System:** Ubuntu
- **Testing Framework:** pytest
- **Test Cases:** Automated tests were designed to cover edge cases related to display names and message contents.

#### Test Cases and Results

1. **Display Name Length Exceeding Maximum**
   - **Input:** A display name with more than 20 characters.
   - **Expected Behavior:** The client should handle the input gracefully, truncate the display name to the maximum length, and display an appropriate error message.
   - **Actual Behavior:** The client correctly truncated the display name to the maximum allowed length and displayed an error message indicating the truncation.

2. **Message Content Length Exceeding Maximum**
   - **Input:** A message with more than 1400 characters.
   - **Expected Behavior:** The client should detect the excessive message length, truncate the message to the maximum allowed length, and send the truncated message to the server.
   - **Actual Behavior:** The client successfully detected the excessive message length, truncated the message to the maximum allowed length, and sent the truncated message to the server without errors.

#### Conclusion
Automated testing of edge cases ensured that the client application could handle unexpected inputs and boundary conditions effectively. By verifying the behavior under extreme scenarios, the automated tests enhanced the robustness and reliability of the client, contributing to its overall quality and stability.


### Bibliography


*1. Linux man pages: [https://man7.org/linux/man-pages/](https://man7.org/linux/man-pages/)
2. Python Documentation: [https://docs.python.org/](https://docs.python.org/)
3. Ubuntu Documentation: [https://help.ubuntu.com/](https://help.ubuntu.com/)
4. Visual Studio Code Documentation: [https://code.visualstudio.com/docs](https://code.visualstudio.com/docs)
5. pytest Documentation: [https://docs.pytest.org/en/stable/](https://docs.pytest.org/en/stable/)
6.  C Programming: [https://en.cppreference.com/w/c/thread](https://en.cppreference.com/w/c/thread)*
