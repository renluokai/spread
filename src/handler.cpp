#include <iostream>
#include <cstddef>
#include "../include/handler.h"
using namespace std;
Handler::Handler()
{
}

//get data from main data list
shared_ptr<Data> Handler::pop()
{
	std::unique_lock<std::mutex> lk(m_mutex);
	while(main_data.size()==0){
		m_cond.wait(lk);
	}
	shared_ptr<Data> data = main_data.front();
	main_data.pop_front();
	return data;
}

void Handler::push(shared_ptr<Data> data)
{
	std::unique_lock<std::mutex> lk(m_mutex);
	main_data.push_back(data);
	m_cond.notify_one();
}
