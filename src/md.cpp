#include "../channel/ctp/ThostFtdcMdApi.h"
#include "../channel/ctp/ThostFtdcTraderApi.h"

#include <float.h>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <chrono>
#include <memory>
#include <ctime>
#include <thread>
#include <cstring>
#include <list>
#include <mutex>
#include <condition_variable>
#include <signal.h>
using namespace std;
fstream f;
fstream md_file;
string md_file_name;
int reqid = 0;
mutex mtx;
condition_variable cond;
list<shared_ptr<CThostFtdcDepthMarketDataField>> mdlist;
list<string> sublist;

void log(string msg)
{
	time_t t = time(nullptr);
	f<<ctime(&t)<<"\t"<<msg<<endl;
};
class CtpMdHandler : public CThostFtdcMdSpi
{
public:
    CtpMdHandler(CThostFtdcMdApi *api) : md_api(api) {}
    virtual void OnFrontConnected()
    {
		log(__func__);
        CThostFtdcReqUserLoginField login = {0};
        strcpy(login.UserID, "mymd");
        strcpy(login.Password, "mymd");
        md_api->ReqUserLogin(&login, reqid++);
    };
    virtual void OnFrontDisconnected(int nReason)
    {
		log(__func__);
    };
    virtual void OnHeartBeatWarning(int nTimeLapse)
	{
		log(__func__);
	};
    virtual void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
    {
		log(__func__);
        if (pRspInfo == nullptr || pRspInfo->ErrorID == 0)
        {
            f << "登陆成功" << endl;
        }
        else
        {
            f << "登陆失败 : errorid " << pRspInfo->ErrorID << ", errormsg " << pRspInfo->ErrorMsg << endl;
            exit(1);
        }
        f << md_api->GetApiVersion() << endl;
        md_file_name = md_api->GetTradingDay();
        f << md_file_name << endl;
        md_file_name += ".md";
        md_file.open(md_file_name, ios_base::out | ios_base::app);
        if (md_file.bad())
        {
            f << "行情文件打开失败" << endl;
            cerr << "行情文件打开失败" << endl;
            exit(1);
        }
        for (auto i : sublist)
        {
            const char *ins[1] = {i.c_str()};
            md_api->SubscribeMarketData((char **)ins, 1);
        }
    };
    virtual void OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
	{
		log(__func__);
	};
    virtual void OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
	{
		log(__func__);
	};
    virtual void OnRspSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast){
		log(__func__);
        if(pRspInfo!=nullptr && pRspInfo->ErrorID!=0){
            f<<"订阅失败: errorid "<<pRspInfo->ErrorID<<", errormsg "<<pRspInfo->ErrorMsg<<endl;
            f<<"订阅合约: "<<pSpecificInstrument->InstrumentID<<endl;
        }
    }
    virtual void OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
	{
		log(__func__);
	}
    virtual void OnRspSubForQuoteRsp(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
	{
		log(__func__);
	};
    virtual void OnRspUnSubForQuoteRsp(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
	{
		log(__func__);
    };
    virtual void OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData){
        shared_ptr<CThostFtdcDepthMarketDataField> md(new CThostFtdcDepthMarketDataField);
        *md = *pDepthMarketData;
		{
        lock_guard<mutex> lk(mtx);
        mdlist.push_back(md);
		}
		cond.notify_one();
    };
    virtual void OnRtnForQuoteRsp(CThostFtdcForQuoteRspField *pForQuoteRsp){};

private:
    CThostFtdcMdApi *md_api;
};

void write_md()
{
    while (true)
    {
        shared_ptr<CThostFtdcDepthMarketDataField> md;
        {
            unique_lock<mutex> lk(mtx);
            while (mdlist.size() == 0)
            {
                cond.wait(lk);
            }
            md = mdlist.front();
            mdlist.pop_front();
        }
        md_file<<md->TradingDay<<","
        <<md->InstrumentID<<","
        <<md->LastPrice<<","
        <<md->PreSettlementPrice<<","
        <<md->PreClosePrice<<","
        <<md->PreOpenInterest<<","
        <<md->OpenPrice<<","
        <<md->HighestPrice<<","
        <<md->LowestPrice<<","
        <<md->Volume<<","
        <<md->Turnover<<","
	    <<md->OpenInterest<<","
        <<(md->ClosePrice == DBL_MAX ? 0 : md->ClosePrice)<<","
        <<(md->SettlementPrice == DBL_MAX ? 0 : md->SettlementPrice)<<","
        <<md->UpperLimitPrice<<","
        <<md->LowerLimitPrice<<","
        <<md->UpdateTime<<'.'<<md->UpdateMillisec<<","
        <<md->BidPrice1<<","
        <<md->BidVolume1<<","
        <<md->AskPrice1<<","
        <<md->AskVolume1<<","
        <<md->AveragePrice<<","
        <<md->ActionDay<<endl;
    }
}
int main(int argc, char **argv)
{
signal(SIGHUP, SIG_IGN);
    time_t t = chrono::system_clock::to_time_t(chrono::system_clock::now());
    std::tm tm = *std::localtime(&t);
    char buffer[128] = {0};
    sprintf(buffer, "%04d-%02d-%02d_%02d%02d%02d.md.log", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    cout << "log file:" << buffer << endl;

    f.open(buffer, ios_base::out | ios_base::app);

    if (f.bad())
    {
        cerr << "日志初始化失败" << endl;
        exit(1);
    }
    else
    {
		log("日志启动成功");
    }

    fstream subfile;
    subfile.open("instruments.txt", ios_base::in);
    if (subfile.is_open() == false)
    {
        string msg("订阅列表文件 instruments.txt 打开失败");
        cerr << msg << endl;
		log(msg);
        exit(1);
    }
    else
    {
        while (subfile.eof() == false)
        {
            char buffer[64] = {0};
            subfile.getline(buffer, 64);
            cout << buffer << endl;
            sublist.push_back(buffer);
        }
        subfile.close();
    }

    CThostFtdcMdApi *md_api = CThostFtdcMdApi::CreateFtdcMdApi();
    if (md_api == nullptr)
    {
		log("行情Api创建失败");
        exit(1);
    }
    else
    {
		log("行情Api创建成功");
    }

    CtpMdHandler *mdHandler = new CtpMdHandler(md_api);
    if (mdHandler == nullptr)
    {
		log("行情处理器创建失败");
        cerr << "行情处理器创建失败" << endl;
        exit(1);
    }
    else
    {
        f << "行情处理器创建成功" << endl;
    }
    

    thread write_thread(write_md);
    md_api->RegisterSpi(mdHandler);
    char front[128] = "tcp://183.194.213.67:53313";
    if(argc == 1){
		log("使用默认行情服务器地址 tcp://183.194.213.67:53313");
        md_api->RegisterFront(front);
    }else{
		string msg;
		msg +="使用提供的行情服务器地址 ";
		msg += argv[1];
		log(msg);
        md_api->RegisterFront(argv[1]);
    }
    md_api->Init();
    string input;
    cin >> input;
    while (input != "exit")
    {
        this_thread::sleep_for(chrono::seconds(2));
    }
    cout << md_api->GetApiVersion();
    cout << md_api->GetTradingDay();
    cout << "" << endl;
}
