#ifndef INSTRUMENT_H__
#define INSTRUMENT_H__
#include <list>
#include <vector>
#include "../include/trader.h"

class Quote;
class Order;
struct LockedSpread{
	int date;
	int volume;
	double spread;
	bool operator <(const LockedSpread& lkP){
		return spread - lkP.spread < 0.000001;
	}
};
struct MatchInfo{
	int date;
	int volume;
	double price;
};
enum InsType
{
	E_INS_INVALID,
	E_INS_FORWARD,
	E_INS_RECENT,
};

enum ForecastType
{
	E_FORECAST_NONE,
	E_FORECAST_UP,
	E_FORECAST_DOWN,
};

enum StopLoss{
	E_STOPLOSS_NO,
	E_STOPLOSS_AVERAGE,
	E_STOPLOSS_TICKBYTICK,
};

enum EDirection
{

	E_DIR_INVALID,
	E_DIR_UP,
	E_DIR_DOWN,
};

class Instrument
{
public:
	double priceTick;
public:
	Instrument(char*, int vf=1, int mf=1);
	void on_quote(Quote*);	
	void on_match(Order*);	
	void on_reject(Order*);	
	void on_cancel(Order*);	
	void on_insert(Order*);	
	void ShowState();
	void ShowQuote();
	void CheckStopLoss();
	void CalcSpread(bool rct=true);
	void CancelOrders(vector<Order*> &ods);
	int	 CalcLockedPosition();
	int	 CalcLockedPositionYesterday();
	int	 CalcLockedPositionToday();
	void CalcQuoteDirection();
	void FullOpenLong(int lockedPosition);
	void DoNotFullOpenLong();

	void FullCloseLong(int lockedPosition);
	void DoNotFullCloseLong();

	void FullOpenShort(int lockedPosition);
	void DoNotFullOpenShort();

	void FullCloseShort(int lockedPosition);
	void DoNotFullCloseShort();

	void ProcessOpenLong(int lockedPosition);
	void ProcessOpenShort(int lockedPosition);

	void UpdateLockedSpread(LockedSpread &lockedSpread, bool isStopLoss, bool isToday);
	double GetAverageSpread();
	double GetBadSpread();
	bool IsOpenLong();
	bool IsOpenShort();
	bool IsCloseLong();
	bool IsCloseShort();
	bool IsStopLoss(double tradedSpread);
	bool IsForecast(EOpenClose oc, ELongShort ls);
static void ShowLockedSpread();

	const double MAIN_INS_GAP = 1.5;
	const double SECOND_MAIN_INS_GAP = 2.5;

	char 		name[64];
	bool 		reached;
	bool		rangeFirst=true;
	InsType 	insType;

	int volumeForecastBase;
	int matchForecastBase;
	ForecastType forecastByVolume;
	int			volumeForecastCnt;
	int			volumeForecastWin;
	int			volumeScore=0;
	ForecastType forecastByMatch;
	int			matchForecastCnt;
	int			matchForecastWin;
	Instrument	*relativeIns;
	Quote		*previousQuote;
	Quote		*currentQuote;
	int			cancelMax;

	Trader		*trader;

	
	static list<LockedSpread> lockedSpreadT;
	static list<LockedSpread> lockedSpreadY;
	static list<MatchInfo> firstOpenMatch;
	static list<MatchInfo> firstCloseMatch;

	static Instrument*	firstOpenIns;
	static Instrument*	firstCloseIns;
	static Instrument*	secondOpenIns;
	static Instrument*	secondCloseIns;
	static Instrument*	mainIns;

	static double		last_bidSpread;
	static double		last_askSpread;
	static double		bidSpread;
	static double		askSpread;
	static double		openThreshold;
	static double		closeThreshold;
	static InsType		openWith;
	static InsType		closeWith; 
	static InsType		triggerSpread; 

	static StopLoss		stopLossType;
	static int			stopLoss;
	static EDirection	direction;
	static int			maxPosition;
	static int			openCount;
	static int			submitMax;
	static bool			loop;
	static bool			needToStopLoss;
	static vector<int>	openTime;
	static vector<int>	closeTime;
	static int forecast_score_openlow;
	static int forecast_score_openhigh;
	static int forecast_score_closelow;
	static int forecast_score_closehigh;

	static int firstOpenInsSubmit;
	static int firstOpenInsMatch;
	static int secondOpenInsSubmit;
	static int secondOpenInsMatch;

	static int firstCloseInsSubmit;
	static int firstCloseInsMatch;
	static int secondCloseInsSubmit;
	static int secondCloseInsMatch;
};
#endif /* INSTRUMENT_H__ */ 
