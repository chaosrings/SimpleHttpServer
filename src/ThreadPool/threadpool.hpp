#pragma once
#include<atomic>
#include <thread>
#include <vector>
#include <functional>
#include<mutex>
#include <future>
#include <stdio.h>

template<typename T>
class threadsafe_queue {
private:
	struct node
	{
		T data;
		node* next;
		node(T &&val) :data(std::move(val)), next(nullptr) {}
	};
	std::mutex head_mutex;
	node* head;
	std::mutex tail_mutex;
	node* tail;
	//std::condition_variable cond;
	node* get_tail() {
		std::lock_guard<std::mutex> tail_lock(tail_mutex);
		return tail;
	}
public:
	threadsafe_queue() :head(new node(T())), tail(head) {}
	~threadsafe_queue() 
	{ 
		std::lock_guard <std::mutex> head_lock(head_mutex);
		std::lock_guard<std::mutex> tail_lock(tail_mutex);
		auto iter = head;
		auto old = iter;
		while (iter != nullptr)
		{
			old = iter;
			iter = iter->next;
			delete old;
		}
		iter = nullptr;
	}
	threadsafe_queue(const threadsafe_queue& rhs) = delete;
	threadsafe_queue& operator=(const threadsafe_queue& rhs) = delete;

	bool try_pop(T& dst) {
		//锁住head指针
		std::lock_guard <std::mutex> head_lock(head_mutex);
		//短暂封锁tail 判断是否head==tail,是则说明队列为空，try_pop失败
		if (head== get_tail())
			return false;
		//队列不为空的话pop只需要操作head指针
		auto old_head = head;
		head = head->next;
		delete old_head;
		dst = std::move(head->data);
		return true;
	}
	void push(T newval) {
		node* new_tail = new node(std::move(newval));
		{
			//锁住tail
			std::lock_guard<std::mutex> tail_lock(tail_mutex);
			//更新tail指针
			tail->next = new_tail;
			tail = new_tail;
		}
	}
	bool empty() {
		std::lock_guard<std::mutex> head_lock(head_mutex);
		return head== get_tail();
	}
};
class thread_pool {
private:
	class function_wrapper
	{
	private:
		struct impl_base {
			virtual void call() = 0;
			virtual ~impl_base() {}
		};
		template<typename F>
		struct impl_movable :impl_base
		{
			F f;
			impl_movable(F&& callable) :f(std::move(callable)) {}
			void call() override { f(); }
		};
		//实际函数的指针
		std::unique_ptr<impl_base> impl;
	public:
		function_wrapper() = default;
		template<typename F>
		function_wrapper(F &&movable) :impl(new impl_movable<F>(std::move(movable))) {}
		function_wrapper(function_wrapper&& rhs) :impl(std::move(rhs.impl)) {}
		function_wrapper& operator=(function_wrapper&& rhs) {
			impl = std::move(rhs.impl);
			return *this;
		}
		void operator()() { impl->call(); }
		//禁止拷贝
		function_wrapper(const function_wrapper&) = delete;
		function_wrapper& operator=(const function_wrapper&) = delete;
	};
	std::atomic<bool> done;
	std::atomic<unsigned int> idleNum;
	std::atomic<unsigned int> totalNum;
	threadsafe_queue<function_wrapper> work_queue;
	std::vector<std::thread> threads;
	void worker_thread() {
		while (!done) 
		{
			function_wrapper task;
			if (work_queue.try_pop(task) == true) 
			{
				--idleNum;
				task();
				++idleNum;
			}
			else 
				std::this_thread::yield();
		}
	}
public:
	thread_pool() :done(false) {
		const unsigned thread_count = std::thread::hardware_concurrency();
		totalNum.store(thread_count);
		idleNum.store(thread_count);
		try
		{
			for (unsigned i = 0; i < thread_count; ++i)
			{
				threads.emplace_back(std::move(
					std::thread(&thread_pool::worker_thread,this)
				)
				);
			}
		}
		catch (...)
		{
			done = true;
			throw;
		}
	}
	~thread_pool() {
		done = true;
		for (auto &t : threads)
		{
			t.join();
		}
		
	}
	//返回一个future<T> 得到任务的返回值
	template<typename FuncType> 
	std::future<typename std::result_of<FuncType()>::type> sumbit(FuncType f)
	{	
    
		using result_type = typename std::result_of<FuncType()>::type;
		std::packaged_task<result_type()> temp(std::move(f));
		std::future<result_type> res(temp.get_future());
		function_wrapper task(std::move(temp));
		work_queue.push(std::move(task));
		return res;
	}
	void printStatus()
	{
		char line[1024];
		size_t s=sprintf(line,"idle thread : %d total thread : %d \n",idleNum.load(),totalNum.load());
		write(STDOUT_FILENO,line,s);
	}
};