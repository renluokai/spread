#ifndef DATA_TYPES_H__
#define DATA_TYPES_H__
#include <string.h>
#include <string>
#include <stdio.h>
enum EOpenClose
{
	E_OPEN,
	E_CLOSE,
	E_CLOSE_T,
	E_CLOSE_Y,
	E_OPENCLOSE,
};

enum ELongShort
{
	E_LONG,
	E_SHORT,
	E_LONGSHORT,
};

enum EPositionType
{
	P_YESTERDAY_LONG,
	P_YESTERDAY_SHORT,
	P_TODAY_LONG,
	P_TODAY_SHORT,
	P_LONGSHORT,
};

enum EOrderState
{
	E_ORIGINAL,
	E_INSERT,
	E_REJECT,
	E_CANCEL,
	E_MATCH,
};


enum EDataType
{
	E_ORDER_TYPE,
	E_QUOTE_TYPE,
	E_ERROR_TYPE,
	E_NOTIFY_TYPE,
	E_COMMAND,
	E_DATA_TYPE,
};

struct Data
{
public:
	EDataType type;	
	virtual void clear_data(){};
};

struct traded_info_t
{
	int traded_volume_;
	int traded_price_;
};

struct Order : public Data{
	Order(){
		type = E_ORDER_TYPE;
		memset(instrument, 0, sizeof(instrument));
		memset(order_system_id, 0, sizeof(order_system_id));
		memset(state_msg, 0, sizeof(state_msg));
		submit_price = 0.0;
		total_matched = 0;
		canceled_volume = 0;
		open_close = E_OPENCLOSE;
		long_short = E_LONGSHORT;
		state = E_ORIGINAL;
		canceling = false;
		date = 0;
	}
	virtual void clear_data()
	{
	}
	void ShowOrder()
	{
		const char *tmp=NULL;
		if(open_close == E_OPEN){
			if(long_short == E_LONG){
				tmp = "OL";
			}else{
				tmp = "OS";
			}
		}else{
			if(long_short == E_LONG){
				tmp = "CL";
			}else{
				tmp = "CS";
			}
		}
		printf("%s\t%s\t%s\t%s\t%f\t[%s]\n",instrument, tmp, exchange_id, state_msg, submit_price, order_system_id);
	}
	char instrument[64];
	char order_system_id[24];
	char exchange_id[12];
	char state_msg[96];
	double submit_price;
	int submit_volume;
	int total_matched; //total matched volume
	EOpenClose open_close;
	ELongShort long_short;
	EOrderState state;
	int canceled_volume;
	int order_local_id;
	int insert_date;
	int insert_time;
	int date;
	bool canceling;

	int match_volume;//each match volume
	double match_price;//each match volume
	//traded_info_t *traded_info_;
};

struct Error : public Data
{
	virtual void clear_data()
	{
	}
};

struct Quote : public Data
{
	Quote()
	{
		type = E_QUOTE_TYPE;
		LastPrice = 0;
		AveragePrice = 0;
		BidPrice1 = 0;
		BidVolume1 = 0;
		AskPrice1 = 0;
		AskVolume1 = 0;
		memset(InstrumentID, 0, sizeof(InstrumentID));
		memset(UpdateTime, 0, sizeof(UpdateTime));
		memset(LocalTime, 0, sizeof(LocalTime));
	}
	virtual void clear_data()
	{
		LastPrice = 0;
		AveragePrice = 0;
		BidPrice1 = 0;
		BidVolume1 = 0;
		AskPrice1 = 0;
		AskVolume1 = 0;
		memset(InstrumentID, 0, sizeof(InstrumentID));
		memset(UpdateTime, 0, sizeof(UpdateTime));
		memset(LocalTime, 0, sizeof(LocalTime));
	}
	char InstrumentID[32];
	double LastPrice;
	double AveragePrice;
	double BidPrice1;
	int BidVolume1;
	double AskPrice1;
	int AskVolume1;
	char UpdateTime[24];
	char LocalTime[24];
};

struct Notify : public Data
{

	virtual void clear_data(){}
};

struct Command : public Data
{
	Command()
	{ 
		type = E_COMMAND;
		memset(buffer, 0 ,sizeof(buffer));
	}
	char buffer[256];
	Command& operator = (const Command& cmd){
		memcpy(buffer, cmd.buffer, sizeof(buffer));
	}
	virtual void clear_data()
	{
		memset(buffer, 0 ,sizeof(buffer));
	}
};

struct InstrumentInfo
{
	std::string InstrumentID;
	std::string ExchangeID;
	int			VolumeMultiple;
	double		PriceTick;
	int			MinBuyVolume;
	int			MinSellVolume;
};
#endif /* DATA_TYPES_H__ */
