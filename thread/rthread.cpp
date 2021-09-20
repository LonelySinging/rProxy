#include "rthread.h"

namespace THREAD{
	std::vector<Runnable *> ThreadPool::_tasks;
	Mutex ThreadPool::_tasks_mtx;
};
