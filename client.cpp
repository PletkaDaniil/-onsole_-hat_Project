#include <iostream>
#include <chrono>
#include <string>
#include <thread>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <sys/socket.h>
#include <termios.h>
#include <unistd.h>
#include <fstream>
#include "queue.h" //special queue for clients to write messages

int clientSocket;

//size of a all buffers
int BufferSize;

//Entered IP address
const char* Ip_Address;

void readConfig_File() {
    std::ifstream config_data_reading("client_config.cfg");

    if (!config_data_reading.is_open()) {
        std::cerr << "Problems with config.cfg" << std::endl;
        return;
    }

    std::string line;
    while (std::getline(config_data_reading, line)) {
        // ignoring the lines with comments
        if (line.find("//") != std::string::npos) {
            continue;
        }

        // looking for "=" in the line
        size_t equalPos = line.find("=");
        if (equalPos != std::string::npos) {
            // get the variable name and its value
            std::string variableName = line.substr(0, equalPos);
            std::string valueStr = line.substr(equalPos + 1);

            // remove the spaces at the beginning and at the end
            variableName.erase(0, variableName.find_first_not_of(" \t\r\n"));
            variableName.erase(variableName.find_last_not_of(" \t\r\n") + 1);
            valueStr.erase(0, valueStr.find_first_not_of(" \t\r\n"));
            valueStr.erase(valueStr.find_last_not_of(" \t\r\n") + 1);

            // assign a value to a variable
            if (variableName == "Ip_Address") {
                Ip_Address = valueStr.c_str();
            } else if (variableName == "BufferSize") {
                BufferSize = std::stoi(valueStr);
            }
        }
    }

    config_data_reading.close();
}


// function to enable character input mode
void enableMode() {
    struct termios term;
    tcgetattr(STDIN_FILENO, &term);
    term.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &term);
}

// function to restore terminal mode
void disableMode() {
    struct termios term;
    tcgetattr(STDIN_FILENO, &term);
    term.c_lflag |= (ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &term);
}

std::string sign_in_password(){
    enableMode(); //enabling character input mode
    std::string password;
    char elem;
    while (true) {
        std::cin.get(elem);
        if (elem == '\n') {
            break;
        }else if (static_cast<int>(elem) == 8 || static_cast<int>(elem) == 127) { // 8 - Backspace 127 - Delete
            if (!password.empty()) {
                password.pop_back();
                std::cout << "\b \b"; // Remove characters from terminal
            }
        }else{
            password += elem;
            std::cout << '*'; // display an asterisk on the screen
        }
    }
    std::cout << std::endl;
    disableMode(); // restoring the terminal mode
    return password;
}


void *receiveThread(void *arg) {
    char buffer[BufferSize];
    while (true) {
        //get information from the server
        memset(buffer, 0, sizeof(buffer));
        int bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesRead < 0) {
            std::cerr << "Receive failed for Server" << std::endl;
            break;
        } else if (bytesRead == 0) {
            std::cout << "Сhat room has been discontinued" << std::endl;
            break;
        } else {
            std::string receivedData(buffer);
            size_t newlinePos = receivedData.find('\n');

            //output all valid messages to the chat room
            if (newlinePos != std::string::npos) {
                std::cout << receivedData;
            } else {
                std::cout << receivedData << std::endl;
            }
        }
    }

    // end the thread
    pthread_exit(NULL);
}

//validate messages
void Line_Checking(int clientSocket, std::string message){
    //special comand (can be more special comands)
    if (message.find("/CHANGE_LOGIN: ") == 0) {
        //special message to the server if the client wants to change his login
        send(clientSocket, message.c_str(), message.size(), 0);
        return;
    }

    bool correctness_of_spelling = true;
    int size_of_a_message = message.length();

    //checking the entered message
    for (int index = 0; index < size_of_a_message; ++index){
        int code_of_elem = static_cast<int>(message[index]);
        //checking codes of letters (>=32  ->  correct)
        if (code_of_elem >= 32){continue;}
        else{
            correctness_of_spelling = false;
            break;
        }
    }

    //checking last element in the message
    if (message[size_of_a_message - 1] != '.' &&
        message[size_of_a_message - 1] != '!' && 
        message[size_of_a_message - 1] != '?'){
            correctness_of_spelling = false;
    }

    //send messages to the server
    if (message.length() > 64){
        std::string error = "Sorry but: (\"Error: The message is too long\") ";
        send(clientSocket, error.c_str(), error.size(), 0);
    }else if (!correctness_of_spelling){
        std::string error = "Sorry but: (\"Error: Misspelled message\")";
        send(clientSocket, error.c_str(), error.size(), 0);
    }else{
        send(clientSocket, message.c_str(), message.size(), 0);
    }
}

int main() {

    readConfig_File();

    struct sockaddr_in serverAddr;

    // create a socket
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // set up the server address structure
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(2023);
    serverAddr.sin_addr.s_addr = inet_addr(Ip_Address);  // Your server's IP

    // connect to the server
    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    std::string registration;
    std::cout << "Are you already registered? (answer: yes or no)" << std::endl;
    std::cout << std::endl;

    //check if client has already registered or not
    while (registration != "yes" && registration != "no"){
        std::cout << "-";
        std::getline(std::cin, registration);
        if (registration != "yes" && registration != "no"){
            std::cout << std::endl;
            std::cout << "Please, be careful with your answer. Try again."<< std::endl;
            std::cout << std::endl;
        }
    }
    
    //send "yes" or "no" message to the server
    send(clientSocket, registration.c_str(), registration.size(), 0);

    //allow the client to register in the chat room
    if (registration == "no"){
        std::cout << std::endl << "Registration part:" << std::endl;

        std::string new_login;
        std::cout << "NEW_LOGIN:  ";
        std::getline(std::cin, new_login);
        send(clientSocket, new_login.c_str(), new_login.size(), 0);

        std::cout << "NEW_PASSWORD:  ";
        std::string new_password1 = sign_in_password();
        int password_verification = 0;
         while(true){
            std::cout << "--Please, confirm your password: ";
            std::string new_password2 = sign_in_password();
            if (new_password1 == new_password2){
                send(clientSocket, new_password1.c_str(), new_password1.size(), 0);
                break;
            }else{
                if (password_verification > 1){
                    close(clientSocket);
                    return 0;
                }
                std::cout << "⟳ Please, try again: " << std::endl;
                ++password_verification;
            }
        }
    }
    
    char buffer[BufferSize]; //to get messages from the server
    int num_of_tryings = 0; //the client must log on in 3 tries

    std::cout << std::endl;
    std::cout << "ꕤ「 Please log in 」ꕤ" << std::endl;
    std::cout << std::endl;


    while (true){
        // Send the login
        std::string login;
        std::cout << "LOGIN:  ";
        std::getline(std::cin, login);
        send(clientSocket, login.c_str(), login.size(), 0);

        // Send the password
        std::cout << "PASSWORD:  ";
        std::string password = sign_in_password();
        send(clientSocket, password.c_str(), password.size(), 0);

        memset(buffer, 0, sizeof(buffer));
        int bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
        std::string write(buffer); 
        if (bytesRead <= 0 || write == "The_data_entered_was_correct") {
            break;
        }else if (write == "Such a client is already in the chat room!"){
            std::cout << std::endl;
            std::cout << "✘ " << write << std::endl;
            close(clientSocket);
            return 0;
        }else{

            if (num_of_tryings > 1){ //client used all 3 tries
                close(clientSocket);
                return 0;
            }

            //if login or password uncorrect
            std::cout << std::endl;
            std::cout << "▷  Please, try again ◁" << std::endl;
            std::cout << std::endl;
            ++num_of_tryings;
        }
    }


    sleep(1); //pre-commencement preparation

    std::cout  << std::endl;
    std::cout << "‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾" << std::endl;
    std::cout << "Welcome to this chat room!" << std::endl;
    std::cout << "__________________________" << std::endl;
    std::cout  << std::endl;

    //Receive and send data for other clients
    while (true) {
        //To check any information from the server
        pthread_t thread;
        if (pthread_create(&thread, NULL, receiveThread, NULL) != 0) {
            perror("Thread creation failed");
            exit(EXIT_FAILURE);
        }
        //send messages to the other clients in the chat room
        Queue<std::string> message_flow;
        std::thread input_thread([&message_flow]() {
            while (true) {
                std::string input;
                std::getline(std::cin, input);
                message_flow.push(input);
            }
        });
        auto start = std::chrono::high_resolution_clock::now();
        bool away = true;
        while(true){ 
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            if (!message_flow.empty()) {
                start = std::chrono::high_resolution_clock::now();
                std::string message = message_flow.pop();
                Line_Checking(clientSocket, message);
            }
        }
        input_thread.join();
        pthread_join(thread, NULL);
    }

    // Close the client's socket
    close(clientSocket);

    return 0;
}
