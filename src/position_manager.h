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
	void UpdatePosition(string instrument, EOpenClose oc, ELongShort ls, int volume, double price, EPositionType pt=P_LONGSHORT);
	void ShowPosition(const char* ins=NULL);
	int  GetPosition(const char* ins, EPositionType posType);
private:
	void GeneratePositionFromQryMatch();	
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
	struct PositionInfo{
		string instrument;
		Position position[P_LONGSHORT];
	};

	map<string, PositionInfo> instrument_position_info;
	map<string, QryMatch> instrument_qry_match;
	
};
#endif /* POSITION_MANAGER_H__ */
