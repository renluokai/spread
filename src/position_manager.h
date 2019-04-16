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
	void UpdatePosition(string instrument, EOpenClose oc, ELongShort ls, int volume);
private:
	struct Positions{
		int yesterday_long;
		int yesterday_short;
		int today_long;
		int today_short;	
	};
	map<string, Positions> instrument_position_info;
};
#endif /* POSITION_MANAGER_H__ */
