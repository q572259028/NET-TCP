#ifndef _MessageHeader_hpp_
#define _MessageHeader_hpp_
enum CMD
{
	CMD_LOGIN,
	CMD_NEW_USER_INFO,
	CMD_LOGIN_RESULT,
	CMD_LOGINOUT,
	CMD_LOGINOUT_RESULT,
	CMD_ERROR,
	CMD_TEST
};
//数据包头
struct DataHeader {
	//数据长度 < 32767
	DataHeader()
	{
		dataLength = sizeof(DataHeader);
		cmd = CMD_ERROR;
	}
	short dataLength;
	//命令
	short cmd;
};
struct TEST : public DataHeader
{
	TEST() {
		dataLength = sizeof(TEST);
		cmd = CMD_TEST;
	}
	//账户名Socket creation failed
	//密码
};
//登陆数据包
struct LoginInfo : public DataHeader
{
	LoginInfo() {
		dataLength = sizeof(LoginInfo);
		cmd = CMD_LOGIN;
	}
	//账户名Socket creation failed
	char userName[32];
	//密码
	char passWord[32];
	char data[956];
};
//新用户信息
struct NewUserInfo : public DataHeader
{
	NewUserInfo() {
		dataLength = sizeof(NewUserInfo);
		cmd = CMD_NEW_USER_INFO;
		sock = 0;
	}
	int sock;
};
//登陆结果
struct LoginResult : public DataHeader
{
	LoginResult() {
		dataLength = sizeof(LoginResult);
		cmd = CMD_LOGIN_RESULT;
		//为零时正确
		result = 0;
	}
	char data[1016];
	int result;
};
//登出数据包
struct LoginOut : public DataHeader
{
	LoginOut() {
		dataLength = sizeof(LoginOut);
		cmd = CMD_LOGINOUT;
	}
	char userName[32];
};
//登出结果
struct LoginOutResult : public DataHeader
{
	LoginOutResult() {
		dataLength = sizeof(LoginOutResult);
		cmd = CMD_LOGINOUT_RESULT;
		//为零时正确
		outResult = 0;
	}
	int outResult;

};
#endif // !_MessageHeader_hpp_


