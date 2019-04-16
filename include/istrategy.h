#ifndef I_STRATEGY_H__
#define I_STRATEGY_H__
#include "data_types.h"
class Strategy{
public:
	virtual bool on_init()=0;
	virtual void on_order(Order *o)=0;
	virtual void on_quote(Quote *q)=0;
	virtual void on_error(Error *e)=0;
	virtual void on_notify(Notify *n)=0;
};
#endif /* I_STRATEGY_H__ */
