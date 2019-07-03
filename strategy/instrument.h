#ifndef INSTRUMENT_H__
#define INSTRUMENT_H__
#include <list>
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
	Instrument(char*);
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
	void FullOpenLong(int lockedPosition);
	void DoNotFullOpenLong();

	void FullCloseLong(int lockedPosition);
	void DoNotFullCloseLong();

	void FullOpenShort(int lockedPosition);
	void DoNotFullOpenShort();

	void FullCloseShort(int lockedPosition);
	void DoNotFullCloseShort();

	void UpdateLockedSpread(LockedSpread &lockedSpread, bool isStopLoss, bool isToday);
	double GetAverageSpread();
	double GetBadSpread();
	bool IsOpenLong();
	bool IsOpenShort();
	bool IsCloseLong();
	bool IsCloseShort();
	bool IsStopLoss(double tradedSpread);
static void ShowLockedSpread();
	char 		name[64];
	bool 		reached;
	InsType 	insType;
	Instrument	*relativeIns;
	Quote		*lastQuote;
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
};
#endif /* INSTRUMENT_H__ */ 
