#ifndef POSITION_MANAGER_H__
#define POSITION_MANAGER_H__
#include <map>
#include <string>
#include "../include/data_types.h"
#include "../include/safe_list.h"

using namespace std;
class PositionManager
{
public:
	void UpdateQryMatch(string instrument, EOpenClose oc, ELongShort ls, int volume, double price);
	void UpdateYesterdayPosition(string instrument, ELongShort ls, int volume, double price);
	void UpdatePosition(string instrument, EOpenClose oc, ELongShort ls, int volume, double price, EPositionType pt=P_LONGSHORT);
	void ShowPosition(const char* ins=NULL);
	void ShowQryMatch();
	int  GetPosition(const char* ins, EPositionType posType);
	void GeneratePositionFromQryMatch();	

	double GetHeadSpread(const char *forward, const char *recent,
						ELongShort ls, bool forwardIsMain);
	double GetAverageSpread(const char *forward, const char *recent, 
						int volumeYesterday, int volumeToday,
						ELongShort ls, bool forwardIsMain);
public:
	struct PositionEntry{
		string instrument;
		int volume;
		double price;
		EPositionType positionType;
		bool operator <(const PositionEntry& ref){
			return price - ref.price < 0.000001;	
		}
	};
	struct Position{
		int totalPosition;
		list<PositionEntry> positionList;
	};
	struct QryMatch{
		list<PositionEntry> qryMatchList[E_OPENCLOSE][E_LONGSHORT];
	};
	struct YesterdayPosition{
		int long_volume;
		int short_volume;
		double long_price;
		double short_price;
	};
	struct PositionInfo{
		string instrument;
		Position position[P_LONGSHORT];
	};
private:
	map<string, PositionInfo> insPositionInfo;
	map<string, QryMatch> instrument_qry_match;
	map<string, YesterdayPosition> instrument_yesterday_position;
};
#endif /* POSITION_MANAGER_H__ */
