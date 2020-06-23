# NET-TCP
跨平台TCP通信模块-基于select

## TcpServer 
``服务端类``
- ### OnRun()
建立select模型用于新客户端加入
- ### Accept()
处理新客户端加入
- ### addClientToCellServer()
找到合适的消息处理子服务端并将客户端加入
- ### Start()
启动子服务端线程并将``TcpServer``注册为子服务端的网络事件接收对象
- ### timeForMsg()
打印收发数据计数日志

 ## CellServer
``用于线程分离的子服务端类，负责接收与处理消息``
- ### addClient()
在子服务类的管辖范围内加入新的客户端
- ### setEventObj()
设置网络事件接收对象
- ### recvData()
接收客户端数据（添加了接收缓冲区）
- ### sendData()
向客户端发送数据
- ### OnRun()
建立select模型 处理客户端请求
- ### Start()
启动子线程，执行``OnRun()``
- ### onNetMsg()
处理网络消息
- ### addSendTask()
向网络任务管理器``(CellTaskServer)``添加任务

## ClientSocket
``客户端类`` 设置收发缓冲区以及相应的消息指针位置

## INetEvent
``网络事件接口``

## CellTask
``网络任务基类``

## CellSendMsgToClientTask
``发送网络消息任务类``

## CellTaskServer
``网络任务(CellTask)管理器类``

## MemoryMgr.hpp Alloctor.cpp 
实现内存池
## CellTimeStamp.hpp
高精度计时
## MessageHeader.hpp
消息格式

## TcpClient
客户端类
