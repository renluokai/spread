#ifndef I_STRATEGY_H__
#define I_STRATEGY_H__
#include "data_types.h"
#include <memory>
using std::shared_ptr;
class Strategy{
public:
	virtual bool on_init()=0;
	virtual void on_order(shared_ptr<Order>)=0;
	virtual void on_quote(shared_ptr<Quote>)=0;
	virtual void on_error(shared_ptr<Error>)=0;
	virtual void on_notify(shared_ptr<Notify>)=0;
	virtual void set_option(const char*)=0;
	virtual void get_option(char *buffer)=0;
};
#endif /* I_STRATEGY_H__ */
