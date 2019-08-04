#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <iostream>
#include "ctp_trade_channel.h"

using namespace std;
#define STRCPY(a,b) strncpy((a),(b),sizeof(a))
CtpTradeChannel::CtpTradeChannel()
{
	trade_api_ = NULL;
	cfg_ = NULL;
	login_ok_ = false;
	request_id_ = 0;
	SettlementInfoConfirmed = false;

	memset(investor_id_, 0, 13);
	session_id_  = 0;
	memset(max_order_ref_, 0, 13);
	order_ref_init_ = 0;
	order_ref_delta_ = 0;
	order_ref_high_ = 0;
#include <fstream>
	trader_ = Trader::GetTrader();
}

bool CtpTradeChannel::open(Config *cfg, Handler *hdlr)
{
	cfg_ = cfg;
	handler_ = hdlr;
	int ret = 0;
	ret = sem_init(&request_sem_, 0, 0);
	if (ret != 0)return false;
	ret = sem_init(&sem_, 0, 0);
	if (ret != 0)return false;

	time_t now_sec = time(NULL);
	tm tm_now = *localtime(&now_sec);
	char buffer[256] = {0};
	sprintf(buffer, "./ctp_trade_channel_%04d%02d%02d_%02d%02d%02d.log",
			tm_now.tm_year+1900, tm_now.tm_mon+1, tm_now.tm_mday,
			tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec);
	log_stream_.open(buffer, fstream::out|fstream::app);
	log_stream_<<"*****************CTP TRADE CHANNEL STARTED*****************"<<endl;
	trade_api_ = CThostFtdcTraderApi::CreateFtdcTraderApi("./");
	if(trade_api_ == NULL){
		return false;
	}
	trade_api_->RegisterSpi(this);
	trade_api_->RegisterFront(cfg_->front);
	trade_api_->SubscribePrivateTopic(THOST_TERT_QUICK);
	trade_api_->SubscribePublicTopic(THOST_TERT_QUICK);
	trade_api_->Init();

	Wait(30,"Init");
	
	bool bret = false;
	bret = DoQryInvestor();
	if(bret == false) return false;
#if 0
	bret = DoQrySettlementInfoConfirm();
	if(bret == false) return false;
#endif
	if(SettlementInfoConfirmed == false){
		bret = DoSettlementInfoConfirm();
		if(bret == false) return false;
	}
	bret = DoQryInstrument();
	if(bret == false) return false;

#if 0
	bret = DoQryOrder();
	if(bret == false) return false;


	bret = DoQryPosition();
	if(bret == false) return false;

//	bret = DoQryPositionDetail();
//	if(bret == false) return false;

	bret = DoQryTrade();
	if(bret == false) return false;
#endif
	return true;
}

bool CtpTradeChannel::close()
{
	return true;
}

bool CtpTradeChannel::submit(Order *o)
{
	CThostFtdcInputOrderField new_order={0};
	NewOrder(&new_order);
	
	cout<<o->instrument<<endl;
	STRCPY(new_order.ExchangeID,o->exchange_id);
	STRCPY(new_order.InstrumentID, o->instrument);
	new_order.LimitPrice = o->submit_price;
	new_order.VolumeTotalOriginal = o->submit_volume;
	
	switch(o->long_short){
		case E_LONG:
			if(o->open_close == E_OPEN){
				new_order.Direction = THOST_FTDC_D_Buy;
			}else{
				new_order.Direction = THOST_FTDC_D_Sell;
			}
			break;
		case E_SHORT:
			if(o->open_close == E_OPEN){
				new_order.Direction = THOST_FTDC_D_Sell;
			}else{
				new_order.Direction = THOST_FTDC_D_Buy;
			}
			break;
		default:
			break;
	}
	new_order.RequestID=request_id_;
	memset(new_order.CombOffsetFlag, 0, sizeof(new_order.CombOffsetFlag));
	switch(o->open_close){
		case E_OPEN:
			new_order.CombOffsetFlag[0] = THOST_FTDC_OF_Open;
			break;
		case E_CLOSE:
			if(strcmp(o->exchange_id,"SHFE")==0){
				new_order.CombOffsetFlag[0] = THOST_FTDC_OF_CloseToday;
			}else{
				new_order.CombOffsetFlag[0] = THOST_FTDC_OF_Close;
			}
			break;
		case E_CLOSE_T:
			new_order.CombOffsetFlag[0] = THOST_FTDC_OF_CloseToday;
			break;
		case E_CLOSE_Y:
			new_order.CombOffsetFlag[0]= THOST_FTDC_OF_CloseYesterday;
			break;
		default:
			break;
	}
	
	int ret = 0;
log_stream_<<"[ ReqOrderInsert] "
<<"BrokerID="<<new_order.BrokerID<<" | "
<<"InvestorID="<<new_order.InvestorID<<" | "
<<"InstrumentID="<<new_order.InstrumentID<<" | "
<<"OrderRef="<<new_order.OrderRef<<" | "
<<"UserID="<<new_order.UserID<<" | "
<<"OrderPriceType="<<new_order.OrderPriceType<<" | "
<<"Direction="<<new_order.Direction<<" | "
<<"CombOffsetFlag="<<new_order.CombOffsetFlag<<" | "
<<"CombHedgeFlag="<<new_order.CombHedgeFlag<<" | "
<<"LimitPrice="<<new_order.LimitPrice<<" | "
<<"VolumeTotalOriginal="<<new_order.VolumeTotalOriginal<<" | "
<<"TimeCondition="<<new_order.TimeCondition<<" | "
<<"GTDDate="<<new_order.GTDDate<<" | "
<<"VolumeCondition="<<new_order.VolumeCondition<<" | "
<<"MinVolume="<<new_order.MinVolume<<" | "
<<"ContingentCondition="<<new_order.ContingentCondition<<" | "
<<"StopPrice="<<new_order.StopPrice<<" | "
<<"ForceCloseReason="<<new_order.ForceCloseReason<<" | "
<<"IsAutoSuspend="<<new_order.IsAutoSuspend<<" | "
<<"BusinessUnit="<<new_order.BusinessUnit<<" | "
<<"RequestID="<<new_order.RequestID<<" | "
<<"UserForceClose="<<new_order.UserForceClose<<" | "
<<"IsSwapOrder="<<new_order.IsSwapOrder<<" | "
<<"ExchangeID="<<new_order.ExchangeID<<" | "
<<"InvestUnitID="<<new_order.InvestUnitID<<" | "
<<"AccountID="<<new_order.AccountID<<" | "
<<"CurrencyID="<<new_order.CurrencyID<<" | "
<<"ClientID="<<new_order.ClientID<<" | "
<<"IPAddress="<<new_order.IPAddress<<" | "
<<"MacAddress="<<new_order.MacAddress<<endl;

	ret = trade_api_->ReqOrderInsert(&new_order, request_id_++);
	order_ref_2_order_local_id.insert(std::make_pair(new_order.OrderRef, o->order_local_id));
	return ret == 0 ? true : false;
}

bool CtpTradeChannel::cancel(Order *o)
{
	CThostFtdcInputOrderActionField a = { 0 };
	STRCPY(a.BrokerID, cfg_->broker_id);
	STRCPY(a.InvestorID, investor_id_);
	STRCPY(a.UserID, cfg_->user);
	STRCPY(a.OrderSysID, o->order_system_id);  //对应要撤报单的OrderSysID
	STRCPY(a.ExchangeID, o->exchange_id);
	STRCPY(a.InstrumentID, o->instrument);
	a.ActionFlag = THOST_FTDC_AF_Delete;
	int ret = 0;
log_stream_<<"[ ReqOrderAction ] "
<<"BrokerID="<<a.BrokerID<<" | "
<<"InvestorID="<<a.InvestorID<<" | "
<<"UserID="<<a.UserID<<" | "
<<"OrderSysID="<<a.OrderSysID<<" | "
<<"ExchangeID="<<a.ExchangeID<<" | "
<<"InstrumentID="<<a.InstrumentID<<" | "
<<"ActionFlag="<<a.ActionFlag<<endl;
	ret = trade_api_->ReqOrderAction(&a, request_id_++);
	return ret == 0 ? true : false;
}

void CtpTradeChannel::OnFrontConnected()
{
	log_stream_<<'['<<__FUNCTION__<<']'<<endl;
	CThostFtdcReqAuthenticateField authField={0};

	STRCPY(authField.BrokerID, cfg_->broker_id);
	STRCPY(authField.UserID, cfg_->user);
	STRCPY(authField.AppID, cfg_->app_id);
	STRCPY(authField.AuthCode, cfg_->auth_code);
	trade_api_->ReqAuthenticate(&authField, request_id_++);
log_stream_<<"ReqAuthenticate ["<<authField.BrokerID<<"] ["<<authField.UserID<<"] ["<<authField.AppID<<"] ["<<authField.AuthCode<<"]\n";
	return;
}

void CtpTradeChannel::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
if(login_ok_==true){
	return;
}
if(login_ok_ == false && pRspUserLogin){
log_stream_<<'['<<__FUNCTION__<<']'<<"TradingDay="<<pRspUserLogin->TradingDay<<" | "
<<"LoginTime="<<pRspUserLogin->LoginTime<<" | "
<<"BrokerID="<<pRspUserLogin->BrokerID<<" | "
<<"UserID="<<pRspUserLogin->UserID<<" | "
<<"SystemName="<<pRspUserLogin->SystemName<<" | "
<<"FrontID="<<pRspUserLogin->FrontID<<" | "
<<"SessionID=|"<<pRspUserLogin->SessionID<<" | "
<<"MaxOrderRef="<<pRspUserLogin->MaxOrderRef<<" | "
<<"SHFETime="<<pRspUserLogin->SHFETime<<" | "
<<"DCETime="<<pRspUserLogin->DCETime<<" | "
<<"CZCETime="<<pRspUserLogin->CZCETime<<" | "
<<"FFEXTime="<<pRspUserLogin->FFEXTime<<" | "
<<"INETime="<<pRspUserLogin->INETime<<endl;
}
if(pRspInfo==NULL){
log_stream_<<"pRspInfo==NULL"<<" | ";
}else{
log_stream_<<"ErrorID="<<pRspInfo->ErrorID<<" ErrorMsg="<<pRspInfo->ErrorMsg<<" | ";
}
log_stream_<<" nRequestID="<<nRequestID<<" bIsLast="<<bIsLast<<endl;
	if(bIsLast == true){
		if(login_ok_ == false && (pRspInfo == NULL || pRspInfo->ErrorID == 0)){
			
			tradingDay = atoi(trade_api_->GetTradingDay());
			if(pRspUserLogin){
				session_id_ = pRspUserLogin->SessionID;
				front_id_ = pRspUserLogin->FrontID;
				STRCPY(max_order_ref_, pRspUserLogin->MaxOrderRef);
				struct timeval tv;
				gettimeofday(&tv, NULL);
				order_ref_high_ = tv.tv_usec;
				order_ref_init_ = atoi(max_order_ref_ )%1000000;
				login_ok_ = true;
				sem_post(&sem_);
			}
		}
	}
}

void CtpTradeChannel::OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if(pInputOrder != NULL){
log_stream_<<'['<<__FUNCTION__<<"] "<<"BrokerID="<<pInputOrder->BrokerID<<" | "
<<"InvestorID="<<pInputOrder->InvestorID<<" | "
<<"InstrumentID="<<pInputOrder->InstrumentID<<" | "
<<"OrderRef="<<pInputOrder->OrderRef<<" | "
<<"UserID="<<pInputOrder->UserID<<" | "
<<"OrderPriceType="<<pInputOrder->OrderPriceType<<" | "
<<"Direction="<<pInputOrder->Direction<<" | "
<<"CombOffsetFlag="<<pInputOrder->CombOffsetFlag<<" | "
<<"CombHedgeFlag="<<pInputOrder->CombHedgeFlag<<" | "
<<"LimitPrice="<<pInputOrder->LimitPrice<<" | "
<<"VolumeTotalOriginal="<<pInputOrder->VolumeTotalOriginal<<" | "
<<"TimeCondition="<<pInputOrder->TimeCondition<<" | "
<<"GTDDate="<<pInputOrder->GTDDate<<" | "
<<"VolumeCondition="<<pInputOrder->VolumeCondition<<" | "
<<"MinVolume="<<pInputOrder->MinVolume<<" | "
<<"ContingentCondition="<<pInputOrder->ContingentCondition<<" | "
<<"StopPrice="<<pInputOrder->StopPrice<<" | "
<<"ForceCloseReason="<<pInputOrder->ForceCloseReason<<" | "
<<"IsAutoSuspend="<<pInputOrder->IsAutoSuspend<<" | "
<<"BusinessUnit="<<pInputOrder->BusinessUnit<<" | "
<<"RequestID="<<pInputOrder->RequestID<<" | "
<<"UserForceClose="<<pInputOrder->UserForceClose<<" | "
<<"IsSwapOrder="<<pInputOrder->IsSwapOrder<<" | "
<<"ExchangeID="<<pInputOrder->ExchangeID<<" | "
<<"InvestUnitID="<<pInputOrder->InvestUnitID<<" | "
<<"AccountID="<<pInputOrder->AccountID<<" | "
<<"CurrencyID="<<pInputOrder->CurrencyID<<" | "
<<"ClientID="<<pInputOrder->ClientID<<" | "
<<"IPAddress="<<pInputOrder->IPAddress<<" | "
<<"MacAddress="<<pInputOrder->MacAddress<<" | ";
	}
	if(pRspInfo != NULL){
		log_stream_<<"ErrorID="<<pRspInfo->ErrorID<<" | "
		<<"ErrorMsg="<<pRspInfo->ErrorMsg;
	}
	log_stream_<<endl;
	log_stream_.flush();
}
void CtpTradeChannel::OnErrRtnOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo)
{

log_stream_<<"["<<__FUNCTION__<<"] "<<"BrokerID="<<pInputOrder->BrokerID<<" | "
<<"InvestorID="<<pInputOrder->InvestorID<<" | "
<<"InstrumentID="<<pInputOrder->InstrumentID<<" | "
<<"OrderRef="<<pInputOrder->OrderRef<<" | "
<<"UserID="<<pInputOrder->UserID<<" | "
<<"OrderPriceType="<<pInputOrder->OrderPriceType<<" | "
<<"Direction="<<pInputOrder->Direction<<" | "
<<"CombOffsetFlag="<<pInputOrder->CombOffsetFlag<<" | "
<<"CombHedgeFlag="<<pInputOrder->CombHedgeFlag<<" | "
<<"LimitPrice="<<pInputOrder->LimitPrice<<" | "
<<"VolumeTotalOriginal="<<pInputOrder->VolumeTotalOriginal<<" | "
<<"TimeCondition="<<pInputOrder->TimeCondition<<" | "
<<"GTDDate="<<pInputOrder->GTDDate<<" | "
<<"VolumeCondition="<<pInputOrder->VolumeCondition<<" | "
<<"MinVolume="<<pInputOrder->MinVolume<<" | "
<<"ContingentCondition="<<pInputOrder->ContingentCondition<<" | "
<<"StopPrice="<<pInputOrder->StopPrice<<" | "
<<"ForceCloseReason="<<pInputOrder->ForceCloseReason<<" | "
<<"IsAutoSuspend="<<pInputOrder->IsAutoSuspend<<" | "
<<"BusinessUnit="<<pInputOrder->BusinessUnit<<" | "
<<"RequestID="<<pInputOrder->RequestID<<" | "
<<"UserForceClose="<<pInputOrder->UserForceClose<<" | "
<<"IsSwapOrder="<<pInputOrder->IsSwapOrder<<" | "
<<"ExchangeID="<<pInputOrder->ExchangeID<<" | " <<"InvestUnitID="<<pInputOrder->InvestUnitID<<" | " <<"AccountID="<<pInputOrder->AccountID<<" | " <<"CurrencyID="<<pInputOrder->CurrencyID<<" | "
<<"ClientID="<<pInputOrder->ClientID<<" | "
<<"IPAddress="<<pInputOrder->IPAddress<<" | "
<<"MacAddress="<<pInputOrder->MacAddress<<" | ";

	if(pRspInfo != NULL){
		log_stream_<<"ErrorID="<<pRspInfo->ErrorID<<" | "
		<<"ErrorMsg="<<pRspInfo->ErrorMsg<<endl;
	}
	Order o;
	STRCPY(o.instrument, pInputOrder->InstrumentID);
	o.state = E_REJECT;
	o.order_local_id = order_ref_2_order_local_id[pInputOrder->OrderRef];
	STRCPY(o.state_msg, pRspInfo->ErrorMsg);
	handler_->push(&o);
	log_stream_<<endl;
}

void CtpTradeChannel::OnRtnOrder(CThostFtdcOrderField *pOrder)
{
log_stream_<<"["<<__FUNCTION__<<"] "<<"BrokerID="<<pOrder->BrokerID<<" | "
<<"InvestorID="<<pOrder->InvestorID<<" | "
<<"InstrumentID="<<pOrder->InstrumentID<<" | "
<<"OrderRef="<<pOrder->OrderRef<<" | "
<<"UserID="<<pOrder->UserID<<" | "
<<"OrderPriceType="<<pOrder->OrderPriceType<<" | "
<<"Direction="<<pOrder->Direction<<" | "
<<"CombOffsetFlag="<<pOrder->CombOffsetFlag<<" | "
<<"CombHedgeFlag="<<pOrder->CombHedgeFlag<<" | "
<<"LimitPrice="<<pOrder->LimitPrice<<" | "
<<"VolumeTotalOriginal="<<pOrder->VolumeTotalOriginal<<" | "
<<"TimeCondition="<<pOrder->TimeCondition<<" | "
<<"GTDDate="<<pOrder->GTDDate<<" | "
<<"VolumeCondition="<<pOrder->VolumeCondition<<" | "
<<"MinVolume="<<pOrder->MinVolume<<" | "
<<"ContingentCondition="<<pOrder->ContingentCondition<<" | "
<<"StopPrice="<<pOrder->StopPrice<<" | "
<<"ForceCloseReason="<<pOrder->ForceCloseReason<<" | "
<<"IsAutoSuspend="<<pOrder->IsAutoSuspend<<" | "
<<"BusinessUnit="<<pOrder->BusinessUnit<<" | "
<<"RequestID="<<pOrder->RequestID<<" | "
<<"OrderLocalID="<<pOrder->OrderLocalID<<" | "
<<"ExchangeID="<<pOrder->ExchangeID<<" | "
<<"ParticipantID="<<pOrder->ParticipantID<<" | "
<<"ClientID="<<pOrder->ClientID<<" | "
<<"ExchangeInstID="<<pOrder->ExchangeInstID<<" | "
<<"TraderID="<<pOrder->TraderID<<" | "
<<"InstallID="<<pOrder->InstallID<<" | "
<<"OrderSubmitStatus="<<pOrder->OrderSubmitStatus<<" | "
<<"NotifySequence="<<pOrder->NotifySequence<<" | "
<<"TradingDay="<<pOrder->TradingDay<<" | "
<<"SettlementID="<<pOrder->SettlementID<<" | "
<<"OrderSysID="<<pOrder->OrderSysID<<" | "
<<"OrderSource="<<pOrder->OrderSource<<" | "
<<"OrderStatus="<<pOrder->OrderStatus<<" | "
<<"OrderType="<<pOrder->OrderType<<" | "
<<"VolumeTraded="<<pOrder->VolumeTraded<<" | "
<<"VolumeTotal="<<pOrder->VolumeTotal<<" | "
<<"InsertDate="<<pOrder->InsertDate<<" | "
<<"InsertTime="<<pOrder->InsertTime<<" | "
<<"ActiveTime="<<pOrder->ActiveTime<<" | "
<<"SuspendTime="<<pOrder->SuspendTime<<" | "
<<"UpdateTime="<<pOrder->UpdateTime<<" | "
<<"CancelTime="<<pOrder->CancelTime<<" | "
<<"ActiveTraderID="<<pOrder->ActiveTraderID<<" | "
<<"ClearingPartID="<<pOrder->ClearingPartID<<" | "
<<"SequenceNo="<<pOrder->SequenceNo<<" | "
<<"FrontID="<<pOrder->FrontID<<" | "
<<"SessionID="<<pOrder->SessionID<<" | "
<<"UserProductInfo="<<pOrder->UserProductInfo<<" | "
<<"StatusMsg="<<pOrder->StatusMsg<<" | "
<<"UserForceClose="<<pOrder->UserForceClose<<" | "
<<"ActiveUserID="<<pOrder->ActiveUserID<<" | "
<<"BrokerOrderSeq="<<pOrder->BrokerOrderSeq<<" | "
<<"RelativeOrderSysID="<<pOrder->RelativeOrderSysID<<" | "
<<"ZCETotalTradedVolume="<<pOrder->ZCETotalTradedVolume<<" | "
<<"IsSwapOrder="<<pOrder->IsSwapOrder<<" | "
<<"BranchID="<<pOrder->BranchID<<" | "
<<"InvestUnitID="<<pOrder->InvestUnitID<<" | "
<<"AccountID="<<pOrder->AccountID<<" | "
<<"CurrencyID="<<pOrder->CurrencyID<<" | "
<<"IPAddress="<<pOrder->IPAddress<<" | "
<<"MacAddress="<<pOrder->MacAddress<<endl;
		Order o;

		STRCPY(o.instrument, pOrder->InstrumentID);
		if(pOrder->CombOffsetFlag[0] == THOST_FTDC_OF_Open){
			o.open_close =  E_OPEN;
			if(pOrder->Direction == THOST_FTDC_D_Buy){
				o.long_short = E_LONG;
			}else{
				o.long_short = E_SHORT;
			}
		}else{
			if(pOrder->CombOffsetFlag[0] == THOST_FTDC_OF_Close){
				o.open_close = E_CLOSE;
			}else if(pOrder->CombOffsetFlag[0] == THOST_FTDC_OF_CloseToday){
				o.open_close = E_CLOSE_T;
			}else if(pOrder->CombOffsetFlag[0] == THOST_FTDC_OF_CloseYesterday){
				o.open_close = E_CLOSE_Y;
			}else{
				o.open_close = E_CLOSE;
			}

			if(pOrder->Direction == THOST_FTDC_D_Buy){
				o.long_short = E_SHORT;
			}else{
				o.long_short = E_LONG;
			}
		}
		o.submit_price = pOrder->LimitPrice;
		o.order_local_id = order_ref_2_order_local_id[pOrder->OrderRef];
		STRCPY(o.state_msg, pOrder->StatusMsg);
		if((pOrder->OrderSubmitStatus == THOST_FTDC_OSS_InsertSubmitted
		|| pOrder->OrderSubmitStatus == THOST_FTDC_OSS_Accepted)
		&& (strlen(pOrder->OrderSysID) > 0)
		&& (order_ref_has_inserted.count(pOrder->OrderRef) == 0)){
			order_ref_has_inserted.insert(pOrder->OrderRef);
			o.state = E_INSERT;
			STRCPY(o.order_system_id, pOrder->OrderSysID);
			o.submit_volume = pOrder->VolumeTotalOriginal;
			log_stream_<<"REPORT insert"<<endl;
		}else if((pOrder->OrderSubmitStatus == THOST_FTDC_OSS_InsertRejected
		||pOrder->OrderSubmitStatus ==THOST_FTDC_OSS_CancelRejected)
		&& pOrder->OrderStatus == THOST_FTDC_OST_Canceled){
			o.state = E_REJECT;
			log_stream_<<"REPORT reject"<<endl;
		}else if( pOrder->OrderSubmitStatus == THOST_FTDC_OSS_Accepted
		&& pOrder->OrderStatus == THOST_FTDC_OST_Canceled){
			o.state = E_CANCEL;
			o.canceled_volume = pOrder->VolumeTotal - pOrder->VolumeTraded;
			log_stream_<<"REPORT cancel"<<endl;
		}
		handler_->push(&o);
}
#if 1

void CtpTradeChannel::OnRtnTrade(CThostFtdcTradeField *pTrade)
{
log_stream_<<"["<<__FUNCTION__<<"] "<<"BrokerID="<<pTrade->BrokerID<<" | "
<<"InvestorID="<<pTrade->InvestorID<<" | "
<<"InstrumentID="<<pTrade->InstrumentID<<" | "
<<"OrderRef="<<pTrade->OrderRef<<" | "
<<"UserID="<<pTrade->UserID<<" | "
<<"ExchangeID="<<pTrade->ExchangeID<<" | "
<<"TradeID="<<pTrade->TradeID<<" | "
<<"Direction="<<pTrade->Direction<<" | "
<<"OrderSysID="<<pTrade->OrderSysID<<" | "
<<"ParticipantID="<<pTrade->ParticipantID<<" | "
<<"ClientID="<<pTrade->ClientID<<" | "
<<"TradingRole="<<pTrade->TradingRole<<" | "
<<"ExchangeInstID="<<pTrade->ExchangeInstID<<" | "
<<"OffsetFlag="<<pTrade->OffsetFlag<<" | "
<<"HedgeFlag="<<pTrade->HedgeFlag<<" | "
<<"Price="<<pTrade->Price<<" | "
<<"Volume="<<pTrade->Volume<<" | "
<<"TradeDate="<<pTrade->TradeDate<<" | "
<<"TradeTime="<<pTrade->TradeTime<<" | "
<<"TradeType="<<pTrade->TradeType<<" | "
<<"PriceSource="<<pTrade->PriceSource<<" | "
<<"TraderID="<<pTrade->TraderID<<" | "
<<"OrderLocalID="<<pTrade->OrderLocalID<<" | "
<<"ClearingPartID="<<pTrade->ClearingPartID<<" | "
<<"BusinessUnit="<<pTrade->BusinessUnit<<" | "
<<"SequenceNo="<<pTrade->SequenceNo<<" | "
<<"TradingDay="<<pTrade->TradingDay<<" | "
<<"SettlementID="<<pTrade->SettlementID<<" | "
<<"BrokerOrderSeq="<<pTrade->BrokerOrderSeq<<" | "
<<"TradeSource="<<pTrade->TradeSource<<endl;
	Order o;
	o.state = E_MATCH;
	STRCPY(o.instrument, pTrade->InstrumentID);
	o.order_local_id = order_ref_2_order_local_id[pTrade->OrderRef];	
	o.match_volume = pTrade->Volume;
	o.match_price = pTrade->Price;
	o.date = atoi(pTrade->TradingDay);

	if(pTrade->OffsetFlag == THOST_FTDC_OF_Open){
		if(pTrade->Direction==THOST_FTDC_D_Buy){
			o.long_short = E_LONG;
		}else{
			o.long_short = E_SHORT;
		}
	}else{
		if(pTrade->Direction==THOST_FTDC_D_Buy){
			o.long_short = E_SHORT;
		}else{
			o.long_short = E_LONG;
		}
	}
	switch(pTrade->OffsetFlag){
		case THOST_FTDC_OF_Open:
			o.open_close = E_OPEN;
			break;
		case THOST_FTDC_OF_Close:
			o.open_close = E_CLOSE;
			break;
		case THOST_FTDC_OF_CloseToday:
			o.open_close = E_CLOSE_T;
			break;
		case THOST_FTDC_OF_CloseYesterday:
			o.open_close = E_CLOSE_Y;
			break;
		default:
			cout<<"ERROR LONG SHORT"<<endl;
			break;
	}
	handler_->push(&o);
}
#endif
char* CtpTradeChannel::GetOrderRef()
{
	memset(order_ref_, 0 , 13);
	int ref = order_ref_init_ + order_ref_delta_++;
	sprintf(order_ref_, "%06d%06d", order_ref_high_, ref);	
	return order_ref_;
}
void CtpTradeChannel::NewOrder(CThostFtdcInputOrderField *o)
{
	///经纪公司代码
	STRCPY(o->BrokerID, cfg_->broker_id);
	///投资者代码
	STRCPY(o->InvestorID, investor_id_);
	///合约代码
	//STRCPY(o->InstrumentID, "cu1905");
	///报单引用
	STRCPY(o->OrderRef, GetOrderRef());
	cout<<"OrderRef is:"<<o->OrderRef<<endl;
	cout<<"time is:"<<time(NULL)<<endl;
	///用户代码
	STRCPY(o->UserID, cfg_->user);
	///报单价格条件
	o->OrderPriceType = THOST_FTDC_OPT_LimitPrice;///限价
	///买卖方向
	//	TThostFtdcDirectionType	Direction;
	///组合开平标志
	//	TThostFtdcCombOffsetFlagType	CombOffsetFlag;
	///组合投机套保标志
	o->CombHedgeFlag[0] =  THOST_FTDC_BHF_Speculation;
	///价格
	//	TThostFtdcPriceType	LimitPrice;
	///数量
	//TThostFtdcVolumeType	VolumeTotalOriginal;
	///有效期类型
	o->TimeCondition = THOST_FTDC_TC_GFD;
	///GTD日期
	STRCPY(o->GTDDate,"");
	///成交量类型
	o->VolumeCondition = THOST_FTDC_VC_AV;
	///最小成交量
	o->MinVolume = 1;
	///触发条件
	o->ContingentCondition = THOST_FTDC_CC_Immediately;
	///止损价
	o->StopPrice = 0;
	///强平原因
	o->ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
	///自动挂起标志
	o->IsAutoSuspend = 0;
	///业务单元
	TThostFtdcBusinessUnitType	BusinessUnit;
	///请求编号
	//TThostFtdcRequestIDType	RequestID;
	///用户强评标志
	TThostFtdcBoolType	UserForceClose;
	///互换单标志
	TThostFtdcBoolType	IsSwapOrder;
	///交易所代码
	///TThostFtdcExchangeIDType	ExchangeID;
	///投资单元代码
	TThostFtdcInvestUnitIDType	InvestUnitID;
	///资金账号
	TThostFtdcAccountIDType	AccountID;
	///币种代码
	TThostFtdcCurrencyIDType	CurrencyID;
	///交易编码
	//TThostFtdcClientIDType	ClientID;
	///IP地址
	//TThostFtdcIPAddressType	IPAddress;
	///Mac地址
	//TThostFtdcMacAddressType	MacAddress;
}

void CtpTradeChannel::OnRspQryInvestor(CThostFtdcInvestorField *pInvestor, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{

	if(pInvestor != NULL){
log_stream_<<"["<<__FUNCTION__<<"] "<<"InvestorID="<<pInvestor->InvestorID<<" | "
<<"BrokerID="<<pInvestor->BrokerID<<" | "
<<"InvestorGroupID="<<pInvestor->InvestorGroupID<<" | "
<<"InvestorName="<<pInvestor->InvestorName<<" | "
<<"IdentifiedCardType="<<pInvestor->IdentifiedCardType<<" | "
<<"IdentifiedCardNo="<<pInvestor->IdentifiedCardNo<<" | "
<<"IsActive="<<pInvestor->IsActive<<" | "
<<"Telephone="<<pInvestor->Telephone<<" | "
<<"Address="<<pInvestor->Address<<" | "
<<"OpenDate="<<pInvestor->OpenDate<<" | "
<<"Mobile="<<pInvestor->Mobile<<" | "
<<"CommModelID="<<pInvestor->CommModelID<<" | "
<<"MarginModelID="<<pInvestor->MarginModelID<<endl;
		STRCPY(investor_id_, pInvestor->InvestorID);
	}
	if(bIsLast == true){
		if(pRspInfo == NULL || pRspInfo->ErrorID == 0){
			sem_post(&sem_);
		}
	}
}

void CtpTradeChannel::OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if(pInstrument){
		CThostFtdcInstrumentField *ins= new CThostFtdcInstrumentField;
		memcpy(ins, pInstrument, sizeof(CThostFtdcInstrumentField));
		ins_info_.insert(make_pair(string(ins->InstrumentID), ins));

		InstrumentInfo instInfo;
		instInfo.InstrumentID 	= std::string(pInstrument->InstrumentID);
		instInfo.ExchangeID		= std::string(pInstrument->ExchangeID);
		instInfo.VolumeMultiple	= pInstrument->VolumeMultiple;
		instInfo.PriceTick		= pInstrument->PriceTick;
		//instInfo.MinBuyVolume	= pInstrument->MinBuyVolume;
		//instInfo.MinSellVolume	= pInstrument->MinSellVolume;
		trader_->add_instrument_info(&instInfo);
log_stream_<<"["<<__FUNCTION__<<"] "<<"InstrumentID="<<pInstrument->InstrumentID<<" | "
<<"ExchangeID="<<pInstrument->ExchangeID<<" | "
<<"InstrumentName="<<pInstrument->InstrumentName<<" | "
<<"ExchangeInstID="<<pInstrument->ExchangeInstID<<" | "
<<"ProductID="<<pInstrument->ProductID<<" | "
<<"ProductClass="<<pInstrument->ProductClass<<" | "
<<"DeliveryYear="<<pInstrument->DeliveryYear<<" | "
<<"DeliveryMonth="<<pInstrument->DeliveryMonth<<" | "
<<"MaxMarketOrderVolume="<<pInstrument->MaxMarketOrderVolume<<" | "
<<"MinMarketOrderVolume="<<pInstrument->MinMarketOrderVolume<<" | "
<<"MaxLimitOrderVolume="<<pInstrument->MaxLimitOrderVolume<<" | "
<<"MinLimitOrderVolume="<<pInstrument->MinLimitOrderVolume<<" | "
<<"VolumeMultiple="<<pInstrument->VolumeMultiple<<" | "
<<"PriceTick="<<pInstrument->PriceTick<<" | "
<<"CreateDate="<<pInstrument->CreateDate<<" | "
<<"OpenDate="<<pInstrument->OpenDate<<" | "
<<"ExpireDate="<<pInstrument->ExpireDate<<" | "
<<"StartDelivDate="<<pInstrument->StartDelivDate<<" | "
<<"EndDelivDate="<<pInstrument->EndDelivDate<<" | "
<<"InstLifePhase="<<pInstrument->InstLifePhase<<" | "
<<"IsTrading="<<pInstrument->IsTrading<<" | "
<<"PositionType="<<pInstrument->PositionType<<" | "
<<"PositionDateType="<<pInstrument->PositionDateType<<" | "
<<"LongMarginRatio="<<pInstrument->LongMarginRatio<<" | "
<<"ShortMarginRatio="<<pInstrument->ShortMarginRatio<<" | "
<<"MaxMarginSideAlgorithm="<<pInstrument->MaxMarginSideAlgorithm<<" | "
<<"UnderlyingInstrID="<<pInstrument->UnderlyingInstrID<<" | "
<<"StrikePrice="<<pInstrument->StrikePrice<<" | "
<<"OptionsType="<<pInstrument->OptionsType<<" | "
<<"UnderlyingMultiple="<<pInstrument->UnderlyingMultiple<<" | "
<<"CombinationType="<<pInstrument->CombinationType<<" | ";

if(pRspInfo){
log_stream_<<"pRspInfo->ErrorID="<<pRspInfo->ErrorID<<" | "
<<"pRspInfo->ErrorMsg="<<pRspInfo->ErrorMsg<<" | ";
}
log_stream_<<"bIslast="<<bIsLast<<endl;
	};
	if(bIsLast == true){
		cout<<"Unlock sem"<<endl;
		sem_post(&sem_);
	}
}
void CtpTradeChannel::OnRspQrySettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if(SettlementInfoConfirmed==false && pSettlementInfoConfirm != NULL){
log_stream_<<"["<<__FUNCTION__<<"] "<<"BrokerID="<<pSettlementInfoConfirm->BrokerID<<" | "
<<"InvestorID="<<pSettlementInfoConfirm->InvestorID<<" | "
<<"ConfirmDate="<<pSettlementInfoConfirm->ConfirmDate<<" | "
<<"ConfirmTime="<<pSettlementInfoConfirm->ConfirmTime<<endl;
		SettlementInfoConfirmed = true;
		sem_post(&sem_);
	}
	
}
void CtpTradeChannel::OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if(SettlementInfoConfirmed==false
	&& pSettlementInfoConfirm != NULL){
log_stream_<<"["<<__FUNCTION__<<"] "<<"BrokerID="<<pSettlementInfoConfirm->BrokerID<<" | "
<<"InvestorID="<<pSettlementInfoConfirm->InvestorID<<" | "
<<"ConfirmDate="<<pSettlementInfoConfirm->ConfirmDate<<" | "
<<"ConfirmTime="<<pSettlementInfoConfirm->ConfirmTime<<endl;
		SettlementInfoConfirmed = true;
		sem_post(&sem_);
	}
}
void CtpTradeChannel::Delay(int seconds)
{
	sleep(seconds);
}
bool CtpTradeChannel::Wait(int seconds, const char* msg)
{
	int ret;
	struct timespec ts;	
	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_sec += seconds;
	ret = sem_timedwait(&sem_, &ts);
	perror(msg);
	if(ret != 0){
		return false;
	}
	return true;
}

bool CtpTradeChannel::DoQryInvestor()
{
	Delay(1);
	CThostFtdcQryInvestorField QryInvestor={0};
log_stream_<<"["<<__FUNCTION__<<"]"<<endl;
	trade_api_->ReqQryInvestor(&QryInvestor, request_id_++);	
	return Wait(30, "ReqQryInvestor");
}

bool CtpTradeChannel::DoQryInstrument()
{
	Delay(1);
	CThostFtdcQryInstrumentField qry_ins = {0};
log_stream_<<"["<<__FUNCTION__<<"]"<<endl;
	trade_api_->ReqQryInstrument(&qry_ins, request_id_++);
	return Wait(30, "ReqQryInstrument");
}

bool CtpTradeChannel::DoQryOrder()
{
	Delay(1);
	CThostFtdcQryOrderField qryOrder={0};
log_stream_<<"["<<__FUNCTION__<<"]"<<endl;
	trade_api_->ReqQryOrder(&qryOrder, request_id_++);
	return Wait(30, "ReqQryOrder");
}
bool CtpTradeChannel::DoQryPosition()
{
	Delay(1);
	CThostFtdcQryInvestorPositionField Position={0};
log_stream_<<"["<<__FUNCTION__<<"]"<<endl;
	trade_api_->ReqQryInvestorPosition(&Position, request_id_++);
	return Wait(30, "ReqQryInvestorPosition");
}

bool CtpTradeChannel::DoQryPositionDetail()
{
	Delay(1);
CThostFtdcQryInvestorPositionDetailField PositionDetail={0};
	trade_api_->ReqQryInvestorPositionDetail(&PositionDetail,request_id_++);
	return Wait(30, "ReqQryInvestorPositionDetail");
}
bool CtpTradeChannel::DoQrySettlementInfoConfirm ()
{
	Delay(1);
	CThostFtdcQrySettlementInfoConfirmField qry_cfm = {0};
	STRCPY(qry_cfm.BrokerID, cfg_->broker_id);
	STRCPY(qry_cfm.InvestorID, investor_id_);

log_stream_<<"["<<__FUNCTION__<<"]"<<endl;
	trade_api_->ReqQrySettlementInfoConfirm(&qry_cfm, request_id_++);
	return Wait(30,"ReqQrySettlementInfoConfirm");
}

bool CtpTradeChannel::DoSettlementInfoConfirm ()
{
	CThostFtdcSettlementInfoConfirmField settl_cfm = {0};
	STRCPY(settl_cfm.BrokerID, cfg_->broker_id);
	STRCPY(settl_cfm.InvestorID, investor_id_);
	///确认日期
	TThostFtdcDateType	ConfirmDate;
	///确认时间
	TThostFtdcTimeType	ConfirmTime;
log_stream_<<"["<<__FUNCTION__<<"]"<<endl;
	trade_api_->ReqSettlementInfoConfirm(&settl_cfm, request_id_++);
	Wait(30,"ReqSettlementInfoConfirm");
}

void CtpTradeChannel::OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pInvestorPosition, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
if(pInvestorPosition == NULL){
	log_stream_<<"["<<__FUNCTION__<<"] "<<"pInvestorPosition=NULL"<<endl;
}else{
log_stream_<<"["<<__FUNCTION__<<"] "
<<"InstrumentID="<<pInvestorPosition->InstrumentID<<" | "
<<"BrokerID="<<pInvestorPosition->BrokerID<<" | "
<<"InvestorID="<<pInvestorPosition->InvestorID<<" | "
<<"PosiDirection="<<pInvestorPosition->PosiDirection<<" | "
<<"HedgeFlag="<<pInvestorPosition->HedgeFlag<<" | "
<<"PositionDate="<<pInvestorPosition->PositionDate<<" | "
<<"YdPosition="<<pInvestorPosition->YdPosition<<" | "
<<"Position="<<pInvestorPosition->Position<<" | "
<<"LongFrozen="<<pInvestorPosition->LongFrozen<<" | "
<<"ShortFrozen="<<pInvestorPosition->ShortFrozen<<" | "
<<"LongFrozenAmount="<<pInvestorPosition->LongFrozenAmount<<" | "
<<"ShortFrozenAmount="<<pInvestorPosition->ShortFrozenAmount<<" | "
<<"OpenVolume="<<pInvestorPosition->OpenVolume<<" | "
<<"CloseVolume="<<pInvestorPosition->CloseVolume<<" | "
<<"OpenAmount="<<pInvestorPosition->OpenAmount<<" | "
<<"CloseAmount="<<pInvestorPosition->CloseAmount<<" | "
<<"PositionCost="<<pInvestorPosition->PositionCost<<" | "
<<"PreMargin="<<pInvestorPosition->PreMargin<<" | "
<<"UseMargin="<<pInvestorPosition->UseMargin<<" | "
<<"FrozenMargin="<<pInvestorPosition->FrozenMargin<<" | "
<<"FrozenCash="<<pInvestorPosition->FrozenCash<<" | "
<<"FrozenCommission="<<pInvestorPosition->FrozenCommission<<" | "
<<"CashIn="<<pInvestorPosition->CashIn<<" | "
<<"Commission="<<pInvestorPosition->Commission<<" | "
<<"CloseProfit="<<pInvestorPosition->CloseProfit<<" | "
<<"PositionProfit="<<pInvestorPosition->PositionProfit<<" | "
<<"PreSettlementPrice="<<pInvestorPosition->PreSettlementPrice<<" | "
<<"SettlementPrice="<<pInvestorPosition->SettlementPrice<<" | "
<<"TradingDay="<<pInvestorPosition->TradingDay<<" | "
<<"SettlementID="<<pInvestorPosition->SettlementID<<" | "
<<"OpenCost="<<pInvestorPosition->OpenCost<<" | "
<<"ExchangeMargin="<<pInvestorPosition->ExchangeMargin<<" | "
<<"CombPosition="<<pInvestorPosition->CombPosition<<" | "
<<"CombLongFrozen="<<pInvestorPosition->CombLongFrozen<<" | "
<<"CombShortFrozen="<<pInvestorPosition->CombShortFrozen<<" | "
<<"CloseProfitByDate="<<pInvestorPosition->CloseProfitByDate<<" | "
<<"CloseProfitByTrade="<<pInvestorPosition->CloseProfitByTrade<<" | "
<<"TodayPosition="<<pInvestorPosition->TodayPosition<<" | "
<<"MarginRateByMoney="<<pInvestorPosition->MarginRateByMoney<<" | "
<<"MarginRateByVolume="<<pInvestorPosition->MarginRateByVolume<<" | "
<<"StrikeFrozen="<<pInvestorPosition->StrikeFrozen<<" | "
<<"StrikeFrozenAmount="<<pInvestorPosition->StrikeFrozenAmount<<" | "
<<"AbandonFrozen="<<pInvestorPosition->AbandonFrozen<<" | "
<<"ExchangeID="<<pInvestorPosition->ExchangeID<<" | "
<<"YdStrikeFrozen="<<pInvestorPosition->YdStrikeFrozen<<endl;
if(pInvestorPosition->YdPosition > 0){
	trader_->UpdateYesterdayPosition(pInvestorPosition->InstrumentID, pInvestorPosition->PosiDirection ==THOST_FTDC_PD_Long?E_LONG:E_SHORT, pInvestorPosition->YdPosition, pInvestorPosition->PreSettlementPrice);
}
}
	if(bIsLast == true){
		if(pRspInfo == NULL || pRspInfo->ErrorID == 0){
			sem_post(&sem_);
		}
	}
}

bool CtpTradeChannel::DoQryTrade()
{
	Delay(1);
	CThostFtdcQryTradeField qryTrade={0};
	trade_api_->ReqQryTrade(&qryTrade,request_id_++);
	return Wait(60, "ReqQryTrade");
}
void CtpTradeChannel::OnRspQryTrade(CThostFtdcTradeField *pTrade, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
if(pTrade==NULL){
log_stream_<<"["<<__FUNCTION__<<"] "<<"pTrade=NULL"<<endl;
}else{
log_stream_<<"["<<__FUNCTION__<<"] "
<<"BrokerID="<<pTrade->BrokerID<<" | "
<<"InvestorID="<<pTrade->InvestorID<<" | "
<<"InstrumentID="<<pTrade->InstrumentID<<" | "
<<"OrderRef="<<pTrade->OrderRef<<" | "
<<"UserID="<<pTrade->UserID<<" | "
<<"ExchangeID="<<pTrade->ExchangeID<<" | "
<<"TradeID="<<pTrade->TradeID<<" | "
<<"Direction="<<pTrade->Direction<<" | "
<<"OrderSysID="<<pTrade->OrderSysID<<" | "
<<"ParticipantID="<<pTrade->ParticipantID<<" | "
<<"ClientID="<<pTrade->ClientID<<" | "
<<"TradingRole="<<pTrade->TradingRole<<" | "
<<"ExchangeInstID="<<pTrade->ExchangeInstID<<" | "
<<"OffsetFlag="<<pTrade->OffsetFlag<<" | "
<<"HedgeFlag="<<pTrade->HedgeFlag<<" | "
<<"Price="<<pTrade->Price<<" | "
<<"Volume="<<pTrade->Volume<<" | "
<<"TradeDate="<<pTrade->TradeDate<<" | "
<<"TradeTime="<<pTrade->TradeTime<<" | "
<<"TradeType="<<pTrade->TradeType<<" | "
<<"PriceSource="<<pTrade->PriceSource<<" | "
<<"TraderID="<<pTrade->TraderID<<" | "
<<"OrderLocalID="<<pTrade->OrderLocalID<<" | "
<<"ClearingPartID="<<pTrade->ClearingPartID<<" | "
<<"BusinessUnit="<<pTrade->BusinessUnit<<" | "
<<"SequenceNo="<<pTrade->SequenceNo<<" | "
<<"TradingDay="<<pTrade->TradingDay<<" | "
<<"SettlementID="<<pTrade->SettlementID<<" | "
<<"BrokerOrderSeq="<<pTrade->BrokerOrderSeq<<" | "
<<"TradeSource="<<pTrade->TradeSource<<endl;
	string ins = pTrade->InstrumentID;
	EOpenClose oc;
	ELongShort ls;
	if(pTrade->OffsetFlag == THOST_FTDC_OF_Open){
		oc = E_OPEN;
		if(pTrade->Direction==THOST_FTDC_D_Buy){
			ls = E_LONG;
		}else{
			ls = E_SHORT;
		}
	}else{
		if(pTrade->Direction==THOST_FTDC_D_Buy){
			ls = E_SHORT;
		}else{
			ls = E_LONG;
		}
		if(pTrade->OffsetFlag == THOST_FTDC_OF_Close){
			oc = E_CLOSE;
		}else if(pTrade->OffsetFlag == THOST_FTDC_OF_ForceClose){
			oc = E_CLOSE;
		}else if(pTrade->OffsetFlag == THOST_FTDC_OF_CloseToday){
			oc = E_CLOSE_T;
		}else if(pTrade->OffsetFlag == THOST_FTDC_OF_CloseYesterday){
			oc = E_CLOSE_Y;
		}else if(pTrade->OffsetFlag == THOST_FTDC_OF_ForceOff){
			oc = E_CLOSE;
		}else if(pTrade->OffsetFlag == THOST_FTDC_OF_LocalForceClose){
			oc = E_CLOSE;
		}
	}
	int volume = pTrade->Volume;
	double price = pTrade->Price;
	trader_->UpdateQryMatch(ins, oc, ls, volume, price);
}
	if(bIsLast == true){
		if(pRspInfo == NULL || pRspInfo->ErrorID == 0){
			sem_post(&sem_);
		}
	}
}
void CtpTradeChannel::OnRspQryInvestorPositionDetail(CThostFtdcInvestorPositionDetailField *pInvestorPositionDetail, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
if(pInvestorPositionDetail==NULL){
log_stream_<<"["<<__FUNCTION__<<"] "<<"pInvestorPositionDetail=NULL"<<endl;
}else{
log_stream_<<"["<<__FUNCTION__<<"] "
<<"InstrumentID="<<pInvestorPositionDetail->InstrumentID<<" | "
<<"BrokerID="<<pInvestorPositionDetail->BrokerID<<" | "
<<"InvestorID="<<pInvestorPositionDetail->InvestorID<<" | "
<<"HedgeFlag="<<pInvestorPositionDetail->HedgeFlag<<" | "
<<"Direction="<<pInvestorPositionDetail->Direction<<" | "
<<"OpenDate="<<pInvestorPositionDetail->OpenDate<<" | "
<<"TradeID="<<pInvestorPositionDetail->TradeID<<" | "
<<"Volume="<<pInvestorPositionDetail->Volume<<" | "
<<"OpenPrice="<<pInvestorPositionDetail->OpenPrice<<" | "
<<"TradingDay="<<pInvestorPositionDetail->TradingDay<<" | "
<<"SettlementID="<<pInvestorPositionDetail->SettlementID<<" | "
<<"TradeType="<<pInvestorPositionDetail->TradeType<<" | "
<<"CombInstrumentID="<<pInvestorPositionDetail->CombInstrumentID<<" | "
<<"ExchangeID="<<pInvestorPositionDetail->ExchangeID<<" | "
<<"CloseProfitByDate="<<pInvestorPositionDetail->CloseProfitByDate<<" | "
<<"CloseProfitByTrade="<<pInvestorPositionDetail->CloseProfitByTrade<<" | "
<<"PositionProfitByDate="<<pInvestorPositionDetail->PositionProfitByDate<<" | "
<<"PositionProfitByTrade="<<pInvestorPositionDetail->PositionProfitByTrade<<" | "
<<"Margin="<<pInvestorPositionDetail->Margin<<" | "
<<"ExchMargin="<<pInvestorPositionDetail->ExchMargin<<" | "
<<"MarginRateByMoney="<<pInvestorPositionDetail->MarginRateByMoney<<" | "
<<"MarginRateByVolume="<<pInvestorPositionDetail->MarginRateByVolume<<" | "
<<"LastSettlementPrice="<<pInvestorPositionDetail->LastSettlementPrice<<" | "
<<"SettlementPrice="<<pInvestorPositionDetail->SettlementPrice<<" | "
<<"CloseVolume="<<pInvestorPositionDetail->CloseVolume<<" | "
<<"CloseAmount="<<pInvestorPositionDetail->CloseAmount<<endl;
	int remainder = pInvestorPositionDetail->Volume - pInvestorPositionDetail->CloseVolume;
	if(remainder > 0){
		ELongShort ls = pInvestorPositionDetail->Direction=='0'?E_LONG:E_SHORT;

		EPositionType pe = P_LONGSHORT;
		if(atoi(pInvestorPositionDetail->OpenDate) < atoi(pInvestorPositionDetail->TradingDay)){
			pe = ls==E_LONG?P_YESTERDAY_LONG:P_YESTERDAY_SHORT;
		}else{
			pe = ls==E_LONG?P_TODAY_LONG:P_TODAY_SHORT;
		}
		double price = pInvestorPositionDetail->OpenPrice;
		trader_->UpdatePosition(pInvestorPositionDetail->InstrumentID, E_OPEN, ls, remainder, price, pe);
	}
}

	if(bIsLast == true){
		if(pRspInfo == NULL || pRspInfo->ErrorID == 0){
			sem_post(&sem_);
		}
	}

}
void CtpTradeChannel::OnRspOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast){
if(pInputOrderAction==NULL){
log_stream_<<"["<<__FUNCTION__<<"] "<<"pRspInfo=NULL"<<endl;
}else{
log_stream_<<"["<<__FUNCTION__<<"] BrokerID="<<pInputOrderAction->BrokerID<<" | "
<<"InvestorID="<<pInputOrderAction->InvestorID<<" | "
<<"OrderActionRef="<<pInputOrderAction->OrderActionRef<<" | "
<<"OrderRef="<<pInputOrderAction->OrderRef<<" | "
<<"RequestID="<<pInputOrderAction->RequestID<<" | "
<<"FrontID="<<pInputOrderAction->FrontID<<" | "
<<"SessionID="<<pInputOrderAction->SessionID<<" | "
<<"ExchangeID="<<pInputOrderAction->ExchangeID<<" | "
<<"OrderSysID="<<pInputOrderAction->OrderSysID<<" | "
<<"ActionFlag="<<pInputOrderAction->ActionFlag<<" | "
<<"LimitPrice="<<pInputOrderAction->LimitPrice<<" | "
<<"VolumeChange="<<pInputOrderAction->VolumeChange<<" | "
<<"UserID="<<pInputOrderAction->UserID<<" | "
<<"InstrumentID="<<pInputOrderAction->InstrumentID<<" | "
<<"InvestUnitID="<<pInputOrderAction->InvestUnitID<<" | "
<<"IPAddress="<<pInputOrderAction->IPAddress<<" | "
<<"MacAddress="<<pInputOrderAction->MacAddress<<" | ";
}
if(pRspInfo==NULL){
log_stream_<<"pRspInfo==NULL";
}else{
log_stream_<<"ErrorId="<<pRspInfo->ErrorID<<" | "
<<"ErrorMsg="<<pRspInfo->ErrorMsg;
}
log_stream_<<" | nRequestID="<<nRequestID<<" | bIsLast="<<"bIsLast="<<bIsLast<<endl;
log_stream_<<endl;
}
void CtpTradeChannel::OnRspQryOrder(CThostFtdcOrderField *pOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) 
{
if(pOrder==NULL){
log_stream_<<"["<<__FUNCTION__<<"] "<<"pInvestorPositionDetail=NULL"<<endl;
}else{
log_stream_<<"["<<__FUNCTION__<<"] "
<<"BrokerID="<<pOrder->BrokerID<<" | "
<<"InvestorID="<<pOrder->InvestorID<<" | "
<<"InstrumentID="<<pOrder->InstrumentID<<" | "
<<"OrderRef="<<pOrder->OrderRef<<" | "
<<"UserID="<<pOrder->UserID<<" | "
<<"OrderPriceType="<<pOrder->OrderPriceType<<" | "
<<"Direction="<<pOrder->Direction<<" | "
<<"CombOffsetFlag="<<pOrder->CombOffsetFlag<<" | "
<<"CombHedgeFlag="<<pOrder->CombHedgeFlag<<" | "
<<"LimitPrice="<<pOrder->LimitPrice<<" | "
<<"VolumeTotalOriginal="<<pOrder->VolumeTotalOriginal<<" | "
<<"TimeCondition="<<pOrder->TimeCondition<<" | "
<<"GTDDate="<<pOrder->GTDDate<<" | "
<<"VolumeCondition="<<pOrder->VolumeCondition<<" | "
<<"MinVolume="<<pOrder->MinVolume<<" | "
<<"ContingentCondition="<<pOrder->ContingentCondition<<" | "
<<"StopPrice="<<pOrder->StopPrice<<" | "
<<"ForceCloseReason="<<pOrder->ForceCloseReason<<" | "
<<"IsAutoSuspend="<<pOrder->IsAutoSuspend<<" | "
<<"BusinessUnit="<<pOrder->BusinessUnit<<" | "
<<"RequestID="<<pOrder->RequestID<<" | "
<<"OrderLocalID="<<pOrder->OrderLocalID<<" | "
<<"ExchangeID="<<pOrder->ExchangeID<<" | "
<<"ParticipantID="<<pOrder->ParticipantID<<" | "
<<"ClientID="<<pOrder->ClientID<<" | "
<<"ExchangeInstID="<<pOrder->ExchangeInstID<<" | "
<<"TraderID="<<pOrder->TraderID<<" | "
<<"InstallID="<<pOrder->InstallID<<" | "
<<"OrderSubmitStatus="<<pOrder->OrderSubmitStatus<<" | "
<<"NotifySequence="<<pOrder->NotifySequence<<" | "
<<"TradingDay="<<pOrder->TradingDay<<" | "
<<"SettlementID="<<pOrder->SettlementID<<" | "
<<"OrderSysID="<<pOrder->OrderSysID<<" | "
<<"OrderSource="<<pOrder->OrderSource<<" | "
<<"OrderStatus="<<pOrder->OrderStatus<<" | "
<<"OrderType="<<pOrder->OrderType<<" | "
<<"VolumeTraded="<<pOrder->VolumeTraded<<" | "
<<"VolumeTotal="<<pOrder->VolumeTotal<<" | "
<<"InsertDate="<<pOrder->InsertDate<<" | "
<<"InsertTime="<<pOrder->InsertTime<<" | "
<<"ActiveTime="<<pOrder->ActiveTime<<" | "
<<"SuspendTime="<<pOrder->SuspendTime<<" | "
<<"UpdateTime="<<pOrder->UpdateTime<<" | "
<<"CancelTime="<<pOrder->CancelTime<<" | "
<<"ActiveTraderID="<<pOrder->ActiveTraderID<<" | "
<<"ClearingPartID="<<pOrder->ClearingPartID<<" | "
<<"SequenceNo="<<pOrder->SequenceNo<<" | "
<<"FrontID="<<pOrder->FrontID<<" | "
<<"SessionID="<<pOrder->SessionID<<" | "
<<"UserProductInfo="<<pOrder->UserProductInfo<<" | "
<<"StatusMsg="<<pOrder->StatusMsg<<" | "
<<"UserForceClose="<<pOrder->UserForceClose<<" | "
<<"ActiveUserID="<<pOrder->ActiveUserID<<" | "
<<"BrokerOrderSeq="<<pOrder->BrokerOrderSeq<<" | "
<<"RelativeOrderSysID="<<pOrder->RelativeOrderSysID<<" | "
<<"ZCETotalTradedVolume="<<pOrder->ZCETotalTradedVolume<<" | "
<<"IsSwapOrder="<<pOrder->IsSwapOrder<<" | "
<<"BranchID="<<pOrder->BranchID<<" | "
<<"InvestUnitID="<<pOrder->InvestUnitID<<" | "
<<"AccountID="<<pOrder->AccountID<<" | "
<<"CurrencyID="<<pOrder->CurrencyID<<" | "
<<"IPAddress="<<pOrder->IPAddress<<" | "
<<"MacAddress="<<pOrder->MacAddress<<endl;
	if(pOrder->OrderStatus == THOST_FTDC_OST_PartTradedQueueing 
	|| pOrder->OrderStatus == THOST_FTDC_OST_NoTradeQueueing){
		Order o;
		//order_ref_has_inserted.insert(pOrder->OrderRef);
		STRCPY(o.instrument, pOrder->InstrumentID);
		if(pOrder->Direction == THOST_FTDC_D_Buy){
			if(pOrder->CombOffsetFlag[0] == THOST_FTDC_OF_Open){
				o.long_short = E_LONG;
			}else{
				o.long_short = E_SHORT;
			}
		}else{
			if(pOrder->CombOffsetFlag[0] == THOST_FTDC_OF_Open){
				o.long_short = E_SHORT;
			}else{
				o.long_short = E_LONG;
			}
		}
		if(pOrder->CombOffsetFlag[0] == THOST_FTDC_OF_Open){
			o.open_close = E_OPEN;
		}else{
			o.open_close = E_CLOSE;
		}
		o.state = E_INSERT;
		o.order_local_id = -1;
		STRCPY(o.order_system_id, pOrder->OrderSysID);
		STRCPY(o.exchange_id, pOrder->ExchangeInstID);
		STRCPY(o.state_msg, pOrder->StatusMsg);
		handler_->push(&o);
	}
}
	if(bIsLast == true){
		if(pRspInfo == NULL || pRspInfo->ErrorID == 0){
			sem_post(&sem_);
		}
	}
}

void CtpTradeChannel::OnRspAuthenticate(CThostFtdcRspAuthenticateField *pRspAuthenticateField, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if(pRspAuthenticateField==NULL){
log_stream_<<"[ "<<__FUNCTION__<<" ] pRspAuthenticateField=NULL | ";
	}else{
log_stream_<<"[ "<<__FUNCTION__<<" ] BrokerID="<<pRspAuthenticateField->BrokerID<<" | "
<<"UserID="<<pRspAuthenticateField->UserID<<" | "
<<"UserProductInfo="<<pRspAuthenticateField->UserProductInfo<<" | "
<<"AppID="<<pRspAuthenticateField->AppID<<" | "
<<"AppType="<<pRspAuthenticateField->AppType<<" | ";
	}
if(pRspInfo==NULL){
log_stream_<<"pRspInfo==NULL | ";
}else{
log_stream_<<"ErrorID="<<pRspInfo->ErrorID<<" ErrorMsg="<<pRspInfo->ErrorMsg<<" | ";
}
log_stream_<<" nRequestID="<<nRequestID<<" bIsLast="<<bIsLast<<endl;
	if(login_ok_ == false){
		CThostFtdcReqUserLoginField LoginField={0};
		STRCPY(LoginField.BrokerID, cfg_->broker_id);
		STRCPY(LoginField.UserID, cfg_->user);
		STRCPY(LoginField.Password, cfg_->password);
		log_stream_<<"ReqUserLogin ["<<LoginField.BrokerID<<"] ["<<LoginField.UserID<<"] ["<<LoginField.Password<<"]\n";
		trade_api_->ReqUserLogin(&LoginField, request_id_++);
	}
}

void CtpTradeChannel::OnFrontDisconnected(int nReason){
	login_ok_=false;
	session_id_=0;
log_stream_<<"[ "<<__FUNCTION__<<" ] ";
	switch(nReason){
		case 0x1001:
			log_stream_<<"网络读失败"<<endl;
			break;
		case 0x1002:
			log_stream_<<"网络写失败"<<endl;
			break;
		case 0x2001:
			log_stream_<<"接收心跳超时"<<endl;
			break;
		case 0x2002:
			log_stream_<<"发送心跳失败"<<endl;
			break;
		case 0x2003:
			log_stream_<<"收到错误报文"<<endl;
			break;
	}
}
