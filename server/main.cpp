#include <iostream>
#include <string>
#include <vector>
#include <winsock2.h>
#include <mysql/jdbc.h>
#include <thread>

#pragma comment(lib, "ws2_32.lib")

const std::string DB_HOST = "tcp://127.0.0.1:3306";
const std::string DB_USER = "root";
std::string DB_PASS;
std::string DB_NAME;

void handleClient(SOCKET clientSocket) {
    std::vector<char> buffer(1024);
    int loggedInUserId = -1;

    while (true) {
        int recvLen = recv(clientSocket, buffer.data(), buffer.size(), 0);
        if (recvLen <= 0) {
            std::cout << "[Client disconnected]" << std::endl;
            closesocket(clientSocket);
            return;
        }

        std::string msg(buffer.data(), recvLen);
        std::cout << "[RECV] " << msg << std::endl;

        if (msg.rfind("REGISTER:", 0) == 0) {
            size_t pos1 = msg.find(":", 9);
            size_t pos2 = msg.find(":", pos1 + 1);

            if (pos1 == std::string::npos || pos2 == std::string::npos) {
                std::string error = "Invalid Format";
                send(clientSocket, error.c_str(), error.length(), 0);
                continue;
            }

            std::string username = msg.substr(9, pos1 - 9);
            std::string password = msg.substr(pos1 + 1, pos2 - pos1 - 1);

            try {
                sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
                std::unique_ptr<sql::Connection> conn(driver->connect(DB_HOST, DB_USER, DB_PASS));
                conn->setSchema(DB_NAME);

                std::unique_ptr<sql::PreparedStatement> checkStmt(
                    conn->prepareStatement("SELECT user_id FROM users WHERE username = ?")
                );
                checkStmt->setString(1, username);
                std::unique_ptr<sql::ResultSet> checkRes(checkStmt->executeQuery());

                if (checkRes->next()) {
                    std::string result = "ID already exists";
                    send(clientSocket, result.c_str(), result.length(), 0);
                }
                else {
                    std::unique_ptr<sql::PreparedStatement> insertStmt(
                        conn->prepareStatement("INSERT INTO users (username, password) VALUES (?, ?)")
                    );
                    insertStmt->setString(1, username);
                    insertStmt->setString(2, password);
                    insertStmt->execute();

                    std::string result = "Register Success";
                    send(clientSocket, result.c_str(), result.length(), 0);
                }
            }
            catch (sql::SQLException& e) {
                std::cerr << "MySQL Error: " << e.what() << std::endl;
                std::string err = "DB Error";
                send(clientSocket, err.c_str(), err.length(), 0);
            }
        }
        else if (msg.rfind("LOGIN:", 0) == 0) {
            size_t pos1 = msg.find(":", 6);
            size_t pos2 = msg.find(":", pos1 + 1);

            if (pos1 == std::string::npos || pos2 == std::string::npos) {
                std::string error = "Invalid Format";
                send(clientSocket, error.c_str(), error.length(), 0);
                continue;
            }

            std::string username = msg.substr(6, pos1 - 6);
            std::string password = msg.substr(pos1 + 1, pos2 - pos1 - 1);

            try {
                sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
                std::unique_ptr<sql::Connection> conn(driver->connect(DB_HOST, DB_USER, DB_PASS));
                conn->setSchema(DB_NAME);

                std::unique_ptr<sql::PreparedStatement> pstmt(
                    conn->prepareStatement("SELECT user_id FROM users WHERE username = ? AND password = ?")
                );
                pstmt->setString(1, username);
                pstmt->setString(2, password);
                std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

                if (res->next()) {
                    loggedInUserId = res->getInt("user_id");
                    std::string result = "Login Success";
                    send(clientSocket, result.c_str(), result.length(), 0);
                }
                else {
                    std::string result = "Login Failed";
                    send(clientSocket, result.c_str(), result.length(), 0);
                }
            }
            catch (sql::SQLException& e) {
                std::cerr << "MySQL Error: " << e.what() << std::endl;
                std::string err = "DB Error";
                send(clientSocket, err.c_str(), err.length(), 0);
            }
        }
        else if (msg.rfind("CHAT:", 0) == 0) {
            if (loggedInUserId == -1) {
                std::string err = "Not Logged In";
                send(clientSocket, err.c_str(), err.length(), 0);
            }
            else {
                std::string chatContent = msg.substr(5);

                try {
                    sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
                    std::unique_ptr<sql::Connection> conn(driver->connect(DB_HOST, DB_USER, DB_PASS));
                    conn->setSchema(DB_NAME);

                    std::unique_ptr<sql::PreparedStatement> pstmt(
                        conn->prepareStatement("INSERT INTO message_log (sender_id, content, sent_at) VALUES (?, ?, NOW())")
                    );
                    pstmt->setInt(1, loggedInUserId);
                    pstmt->setString(2, chatContent);
                    pstmt->execute();

                    std::string echo = "Echo: " + chatContent;
                    send(clientSocket, echo.c_str(), echo.length(), 0);
                }
                catch (sql::SQLException& e) {
                    std::cerr << "MySQL Error (CHAT): " << e.what() << std::endl;
                    std::string err = "Chat Save Error";
                    send(clientSocket, err.c_str(), err.length(), 0);
                }
            }
        }
        else if (msg == "/exit") {
            if (loggedInUserId != -1) {
                try {
                    sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
                    std::unique_ptr<sql::Connection> conn(driver->connect(DB_HOST, DB_USER, DB_PASS));
                    conn->setSchema(DB_NAME);

                    std::unique_ptr<sql::PreparedStatement> pstmt(
                        conn->prepareStatement("UPDATE user_sessions SET logout_time = NOW() WHERE user_id = ? ORDER BY session_id DESC LIMIT 1")
                    );
                    pstmt->setInt(1, loggedInUserId);
                    pstmt->execute();

                    std::cout << "[로그아웃 되었습니다.]" << std::endl;
                }
                catch (sql::SQLException& e) {
                    std::cerr << "MySQL Error (LOGOUT): " << e.what() << std::endl;
                }
            }

            std::string bye = "Bye!";
            send(clientSocket, bye.c_str(), bye.length(), 0);
            break;
        }
        else {
            std::string error = "Unknown Command";
            send(clientSocket, error.c_str(), error.length(), 0);
        }
    }

    closesocket(clientSocket);
}

int main() {
    std::cout << "Password: ";
    std::cin >> DB_PASS;
    std::cout << "DB Name: ";
    std::cin >> DB_NAME;
    std::cout << "[DB Connect]" << std::endl;

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        return 1;
    }

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Socket Failed" << std::endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(12345);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
    listen(serverSocket, SOMAXCONN);
    std::cout << "[Server Connected] Listen in 12345..." << std::endl;

    while (true) {
        SOCKET clientSocket = accept(serverSocket, NULL, NULL);
        if (clientSocket != INVALID_SOCKET) {
            std::thread(handleClient, clientSocket).detach();
        }
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}