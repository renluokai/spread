#ifndef CTP_QUOTE_CHANNEL_H__
#define CTP_QUOTE_CHANNEL_H__
#include <list>
#include <fstream>
#include <semaphore.h>
#include "ThostFtdcMdApi.h"
#include "../../include/handler.h"
#include "../../include/config.h"
#include "../../include/iquote.h"
#include "../../include/helper.h"

using std::list;

class CtpQuoteChannel: public QuoteChannel, public CThostFtdcMdSpi{
public:
	CtpQuoteChannel();
	~CtpQuoteChannel();
public:
	virtual bool open(Config*, Handler*);
	virtual bool close();
	virtual bool subscribe(const char*);
	virtual bool unsubscribe(const char*);	
public:
	virtual void OnFrontConnected();
	virtual void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRspSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData);
private:
	pthread_mutex_t mtx_;
	sem_t 			sem_;
	sem_t 			quit_sem_;
	sem_t 			request_sem_;

private:
	CThostFtdcMdApi	*quote_api_;
	Config			*cfg_;
	Handler			*handler_;
	bool			login_ok_;
	int				request_id_;
	std::fstream	log_stream_;
	std::list<char**>	instruments_;
};
#endif /* CTP_QUOTE_CHANNEL_H__ */
