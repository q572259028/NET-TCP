#ifndef CELLTIMESTAMP_HPP
#define CELLTIMESTAMP_HPP
#include <chrono>
using namespace std::chrono;
class CellTimeStamp
{
public:
	CellTimeStamp()
	{
		//QueryPerformanceFrequency(&_frequency);
		//QueryPerformanceCounter(&_startCount);
		update();
	}
	~CellTimeStamp(){}
	void update()
	{
	///	QueryPerformanceCounter(&_startCount);
		_begin = high_resolution_clock::now();
	}
	double getSecond()
	{
		return getMicro() * 0.000001;
	}
	double getMilli()
	{
		return getMicro() * 0.001;
	}
	double getMicro()
	{	
		/*LARGE_INTEGER _endCount;
		QueryPerformanceCounter(&_endCount);
		double startT = _startCount.QuadPart * (1000000.0/_frequency.QuadPart);
		double endT = _endCount.QuadPart * (1000000.0 / _frequency.QuadPart);
		return endT - startT;*/ 
		return duration_cast<microseconds>(high_resolution_clock::now() - _begin).count();
	}

protected:
	time_point<high_resolution_clock> _begin;
};


#endif // !CELLTIMESTAMP_HPP




