#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include <thread>
#include "TcpClient.hpp"
#include "../Server/CellTimeStamp.hpp"
#include <atomic>
int g_bRun = 1;
//客户端数量
const int cCount = 100;
//线程数量
const int tCount = 4;
TcpClient* client[cCount];
atomic_int sendCount = 0;
atomic_int readyCount = 0;
void cmdThread(TcpClient* client);
int sendThread(int id);
int main() {
		
//	thread t(cmdThread);
	//线程和主线程分离互不影响，预防主线程比线程先关闭导致报错
	//t.detach();
	for(int i = 0; i<tCount; i++)
	{
		thread t(sendThread, i+1);
		t.detach();
	}
	CellTimeStamp tTime;
	while (g_bRun)
	{
		auto t = tTime.getSecond();
		if (t >= 1.0)
		{
			printf("thread< %d > , clients< %d > , time< %lf > , send< %d >\n", tCount, cCount, t, (int)sendCount);
			sendCount = 0;
			tTime.update();
		}
		Sleep(1);
	}
	
	cout << "Client program exit" << endl;
	getchar(); getchar();
	return 0;
}
//实现接收服务器消息


void cmdThread(TcpClient* client) {
	while (true)
	{	
		/* 3. 接收服务器消息 */
		char sendBuf[128] = { "login" };
		//recv 会返回接收数据的长度
		//cin>>sendBuf;
		
		cout << "CMD length: " << strlen(sendBuf) << endl;
		if (0 == strcmp(sendBuf, "exit")) {
			client->Close();
			cout << "Client socket has been close " << endl;
			break;
		}
		else if (0 == strcmp(sendBuf, "login")) {
			//定义数据体(数据头自动初始化)
			LoginInfo logInfo;
			strcpy(logInfo.userName, "Ghosthh2");
			strcpy(logInfo.passWord, "181685");
			//发送数据体
			client->sendData(&logInfo,logInfo.dataLength);
			
		}
		else if (0 == strcmp(sendBuf, "logOut")) {
			//定义登出数据体
			LoginOut logout;
			//定义登出数据结果
			LoginOutResult logOutRes;
			cout << "please the login out username : ";
			scanf("%s", logout.userName);
			//发送登出数据包
			client->sendData(&logout, logout.dataLength);
		}
		else {
			cout << "Unsupported instructions" << "socket = " << client->getSock() << endl;
		}
	}

}
int sendThread(int id) {
	
	int begin = (id - 1) * (cCount / tCount);
	int end = id * (cCount / tCount);
	for (int i = begin; i < end; i++)
	{
		if (!g_bRun)
		{
			return 0;
		}
		client[i] = new TcpClient();
	}
	for (int i = begin; i < end; i++)
	{
		if (!g_bRun)
		{
			return 0;
		}
		client[i]->Connect("127.0.0.1", 4567);
	}
	printf("thread< %d > , begin< %d > , end< %d > \n", id, begin, end);
	readyCount++;
	while (readyCount < tCount)
	{	
		//等待其他线程准备好发送数据
		chrono::milliseconds t(10);
		this_thread::sleep_for(t);
	}
	TEST test[10];
	while (g_bRun)
	{
		for (int i = begin; i < end; i++)
		{	
			if (SOCKET_ERROR != client[i]->sendData(test, sizeof(test)))
			{
				sendCount++;
			}
			client[i]->onRun();
		}
	}
	for (int i = begin; i < end; i++)
	{
		client[i]->Close();
		delete client[i];
	}
	return 1;

}