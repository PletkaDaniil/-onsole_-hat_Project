#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <thread>
#include <mutex>
#include <vector>
#include <stdio.h>
#include <time.h>
#include <map>
#include <algorithm>
#include <fstream>

std::ofstream log_file("log.txt"); //make a log file for checking errors and other processes

struct Client {
    int clientSocket;
    int clientId;
    int number;     //number of the index in the list of logins
};

// list of clients logins
std::vector<std::string> logins;
// list of clients passwords
std::vector<std::string> passwords;
// list to prevent a logged-in client from logging in again
std::map<std::string, bool> attendance;

// list of connected clients
std::vector<Client> clients;
// chat history to store the last 10 messages
std::vector<std::string> chatHistory;

// Get current date/time, format is DD-MM-YYYY.HH:mm:ss
const std::string currentDateTime() {
    time_t now = time(0);
    struct tm tstruct;
    char buf[50];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%d-%m-%Y -- %X", &tstruct);
    std::string ans(buf);
    return ans;
}

void saveChangesIntoFile(const std::string& login_before, const std::string& new_login){ //TODO
    std::ifstream inputFile("sensitive_data.txt");
    if (!inputFile.is_open()) {
        std::cerr << "Problems with sensitive_data.txt" << std::endl;
    }

    std::string fileContent((std::istreambuf_iterator<char>(inputFile)), std::istreambuf_iterator<char>());
    inputFile.close();

    size_t found = fileContent.find(login_before);
    if (found != std::string::npos) {
        fileContent.replace(found, login_before.length(), new_login);
    }
    inputFile.close();

    std::ofstream outputFile("sensitive_data.txt");
    if (!inputFile.is_open()) {
        std::cerr << "Problems with sensitive_data.txt" << std::endl;
    }
    outputFile << fileContent;
    outputFile.close();
}

void handleClients(int clientId) {
    char buffer[64];    //to get messages from the client

    Client clientInfo;  
    clientInfo = clients[clientId]; //find client with the help of the ID

    // send chat history to the client upon connection
    if (chatHistory.empty()){   //if there is no information to send
        std::string to_send = "--No messages before--";
        send(clientInfo.clientSocket, to_send.c_str(), to_send.size(), 0);
    }

    for (std::string to_send: chatHistory) {
        to_send = "<Chat before>  -- " + to_send + "\n";
        send(clientInfo.clientSocket, to_send.c_str(), to_send.size(), 0);
    }

    // Receive and send data for this client
    while (true) {

        memset(buffer, 0, sizeof(buffer));
        int bytesReceived = recv(clientInfo.clientSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived <= 0) { //checking for problems from the client
            if (bytesReceived == 0) {
                // Connection closed by the client
                std::string disconnection = "-" + logins[clientInfo.number] + " disconnected-";
                std::cout << disconnection << std::endl;
                log_file << currentDateTime() << "  --  " << disconnection << std::endl;

                //send to all clients if someone has already disconnected from the chat room
                for (Client cur: clients) {
                    if (cur.clientSocket != clientInfo.clientSocket){
                        send(cur.clientSocket, disconnection.c_str(), disconnection.size(), 0);
                    }
                }
            } else {
                // Error or connection lost
                std::cerr << "Receive failed for Client " << logins[clientInfo.number] << std::endl;
                log_file << currentDateTime() << "  --  " << "Receive failed for Client " << logins[clientInfo.number] << std::endl;
            }
            break;
        }

        std::string message(buffer);
        if (message.find("/CHANGE_LOGIN: ") == 0) {
            std::string client_new_login = message.substr(15);
            std::cout << client_new_login << "!!" << std::endl;
            // Trim leading and trailing whitespace
            client_new_login.erase(client_new_login.find_last_not_of(" \t\r\n") + 1);
            client_new_login.erase(0, client_new_login.find_first_not_of(" \t\r\n"));
            bool isPresent = true;
            for (int index = 0; index < logins.size(); ++index){
                if (logins[index] == client_new_login && index != clientInfo.number){
                    isPresent = false;
                    break;
                }
            }
            if (isPresent) {
                std::string str = "♮♮♮ " + logins[clientInfo.number] + " changed his login to " + client_new_login;
                saveChangesIntoFile(logins[clientInfo.number], client_new_login);
                logins[clientInfo.number] = client_new_login;
                send(clientInfo.clientSocket, "Success: Login changed", 25, 0);
                for (Client cur: clients) {
                    if (cur.clientSocket != clientInfo.clientSocket){
                        send(cur.clientSocket, str.c_str(), str.size(), 0);
                    }
                }

                // add the received message to the chat history
                chatHistory.push_back(str);
                if (chatHistory.size() > 10) {
                    chatHistory.erase(chatHistory.begin());
                }
            } else {
                send(clientInfo.clientSocket, "Error: Username already exists", 30, 0);
            }
        }else{
            std::string to_send = "> " + logins[clientInfo.number] + ": " + message;

            //checking for misspellings
            if (message == "Sorry but: (\"Error: Misspelled message\")" || message == "Sorry but: (\"Error: The message is too long\") "){
                
                send(clientInfo.clientSocket, message.c_str(), message.size(), 0);
                if (message == "Sorry but: (\"Error: Misspelled message\")"){
                    log_file << currentDateTime() << "  --  " << "Client_" << logins[clientInfo.number] << "_ran into an error: Misspelled message" << std::endl;
                }else{
                    log_file << currentDateTime() << "  --  " << "Client_" << logins[clientInfo.number] << "_ran into an error: The message is too long" << std::endl;
                }

            }else{
                log_file << currentDateTime() << "  --  " << "Client_" << logins[clientInfo.number] << "_wrote: " << message << std::endl;
                
                //dump all the messages that clients write into the chat room.
                for (Client cur: clients) {
                    if (cur.clientSocket != clientInfo.clientSocket ){
                        send(cur.clientSocket, to_send.c_str(), to_send.size(), 0);
                    }
                }

                // add the received message to the chat history
                chatHistory.push_back(to_send);
                if (chatHistory.size() > 10) {
                    chatHistory.erase(chatHistory.begin());
                }

            }
            
            std::cout << to_send << std::endl;
        }
    }
    attendance[logins[clientInfo.number]] = false;
    // Close the client socket
    close(clientInfo.clientSocket);
}



int main() {
    int serverSocket, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);

    //checjing for errors
    if (!log_file.is_open()) {
        std::cerr << "Can not open file log.txt" << std::endl;
        return 1;
    }

    // create a socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        log_file << currentDateTime() << "  --  " << "Socket creation failed" << std::endl;
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // set up the server address structure
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(2023);

    // bind the server socket
    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        log_file << currentDateTime() << "  --  " << "Binding failed" << std::endl;
        perror("Binding failed");
        exit(EXIT_FAILURE);
    }

    // listen for incoming connections and limit the queue of incoming connections.
    if (listen(serverSocket, 5) < 0) {
        log_file << currentDateTime() << "  --  " << "Listening failed" << std::endl;
        perror("Listening failed");
        exit(EXIT_FAILURE);
    }

    std::cout << "A server has been set up!" << std::endl;
    log_file << currentDateTime() << "  --  " << "A server has been set up!" << std::endl;

    char buffer[64]; //to get messages from the client

    int clientId = 0; //each client needs his/her own ID
    bool unsuccessfulAttempts; // needed to verify the client's authorization to the system

    while (true) {

        // Accept incoming connections
        clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);
        if (clientSocket < 0) {
            log_file << currentDateTime() << "  --  " << "Accepting connection failed" << std::endl;
            perror("Accepting connection failed");
            continue; // Continue listening for more connections
        }


        //registration part. If the client want to be in the chat he/she must authorize
        memset(buffer, 0, sizeof(buffer));
        int bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesRead <= 0) { //checking for problemss from the client
                continue;
        } else {
            std::string registration = static_cast<std::string>(buffer);
            // Сhecking the information from the client whether he/she is registered or not 
            if (registration == "no"){
                //open file for pre-recording
                std::ofstream writing_into_the_file("sensitive_data.txt", std::ios::app);

                //checking the servers file
                if (!writing_into_the_file.is_open()) {
                    std::cerr << "Problems with sensitive_data.txt" << std::endl;
                    return 1;
                }

                //let's memorize and write down the login
                memset(buffer, 0, sizeof(buffer));
                bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
                if (bytesRead <= 0) {
                    continue;
                }
                registration = static_cast<std::string>(buffer);
                writing_into_the_file << "\n";
                writing_into_the_file << registration;

                //let's memorize and write down the password for this client
                memset(buffer, 0, sizeof(buffer));
                bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
                if (bytesRead <= 0) {
                    continue;
                }
                registration = static_cast<std::string>(buffer);
                writing_into_the_file << "\n";
                writing_into_the_file << registration;

                writing_into_the_file.close(); //don't forget to close the file
            }
        }


        // checking all logins and passwordsd (in the file)
        std::ifstream reading_the_file("sensitive_data.txt"); //file for reading information
        //checking the file
        if (!reading_the_file.is_open()) {
            std::cerr << "Problems with sensitive_data.txt" << std::endl;
            return 1;
        }
        //on the first line goes login then on the second line client's password
        int number_of_lines = 0; //check all lines in the file
        std::string line_by_line;
        while(std::getline(reading_the_file, line_by_line)){

            // Trim leading and trailing whitespace
            line_by_line.erase(line_by_line.find_last_not_of(" \t\r\n") + 1);
            line_by_line.erase(0, line_by_line.find_first_not_of(" \t\r\n"));

            if (number_of_lines % 2 == 0){
                logins.push_back(line_by_line);
                if (attendance.count(line_by_line) == 0){
                    attendance[line_by_line] = false;
                }
            }else{
                passwords.push_back(line_by_line);
            }
            ++number_of_lines;
        }
        reading_the_file.close();//don't forget to close the file


        int client_index = 0;
        while (true){
            unsuccessfulAttempts = false; //so far we don't know if the client is logged in.   
            bool login_checking = false;
            bool reentry = false; //to find repeated client accounts

            memset(buffer, 0, sizeof(buffer));
            int bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
            if (bytesRead <= 0) { //checking for problems from the client
                std::string str = "Сlient used all attempts for authorization or disconnected";
                log_file << currentDateTime() << "  --  " << str << std::endl;
                unsuccessfulAttempts = true;     //something goes wrong so we are going to wait other clients
                break;
            } else {    //if everything is okey
                std::string cur_line(buffer);

                // Trim leading and trailing whitespace
                cur_line.erase(cur_line.find_last_not_of(" \t\r\n") + 1);
                cur_line.erase(0, cur_line.find_first_not_of(" \t\r\n"));
                
                //Go throw all logins. Here we try to find existing login
                for (int index = 0; index < number_of_lines; ++index){
                    if (logins[index] == cur_line){
                        login_checking = true;
                        if (attendance[logins[index]]){
                            reentry = true;
                        }else{
                            attendance[logins[index]] = true;
                        }
                        client_index = index;
                        break;
                    }
                }

            }

            bool password_checking = false;
            memset(buffer, 0, sizeof(buffer));
            bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
            if (bytesRead <= 0) { //checking for problems from the client
                std::string str = "Сlient used all attempts for authorization or disconnected";
                log_file << currentDateTime() << "  --  " << str << std::endl;
                unsuccessfulAttempts = true;    //something goes wrong so we are going to wait other clients
                break;
            } else {    //if everything is okey
                std::string cur_line(buffer);

                // Trim leading and trailing whitespace
                cur_line.erase(cur_line.find_last_not_of(" \t\r\n") + 1);
                cur_line.erase(0, cur_line.find_first_not_of(" \t\r\n"));

                //Here we try to check password for the client (we already know his/her login)
                if (passwords[client_index] == cur_line){
                    password_checking = true;
                }
            }
            if (reentry && password_checking){
                std::string okey_message = "Such a client is already in the chat room!";
                send(clientSocket, okey_message.c_str(), okey_message.size(), 0);
                unsuccessfulAttempts = true;
                break;
            }
            //checking the correctness of logins and passwords
            if (password_checking && login_checking){ //correct
                std::string okey_message = "The_data_entered_was_correct";
                send(clientSocket, okey_message.c_str(), okey_message.size(), 0);
                break;
            }else{                                    //uncorrect
                std::string error_message = "Incorrect login or password";
                send(clientSocket, error_message.c_str(), error_message.size(), 0);
            }
        }


        //something goes wrong so we are going to wait other clients
        if (unsuccessfulAttempts){
            continue;
        }

        //The client is logged in, which means he's / she's going to the chat room.
        std::string message = "+" + logins[client_index] + " connected+";
        std::cout << message << std::endl;
        log_file << currentDateTime() << "  --  " << message << std::endl;

        //send all clients the information that someone connected to the chat room
        for (Client cur: clients) {
            if (cur.clientSocket != clientSocket){
                send(cur.clientSocket, message.c_str(), message.size(), 0);
            }
        }
        //add client to the list of active clients
        clients.push_back(Client{clientSocket, clientId, client_index});

        // creating a new thread to handle this client
        std::thread clientThread(handleClients, clientId);
        clientThread.detach(); // detach the thread to run independently

        ++clientId; //change the ID for a new client who can enter the chat room
    }

    // Close the server socket
    close(serverSocket);

    //Close log file
    log_file.close();

    return 0;
}