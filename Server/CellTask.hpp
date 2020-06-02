#ifndef  _CELL_TASK_HPP_
#define _CELL_TASK_HPP_
#include<thread>
#include<mutex>
#include<list>
//任务基类
class CellTask
{
public:
	CellTask()
	{

	}
	virtual ~CellTask()
	{

	}
	virtual void doTask()
	{

	}

private:

};

//执行任务的服务类型
class CellTaskServer
{
public:
	CellTaskServer()
	{

	}
	~CellTaskServer()
	{

	}
	void addTask(CellTask* task)
	{
		std::lock_guard<std::mutex> lock(_mutex);
		_tasks.emplace_back(task);
	}
	void Start()
	{
		std::thread t(std::mem_fn(&CellTaskServer::onRun), this);
		t.detach();
	}
protected:
	void onRun()
	{	
		while (true)
		{
			if (!_tasksBuf.empty())
			{
				std::lock_guard<std::mutex> lock(_mutex);
				for (auto pTask : _tasksBuf)
				{
					_tasks.emplace_back(pTask);
				}
				_tasksBuf.clear();
			}
			if (_tasks.empty())
			{
				std::chrono::milliseconds t(1);
				std::this_thread::sleep_for(t);
				continue;
			}
			for (auto pTask : _tasks)
			{
				pTask->doTask();
				delete pTask;
			}
			_tasks.clear();
		}
		
	}
private:
	//任务数据
	std::list<CellTask*> _tasks;
	//任务数据缓冲区
	std::list<CellTask*> _tasksBuf;
	std::mutex _mutex;

	
};

#endif // ! _CELL_TASK_HPP_
