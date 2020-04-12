#ifndef I_TRADE_H__
#define I_TRADE_H__
#include <memory>
class Config;
class Handler;
class Order;
using std::shared_ptr;
class TradeChannel{
public:
	virtual bool open(Config*, Handler*)=0;
	virtual bool close()=0;
	virtual bool submit(shared_ptr<Order>)=0;
	virtual bool cancel(shared_ptr<Order>)=0;
	virtual int GetTradingDay()=0;
};
#endif /* I_TRADE_H__ */
