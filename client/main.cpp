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

	// Winsock �ʱ�ȭ
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	// socket()
	clientSocket = socket(AF_INET, SOCK_STREAM, 0);

	// connect()
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(12345);
	inet_pton(AF_INET, "192.168.0.215", &serverAddr.sin_addr); // ���� �ּ� ����

	// ���� ����
	connect(clientSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
	cout << "Connected to server." << endl;

	while (true) {
		cout << "\n======== �ʱ�ȭ�� ========\n";
		cout << "1. ȸ������\n2. �α���\n3. ����\n> ";
		int choice;
		cin >> choice;
		cin.ignore();

		// send()/recv()
		// ȸ������
		if (choice == 1) {
			string username, password;
			cout << "\n======== ȸ������ ========\n";
			cout << "���̵� : ";
			getline(cin, username);
			cout << "��й�ȣ : ";
			getline(cin, password);

			string msg = "REGISTER:" + username + ":" + password + ":";
			sendMessage(clientSocket, msg);
			string res = receiveMessage(clientSocket);
			cout << "[����] " << res << endl;
		}
		// �α���
		else if (choice == 2) {
			string username, password;
			cout << "\n========== �α��� ==========\n";
			cout << "���̵� : ";
			getline(cin, username);
			cout << "��й�ȣ : ";
			getline(cin, password);

			string message = "LOGIN:" + username + ":" + password + ":";
			sendMessage(clientSocket, message);
			string res = receiveMessage(clientSocket);
			cout << "[����] " << res << endl;

			if (res == "Login Success") {
				while (true) {
					cout << "\n======= �α��� �� �޴� =======\n";
					cout << "1. ä���ϱ�\n2. �α׾ƿ�\n3. ����\n> ";
					int subMenu;
					cin >> subMenu;
					cin.ignore();

					if (subMenu == 1) {
						cout << "\n[ä�� ���] 'exit' �Է� �� ä�� ����\n";
						while (true) {
							string chatMsg;
							cout << ">> ";
							getline(cin, chatMsg);

							if (chatMsg == "exit") break;

							string chatSend = "CHAT:" + chatMsg;
							sendMessage(clientSocket, chatSend);
							string chatReply = receiveMessage(clientSocket);
							cout << "[����] " << chatReply << endl;
						}
					}
					else if (subMenu == 2) {
						cout << "[�α׾ƿ� �Ǿ����ϴ�]\n";
						break;
					}
					else {
						cout << "[���α׷��� �����մϴ�]\n";
						closesocket(clientSocket);
						WSACleanup();
						return 0;
					}
				}
			}
			else {
				cerr << "[���� ����] �α��� ����: �ٽ� �õ����ּ���." << endl;
			}
		}
		else if (choice == 3) {
			cout << "[���α׷��� �����մϴ�]\n";
			closesocket(clientSocket);
			WSACleanup();
			return 0;
		}
		else {
			cout << "�߸��� �Է��Դϴ�. �ٽ� �������ּ���.\n";
		}
	}

	return 0;
}