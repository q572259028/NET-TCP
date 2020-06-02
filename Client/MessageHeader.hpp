#ifndef _MessageHeader_hpp_
#define _MessageHeader_hpp_
enum CMD
{
	CMD_LOGIN,
	CMD_NEW_USER_INFO,
	CMD_LOGIN_RESULT,
	CMD_LOGINOUT,
	CMD_LOGINOUT_RESULT,
	CMD_ERROR
};
//���ݰ�ͷ
struct DataHeader {
	//���ݳ��� < 32767
	short dataLength;
	//����
	short cmd;
};
//��½���ݰ�
struct LoginInfo : public DataHeader
{
	LoginInfo() {
		dataLength = sizeof(LoginInfo);
		cmd = CMD_LOGIN;
	}
	//�˻���Socket creation failed
	char userName[32];
	//����
	char passWord[32];
};
//���û���Ϣ
struct NewUserInfo : public DataHeader
{
	NewUserInfo() {
		dataLength = sizeof(NewUserInfo);
		cmd = CMD_NEW_USER_INFO;
		sock = 0;
	}
	int sock;
};
//��½���
struct LoginResult : public DataHeader
{
	LoginResult() {
		dataLength = sizeof(LoginResult);
		cmd = CMD_LOGIN_RESULT;
		//Ϊ��ʱ��ȷ
		result = 0;
	}
	int result;
};
//�ǳ����ݰ�
struct LoginOut : public DataHeader
{
	LoginOut() {
		dataLength = sizeof(LoginOut);
		cmd = CMD_LOGINOUT;
	}
	char userName[32];
};
//�ǳ����
struct LoginOutResult : public DataHeader
{
	LoginOutResult() {
		dataLength = sizeof(LoginOutResult);
		cmd = CMD_LOGINOUT_RESULT;
		//Ϊ��ʱ��ȷ
		outResult = 0;
	}
	int outResult;

};
#endif // !_MessageHeader_hpp_


