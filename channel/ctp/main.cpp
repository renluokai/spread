#include <iostream>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#include "include/ThostFtdcMdApi.h"
#include "include/ThostFtdcTraderApi.h"

#define STRCPY(a,b) strncpy((a), (b), sizeof(a))
#define SIM
#ifdef SIM
char *trade_front = "tcp://180.168.146.187:10030";
char *quote_front = "tcp://180.168.146.187:10031";
char *userid = "115385";
char *password = "Rlk170310";
char *brokerid = "9999";
#else
char *trade_front = "tcp://180.168.212.56:42205";
char *quote_front = "tcp://180.168.212.51:41213";
char *userid = "8005014038";
char *password = "Rlk170310";
char *brokerid = "88888";
#endif
using namespace std;

CThostFtdcMdApi *quote_api_;
CThostFtdcTraderApi *trade_api_;

bool MKDIR(const char* path){
	int status = mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	if(status == -1 && errno != EEXIST){
		perror(path);
		return false;
	} 
	return true;
}
class trade_handler_t: public CThostFtdcTraderSpi
{
	void Delay(unsigned seconds){sleep(seconds);}
public:
	///当客户端与交易后台建立起通信连接时（还未登录前），该方法被调用。
	virtual void OnFrontConnected(){
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
		CThostFtdcReqUserLoginField login_field = {};		
		STRCPY(login_field.BrokerID, brokerid);
		STRCPY(login_field.UserID, userid);
		STRCPY(login_field.Password, password);
		trade_api_->ReqUserLogin(&login_field, 0);
	};
	
	///当客户端与交易后台通信连接断开时，该方法被调用。当发生这个情况后，API会自动重新连接，客户端可不做处理。
	///@param nReason 错误原因
	///        0x1001 网络读失败
	///        0x1002 网络写失败
	///        0x2001 接收心跳超时
	///        0x2002 发送心跳失败
	///        0x2003 收到错误报文
	virtual void OnFrontDisconnected(int nReason){
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
		switch(nReason){
			case 0x1001:
				cout<<"网络读失败"<<endl;
				break;
			case 0x1002:break;
				cout<<"网络写失败"<<endl;
				break;
			case 0x2001:break;
				cout<<"接收心跳超时"<<endl;
				break;
			case 0x2002:break;
				cout<<"发送心跳失败"<<endl;
				break;
			case 0x2003:break;
				cout<<"收到错误报文"<<endl;
				break;
			default:
				cout<<"未知原因"<<endl;
		}
	};
		
	///心跳超时警告。当长时间未收到报文时，该方法被调用。
	///@param nTimeLapse 距离上次接收报文的时间
	virtual void OnHeartBeatWarning(int nTimeLapse){}
	
	///客户端认证响应
	virtual void OnRspAuthenticate(CThostFtdcRspAuthenticateField *pRspAuthenticateField, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};
	

	///登录请求响应
	virtual void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
		cout<<"TradingDay "<<pRspUserLogin->TradingDay<<endl;
		cout<<"LoginTime "<<pRspUserLogin->LoginTime<<endl;
		cout<<"BrokerID "<<pRspUserLogin->BrokerID<<endl;
		cout<<"UserID "<<pRspUserLogin->UserID<<endl;
		cout<<"SystemName "<<pRspUserLogin->SystemName<<endl;
		cout<<"FrontID "<<pRspUserLogin->FrontID<<endl;
		cout<<"SessionID "<<pRspUserLogin->SessionID<<endl;
		cout<<"MaxOrderRef "<<pRspUserLogin->MaxOrderRef<<endl;
		cout<<"SHFETime "<<pRspUserLogin->SHFETime<<endl;
		cout<<"DCETime "<<pRspUserLogin->DCETime<<endl;
		cout<<"CZCETime "<<pRspUserLogin->CZCETime<<endl;
		cout<<"FFEXTime "<<pRspUserLogin->FFEXTime<<endl;
		cout<<"INETime "<<pRspUserLogin->INETime<<endl;
		
		cout<<"ErrorID "<<pRspInfo->ErrorID<<endl;
		cout<<"ErrorMsg "<<pRspInfo->ErrorMsg<<endl;
		
		cout<<"nRequestID "<<nRequestID<<endl;
		cout<<"bIsLast "<<bIsLast<<endl;
		cout<<"=====================================\n"<<"GetTradingDay"<<endl;
		STRCPY(trading_day_, trade_api_->GetTradingDay());

		const char *dir_instrument = "instrument";
		const char *dir_log        = "log";
		const char *dir_path[] = {
			dir_instrument,
			dir_log,
			NULL,
		};
		for(int i = 0; dir_path[i] != NULL; i++){
			if(MKDIR(dir_path[i]) == false){
				perror(__FUNCTION__);
			}
		}

		char path_buffer[256] = {0};
		sprintf(path_buffer, "%s/%s",dir_instrument, trading_day_);
		if(MKDIR(path_buffer) == false){
			perror(__FUNCTION__);
		}
		memset(path_buffer, 0 ,256);
		sprintf(path_buffer, "%s/%s",dir_log, trading_day_);
		if(MKDIR(path_buffer) == false){
			perror(__FUNCTION__);
		}

		memset(instrument_file_, 0, sizeof(instrument_file_));
		sprintf(instrument_file_, "%s/%s/all_ins.text", dir_instrument, trading_day_);
	
		struct stat buffer;
        int         status;
        status = stat(instrument_file_, &buffer);
	
		if(status != 0 && errno == ENOENT){
			perror("stat");
			cout<<ENOENT<<endl;
			cout<<"errno is "<<errno<<endl;
		//	exit(1);
		}

		cout<<trade_api_->GetTradingDay()<<endl;
		CThostFtdcQryInvestorField InverstorField = {0};
		trade_api_->ReqQryInvestor(&InverstorField, 0);
		
	//CThostFtdcQrySettlementInfoConfirmField query_settle_confirm = {0};
		//trade_api_->ReqQrySettlementInfoConfirm(&query_settle_confirm,0);
	};

	///登出请求响应
	virtual void OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///用户口令更新请求响应
	virtual void OnRspUserPasswordUpdate(CThostFtdcUserPasswordUpdateField *pUserPasswordUpdate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///资金账户口令更新请求响应
	virtual void OnRspTradingAccountPasswordUpdate(CThostFtdcTradingAccountPasswordUpdateField *pTradingAccountPasswordUpdate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///报单录入请求响应
	virtual void OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///预埋单录入请求响应
	virtual void OnRspParkedOrderInsert(CThostFtdcParkedOrderField *pParkedOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///预埋撤单录入请求响应
	virtual void OnRspParkedOrderAction(CThostFtdcParkedOrderActionField *pParkedOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///报单操作请求响应
	virtual void OnRspOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///查询最大报单数量响应
	virtual void OnRspQueryMaxOrderVolume(CThostFtdcQueryMaxOrderVolumeField *pQueryMaxOrderVolume, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///投资者结算结果确认响应
	virtual void OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
		if(pSettlementInfoConfirm == NULL){
			cout<<"pSettlementInfoConfirm"<<" is null"<<endl;
		}else{
			cout<<"BrokerID "<<pSettlementInfoConfirm->BrokerID<<endl;
			cout<<"InvestorID "<<pSettlementInfoConfirm->InvestorID<<endl;
			cout<<"ConfirmDate "<<pSettlementInfoConfirm->ConfirmDate<<endl;
			cout<<"ConfirmTime "<<pSettlementInfoConfirm->ConfirmTime<<endl;
		}
		if(pRspInfo==NULL){
			cout<<"pRspInfo"<<" is null"<<endl;
		}else{
			cout<<"ErrorID "<<pRspInfo->ErrorID<<endl;
			cout<<"ErrorMsg "<<pRspInfo->ErrorMsg<<endl;
		}
		cout<<"nRequestID "<<nRequestID<<endl;
		cout<<"bIsLast "<<bIsLast<<endl;
	};

	///删除预埋单响应
	virtual void OnRspRemoveParkedOrder(CThostFtdcRemoveParkedOrderField *pRemoveParkedOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///删除预埋撤单响应
	virtual void OnRspRemoveParkedOrderAction(CThostFtdcRemoveParkedOrderActionField *pRemoveParkedOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///执行宣告录入请求响应
	virtual void OnRspExecOrderInsert(CThostFtdcInputExecOrderField *pInputExecOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///执行宣告操作请求响应
	virtual void OnRspExecOrderAction(CThostFtdcInputExecOrderActionField *pInputExecOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///询价录入请求响应
	virtual void OnRspForQuoteInsert(CThostFtdcInputForQuoteField *pInputForQuote, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///报价录入请求响应
	virtual void OnRspQuoteInsert(CThostFtdcInputQuoteField *pInputQuote, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///报价操作请求响应
	virtual void OnRspQuoteAction(CThostFtdcInputQuoteActionField *pInputQuoteAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///锁定应答
	virtual void OnRspLockInsert(CThostFtdcInputLockField *pInputLock, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///批量报单操作请求响应
	virtual void OnRspBatchOrderAction(CThostFtdcInputBatchOrderActionField *pInputBatchOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///申请组合录入请求响应
	virtual void OnRspCombActionInsert(CThostFtdcInputCombActionField *pInputCombAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///请求查询报单响应
	virtual void OnRspQryOrder(CThostFtdcOrderField *pOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///请求查询成交响应
	virtual void OnRspQryTrade(CThostFtdcTradeField *pTrade, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///请求查询投资者持仓响应
	virtual void OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pInvestorPosition, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///请求查询资金账户响应
	virtual void OnRspQryTradingAccount(CThostFtdcTradingAccountField *pTradingAccount, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///请求查询投资者响应
	virtual void OnRspQryInvestor(CThostFtdcInvestorField *pInvestor, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	if(pInvestor != NULL){
	///投资者代码
	cout<<"InvestorID "<<pInvestor->InvestorID<<endl;
	STRCPY(investor_id_, pInvestor->InvestorID);
	///经纪公司代码
	TThostFtdcBrokerIDType	BrokerID;
	cout<<"BrokerID "<<pInvestor->BrokerID<<endl;
	///投资者分组代码
	cout<<"InvestorGroupID "<<pInvestor->InvestorGroupID<<endl;
	///投资者名称
	cout<<"InvestorName "<<pInvestor->InvestorName<<endl;
	///证件类型
	cout<<"IdentifiedCardType "<<pInvestor->IdentifiedCardType<<endl;
	///证件号码
	cout<<"IdentifiedCardNo "<<pInvestor->IdentifiedCardNo<<endl;
	///是否活跃
	cout<<"IsActive "<<pInvestor->IsActive<<endl;
	///联系电话
	cout<<"Telephone "<<pInvestor->Telephone<<endl;
	///通讯地址
	cout<<"Address "<<pInvestor->Address<<endl;
	///开户日期
	cout<<"OpenDate "<<pInvestor->OpenDate<<endl;
	///手机
	cout<<"Mobile "<<pInvestor->Mobile<<endl;
	///手续费率模板代码
	cout<<"CommModelID "<<pInvestor->CommModelID<<endl;
	///保证金率模板代码
	cout<<"MarginModelID "<<pInvestor->MarginModelID<<endl;
	}
		Delay(1);
		CThostFtdcQryInstrumentField qry_instrument = {0};
		trade_api_->ReqQryInstrument(&qry_instrument,0);
		CThostFtdcQrySettlementInfoField qry_settle_info = {0};
		//trade_api_->ReqQrySettlementInfo(&qry_settle_info,0);
	
	};

	///请求查询交易编码响应
	virtual void OnRspQryTradingCode(CThostFtdcTradingCodeField *pTradingCode, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
///交易编码
	if(pTradingCode != NULL){
	///投资者代码
	cout<<"InvestorID "<<pTradingCode->InvestorID<<endl;
	///经纪公司代码
	cout<<"BrokerID "<<pTradingCode->BrokerID<<endl;
	///交易所代码
	cout<<"ExchangeID "<<pTradingCode->ExchangeID<<endl;
	///客户代码
	cout<<"ClientID "<<pTradingCode->ClientID<<endl;
	///是否活跃
	cout<<"IsActive "<<pTradingCode->IsActive<<endl;
	///交易编码类型
	cout<<"ClientIDType "<<pTradingCode->ClientIDType<<endl;
	///营业部编号
	cout<<"BranchID "<<pTradingCode->BranchID<<endl;
	///业务类型
	cout<<"BizType "<<pTradingCode->BizType<<endl;
	}
	};

	///请求查询合约保证金率响应
	virtual void OnRspQryInstrumentMarginRate(CThostFtdcInstrumentMarginRateField *pInstrumentMarginRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///请求查询合约手续费率响应
	virtual void OnRspQryInstrumentCommissionRate(CThostFtdcInstrumentCommissionRateField *pInstrumentCommissionRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///请求查询交易所响应
	virtual void OnRspQryExchange(CThostFtdcExchangeField *pExchange, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///请求查询产品响应
	virtual void OnRspQryProduct(CThostFtdcProductField *pProduct, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///请求查询合约响应
	virtual void OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	if(pInstrument != NULL){
	///合约代码
	cout<<"InstrumentID "<<pInstrument->InstrumentID<<endl;
	///交易所代码
	cout<<"ExchangeID "<<pInstrument->ExchangeID<<endl;
	///合约名称
	cout<<"InstrumentName "<<pInstrument->InstrumentName<<endl;
	///合约在交易所的代码
	cout<<"ExchangeInstID "<<pInstrument->ExchangeInstID<<endl;
	///产品代码
	cout<<"ProductID "<<pInstrument->ProductID<<endl;
	///产品类型
	cout<<"ProductClass "<<pInstrument->ProductClass<<endl;
	///交割年份
	cout<<"DeliveryYear "<<pInstrument->DeliveryYear<<endl;
	///交割月
	cout<<"DeliveryMonth "<<pInstrument->DeliveryMonth<<endl;
	///市价单最大下单量
	cout<<"MaxMarketOrderVolume "<<pInstrument->MaxMarketOrderVolume<<endl;
	///市价单最小下单量
	cout<<"MinMarketOrderVolume "<<pInstrument->MinMarketOrderVolume<<endl;
	///限价单最大下单量
	cout<<"MaxLimitOrderVolume "<<pInstrument->MaxLimitOrderVolume<<endl;
	///限价单最小下单量
	cout<<"MinLimitOrderVolume "<<pInstrument->MinLimitOrderVolume<<endl;
	///合约数量乘数
	cout<<"VolumeMultiple "<<pInstrument->VolumeMultiple<<endl;
	///最小变动价位
	cout<<"PriceTick "<<pInstrument->PriceTick<<endl;
	///创建日
	cout<<"CreateDate "<<pInstrument->CreateDate<<endl;
	///上市日
	cout<<"OpenDate "<<pInstrument->OpenDate<<endl;
	///到期日
	cout<<"ExpireDate "<<pInstrument->ExpireDate<<endl;
	///开始交割日
	cout<<"StartDelivDate "<<pInstrument->StartDelivDate<<endl;
	///结束交割日
	cout<<"EndDelivDate "<<pInstrument->EndDelivDate<<endl;
	///合约生命周期状态
	cout<<"InstLifePhase "<<pInstrument->InstLifePhase<<endl;
	///当前是否交易
	cout<<"IsTrading "<<pInstrument->IsTrading<<endl;
	///持仓类型
	cout<<"PositionType "<<pInstrument->PositionType<<endl;
	///持仓日期类型
	cout<<"PositionDateType "<<pInstrument->PositionDateType<<endl;
	///多头保证金率
	cout<<"LongMarginRatio "<<pInstrument->LongMarginRatio<<endl;
	///空头保证金率
	cout<<"ShortMarginRatio "<<pInstrument->ShortMarginRatio<<endl;
	///是否使用大额单边保证金算法
	cout<<"MaxMarginSideAlgorithm "<<pInstrument->MaxMarginSideAlgorithm<<endl;
	///基础商品代码
	cout<<"UnderlyingInstrID "<<pInstrument->UnderlyingInstrID<<endl;
	///执行价
	cout<<"StrikePrice "<<pInstrument->StrikePrice<<endl;
	///期权类型
	cout<<"OptionsType "<<pInstrument->OptionsType<<endl;
	///合约基础商品乘数
	cout<<"UnderlyingMultiple "<<pInstrument->UnderlyingMultiple<<endl;
	///组合类型
	cout<<"CombinationType "<<pInstrument->CombinationType<<endl;
	///最小买下单单位
	cout<<"MinBuyVolume "<<pInstrument->MinBuyVolume<<endl;
	///最小卖下单单位
	cout<<"MinSellVolume "<<pInstrument->MinSellVolume<<endl;
	///合约标识码
	cout<<"InstrumentCode "<<pInstrument->InstrumentCode<<endl;

	}
	if(bIsLast == true){
	Delay(1);
	///查询交易编码
	CThostFtdcQryTradingCodeField  trade_code = {0};
	///经纪公司代码
	STRCPY(trade_code.BrokerID, brokerid);
	///投资者代码
	STRCPY(trade_code.InvestorID, investor_id_);
	///交易所代码
	//TThostFtdcExchangeIDType	ExchangeID;
	///客户代码
	//TThostFtdcClientIDType	ClientID;
	///交易编码类型
	//TThostFtdcClientIDTypeType	ClientIDType;


	trade_api_->ReqQryTradingCode(&trade_code, 0);
	}
};

	///请求查询行情响应
	virtual void OnRspQryDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///请求查询投资者结算结果响应
	virtual void OnRspQrySettlementInfo(CThostFtdcSettlementInfoField *pSettlementInfo, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
		if(pSettlementInfo==NULL)
		{
			cout<<"pSettlementInfo is null"<<endl;
		cout<<"=====================================\n"<<"ReqSettlementInfoConfirm"<<endl;
			CThostFtdcSettlementInfoConfirmField ConfirmField = {0};
			///经纪公司代码
			STRCPY(ConfirmField.BrokerID, brokerid);
			///投资者代码
			STRCPY(ConfirmField.InvestorID, investor_id_);
			trade_api_->ReqSettlementInfoConfirm(&ConfirmField,0);
			
		}else{
		cout<<"TradingDay "<<pSettlementInfo->TradingDay<<endl;
		cout<<"SettlementID "<<pSettlementInfo->SettlementID<<endl;
		cout<<"BrokerID "<<pSettlementInfo->BrokerID<<endl;
		cout<<"InvestorID "<<pSettlementInfo->InvestorID<<endl;
		cout<<"SequenceNo "<<pSettlementInfo->SequenceNo<<endl;
		cout<<"Content "<<pSettlementInfo->Content<<endl;
		}
	};

	///请求查询转帐银行响应
	virtual void OnRspQryTransferBank(CThostFtdcTransferBankField *pTransferBank, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///请求查询投资者持仓明细响应
	virtual void OnRspQryInvestorPositionDetail(CThostFtdcInvestorPositionDetailField *pInvestorPositionDetail, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///请求查询客户通知响应
	virtual void OnRspQryNotice(CThostFtdcNoticeField *pNotice, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///请求查询结算信息确认响应
	virtual void OnRspQrySettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///请求查询投资者持仓明细响应
	virtual void OnRspQryInvestorPositionCombineDetail(CThostFtdcInvestorPositionCombineDetailField *pInvestorPositionCombineDetail, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///查询保证金监管系统经纪公司资金账户密钥响应
	virtual void OnRspQryCFMMCTradingAccountKey(CThostFtdcCFMMCTradingAccountKeyField *pCFMMCTradingAccountKey, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///请求查询仓单折抵信息响应
	virtual void OnRspQryEWarrantOffset(CThostFtdcEWarrantOffsetField *pEWarrantOffset, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///请求查询投资者品种/跨品种保证金响应
	virtual void OnRspQryInvestorProductGroupMargin(CThostFtdcInvestorProductGroupMarginField *pInvestorProductGroupMargin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///请求查询交易所保证金率响应
	virtual void OnRspQryExchangeMarginRate(CThostFtdcExchangeMarginRateField *pExchangeMarginRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///请求查询交易所调整保证金率响应
	virtual void OnRspQryExchangeMarginRateAdjust(CThostFtdcExchangeMarginRateAdjustField *pExchangeMarginRateAdjust, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///请求查询汇率响应
	virtual void OnRspQryExchangeRate(CThostFtdcExchangeRateField *pExchangeRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///请求查询二级代理操作员银期权限响应
	virtual void OnRspQrySecAgentACIDMap(CThostFtdcSecAgentACIDMapField *pSecAgentACIDMap, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///请求查询产品报价汇率
	virtual void OnRspQryProductExchRate(CThostFtdcProductExchRateField *pProductExchRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///请求查询产品组
	virtual void OnRspQryProductGroup(CThostFtdcProductGroupField *pProductGroup, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///请求查询做市商合约手续费率响应
	virtual void OnRspQryMMInstrumentCommissionRate(CThostFtdcMMInstrumentCommissionRateField *pMMInstrumentCommissionRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///请求查询做市商期权合约手续费响应
	virtual void OnRspQryMMOptionInstrCommRate(CThostFtdcMMOptionInstrCommRateField *pMMOptionInstrCommRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///请求查询报单手续费响应
	virtual void OnRspQryInstrumentOrderCommRate(CThostFtdcInstrumentOrderCommRateField *pInstrumentOrderCommRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///请求查询期权交易成本响应
	virtual void OnRspQryOptionInstrTradeCost(CThostFtdcOptionInstrTradeCostField *pOptionInstrTradeCost, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///请求查询期权合约手续费响应
	virtual void OnRspQryOptionInstrCommRate(CThostFtdcOptionInstrCommRateField *pOptionInstrCommRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///请求查询执行宣告响应
	virtual void OnRspQryExecOrder(CThostFtdcExecOrderField *pExecOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///请求查询询价响应
	virtual void OnRspQryForQuote(CThostFtdcForQuoteField *pForQuote, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///请求查询报价响应
	virtual void OnRspQryQuote(CThostFtdcQuoteField *pQuote, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///请求查询锁定应答
	virtual void OnRspQryLock(CThostFtdcLockField *pLock, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///请求查询锁定证券仓位应答
	virtual void OnRspQryLockPosition(CThostFtdcLockPositionField *pLockPosition, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///请求查询ETF期权合约手续费响应
	virtual void OnRspQryETFOptionInstrCommRate(CThostFtdcETFOptionInstrCommRateField *pETFOptionInstrCommRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///请求查询投资者分级
	virtual void OnRspQryInvestorLevel(CThostFtdcInvestorLevelField *pInvestorLevel, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///请求查询E+1日行权冻结响应
	virtual void OnRspQryExecFreeze(CThostFtdcExecFreezeField *pExecFreeze, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///请求查询组合合约安全系数响应
	virtual void OnRspQryCombInstrumentGuard(CThostFtdcCombInstrumentGuardField *pCombInstrumentGuard, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///请求查询申请组合响应
	virtual void OnRspQryCombAction(CThostFtdcCombActionField *pCombAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///请求查询转帐流水响应
	virtual void OnRspQryTransferSerial(CThostFtdcTransferSerialField *pTransferSerial, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///请求查询银期签约关系响应
	virtual void OnRspQryAccountregister(CThostFtdcAccountregisterField *pAccountregister, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///错误应答
	virtual void OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///报单通知
	virtual void OnRtnOrder(CThostFtdcOrderField *pOrder) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///成交通知
	virtual void OnRtnTrade(CThostFtdcTradeField *pTrade) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///报单录入错误回报
	virtual void OnErrRtnOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///报单操作错误回报
	virtual void OnErrRtnOrderAction(CThostFtdcOrderActionField *pOrderAction, CThostFtdcRspInfoField *pRspInfo) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///合约交易状态通知
	virtual void OnRtnInstrumentStatus(CThostFtdcInstrumentStatusField *pInstrumentStatus) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///交易所公告通知
	virtual void OnRtnBulletin(CThostFtdcBulletinField *pBulletin) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///交易通知
	virtual void OnRtnTradingNotice(CThostFtdcTradingNoticeInfoField *pTradingNoticeInfo) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///提示条件单校验错误
	virtual void OnRtnErrorConditionalOrder(CThostFtdcErrorConditionalOrderField *pErrorConditionalOrder) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///执行宣告通知
	virtual void OnRtnExecOrder(CThostFtdcExecOrderField *pExecOrder) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///执行宣告录入错误回报
	virtual void OnErrRtnExecOrderInsert(CThostFtdcInputExecOrderField *pInputExecOrder, CThostFtdcRspInfoField *pRspInfo) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///执行宣告操作错误回报
	virtual void OnErrRtnExecOrderAction(CThostFtdcExecOrderActionField *pExecOrderAction, CThostFtdcRspInfoField *pRspInfo) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///询价录入错误回报
	virtual void OnErrRtnForQuoteInsert(CThostFtdcInputForQuoteField *pInputForQuote, CThostFtdcRspInfoField *pRspInfo) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///报价通知
	virtual void OnRtnQuote(CThostFtdcQuoteField *pQuote) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///报价录入错误回报
	virtual void OnErrRtnQuoteInsert(CThostFtdcInputQuoteField *pInputQuote, CThostFtdcRspInfoField *pRspInfo) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///报价操作错误回报
	virtual void OnErrRtnQuoteAction(CThostFtdcQuoteActionField *pQuoteAction, CThostFtdcRspInfoField *pRspInfo) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///询价通知
	virtual void OnRtnForQuoteRsp(CThostFtdcForQuoteRspField *pForQuoteRsp) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///保证金监控中心用户令牌
	virtual void OnRtnCFMMCTradingAccountToken(CThostFtdcCFMMCTradingAccountTokenField *pCFMMCTradingAccountToken) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///锁定通知
	virtual void OnRtnLock(CThostFtdcLockField *pLock) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///锁定错误通知
	virtual void OnErrRtnLockInsert(CThostFtdcInputLockField *pInputLock, CThostFtdcRspInfoField *pRspInfo) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///批量报单操作错误回报
	virtual void OnErrRtnBatchOrderAction(CThostFtdcBatchOrderActionField *pBatchOrderAction, CThostFtdcRspInfoField *pRspInfo) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///申请组合通知
	virtual void OnRtnCombAction(CThostFtdcCombActionField *pCombAction) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///申请组合录入错误回报
	virtual void OnErrRtnCombActionInsert(CThostFtdcInputCombActionField *pInputCombAction, CThostFtdcRspInfoField *pRspInfo) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///请求查询签约银行响应
	virtual void OnRspQryContractBank(CThostFtdcContractBankField *pContractBank, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///请求查询预埋单响应
	virtual void OnRspQryParkedOrder(CThostFtdcParkedOrderField *pParkedOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///请求查询预埋撤单响应
	virtual void OnRspQryParkedOrderAction(CThostFtdcParkedOrderActionField *pParkedOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///请求查询交易通知响应
	virtual void OnRspQryTradingNotice(CThostFtdcTradingNoticeField *pTradingNotice, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///请求查询经纪公司交易参数响应
	virtual void OnRspQryBrokerTradingParams(CThostFtdcBrokerTradingParamsField *pBrokerTradingParams, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///请求查询经纪公司交易算法响应
	virtual void OnRspQryBrokerTradingAlgos(CThostFtdcBrokerTradingAlgosField *pBrokerTradingAlgos, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///请求查询监控中心用户令牌
	virtual void OnRspQueryCFMMCTradingAccountToken(CThostFtdcQueryCFMMCTradingAccountTokenField *pQueryCFMMCTradingAccountToken, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///银行发起银行资金转期货通知
	virtual void OnRtnFromBankToFutureByBank(CThostFtdcRspTransferField *pRspTransfer) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///银行发起期货资金转银行通知
	virtual void OnRtnFromFutureToBankByBank(CThostFtdcRspTransferField *pRspTransfer) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///银行发起冲正银行转期货通知
	virtual void OnRtnRepealFromBankToFutureByBank(CThostFtdcRspRepealField *pRspRepeal) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///银行发起冲正期货转银行通知
	virtual void OnRtnRepealFromFutureToBankByBank(CThostFtdcRspRepealField *pRspRepeal) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///期货发起银行资金转期货通知
	virtual void OnRtnFromBankToFutureByFuture(CThostFtdcRspTransferField *pRspTransfer) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///期货发起期货资金转银行通知
	virtual void OnRtnFromFutureToBankByFuture(CThostFtdcRspTransferField *pRspTransfer) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///系统运行时期货端手工发起冲正银行转期货请求，银行处理完毕后报盘发回的通知
	virtual void OnRtnRepealFromBankToFutureByFutureManual(CThostFtdcRspRepealField *pRspRepeal) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///系统运行时期货端手工发起冲正期货转银行请求，银行处理完毕后报盘发回的通知
	virtual void OnRtnRepealFromFutureToBankByFutureManual(CThostFtdcRspRepealField *pRspRepeal) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///期货发起查询银行余额通知
	virtual void OnRtnQueryBankBalanceByFuture(CThostFtdcNotifyQueryAccountField *pNotifyQueryAccount) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///期货发起银行资金转期货错误回报
	virtual void OnErrRtnBankToFutureByFuture(CThostFtdcReqTransferField *pReqTransfer, CThostFtdcRspInfoField *pRspInfo) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///期货发起期货资金转银行错误回报
	virtual void OnErrRtnFutureToBankByFuture(CThostFtdcReqTransferField *pReqTransfer, CThostFtdcRspInfoField *pRspInfo) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///系统运行时期货端手工发起冲正银行转期货错误回报
	virtual void OnErrRtnRepealBankToFutureByFutureManual(CThostFtdcReqRepealField *pReqRepeal, CThostFtdcRspInfoField *pRspInfo) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///系统运行时期货端手工发起冲正期货转银行错误回报
	virtual void OnErrRtnRepealFutureToBankByFutureManual(CThostFtdcReqRepealField *pReqRepeal, CThostFtdcRspInfoField *pRspInfo) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///期货发起查询银行余额错误回报
	virtual void OnErrRtnQueryBankBalanceByFuture(CThostFtdcReqQueryAccountField *pReqQueryAccount, CThostFtdcRspInfoField *pRspInfo) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///期货发起冲正银行转期货请求，银行处理完毕后报盘发回的通知
	virtual void OnRtnRepealFromBankToFutureByFuture(CThostFtdcRspRepealField *pRspRepeal) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///期货发起冲正期货转银行请求，银行处理完毕后报盘发回的通知
	virtual void OnRtnRepealFromFutureToBankByFuture(CThostFtdcRspRepealField *pRspRepeal) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///期货发起银行资金转期货应答
	virtual void OnRspFromBankToFutureByFuture(CThostFtdcReqTransferField *pReqTransfer, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///期货发起期货资金转银行应答
	virtual void OnRspFromFutureToBankByFuture(CThostFtdcReqTransferField *pReqTransfer, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///期货发起查询银行余额应答
	virtual void OnRspQueryBankAccountMoneyByFuture(CThostFtdcReqQueryAccountField *pReqQueryAccount, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///银行发起银期开户通知
	virtual void OnRtnOpenAccountByBank(CThostFtdcOpenAccountField *pOpenAccount) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///银行发起银期销户通知
	virtual void OnRtnCancelAccountByBank(CThostFtdcCancelAccountField *pCancelAccount) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};

	///银行发起变更银行账号通知
	virtual void OnRtnChangeAccountByBank(CThostFtdcChangeAccountField *pChangeAccount) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};
private:
	char investor_id_[18];
	char trading_day_[9];
	char instrument_file_[256];
	int  instrument_fd;
};
class quote_handler_t: public CThostFtdcMdSpi
{
public:
	///当客户端与交易后台建立起通信连接时（还未登录前），该方法被调用。
	virtual void OnFrontConnected()
	{
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
		CThostFtdcReqUserLoginField login_field = {0};
		STRCPY(login_field.UserID, userid);
		STRCPY(login_field.Password, password);
		
		quote_api_->ReqUserLogin(&login_field,0);
	};
	
	///当客户端与交易后台通信连接断开时，该方法被调用。当发生这个情况后，API会自动重新连接，客户端可不做处理。
	///@param nReason 错误原因
	///        0x1001 网络读失败
	///        0x1002 网络写失败
	///        0x2001 接收心跳超时
	///        0x2002 发送心跳失败
	///        0x2003 收到错误报文
	virtual void OnFrontDisconnected(int nReason){
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};
		
	///心跳超时警告。当长时间未收到报文时，该方法被调用。
	///@param nTimeLapse 距离上次接收报文的时间
	virtual void OnHeartBeatWarning(int nTimeLapse){
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};
	

	///登录请求响应
	virtual void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
		cout<<"TradingDay "<<pRspUserLogin->TradingDay<<endl;
		cout<<"LoginTime "<<pRspUserLogin->LoginTime<<endl;
		cout<<"BrokerID "<<pRspUserLogin->BrokerID<<endl;
		cout<<"UserID "<<pRspUserLogin->UserID<<endl;
		cout<<"SystemName "<<pRspUserLogin->SystemName<<endl;
		cout<<"FrontID "<<pRspUserLogin->FrontID<<endl;
		cout<<"SessionID "<<pRspUserLogin->SessionID<<endl;
		cout<<"MaxOrderRef "<<pRspUserLogin->MaxOrderRef<<endl;
		cout<<"SHFETime "<<pRspUserLogin->SHFETime<<endl;
		cout<<"DCETime "<<pRspUserLogin->DCETime<<endl;
		cout<<"CZCETime "<<pRspUserLogin->CZCETime<<endl;
		cout<<"FFEXTime "<<pRspUserLogin->FFEXTime<<endl;
		cout<<"INETime "<<pRspUserLogin->INETime<<endl;
		
		cout<<"ErrorID "<<pRspInfo->ErrorID<<endl;
		cout<<"ErrorMsg "<<pRspInfo->ErrorMsg<<endl;
		
		cout<<"nRequestID "<<nRequestID<<endl;
		cout<<"bIsLast "<<bIsLast<<endl;
	};
	


	///登出请求响应
	virtual void OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};
	

	///错误应答
	virtual void OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};
	


	///订阅行情应答
	virtual void OnRspSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};
	


	///取消订阅行情应答
	virtual void OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};
	


	///订阅询价应答
	virtual void OnRspSubForQuoteRsp(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};
	


	///取消订阅询价应答
	virtual void OnRspUnSubForQuoteRsp(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};
	


	///深度行情通知
	virtual void OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};
	


	///询价通知
	virtual void OnRtnForQuoteRsp(CThostFtdcForQuoteRspField *pForQuoteRsp) {
		cout<<"=====================================\n"<<__FUNCTION__<<endl;
	};
	

};

void insert_order(CThostFtdcTraderApi *api)
{
///输入报单
CThostFtdcInputOrderField order = {0};
	///经纪公司代码
	TThostFtdcBrokerIDType	BrokerID;
	///投资者代码
	TThostFtdcInvestorIDType	InvestorID;
	///合约代码
	TThostFtdcInstrumentIDType	InstrumentID;
	///报单引用
	TThostFtdcOrderRefType	OrderRef;
	///用户代码
	TThostFtdcUserIDType	UserID;
	///报单价格条件
	TThostFtdcOrderPriceTypeType	OrderPriceType;
	///买卖方向
	TThostFtdcDirectionType	Direction;
	///组合开平标志
	TThostFtdcCombOffsetFlagType	CombOffsetFlag;
	///组合投机套保标志
	TThostFtdcCombHedgeFlagType	CombHedgeFlag;
	///价格
	TThostFtdcPriceType	LimitPrice;
	///数量
	TThostFtdcVolumeType	VolumeTotalOriginal;
	///有效期类型
	TThostFtdcTimeConditionType	TimeCondition;
	///GTD日期
	TThostFtdcDateType	GTDDate;
	///成交量类型
	TThostFtdcVolumeConditionType	VolumeCondition;
	///最小成交量
	TThostFtdcVolumeType	MinVolume;
	///触发条件
	TThostFtdcContingentConditionType	ContingentCondition;
	///止损价
	TThostFtdcPriceType	StopPrice;
	///强平原因
	TThostFtdcForceCloseReasonType	ForceCloseReason;
	///自动挂起标志
	TThostFtdcBoolType	IsAutoSuspend;
	///业务单元
	TThostFtdcBusinessUnitType	BusinessUnit;
	///请求编号
	TThostFtdcRequestIDType	RequestID;
	///用户强评标志
	TThostFtdcBoolType	UserForceClose;
	///互换单标志
	TThostFtdcBoolType	IsSwapOrder;
	///交易所代码
	TThostFtdcExchangeIDType	ExchangeID;
	///投资单元代码
	TThostFtdcInvestUnitIDType	InvestUnitID;
	///资金账号
	TThostFtdcAccountIDType	AccountID;
	///币种代码
	TThostFtdcCurrencyIDType	CurrencyID;
	///交易编码
	TThostFtdcClientIDType	ClientID;
	///IP地址
	TThostFtdcIPAddressType	IPAddress;
	///Mac地址
	TThostFtdcMacAddressType	MacAddress;


	trade_api_->ReqOrderInsert(&order, 0);
}
int main()
{
	char const * dir_runtime = "runtime";
	char const * dir_quote   = "runtime/quote/";
	char const * dir_qbug    = "runtime/quote/quote";
	char const * dir_trade   = "runtime/trade/";
	char const * dir_tbug    = "runtime/trade/trade";
	
	char const *dirs[] = {
		dir_runtime,
		dir_quote,
		dir_qbug,
		dir_trade,
		dir_tbug,
		NULL,
	};
	for(int i = 0; dirs[i] != NULL; i++){
		if(MKDIR(dirs[i]) == false){
			return -1;
		}
	}

	quote_handler_t *p_quote_handler = new quote_handler_t;
	quote_api_ = CThostFtdcMdApi::CreateFtdcMdApi("./");
	quote_api_->RegisterFront(quote_front);
	quote_api_->RegisterSpi(p_quote_handler);
	quote_api_->Init();
	cout<<"this is a test for CTP api"<<endl;
	
	trade_handler_t *p_trade_handler = new trade_handler_t;
	trade_api_ = CThostFtdcTraderApi::CreateFtdcTraderApi(dir_tbug);
	trade_api_->RegisterFront(trade_front);
	trade_api_->RegisterSpi(p_trade_handler);
	trade_api_->SubscribePrivateTopic(THOST_TERT_QUICK);
	trade_api_->SubscribePublicTopic(THOST_TERT_QUICK);
	trade_api_->Init();
	char s;
	while(1){
		cin>>s;
		switch(s){
			case 'i':
				
				insert_order(trade_api_);break;

		}
	}
	return 0;
}
