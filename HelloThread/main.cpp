#include <thread>
#include <string>
#include <iostream>
#include <mutex>
#include <future>

static const int MAX = 10e8;

void Hello(const std::string& name) {
	std::cout << "hello, " << name << std::endl;
}

void init() {
	std::cout << "init..." << std::endl;
}

void worker(std::once_flag* flag) {
	std::call_once(*flag, init);
}

double concurrent_worker(int min, int max) {
	double sum = 0;
	for (int i = min; i <= max; i++) {
		sum += sqrt(i);
	}
	return sum;
}

double concurrent_task(int min, int max) {
	std::vector<std::future<double>> results;

	unsigned int count = std::thread::hardware_concurrency();
	min = 0;
	for (int i = 0; i < (int)count; i++) {
		std::packaged_task<double(int, int)> task(concurrent_worker);
		results.push_back(task.get_future());

		int range = max / count * (i + 1);
		std::thread t(std::move(task), min, range);
		t.detach();

		min = range + 1;
	}

	std::cout << "thread create finish" << std::endl;
	double sum = 0;
	for (auto& r : results) {
		sum += r.get();
	}
	return sum;
}

int main() {
	
	/*std::thread t1(Hello, "zxh");
	std::thread t2([] (const std::string& name) {
		std::cout << "hello, " << name << std::endl;
	}, "xiaoming");

	t1.join();
	t2.join();*/

	/*std::once_flag flag;

	std::thread t1(worker, &flag);
	std::thread t2(worker, &flag);
	std::thread t3(worker, &flag);

	t1.join();
	t2.join();
	t3.join();*/

	//²âÊÔÒì²½½Ó¿Ú£¬async
	/*double result = 0;
	std::cout << "Async task with lambda triggered, thread: " << std::this_thread::get_id() << std::endl;
	auto f2 = std::async(std::launch::async, [&result]() {
		std::cout << "Lambda task in thread: " << std::this_thread::get_id() << std::endl;
		for (int i = 0; i <= MAX; i++) {
			result += sqrt(i);
		}
		});
	f2.wait();
	std::cout << "Async task with lambda finish, result: " << result << std::endl << std::endl;*/

	//²âÊÔpackaged_task
	
	auto start_time = std::chrono::steady_clock::now();

	double r = concurrent_task(0, MAX);

	auto end_time = std::chrono::steady_clock::now();
	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
	std::cout << "Concurrent task finish, " << ms << " ms consumed, Result: " << r << std::endl;
	return 0;
}