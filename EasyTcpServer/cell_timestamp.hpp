#ifndef _CELLTimestamp_hpp_
#define _CELLTimestamp_hpp_

#include <chrono>
using namespace std::chrono;

class CELLTimestamp {
public:
	CELLTimestamp() {
		update();
	}
	void update() {
		_begin = high_resolution_clock::now();
	}
	double GetElapsedTimeInSecond() {
		return GetElapsedTimeInMicroSecond() * 0.000001;
	}
	double GetElapsedTimeInMilliSecond() {
		return GetElapsedTimeInMicroSecond() * 0.001;
	}
	//获取经过的微妙数
	long long GetElapsedTimeInMicroSecond() {
		return duration_cast<microseconds>(high_resolution_clock::now() - _begin).count();
	}
private:
	time_point<high_resolution_clock> _begin;
};
#endif // !_CELLTimestamp_hpp_
