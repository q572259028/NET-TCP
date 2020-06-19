#ifndef _TcpServer_hpp_
#define _TcpServer_hpp_
#include <iostream>
#include <stdio.h>
#include <thread>
#include <vector>
#include <mutex>
#include <map>
#include <atomic>
#include "Alloctor.h"
#include "MessageHeader.hpp"
#include "CellTimeStamp.hpp"
#include "CellTask.hpp"
#ifdef _WIN32
#define FD_SETSIZE 4024
#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS
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
#ifndef RECV_BUFF_SIZE
#define RECV_BUFF_SIZE 10240
#endif // !RECV_BUFF_SIZE
#ifndef SEND_BUFF_SIZE
#define SEND_BUFF_SIZE 10240
#endif // !SEND_BUFF_SIZE
//#ifndef _CellServer_THREAD_COUNT
//#define _CellServer_THREAD_COUNT 4
//#endif // !_CellServer_THREAD_COUNT
class INetEvent;
class ClientSocket;
class CellSendMsgToClientTask;
class CellServer;
class TcpServer;
//封装客户端类
class ClientSocket
{
public:
	ClientSocket(SOCKET sockfd = INVALID_SOCKET)
	{
		_sockfd = sockfd;
		memset(_szRecvBuf, 0, sizeof(_szRecvBuf));
		memset(_szSendBuf, 0, sizeof(_szSendBuf));
		_lastRecvPos = 0;
		 _lastSendPos = 0;
	}
	virtual ~ClientSocket() {}

	SOCKET getSock()
	{
		return _sockfd;
	}
	char* getMsgBuf()
	{
		return _szRecvBuf;
	}
	int getLastPos()
	{
		return _lastRecvPos;
	}
	void setLastPos(int pos)
	{
		_lastRecvPos = pos;
	}
	int sendData(DataHeader* head)
	{	
		int ret = SOCKET_ERROR;
		int nSendLen = head->dataLength;
		const char* pSendData = (const char*)head;
		while (true)
		{
			if (_lastSendPos + nSendLen >= SEND_BUFF_SIZE)
			{
				int nCopyLen = SEND_BUFF_SIZE - _lastSendPos;
				memcpy(_szSendBuf + _lastSendPos, pSendData, nCopyLen);
				pSendData += nCopyLen;
				//!!!
				nSendLen -= nCopyLen;
				ret = send(_sockfd, _szSendBuf, SEND_BUFF_SIZE, 0);
				_lastSendPos = 0;
				if (SOCKET_ERROR == ret)
				{
					return ret;
				}
			}
			else
			{
				memcpy(_szSendBuf + _lastSendPos, pSendData, nSendLen);
				_lastSendPos += nSendLen;
				break; 
			}
		}
		

		if (head)
		{	

			//std::cout << head->cmd << ' ' << head->dataLength << std::endl;
			ret = send(_sockfd, (const char*)head, (int)head ->dataLength, 0);
		}
		return ret;
	}
private:
	SOCKET _sockfd;
	//接收缓存区
	char _szRecvBuf[RECV_BUFF_SIZE * 5];
	char _szSendBuf[SEND_BUFF_SIZE * 5];
	int _lastRecvPos = 0;
	int _lastSendPos = 0;

};
//网络事件接口
class INetEvent
{
public:
	//客户端离开事件
	virtual void onLeave(ClientSocket* pClient) = 0;
	//客户端消息事件
	virtual void onNetMsg(CellServer* pCellServer, ClientSocket* pClient,DataHeader* header ) = 0;
	//客户端加入事件
	virtual void onNetJoin(ClientSocket* pClient) = 0;
	//接收事件
	virtual void onNetRecv(ClientSocket* pClient) = 0;
};
//网络消息发送服务类
class CellSendMsgToClientTask:public CellTask
{
public:
	CellSendMsgToClientTask(ClientSocket* pClient, DataHeader* header)
	{
		_pClient = pClient;
		_pHeader = header;
	}
	void doTask()
	{
		_pClient->sendData(_pHeader);
		delete _pHeader;
	}

private:
	ClientSocket* _pClient;
	DataHeader* _pHeader;
};


//网络消息接收处理服务类
class CellServer
{
public:
	//char _szRecv[MAX_BUFF_SIZE] = {};
	CellServer(SOCKET sock = INVALID_SOCKET)
	{
		_sock = sock;
		_pNetEvent = nullptr;
	}
	~CellServer()
	{	
		Close();
	}

	void addClient(ClientSocket* pClient)
	{	
		std::lock_guard<std::mutex> lock(_mutex);
		_clientsBuff.emplace_back(pClient);
	}
	void setEventObj(INetEvent* event)
	{
		_pNetEvent = event;
	}
	int isRun()
	{
		return _sock != INVALID_SOCKET;
	}
	void Close()
	{
		if (_sock == INVALID_SOCKET)return;
		std::cout << "Server exit socket = " << _sock << std::endl;
#ifdef _WIN32
		for (int n = (int)_clients.size() - 1; n >= 0; n--)
		{
			closesocket(_clients[n]->getSock());
			delete _clients[n];
		}
		closesocket(_sock);

#else
		//Linux MacOS
		for (int n = (int)_clients.size() - 1; n >= 0; n--)
		{
			close(_clients[n]->getSock());
			delete _clients[n];
		}
		close(clientSocket);
#endif
		_clients.clear();
		_sock = INVALID_SOCKET;
		//cout << "sock  "<<_sock << endl;
	}

	int recvData(ClientSocket* pClient)
	{
		//  接收客户端数据
		char*  _szRecv = pClient->getMsgBuf() + pClient->getLastPos();
		int nLen = (int)recv(pClient->getSock(), _szRecv, RECV_BUFF_SIZE * 5 - pClient->getLastPos(), 0);
		_pNetEvent->onNetRecv(pClient);
		if (nLen <= 0)
		{
			//std::cout << "client exit <Socket = " << pClient->getSock() << " >" << std::endl;
			return -1;
		}
		//memcpy(pClient->getMsgBuf() + pClient->getLastPos(), _szRecv, nLen);
		pClient->setLastPos(pClient->getLastPos() + nLen);
		while (pClient->getLastPos() >= sizeof(DataHeader))
		{
			DataHeader* head = (DataHeader*)pClient->getMsgBuf();
			if (pClient->getLastPos() >= head->dataLength)
			{
				int nSize = pClient->getLastPos() - head->dataLength;
				onNetMsg( this,pClient, head);
				memcpy(pClient->getMsgBuf(), pClient->getMsgBuf() + head->dataLength, nSize);
				pClient->setLastPos(nSize);
			}
			else
			{
				break;
			}
		}
		return 1;
	}
	int sendData(DataHeader* head, SOCKET cSock)
	{
		if (isRun() && head)
		{
			//std::cout << head->cmd << ' ' << head->dataLength << std::endl;
			return send(cSock, (const char*)head, head->dataLength, 0);
		}
		return SOCKET_ERROR;
	}
	virtual void onNetMsg(CellServer* pCellServer,ClientSocket* pClient,DataHeader* header)
	{	
		_pNetEvent->onNetMsg(this, pClient, header);
	}
	//客户端socket备份
	fd_set _fdRead_bak;
	//客户端列表变化情况
	bool _clients_change = 0;
	SOCKET _maxSock;
	int onRun()
	{	
		_clients_change = 1;
		while (isRun())
		{
			if (_clientsBuff.size() > 0)
			{
				std::lock_guard<std::mutex> lock(_mutex);
				for (auto pClient : _clientsBuff)
				{	
					_clients[pClient->getSock()] = pClient;
				}
				_clientsBuff.clear();
				_clients_change = 1;
				//printf("00000\n");
			}
			if (_clients.empty())
			{	
				std::chrono::milliseconds t(1);
				std::this_thread::sleep_for(t);
				continue;
			}
			//伯克利 socket
			fd_set fdRead;
			//fd_set fdWrite;
			//fd_set fdExp;


			FD_ZERO(&fdRead);
			if (_clients_change)
			{	
				_clients_change = 0;
				if(_clients.begin()!= _clients.end())
				_maxSock = _clients.begin()->second->getSock();
				for (auto iter : _clients)
				{	
					FD_SET(iter.second->getSock(), &fdRead);
					_maxSock = max(_maxSock, iter.second->getSock());
				}
				memcpy(&_fdRead_bak, &fdRead, sizeof(fd_set));
			}
			else
			{	
				memcpy(&fdRead, &_fdRead_bak, sizeof(fd_set));
			}


			//FD_ZERO(&fdWrite);
			//FD_ZERO(&fdExp);

			//FD_SET(_sock, &fdRead);
			//FD_SET(_sock, &fdWrite);
			//FD_SET(_sock, &fdExp);
			
			timeval t = { 0,0 };
			int ret = select(_maxSock + 1, &fdRead, nullptr, nullptr, &t);
			if (ret < 0)
			{
				printf("select finished\n");
				Close();
				return 0;
			}
			else if(ret == 0)
			{
				continue;
			}
			std::vector<ClientSocket*> temp;
#ifdef _WIN32
			for (int i = 0; i < (int)fdRead.fd_count; i++)
			{
				auto iter = _clients.find(fdRead.fd_array[i]);
				if (iter == _clients.end())
				{
					printf("error");
					break;
				}
				if (-1 == recvData(iter->second))
				{
					if(_pNetEvent)
						_pNetEvent->onLeave(iter->second);
					_clients_change = 1;
					_clients.erase(iter->first);
				}
			}
#else
			for (auto iter:_clients)
			{
				if (FD_ISSET(iter.second->getSock(), &fdRead))
				{
					if (-1 == recvData(iter.second))
					{	
						if (_pNetEvent)
							_pNetEvent->onLeave(iter.second);
						_clients_change = 1;
						temp.push_back(iter.second);
						
						}

					}
				}
			}
		for (auto pClient : temp)
		{	
			_clients.erase(pClient -> getdSock());
			delete pClient;
			
		}
#endif
			//*for ( int i = (int)_clients.size() - 1 ; i >= 0; i--)
			//{
			//	if (FD_ISSET(_clients[i]->getSock(), &fdRead))
			//	{
			//		if (-1 == recvData(_clients[i]))
			//		{
			//			auto iter = _clients.begin() + i;
			//			if (iter != _clients.end())
			//			{	
			//				_clients_change = 1;
			//				if(_pNetEvent)
			//				_pNetEvent->onLeave(_clients[i]);
			//				delete _clients[i];
			//				_clients.erase(iter);
			//			}

			//		}
			//	}
			//}*/
		}
		return 0;
	}
	void start()
	{	
		_thread = std::thread(std::mem_fn(&CellServer::onRun),this);
		_taskServer.Start();
	}
	size_t getClientCount()
	{
		return _clients.size() + _clientsBuff.size();
	}
	void addSendTask(ClientSocket* pClient,DataHeader* header)
	{	
		
		CellSendMsgToClientTask* task = new CellSendMsgToClientTask(pClient, header);
		_taskServer.addTask(task);
	}
private:
	std::map<SOCKET, ClientSocket*> _clients;
	std::vector<ClientSocket*> _clientsBuff;
	//缓冲队列锁
	std::mutex _mutex;
	SOCKET _sock = INVALID_SOCKET;
	std::thread _thread;
	//网络事件对象
	INetEvent* _pNetEvent;
	CellTaskServer _taskServer;
};




class TcpServer:INetEvent
{
public:
	TcpServer()
	{	
		_sock = INVALID_SOCKET;
		_recvCount = 0;
	};
	~TcpServer()
	{
		Close();
	}
	//多个线程触发不安全
	virtual void onNetMsg(CellServer* pCellServer,ClientSocket* pClient, DataHeader* header)
	{
		_recvCount++;
	}
	//多个线程触发不安全
	virtual void onLeave(ClientSocket* pClient)
	{
		_clientCount--;
	}
	//多个线程触发不安全
	virtual void onNetJoin(ClientSocket* pClient)
	{
		_clientCount++;
	}
	virtual void onNetRecv(ClientSocket* pClient)
	{
		_recvCount++;
	}

	void start(int nCellServer)
	{
		for (int n = 0; n < nCellServer; n++)
		{
			auto ser = new CellServer(_sock);
			_cellServers.emplace_back(ser);
			//注册网络事件接受对象
			ser->setEventObj(this);
			ser->start();
		}
	}
	int initSocket()
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
			std::cout << "Socket creation failed " << "socket = " << _sock << std::endl;
			//#ifdef _WIN32
			//		Sleep(3000);
			//#endif
			std::chrono::milliseconds t(5000);
			std::this_thread::sleep_for(t);
			return -1;
		}
		else
		{
			std::cout << "Socket creation success " << "socket = " << _sock << std::endl;
			return 1;
		}
	}
	int Bind(const char* ip, unsigned short port)
	{
		sockaddr_in _sin = {};
		_sin.sin_family = AF_INET;
		_sin.sin_port = htons(port);//host to net unsigned short

#ifdef _WIN32
		if (ip)
		{
			_sin.sin_addr.S_un.S_addr = inet_addr(ip);
		}
		else
		{
			_sin.sin_addr.S_un.S_addr = INADDR_ANY;
		}
#else
		if (ip)
		{
			_sin.sin_addr.s_addr = inet_addr(ip);
		}
		else
		{
			_sin.sin_addr.s_addr = INADDR_ANY;
		}
#endif   
		if (SOCKET_ERROR == bind(_sock, (sockaddr*)&_sin, sizeof(_sin)))
		{
			printf("Failed to bind  port <%d>\n", port);
			return 0;
		}
		else {
			printf("Success to bind  port <%d>\n", port);
			return 1;
		}
	}
	int Listen(int n)
	{
		if (SOCKET_ERROR == listen(_sock, 5))
		{
			printf("Failed to listen port sock = <%d>\n", _sock);
			return 0;
		}
		else {
			printf("Success to listen port sock = <%d>\n", _sock);
			return 1;
		}
	}
	SOCKET Accept()
	{
		sockaddr_in clientAddr = {};
		int nAddrLen = sizeof(sockaddr_in);
		SOCKET cSock = INVALID_SOCKET;
#ifdef _WIN32
		cSock = accept(_sock, (sockaddr*)&clientAddr, &nAddrLen);
#else
		cSock = accept(_sock, (sockaddr*)&clientAddr, (socklen_t *)&nAddrLen);
#endif
		if (INVALID_SOCKET == cSock)
		{
			printf("Received an invalid client socket\n");
			return  INVALID_SOCKET;
		}
		else
		{	
		//	std::cout << "XXXXX" << std::endl;
			NewUserInfo userjoin;
			userjoin.sock = cSock;
			//sendData_all(&userjoin);
		}
		addClientToCellServer(new ClientSocket(cSock));
	//	printf("New client joined：socket = %d,IP = %s \n", (int)cSock, inet_ntoa(clientAddr.sin_addr));
		return cSock;
	}
	void addClientToCellServer(ClientSocket* pClient)
	{
		//_clients.push_back(pClient);
		auto pMinServer = _cellServers[0];
		for (auto pc : _cellServers)
		{	
			if (pMinServer->getClientCount() > pc->getClientCount())
			{
				pMinServer = pc;
			}

		}
		pMinServer->addClient(pClient);
		onNetJoin(pClient); 
	}
	int onRun()
	{
		if (!isRun())return 0;
		timeForMsg();
		//伯克利 socket
		fd_set fdRead;
		fd_set fdWrite;
		fd_set fdExp;

		FD_ZERO(&fdRead);
		FD_ZERO(&fdWrite);
		FD_ZERO(&fdExp);

		FD_SET(_sock, &fdRead);
		FD_SET( _sock, &fdWrite);
		FD_SET(_sock, &fdExp);


		timeval t = { 0,0 };
		int ret = select(_sock + 1, &fdRead, &fdWrite, &fdExp, &t);
		if (ret < 0)
		{
			printf("Accept select finished\n");
			Close();
			return isRun();
		}
		if (FD_ISSET(_sock, &fdRead))
		{
			FD_CLR(_sock, &fdRead);

		//	std::cout << "FFF" << std::endl;
			Accept();
		}
		
		return isRun();
	}
	int isRun()
	{
		return _sock != INVALID_SOCKET;
	}

	//int recvData(ClientSocket* pClient)
	//{
	//	// 5 接收客户端数据
	//	int nLen = (int)recv(pClient->getSock(), _szRecv, MAX_BUFF_SIZE, 0);
	//	if (nLen <= 0)
	//	{
	//		std::cout << "client exit <Socket = " << pClient->getSock() << " >" << std::endl;
	//		return -1;
	//	}
	//	memcpy(pClient->getMsgBuf() + pClient->getLastPos(), _szRecv, nLen);
	//	pClient->setLastPos(pClient->getLastPos() + nLen);
	//	while (pClient->getLastPos() >= sizeof(DataHeader))
	//	{
	//		DataHeader* head = (DataHeader*)pClient->getMsgBuf();
	//		if (pClient->getLastPos() >= head->dataLength)
	//		{
	//			int nSize = pClient->getLastPos() - head->dataLength;
	//			onNetMsg(head, pClient->getSock());
	//			memcpy(pClient->getMsgBuf(), pClient->getMsgBuf() + head->dataLength, nSize);
	//			pClient->setLastPos(nSize);
	//		}
	//		else
	//		{
	//			break;
	//		}
	//	}
	//	return 1;
	//}
	
	//网络消息计数
	void timeForMsg()
	{
		auto t1 = _tTime.getSecond();
		if (t1 >= 1.0)
		{

			printf("thread< %d > , time< %lf > , socket< %d > , clients num :< %d > , recv< %d > , msg< %d >\n",_cellServers.size(),t1,_sock,(int)_clientCount,(int)(_recvCount/t1), (int)(_msgCount / t1));
			_recvCount = 0; 
			_msgCount = 0;
			_tTime.update();
			
		}

	}
	//void onNetMsg(DataHeader* header, SOCKET cSock)
	//{	

	//	switch (header->cmd)
	//	{
	//	case CMD_LOGIN:
	//	{
	////		printf("XX\n");
	//		LoginInfo* login = (LoginInfo*)header;
	//		printf("getCMD: CMD_LOGIN,length：%d,userName=%s passWord=%s\n", login->dataLength, login->userName, login->passWord);
	//		//忽略判断用户密码是否正确的过程
	//		LoginResult ret;
	//		sendData(&ret, cSock);
	//	}
	//	break;
	//	case CMD_LOGINOUT:
	//	{
	//		LoginOut*  loginOut = (LoginOut*)header;
	//		printf("getCMD: CMD_LOGINOUT,length：%d,userName=%s \n", loginOut->dataLength, loginOut->userName);
	//		//忽略判断用户密码是否正确的过程
	//		LoginOutResult ret;
	//		sendData(&ret, cSock);
	//	}
	//	break;
	//	default:
	//	{
	//		DataHeader header;
	//		sendData(&header, cSock);
	//	}
	//	break;
	//	}
	//}

	//int sendData_all(DataHeader* head)
	//{
	//	if (isRun() && head)
	//	{
	//		for (int n = (int)_clients.size() - 1; n >= 0; n--)
	//		{
	//			sendData(head, _clients[n]->getSock());
	//		}
	//	}
	//	return SOCKET_ERROR;
	//}
	void Close()
	{
		if (_sock == INVALID_SOCKET)return;
		std::cout << "Server exit socket = " << _sock << std::endl;
#ifdef _WIN32
		closesocket(_sock);
		//清除 Windows Socket 服务
		WSACleanup();
#else
		//Linux MacOS
		close(clientSocket);
#endif
		_sock = INVALID_SOCKET;
		//cout << "sock  "<<_sock << endl;
	}
	//TcpServer();
	//virtual ~TcpServer();
	////初始化socket
	//int initSocket();
	////绑定IP、端口号
	//int Bind(const char* ip, unsigned short port);
	////监听端口号
	//int Listen(int n);
	////接受客户端连接
	//SOCKET Accept();
	////关闭socket
	//void Close();
	////处理网络消息
	//int onRun();
	////是否工作中
	//int isRun();
	////接收数据 处理粘包 拆分包
	//int recvData(SOCKET cSock);
	////响应网络消息
	//virtual void onNetMsg(DataHeader* header, SOCKET cSock);
	////发送数据
	//int sendData(DataHeader* head, SOCKET cSock);
	//int sendData_all(DataHeader* head);
private:

	SOCKET _sock = INVALID_SOCKET;
	std::vector<CellServer*> _cellServers;
	//每秒消息计时
	CellTimeStamp _tTime;
protected:
	//socket接收计数
	std::atomic_int _recvCount=0;
	//客户端计数
	std::atomic_int _clientCount=0;
	//消息计数
	std::atomic_int _msgCount = 0;
};

//TcpServer::TcpServer()
//{
//};
//TcpServer::~TcpServer()
//{
//	Close();
//}
//int TcpServer::initSocket()
//{
//
//	if (_sock != INVALID_SOCKET)
//	{
//		Close();
//	}
//#ifdef _WIN32
//	//启动 Windows Socket 服务
//	WORD rev = MAKEWORD(2, 2);
//	WSADATA dat;
//	WSAStartup(rev, &dat);
//#endif
//	_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
//	if (INVALID_SOCKET == _sock) {
//		std::cout << "Socket creation failed" << "socket = " << _sock << std::endl;
//		//#ifdef _WIN32
//		//		Sleep(3000);
//		//#endif
//		return -1;
//	}
//	else
//	{
//		std::cout << "Socket creation success" << "socket = " << _sock << std::endl;
//		return 1;
//	}
//}
//int TcpServer::Bind(const char* ip, unsigned short port)
//{
//	sockaddr_in _sin = {};
//	_sin.sin_family = AF_INET;
//	_sin.sin_port = htons(port);//host to net unsigned short
//
//#ifdef _WIN32
//	if (ip)
//	{
//		_sin.sin_addr.S_un.S_addr = inet_addr(ip);
//	}
//	else
//	{
//		_sin.sin_addr.S_un.S_addr = INADDR_ANY;
//	}
//#else
//	if (ip)
//	{
//		_sin.sin_addr.s_addr = inet_addr(ip);
//	}
//	else
//	{
//		_sin.sin_addr.s_addr = INADDR_ANY;
//	}
//#endif   
//	if (SOCKET_ERROR == bind(_sock, (sockaddr*)&_sin, sizeof(_sin)))
//	{
//		printf("Failed to bind  port <%d>\n", port);
//		return 0;
//	}
//	else {
//		printf("Success to bind  port <%d>\n", port);
//		return 1;
//	}
//}
//int TcpServer::Listen(int n)
//{
//	if (SOCKET_ERROR == listen(_sock, 5))
//	{
//		printf("Failed to listen port sock = <%d>\n", _sock);
//		return 0;
//	}
//	else {
//		printf("Success to listen port sock = <%d>\n", _sock);
//		return 1;
//	}
//}
//SOCKET TcpServer::Accept()
//{
//	sockaddr_in clientAddr = {};
//	int nAddrLen = sizeof(sockaddr_in);
//	SOCKET cSock = INVALID_SOCKET;
//#ifdef _WIN32
//	cSock = accept(_sock, (sockaddr*)&clientAddr, &nAddrLen);
//#else
//	cSock = accept(_sock, (sockaddr*)&clientAddr, (socklen_t *)&nAddrLen);
//#endif
//	if (INVALID_SOCKET == cSock)
//	{
//		printf("Received an invalid client socket\n");
//		return  INVALID_SOCKET;
//	}
//	else
//	{
//		NewUserInfo userjoin;
//		sendData_all(&userjoin);
//	}
//	_clients.push_back(cSock);
//	printf("New client joined：socket = %d,IP = %s \n", (int)cSock, inet_ntoa(clientAddr.sin_addr));
//	return cSock;
//}
//int TcpServer::onRun()
//{
//	if (!isRun())return isRun();
//	//伯克利 socket
//	fd_set fdRead;
//	fd_set fdWrite;
//	fd_set fdExp;
//
//	FD_ZERO(&fdRead);
//	FD_ZERO(&fdWrite);
//	FD_ZERO(&fdExp);
//
//	FD_SET(_sock, &fdRead);
//	FD_SET(_sock, &fdWrite);
//	FD_SET(_sock, &fdExp);
//	SOCKET maxSock = _sock;
//	for (int n = (int)_clients.size() - 1; n >= 0; n--)
//	{
//		FD_SET(_clients[n], &fdRead);
//		maxSock = max(maxSock, _clients[n]);
//	}
//	///nfds 是一个整数值 是指fd_set集合中所有描述符(socket)的范围，而不是数量
//	///既是所有文件描述符最大值+1 在Windows中这个参数可以写0
//	timeval t = { 1,0 };
//	int ret = select(maxSock + 1, &fdRead, &fdWrite, &fdExp, &t);
//	if (ret < 0)
//	{
//		printf("select finished\n");
//		Close();
//		return isRun();
//	}
//	if (FD_ISSET(_sock, &fdRead))
//	{
//		FD_CLR(_sock, &fdRead);
//		Accept();
//	}
//	for (auto now : _clients)
//	{
//		if (FD_ISSET(now, &fdRead))
//		{
//			if (-1 == recvData(now))
//			{
//				auto iter = find(_clients.begin(), _clients.end(), now);
//				if (iter != _clients.end())
//				{
//					_clients.erase(iter);
//				}
//
//			}
//		}
//	}
//	return isRun();
//}
//int TcpServer::isRun()
//{
//	return _sock != INVALID_SOCKET;
//}
//int TcpServer::recvData(SOCKET cSock)
//{
//	//缓冲区
//	char szRecv[4096] = {};
//	// 5 接收客户端数据
//	int nLen = (int)recv(cSock, szRecv, sizeof(DataHeader), 0);
//	DataHeader* header = (DataHeader*)szRecv;
//	if (nLen <= 0)
//	{
//		std::cout << "client exit <Socket = " << cSock << " >" << std::endl;
//		return -1;
//	}
//	recv(cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
//	return 1;
//}
//void TcpServer::onNetMsg(DataHeader* header, SOCKET cSock)
//{
//	switch (header->cmd)
//	{
//	case CMD_LOGIN:
//	{
//		
//		LoginInfo* login = (LoginInfo*)header;
//		printf("CMD: CMD_LOGIN,length：%d,userName=%s passWord=%s\n", login->dataLength, login->userName, login->passWord);
//		//忽略判断用户密码是否正确的过程
//		LoginResult ret;
//		sendData(&ret, cSock);
//	}
//	break;
//	case CMD_LOGINOUT:
//	{
//		LoginOut*  loginOut = (LoginOut*)header;
//		printf("CMD:：CMD_LOGINOUT,length：%d,userName=%s \n", loginOut->dataLength, loginOut->userName);
//		//忽略判断用户密码是否正确的过程
//		LoginOutResult ret;
//		sendData(&ret, cSock);
//	}
//	break;
//	default:
//	{
//		DataHeader header = { 0,CMD_ERROR };
//		sendData(&header, cSock);
//	}
//	break;
//	}
//}
//int TcpServer::sendData(DataHeader* head, SOCKET cSock)
//{
//	if (isRun() && head)
//	{
//		return send(cSock, (const char*)head, head->dataLength, 0);
//	}
//	return SOCKET_ERROR;
//}
//int TcpServer::sendData_all(DataHeader* head)
//{
//	if (isRun() && head)
//	{
//		for (int n = (int)_clients.size() - 1; n >= 0; n--)
//		{
//			sendData(head,_clients[n]);
//		}
//	}
//	return SOCKET_ERROR;
//}
//void TcpServer::Close()
//{
//	if (_sock == INVALID_SOCKET)return;
//	std::cout << "Server exit socket = " << _sock << std::endl;
//#ifdef _WIN32
//	closesocket(_sock);
//	//清除 Windows Socket 服务
//	WSACleanup();
//#else
//	//Linux MacOS
//	close(clientSocket);
//#endif
//	_sock = INVALID_SOCKET;
//	//cout << "sock  "<<_sock << endl;
//}



#endif // !_TcpServer_hpp_
