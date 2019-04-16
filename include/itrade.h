#ifndef I_TRADE_H__
#define I_TRADE_H__
class Config;
class Handler;
class Order;
class TradeChannel{
public:
	virtual bool open(Config*, Handler*)=0;
	virtual bool close()=0;
	virtual bool submit(Order*)=0;
	virtual bool cancel(Order*)=0;
};
#endif /* I_TRADE_H__ */
