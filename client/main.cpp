#include <iostream>
#include <winsock2.h>
#include <vector>
#include <ws2tcpip.h>
#include <string>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

void sendMessage(SOCKET sock, const string& msg) {
	send(sock, msg.c_str(), msg.size(), 0);
}

string receiveMessage(SOCKET sock) {
	vector<char> buffer(1024);
	int recvLen = recv(sock, buffer.data(), buffer.size(), 0);
	if (recvLen > 0) return string(buffer.data(), recvLen);
	return "Server Error";
}

int main() {
	WSADATA wsaData;
	SOCKET clientSocket;
	SOCKADDR_IN serverAddr;
	vector<char> buffer(1024);

	// Winsock 초기화
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	// socket()
	clientSocket = socket(AF_INET, SOCK_STREAM, 0);

	// connect()
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(12345);
	inet_pton(AF_INET, "192.168.0.215", &serverAddr.sin_addr); // 서버 주소 설정

	// 서버 연결
	connect(clientSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
	cout << "Connected to server." << endl;

	while (true) {
		cout << "\n======== 초기화면 ========\n";
		cout << "1. 회원가입\n2. 로그인\n3. 종료\n> ";
		int choice;
		cin >> choice;
		cin.ignore();

		// send()/recv()
		// 회원가입
		if (choice == 1) {
			string username, password;
			cout << "\n======== 회원가입 ========\n";
			cout << "아이디 : ";
			getline(cin, username);
			cout << "비밀번호 : ";
			getline(cin, password);

			string msg = "REGISTER:" + username + ":" + password + ":";
			sendMessage(clientSocket, msg);
			string res = receiveMessage(clientSocket);
			cout << "[서버] " << res << endl;
		}
		// 로그인
		else if (choice == 2) {
			string username, password;
			cout << "\n========== 로그인 ==========\n";
			cout << "아이디 : ";
			getline(cin, username);
			cout << "비밀번호 : ";
			getline(cin, password);

			string message = "LOGIN:" + username + ":" + password + ":";
			sendMessage(clientSocket, message);
			string res = receiveMessage(clientSocket);
			cout << "[서버] " << res << endl;

			if (res == "Login Success") {
				while (true) {
					cout << "\n======= 로그인 후 메뉴 =======\n";
					cout << "1. 채팅하기\n2. 로그아웃\n3. 종료\n> ";
					int subMenu;
					cin >> subMenu;
					cin.ignore();

					if (subMenu == 1) {
						cout << "\n[채팅 모드] 'exit' 입력 시 채팅 종료\n";
						while (true) {
							string chatMsg;
							cout << ">> ";
							getline(cin, chatMsg);

							if (chatMsg == "exit") break;

							string chatSend = "CHAT:" + chatMsg;
							sendMessage(clientSocket, chatSend);
							string chatReply = receiveMessage(clientSocket);
							cout << "[서버] " << chatReply << endl;
						}
					}
					else if (subMenu == 2) {
						cout << "[로그아웃 되었습니다]\n";
						break;
					}
					else {
						cout << "[프로그램을 종료합니다]\n";
						closesocket(clientSocket);
						WSACleanup();
						return 0;
					}
				}
			}
			else {
				cerr << "[서버 응답] 로그인 실패: 다시 시도해주세요." << endl;
			}
		}
		else if (choice == 3) {
			cout << "[프로그램을 종료합니다]\n";
			closesocket(clientSocket);
			WSACleanup();
			return 0;
		}
		else {
			cout << "잘못된 입력입니다. 다시 선택해주세요.\n";
		}
	}

	return 0;
}