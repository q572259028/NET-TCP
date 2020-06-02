#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include <thread>
#include "TcpClient.hpp"
#include "../Server/CellTimeStamp.hpp"
#include <atomic>
int g_bRun = 1;
//�ͻ�������
const int cCount = 100;
//�߳�����
const int tCount = 4;
TcpClient* client[cCount];
atomic_int sendCount = 0;
atomic_int readyCount = 0;
void cmdThread(TcpClient* client);
int sendThread(int id);
int main() {
		
//	thread t(cmdThread);
	//�̺߳����̷߳��뻥��Ӱ�죬Ԥ�����̱߳��߳��ȹرյ��±���
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
//ʵ�ֽ��շ�������Ϣ


void cmdThread(TcpClient* client) {
	while (true)
	{	
		/* 3. ���շ�������Ϣ */
		char sendBuf[128] = { "login" };
		//recv �᷵�ؽ������ݵĳ���
		//cin>>sendBuf;
		
		cout << "CMD length: " << strlen(sendBuf) << endl;
		if (0 == strcmp(sendBuf, "exit")) {
			client->Close();
			cout << "Client socket has been close " << endl;
			break;
		}
		else if (0 == strcmp(sendBuf, "login")) {
			//����������(����ͷ�Զ���ʼ��)
			LoginInfo logInfo;
			strcpy(logInfo.userName, "Ghosthh2");
			strcpy(logInfo.passWord, "181685");
			//����������
			client->sendData(&logInfo,logInfo.dataLength);
			
		}
		else if (0 == strcmp(sendBuf, "logOut")) {
			//����ǳ�������
			LoginOut logout;
			//����ǳ����ݽ��
			LoginOutResult logOutRes;
			cout << "please the login out username : ";
			scanf("%s", logout.userName);
			//���͵ǳ����ݰ�
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
		//�ȴ������߳�׼���÷�������
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