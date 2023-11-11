# Simple Console Chat Room

An example of a simple chat room for communication between multiple users.

## Table of Contents

- [Overview](#overview)
- [How it Works](#how-it-works)
- [Features](#features)
- [Usage](#usage)

## Overview

This project is a simple console chat room application that allows multiple users to communicate in real-time. Whether you want to chat with friends, family, or colleagues.

## How it Works

1. ***Registration:*** Users are provided with a unique client to register in the chat room. If they have already registered, they can simply log in.

2. ***Authorization:*** After registration, users are authorized and gain access to the chat room.

3. ***Message History:*** The chat displays the last 'n' (You can choose how many you want. Use server_config file for this) messages, including those sent before the user's authorization. This helps users catch up on the conversation.

4. ***Change Username:*** Users have the flexibility to change their login names. Other users will see the updates in real-time. (write

    "/CHANGE_LOGIN: your_new_name" in the console)

5. ***Message Verification:*** To maintain a clean and orderly chat, each message must end with one of the following characters: '.', '!', or '?'. Messages are also limited to 64 characters with ASCII codes greater than or equal to 32. (if there was an error in the message you will receive a message about the nature of the error)

6. ***Save everything that happens in chat***: The server will display all the history(error messages and time when it happened) of the chat in the log.txt file.

7. ***Work with configuration files***: You can change this in the configuration file depending on your needs

8. ***More Features:*** We are continuously working on adding more features to the chat room for enhanced convenience and functionality.

## Features

- Real-time chat with multiple users
- Message history display
- User-friendly username change
- Saving all chat history in exact time

## Usage

1. Register or log in to the chat room.
2. Start chatting with other users in real-time.
3. Enjoy the features and convenience of our simple chat room.
