#ifndef HANDLER_H__
#define HANDLER_H__ 
#include <memory>
#include <list>
#include <mutex>
#include <condition_variable>
#include "data_types.h"

using std::list;
using std::mutex;
using std::condition_variable;
using std::shared_ptr;
using std::unique_lock;
class Handler{
public:
	Handler();
	shared_ptr<Data> pop();
	void push(const shared_ptr<Data>);

private:
	list<shared_ptr<Data> > main_data;	
	mutex m_mutex;
	condition_variable m_cond;
};
#endif /* HANDLER_H__ */
