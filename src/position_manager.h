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
	void UpdatePosition(string instrument, EOpenClose oc, ELongShort ls, int volume, double price, EPositionType pt=P_LONGSHORT);
	void ShowPosition(const char* ins=NULL);
	int  GetPosition(const char* ins, EPositionType posType);
private:
	
	struct PositionEntry{
		string instrument;
		int volume;
		double price;
		EPositionType positionType;
	};
	struct Position{
		int totalPosition;
		list<PositionEntry> positionList;
	};
	struct PositionInfo{
		string instrument;
		Position position[P_LONGSHORT];
	};
	map<string, PositionInfo> instrument_position_info;
};
#endif /* POSITION_MANAGER_H__ */
