#include "rthread.h"

namespace THREAD{
	std::vector<ThreadPool::Runnable *> ThreadPool::_tasks;
	Mutex ThreadPool::_tasks_mtx;
};
