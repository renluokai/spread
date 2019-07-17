#ifndef CTP_TRADE_CHANNEL_H__
#define CTP_TRADE_CHANNEL_H__
#include <map>
#include <set>
#include <string>
#include <fstream>
#include <semaphore.h>
#include "ThostFtdcTraderApi.h"
#include "../../include/handler.h"
#include "../../include/config.h"
#include "../../include/data_types.h"
#include "../../include/itrade.h"
#include "../../include/trader.h"

using std::map;
using std::string;
class CtpTradeChannel: public TradeChannel, public CThostFtdcTraderSpi
{
public:
	CtpTradeChannel();
public:
	virtual bool open(Config*, Handler*);
	virtual bool close();
	virtual bool submit(Order*);
	virtual bool cancel(Order*);
	virtual int GetTradingDay(){return tradingDay;};

private:

	bool DoQryTrade();
	bool DoQryPosition();
	bool DoQryPositionDetail();
	bool DoQryOrder();
	bool DoQryInvestor();
	bool DoQryInstrument();
	bool DoQrySettlementInfoConfirm();
	bool DoSettlementInfoConfirm ();

private:
	void Delay(int seconds);
	bool Wait(int seconds, const char* msg=NULL);
	char* GetOrderRef();
	void  NewOrder(CThostFtdcInputOrderField *o);	
	virtual void OnFrontConnected();
	virtual void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRspQryInvestor(CThostFtdcInvestorField *pInvestor, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRspOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnErrRtnOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo);
	virtual void OnRtnOrder(CThostFtdcOrderField *pOrder);
	virtual void OnRtnTrade(CThostFtdcTradeField *pTrade);
	virtual void OnRspQrySettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pInvestorPosition, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRspQryInvestorPositionDetail(CThostFtdcInvestorPositionDetailField *pInvestorPositionDetail, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRspQryOrder(CThostFtdcOrderField *pOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRspQryTrade(CThostFtdcTradeField *pTrade, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRspAuthenticate(CThostFtdcRspAuthenticateField *pRspAuthenticateField, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnFrontDisconnected(int nReason);
private:
	sem_t	sem_;	
	sem_t	request_sem_;	

private:
	CThostFtdcTraderApi	*trade_api_;
	Config				*cfg_;
	Trader				*trader_;
	Handler				*handler_;

	bool				login_ok_;
	int					request_id_;
	bool				SettlementInfoConfirmed;
	std::map<std::string, CThostFtdcInstrumentField*> ins_info_;
	std::fstream				log_stream_;
	std::map<std::string, int> order_ref_2_order_local_id;
	std::set<std::string> order_ref_has_inserted;//avoid report repeatly
private:
	TThostFtdcInvestorIDType investor_id_; //char[13]
	TThostFtdcSessionIDType session_id_; //int
	TThostFtdcFrontIDType front_id_; //int
	TThostFtdcOrderRefType max_order_ref_; //char[13]
	TThostFtdcOrderRefType order_ref_;
	int order_ref_init_;
	int order_ref_delta_;
	int order_ref_high_;
	int tradingDay;
};
#endif /* CTP_TRADE_CHANNEL_H__ */
