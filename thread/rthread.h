#ifndef THREAD_H
#define THREAD_H
/*
 *线程池的实现
 *by st
 *
 * */

#ifdef __LINUX
#include <unistd.h>
#endif // _LINUX


#include <thread>
#include <vector>
#include <atomic>
#include <algorithm>

#include <iostream>


namespace THREAD{
	enum{
		POOL_SIZE		= 4,	// 线程池大小
		TASK_MAX_COUNT 	= 20,	// 最大任务数 超过会添加失败
		PRIORITY_MIX	= 100,	// 最小优先级
		PRIORITY_MAX	= 0		// 最大优先级
	};
	
	class Mutex{
	private:
		std::atomic<bool> flag = ATOMIC_VAR_INIT(false);
	public:
		Mutex() = default;
		Mutex(const Mutex&) = delete;
		Mutex& operator= (const Mutex&) = delete;
		~Mutex(){};
		void lock(){
			bool expected = false;
			while(!flag.compare_exchange_strong(expected,true)){
				expected = false;
			}
		}
		void unlock(){
			flag.store(false);
		}
		class Keeper{
		private:
			Mutex* _mtx;
			public:
				Keeper(Mutex& mtx){_mtx = &mtx;_mtx->lock();}
				~Keeper(){_mtx->unlock();};
		};
	};
	
	class ThreadPool{
	public:
		class Runnable;
	private:
		static std::vector<Runnable *> _tasks; 	// 任务队列
		static Mutex _tasks_mtx;
	public:
		ThreadPool(){}
		~ThreadPool(){}
		
		class Runnable{		// 任务接口
		public:
			Runnable(int priority = 1){
				set_priority(priority);
			}
			virtual ~Runnable(){}
			virtual void run(){};
			virtual void set_priority(int priority){
				if(priority > PRIORITY_MIX){priority = 100;}
				if(priority < PRIORITY_MAX){priority = 0;}
				_priority = priority;
			};
			virtual int get_priority(){return _priority;};
		private:
			int _priority;	// 优先级，数字越小级别越高
		};

		static int add_task(Runnable *task){
			if(_tasks.size() > TASK_MAX_COUNT){return -1;}
			if(!task){return -2;};
			if(_tasks.size() > TASK_MAX_COUNT){return -3;}		// 超过任务限制
			
			// 需要确定已有的任务列表中内有待加入的任务
			Mutex::Keeper kp(_tasks_mtx);
			std::vector<Runnable*>::iterator iter = std::find(_tasks.begin(),_tasks.end(),task);
			if(iter == _tasks.end()){
				_tasks.push_back(task);
				// std::cout<<"添加新的任务 _tasks.size() "<<_tasks.size()<<std::endl;
				return 0;
			}
			return -3;
		}

		static void start_pool(){
			for (int i=0;i<POOL_SIZE;i++){
				std::thread* t = new std::thread(get_task, i);
				t->detach();
			}
		}
	
	private:
		// 取得一个比参数优先级大的任务
		static Runnable* fetch_task(int priority = PRIORITY_MIX){
			std::vector<Runnable*>::iterator iter = _tasks.begin();
			Runnable* result = NULL;
			int temp = PRIORITY_MAX;
			for (;iter != _tasks.end();iter++){
				if((*iter)->get_priority() < priority){
					result = (*iter);
					break;
				}
			}
			if(result){
				_tasks.erase(iter);
			}
			return result;
		}
		static void get_task(int index){	// 线程将会一直执行这个方法
			using std::cout;
			using std::endl;
			Runnable* task = NULL;
			while(1){
				while(1){	// 判空需要加锁
					_tasks_mtx.lock();
					if(_tasks.size()){
						task = fetch_task(PRIORITY_MIX);
						_tasks_mtx.unlock();
						break;
					}
					_tasks_mtx.unlock();
				}
				
				// cout<<"线程 "<<index<<" 取得任务"<<"_tasks.size(): "<<_tasks.size()<<endl;
				if(task){
					task->run();
				}else{
					cout<<"指针错误"<<endl;
				}
			}
		};
	
		
	};
}


#endif
