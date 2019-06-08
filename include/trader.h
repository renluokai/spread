#ifndef TRADER_H__
#define TRADER_H__
#include <map>
#include <vector>
#include <string>
#include <fstream>
#include "istrategy.h"
#include "handler.h"
#include "data_types.h"
#include "../include/itrade.h"
#include "../include/iquote.h"
using namespace std;

class OrderManager;
class PositionManager;

class Trader{
public:
	static Trader* GetTrader();
public:
	Trader();
	Handler* get_handler();
public:
	bool run(Strategy *s);
	bool init();
	bool submit_order(Order*, int channel_id=0);
	bool cancel_order(Order*, int channel_id=0);
	bool RegisterQuoteChannel(QuoteChannel*, int id);
	bool RegisterTradeChannel(TradeChannel*, int id);

	int GetPosition(const char* ins, EPositionType posType);
	int GetLongPosition(const char* ins);
	int GetShortPosition(const char* ins);
	int GetYesterdayLongPosition(const char* ins);
	int GetYesterdayShortPosition(const char* ins);
	int GetTodayLongPosition(const char* ins);
	int GetTodayShortPosition(const char* ins);
	void UpdatePosition(string instrument, EOpenClose oc, ELongShort ls, int volume, double price, EPositionType pe=P_LONGSHORT);

	void GetOrder(const char* ins, EOpenClose oc, ELongShort ls, vector<Order*>& odVec);

	Order* NewOrder(const char* instrument, double price, int volume, EOpenClose oc, ELongShort ls);
	void log(const char* msg);
public:
	void add_instrument_info(InstrumentInfo*);
	InstrumentInfo* get_instrument_info(const char* ins); 
private:
	void process_command(Command*);

private:
	//manage all order
	OrderManager *orderManager;
	//manage all position
	PositionManager *positionManager;
	//data pool
	Handler		*handler;
private:
	Strategy	*strategy;
private:
	map<string, InstrumentInfo*> instrumentInfo;
	map<string, InstrumentInfo*>::iterator insInfoIter;
	map<int, QuoteChannel*> quoteChannels;
	map<int, TradeChannel*> tradeChannels;
private:
	pthread_t user_input_thread;	
	std::fstream log_stream;
private:
	static Trader* instance_;
};
#endif /* TRADER_H_  */
