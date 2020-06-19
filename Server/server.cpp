#include "Alloctor.h"
#include "TcpServer.hpp"

class MyServer:public TcpServer
{
public:
	//多个线程触发不安全
	virtual void onNetRecv(ClientSocket* pClient)
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
	virtual void onNetMsg(CellServer* pCellServer,ClientSocket* pClient, DataHeader* header)
	{	
		_msgCount++;
		switch (header->cmd)
		{
		case CMD_TEST:
		{
			//printf("%d\n", (int)_recvCount);
		}
		break;
		case CMD_LOGIN:
		{
			//		printf("XX\n");
			LoginInfo* login = (LoginInfo*)header;
			printf("getCMD: CMD_LOGIN,length：%d,userName=%s passWord=%s\n", login->dataLength, login->userName, login->passWord);
			//忽略判断用户密码是否正确的过程
			LoginResult* ret = new LoginResult();
			pCellServer->addSendTask(pClient,ret);
			delete ret;
		}
		break;
		case CMD_LOGINOUT:
		{
			LoginOut*  loginOut = (LoginOut*)header;
			printf("getCMD: CMD_LOGINOUT,length：%d,userName=%s \n", loginOut->dataLength, loginOut->userName);
			//忽略判断用户密码是否正确的过程
			LoginOutResult ret;
			pClient->sendData(&ret);
		}
		break;
		default:
		{	
			printf("undefined CMD\n");
			DataHeader header;
			pClient->sendData(&header);

		}
		break;
		}
	}

private:

};

int main()
{
	MyServer server;
	server.initSocket();
	server.Bind(nullptr, 4567);
	server.Listen(5);
	//开启线程数量
	server.start(4);
	auto p1 = new int();
	while (server.isRun())
	{
		server.onRun();
	}
	server.Close();
	printf("Server program exit\n");
	getchar();
	getchar();
	getchar();
	return 0;
}
