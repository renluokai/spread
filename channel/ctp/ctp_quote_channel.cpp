#include <time.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <float.h>
#include "ctp_quote_channel.h"
#define STRCPY(a,b) strncpy((a),(b),sizeof(a))
CtpQuoteChannel::CtpQuoteChannel()
{
	login_ok_ = false;
	request_id_ = 0;
}

CtpQuoteChannel::~CtpQuoteChannel()
{
	std::list<char**>::iterator iter;
	iter = instruments_.begin();
	for(;iter != instruments_.end(); iter++){
		char **p = *iter;
		delete [] p;
		p = NULL;
	}
}

bool CtpQuoteChannel::open(Config *cfg, Handler *hdlr)
{
	cfg_ = cfg;
	handler_ = hdlr;

	int ret = 0;
	ret = sem_init(&quit_sem_, 0, 0);
	if (ret != 0)return false;
	ret = sem_init(&request_sem_, 0, 0);
	if (ret != 0)return false;
	ret = sem_init(&sem_, 0, 0);
	if (ret != 0)return false;

	time_t now_sec = time(NULL);
	tm tm_now = *localtime(&now_sec);
	char buffer[256] = {0};
	sprintf(buffer, "./ctp_quote_channel_%04d%02d%02d_%02d%02d%02d.log",
			tm_now.tm_year+1900, tm_now.tm_mon+1, tm_now.tm_mday,
			tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec);
	log_stream_.open(buffer, std::fstream::out|std::fstream::app);
	log_stream_<<"*****************CTP QUOTE CHANNEL STARTED*****************"<<std::endl;
	quote_api_ = CThostFtdcMdApi::CreateFtdcMdApi("./");
	if(quote_api_ == NULL){
		return false;
	}
	quote_api_->RegisterSpi(this);
	quote_api_->RegisterFront(cfg_->front);
	quote_api_->Init();

	struct timespec ts;	
	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_sec += 30;
	ret = sem_timedwait(&sem_, &ts);
	printf("ret status from channel: %d\n",ret);
	perror("");
	if(ret != 0){
		return false;
	}
	return true;
}
bool CtpQuoteChannel::close()
{
	sem_post(&quit_sem_);
	return true;
};
bool CtpQuoteChannel::subscribe(const char *ins)
{
	char ** instrument = new char*[1];	
	instrument[0] = const_cast<char*>(ins);
	
	int ret = quote_api_->SubscribeMarketData(instrument, 1);
log_stream_<<"["<<__FUNCTION__<<"] "<<instrument[0]<<std::endl;
	instruments_.push_back(instrument);
	return !ret;
}

bool CtpQuoteChannel::unsubscribe(const char *ins)
{
	char ** instrument = new char*[1];	
	instrument[0] = const_cast<char*>(ins);
	
	int ret = quote_api_->UnSubscribeMarketData(instrument, 1);
	instruments_.push_back(instrument);
	return !ret;
}	

void CtpQuoteChannel::OnFrontConnected()
{
	if(login_ok_ == false){
		CThostFtdcReqUserLoginField UserLoginField = {0};
		STRCPY(UserLoginField.BrokerID, cfg_->broker_id);
		STRCPY(UserLoginField.UserID, cfg_->user);
		STRCPY(UserLoginField.Password, cfg_->password);
				
log_stream_<<"["<<__FUNCTION__<<"] ["<<UserLoginField.BrokerID
<<"] ["<<UserLoginField.UserID<<"] ["<<UserLoginField.Password<<std::endl;
		quote_api_->ReqUserLogin(&UserLoginField, request_id_++);
	}
}

void CtpQuoteChannel::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if(pRspInfo == NULL || pRspInfo->ErrorID == 0){
		login_ok_ = true;
		sem_post(&sem_);
	}
}

void CtpQuoteChannel::OnRspSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
		
}
void CtpQuoteChannel::OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData)
{
log_stream_<<"["<<__FUNCTION__<<"] "
<<pDepthMarketData->TradingDay<<" "
<<pDepthMarketData->InstrumentID<<" "
<<pDepthMarketData->ExchangeID<<" "
<<pDepthMarketData->ExchangeInstID<<" "
<<pDepthMarketData->LastPrice<<" "
<<pDepthMarketData->PreSettlementPrice<<" "
<<pDepthMarketData->PreClosePrice<<" "
<<pDepthMarketData->PreOpenInterest<<" "
<<pDepthMarketData->OpenPrice<<" "
<<pDepthMarketData->HighestPrice<<" "
<<pDepthMarketData->LowestPrice<<" "
<<pDepthMarketData->Volume<<" "
<<pDepthMarketData->Turnover<<" "
<<pDepthMarketData->OpenInterest<<" "
<<(pDepthMarketData->ClosePrice==DBL_MAX?0:pDepthMarketData->ClosePrice)<<" "
<<(pDepthMarketData->SettlementPrice==DBL_MAX?0:pDepthMarketData->SettlementPrice)<<" "
<<pDepthMarketData->UpperLimitPrice<<" "
<<pDepthMarketData->LowerLimitPrice<<" "
<<pDepthMarketData->PreDelta<<" "
<<(pDepthMarketData->CurrDelta==DBL_MAX?0:pDepthMarketData->CurrDelta)<<" "
<<pDepthMarketData->UpdateTime<<"."
<<pDepthMarketData->UpdateMillisec<<" "
<<pDepthMarketData->BidPrice1<<" "
<<pDepthMarketData->BidVolume1<<" "
<<pDepthMarketData->AskPrice1<<" "
<<pDepthMarketData->AskVolume1<<" "

<<pDepthMarketData->AveragePrice<<" "
<<pDepthMarketData->ActionDay<<std::endl;
Quote qt;
STRCPY(qt.InstrumentID,pDepthMarketData->InstrumentID);
	qt.LastPrice	= pDepthMarketData->LastPrice;
	qt.AveragePrice	= pDepthMarketData->AveragePrice;
	qt.BidPrice1	= pDepthMarketData->BidPrice1;
	qt.BidVolume1	= pDepthMarketData->BidVolume1;
	qt.AskPrice1	= pDepthMarketData->AskPrice1;
	qt.AskVolume1	= pDepthMarketData->AskVolume1;
	qt.TotalVolume	= pDepthMarketData->Volume;
	sprintf(qt.UpdateTime,"%s.%03d",pDepthMarketData->UpdateTime,\
		pDepthMarketData->UpdateMillisec);
	Time::FullTime(qt.LocalTime);

	Quote* q=&qt;
	handler_->push(&qt);
}
#if 0
class CThostFtdcMdSpi
{
public:
	
	///当客户端与交易后台通信连接断开时，该方法被调用。当发生这个情况后，API会自动重新连接，客户端可不做处理。
	///@param nReason 错误原因
	///        0x1001 网络读失败
	///        0x1002 网络写失败
	///        0x2001 接收心跳超时
	///        0x2002 发送心跳失败
	///        0x2003 收到错误报文
	virtual void OnFrontDisconnected(int nReason){};
		
	///登录请求响应
	virtual void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

	///登出请求响应
	virtual void OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

	///错误应答
	virtual void OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

	///订阅行情应答

	///取消订阅行情应答
	virtual void OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

	///订阅询价应答
	virtual void OnRspSubForQuoteRsp(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

	///取消订阅询价应答
	virtual void OnRspUnSubForQuoteRsp(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};


	///询价通知
	virtual void OnRtnForQuoteRsp(CThostFtdcForQuoteRspField *pForQuoteRsp) {};
};

class MD_API_EXPORT CThostFtdcMdApi
{
public:
	///创建MdApi
	///@param pszFlowPath 存贮订阅信息文件的目录，默认为当前目录
	///@return 创建出的UserApi
	///modify for udp marketdata
	
	///获取API的版本信息
	///@retrun 获取到的版本号
	static const char *GetApiVersion();
	
	///删除接口对象本身
	///@remark 不再使用本接口对象时,调用该函数删除接口对象
	virtual void Release() = 0;
	
	
	///等待接口线程结束运行
	///@return 线程退出代码
	virtual int Join() = 0;
	
	///获取当前交易日
	///@retrun 获取到的交易日
	///@remark 只有登录成功后,才能得到正确的交易日
	virtual const char *GetTradingDay() = 0;
	

	///注册名字服务器网络地址
	///@param pszNsAddress：名字服务器网络地址。
	///@remark 网络地址的格式为：“protocol://ipaddress:port”，如：”tcp://127.0.0.1:12001”。 
	///@remark “tcp”代表传输协议，“127.0.0.1”代表服务器地址。”12001”代表服务器端口号。
	///@remark RegisterNameServer优先于RegisterFront
	virtual void RegisterNameServer(char *pszNsAddress) = 0;
	
	///注册名字服务器用户信息
	///@param pFensUserInfo：用户信息。
	virtual void RegisterFensUserInfo(CThostFtdcFensUserInfoField * pFensUserInfo) = 0;
	
	
	///订阅行情。
	///@param ppInstrumentID 合约ID  
	///@param nCount 要订阅/退订行情的合约个数
	///@remark 
	virtual int SubscribeMarketData(char *ppInstrumentID[], int nCount) = 0;

	///退订行情。
	///@param ppInstrumentID 合约ID  
	///@param nCount 要订阅/退订行情的合约个数
	///@remark 
	virtual int UnSubscribeMarketData(char *ppInstrumentID[], int nCount) = 0;
	
	///订阅询价。
	///@param ppInstrumentID 合约ID  
	///@param nCount 要订阅/退订行情的合约个数
	///@remark 
	virtual int SubscribeForQuoteRsp(char *ppInstrumentID[], int nCount) = 0;

	///退订询价。
	///@param ppInstrumentID 合约ID  
	///@param nCount 要订阅/退订行情的合约个数
	///@remark 
	virtual int UnSubscribeForQuoteRsp(char *ppInstrumentID[], int nCount) = 0;

	///用户登录请求
	

	///登出请求
	virtual int ReqUserLogout(CThostFtdcUserLogoutField *pUserLogout, int nRequestID) = 0;
protected:
	~CThostFtdcMdApi(){};
};


#endif
