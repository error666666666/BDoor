#pragma once
#include <string> 
#include <vector>
#include <list>
#include <regex>
#include <thread>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <other/base64.h>
#include <atlimage.h>
#include <fstream>
#define ASCII " !\"#$%&,()*+'-./0123456789:;<=>?@[\\]^_{|}`abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define CMD_RETURN_TXT_NAME "cmd_return.txt"
#define CMDERROR "cmd����ִ��ʧ��"
#define DEBUG_MODE true
using namespace std;

void DebugLog(string str) {
	if (DEBUG_MODE) {
		cout << "DebugLog:" << str << endl;
	}
}
//�ָ��ַ��� 
void split_string(const string str, const string splits, vector<string>& res)
{
	if (str == ""){return;}
	//���ַ���ĩβҲ����ָ����������ȡ���һ��
	string strs = str + splits;
	size_t pos = strs.find(splits);
	int step = splits.size();
 
	// ���Ҳ����������ַ��������������� npos
	while (pos != strs.npos)
	{
		string temp = strs.substr(0, pos);
		res.push_back(temp);
		//ȥ���ѷָ���ַ���,��ʣ�µ��ַ����н��зָ�
		strs = strs.substr(pos + step, strs.size());
		pos = strs.find(splits);
	}
}

void split_string(const string str, const string splits, list<string>& res)
{
	if (str == ""){return;}
	//���ַ���ĩβҲ����ָ����������ȡ���һ��
	string strs = str + splits;
	size_t pos = strs.find(splits);
	int step = splits.size();
 
	// ���Ҳ����������ַ��������������� npos
	while (pos != strs.npos)
	{
		string temp = strs.substr(0, pos);
		res.push_back(temp);
		//ȥ���ѷָ���ַ���,��ʣ�µ��ַ����н��зָ�
		strs = strs.substr(pos + step, strs.size());
		pos = strs.find(splits);
	}
}
int SplitStringInLen(string str, int len, vector<string>& vec) {
	for (int i = 0; i < str.size(); i++) {
		string substr;
		if (str.size() - i >= len) {
			substr = str.substr(i, len);
			i += len - 1;
		}
		else {
			substr = str.substr(i, str.size() - i);
			i += str.size() - 1;
		}
		vec.push_back(substr);
	}
	return 1;
}

template <typename T>
T list_at(list<T>& res,int index){
	int i = 0;
	for(auto e : res){
		if(i == index){
			return e;
		}else{
			i++;
		}
	}
}

//��������� 
int getRand(int min, int max) {
    return ( rand() % (max - min + 1) ) + min ;
}

bool check_ip(string ip){
	
	regex ip_pattern("((25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[0-9])\\.){3}(25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[0-9])");
	smatch s;
	if(regex_match(ip,s,ip_pattern)){
		return true;
	}else{
		return false;
	}
						
}

bool operator==(const sockaddr_in& addr1, const sockaddr_in& addr2) {

	return addr1.sin_family == addr2.sin_family && addr1.sin_port == addr2.sin_port && addr1.sin_addr.s_addr == addr2.sin_addr.s_addr;

}
int bindport(SOCKET s, const struct sockaddr* name, int namelen) {
	int ret = ::bind(s, name, namelen);
	return ret;
}

sockaddr_in CreateSockAddrIn(int port) {
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	return addr;
}
sockaddr_in CreateSockAddrIn(string ip,int port) {
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr(ip.data());
	return addr;
}
int indexOfCode(const char c) {
	const char* code = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	for (unsigned int i = 0; i < 64; i++) {
		if (code[i] == c)
			return i;
	}
	return 0;
}
string splicing_string(vector<string> &vec_str,const string splits) {
	string str;
	for (int i = 0; i < vec_str.size(); i++) {
		str += vec_str[i];
		if (i != vec_str.size() - 1) { str += splits; }
	}
	return str;
}
template<class T>
int VectorFind(vector<T> vec,T t){
	for (int i = 0; i < vec.size(); i++) {
		if (vec[i] == t) { return i; }
	}
	return NULL;
}
template<class T>
int lengtharr(T t[]) {
	return sizeof(t)/ sizeof(T);
}
int run_cmd_with_txt(string cmd_command, string& mess) {
	fstream fst;
	fst.open(CMD_RETURN_TXT_NAME, ios::trunc | ios::out);
	if (!fst.is_open()) {
		DebugLog("�ļ���ʧ��");
		return -1;
	}
	else { DebugLog("�ļ��򿪳ɹ�"); }
	fst.close();
	cmd_command += string(" >> ") + CMD_RETURN_TXT_NAME;
	if (system(cmd_command.data()) == -1) {
		mess = CMDERROR;
		DebugLog("cmd����ִ��ʧ��");
		return -1;
	}
	ifstream ifs;
	ifs.open(CMD_RETURN_TXT_NAME, ios::in);
	string remess = "\n";
	string buf;
	while (getline(ifs, buf)) { remess += buf + string("\n"); }
	mess = remess;
	ifs.close();
	if (remove(string(CMD_RETURN_TXT_NAME).data()) == 0) { DebugLog("�ɹ�ɾ���ļ�"); }
	else {
		DebugLog("�޷�ɾ���ļ�");
		return -1;
	}
	return 1;
}
vector<string> image_format = { "bmp","emf","wmf","jpeg","png","gif","tiff","exif","icon","heif","webp" };
vector<GUID> GUID_vec = { Gdiplus::ImageFormatBMP,Gdiplus::ImageFormatEMF ,Gdiplus::ImageFormatWMF,
Gdiplus::ImageFormatJPEG,Gdiplus::ImageFormatPNG,Gdiplus::ImageFormatGIF,Gdiplus::ImageFormatTIFF,Gdiplus::ImageFormatEXIF,
Gdiplus::ImageFormatIcon,Gdiplus::ImageFormatHEIF,Gdiplus::ImageFormatWEBP };
bool GetScreenShot(string filepath) {
	HDC hdcSrc = GetDC(NULL);
	int nBitPerPixel = GetDeviceCaps(hdcSrc, BITSPIXEL);
	int nWidth = GetDeviceCaps(hdcSrc, HORZRES);
	int nHeight = GetDeviceCaps(hdcSrc, VERTRES);
	CImage image;
	image.Create(nWidth, nHeight, nBitPerPixel);
	BitBlt(image.GetDC(), 0, 0, nWidth, nHeight, hdcSrc, 0, 0, SRCCOPY);
	ReleaseDC(NULL, hdcSrc);
	image.ReleaseDC();
	vector<string> filepath_spl;
	split_string(filepath, "\\", filepath_spl);
	vector<string>filename_spl;
	split_string(filepath_spl.back(), ".", filename_spl);
	int format_pos = VectorFind(image_format, filename_spl.back());
	image.Save((LPCTSTR)CString(filepath_spl.back().c_str()), GUID_vec[format_pos]);
	return true;
}
bool GetScreenShot(string format, IStream *ist) {
	HDC hdcSrc = GetDC(NULL);
	int nBitPerPixel = GetDeviceCaps(hdcSrc, BITSPIXEL);
	int nWidth = GetDeviceCaps(hdcSrc, HORZRES);
	int nHeight = GetDeviceCaps(hdcSrc, VERTRES);
	CImage image;
	image.Create(nWidth, nHeight, nBitPerPixel);
	BitBlt(image.GetDC(), 0, 0, nWidth, nHeight, hdcSrc, 0, 0, SRCCOPY);
	ReleaseDC(NULL, hdcSrc);
	image.ReleaseDC();
	int format_pos = VectorFind(image_format, format);
	image.Save(ist, GUID_vec[format_pos]);
	return true;
}