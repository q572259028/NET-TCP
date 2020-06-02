#ifndef _TcpClient_hpp_
#define _TcpClient_hpp_
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <iostream>
#include <stdio.h>
#include <thread>
#include "../Server/MessageHeader.hpp"
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN		
#include <Windows.h>
#include <WinSock2.h>
#pragma comment(lib,"ws2_32.lib")
#else
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#define SOCKET int
#define INVALID_SOCKET (SOCKET)(-0)
#define SOCKET_ERROR           (-1)
#endif

using namespace std;
class TcpClient
{
public:
	TcpClient();
	virtual ~TcpClient();
	SOCKET getSock();
	//初始化socket
	int initSocket();
	//连接服务器
	int Connect(const char* ip, unsigned short port);
	//关闭socket
	void Close();
	int onRun();
	int isRun();
	//发送数据
	int sendData(DataHeader* head,int len);
	//接收数据
	int recvData();
	//响应网络消息
	int onNetMsg(DataHeader* head);


private:
	SOCKET _sock = INVALID_SOCKET;
	bool _isConnected = 0;
};


TcpClient::TcpClient()
{
};
TcpClient::~TcpClient()
{
	Close();
}
SOCKET TcpClient::getSock()
{
	return _sock;
}
int TcpClient::onNetMsg(DataHeader* head) {


	switch (head->cmd)
	{
	case CMD_NEW_USER_INFO:
	{
		NewUserInfo* newInfo = (NewUserInfo*)head;
		cout << "New client join <Socket = " << newInfo->sock << ">" << endl;

	}
	break;
	case CMD_LOGIN_RESULT:
	{
		LoginResult* login = (LoginResult*)head;
		cout << "Receive server message: CMD_LOGIN_RESULT,Data length: " << login->dataLength << " socket = " << _sock << endl;

	}
	break;
	case CMD_LOGINOUT_RESULT:
	{
		LoginOutResult* loginout = (LoginOutResult*)head;
		cout << "Receive server message: CMD_LOGINOUT_RESULT,Data length: " << loginout->dataLength << " socket = " << _sock << endl;

	}
	break;
	case CMD_ERROR:
	{

	}
	break;
	default:
		cout << "Server cmd is invalid " << "socket = " << _sock << endl;

	}
	return 0;
}
#ifndef MAX_BUFF_SIZE
#define RECV_BUFF_SIZE 10240
#endif
//一级接收缓存区

//二级接收缓存区
char _szMsgBuf[RECV_BUFF_SIZE * 5];
int _lastPos = 0;
int TcpClient::recvData()
{	
	char*  _szRecv = _szMsgBuf + _lastPos;
	int nLen = recv(_sock, _szRecv, RECV_BUFF_SIZE * 5 - _lastPos, 0);
	if (nLen <= 0) {
		cout << "recv is disconnect " << "socket = " << _sock << endl;
		return -1;
	}
//	memcpy(_szMsgBuf + _lastPos, _szRecv, nLen);
	_lastPos += nLen;
	while (_lastPos >= sizeof(DataHeader))
	{
		DataHeader* head = (DataHeader*)_szMsgBuf;
		if (_lastPos >= head->dataLength)
		{
			int nSize = _lastPos - head->dataLength;
			onNetMsg(head);
			memcpy(_szMsgBuf, _szMsgBuf + head->dataLength, nSize);
			_lastPos = nSize;
		}
		else
		{
			break;
		}
	}


	return 1;
}
int TcpClient::initSocket()
{
	if (_sock != INVALID_SOCKET)
	{
		Close();
	}
#ifdef _WIN32
	//启动 Windows Socket 服务
	WORD rev = MAKEWORD(2, 2);
	WSADATA dat;
	WSAStartup(rev, &dat);
#endif
	_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (INVALID_SOCKET == _sock) {
		cout << "Socket creation failed " << "socket = " << _sock << endl;
		//#ifdef _WIN32
		//		Sleep(3000);
		//#endif
		return -1;
	}
	else
	{
		cout << "Socket creation success " << "socket = " << _sock << endl;
		return 1;
	}
}
void TcpClient::Close()
{
	if (_sock == INVALID_SOCKET)return;
	cout << "Client exit socket = " << _sock << endl;
#ifdef _WIN32
	closesocket(_sock);
	//清除 Windows Socket 服务
	WSACleanup();
#else
	//Linux MacOS
	close(clientSocket);
#endif
	_sock = INVALID_SOCKET;
	_isConnected = 0;
	//cout << "sock  "<<_sock << endl;
}
int TcpClient::Connect(const char* ip, unsigned short port)
{
	if (_sock == INVALID_SOCKET)
	{
		if (initSocket() == -1)return -1;
	}
	sockaddr_in _sin = {};
	_sin.sin_family = AF_INET;
	_sin.sin_port = htons(port);
#ifdef _WIN32
	_sin.sin_addr.S_un.S_addr = inet_addr(ip);
#else
	_sin.sin_addr.s_addr = inet_addr(ip);
#endif
	int result = connect(_sock, (sockaddr*)&_sin, sizeof(sockaddr_in));
	if (SOCKET_ERROR == result) {
		cout << "Connection failure  " << "socket = " << _sock << endl;
#ifdef _WIN32
		//Sleep(3000);
#endif
		return -1;
	}
	else
	{
		cout << "Connection success  " << "socket = " << _sock << endl;
		_isConnected = 1;
	}
	return result;
}
int TcpClient::onRun()
{
	if (!isRun())return isRun();
	//cout << clock() << endl;
	fd_set fdReads;
	FD_ZERO(&fdReads);
	FD_SET(_sock, &fdReads);
	timeval t = { 0,0 };
	//cout << _sock << endl;
	int ret = select(_sock + 1, &fdReads, NULL, NULL, &t);
	if (ret < 0) {
		cout << "Disconnect from server 1 : socket = " << _sock << endl;
		Close();
		return isRun();
	}
	if (FD_ISSET(_sock, &fdReads)) {
		FD_CLR(_sock, &fdReads);
		if (-1 == recvData()) {
			cout << "Disconnect from server 2 : socket = " << _sock << endl;
			Close();
			return isRun();
		}
	}
	return isRun();
}
int TcpClient::isRun()
{
	return _sock != INVALID_SOCKET && _isConnected;
}
int TcpClient::sendData(DataHeader* head,int len)
{	
	int ret = SOCKET_ERROR;
	if (isRun() && head)
	{
		ret = send(_sock, (const char*)head, len, 0);
		if (SOCKET_ERROR == ret)
		{
			Close();
		}
	}
	return ret;

}

#endif //!_TcpClient_hpp_