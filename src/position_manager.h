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
	void ShowPosition(const char* ins=NULL);
	int  GetPosition(const char* ins, EPositionType posType);
private:
	struct Positions{
		int position[P_LONGSHORT];	
	};
	map<string, Positions> instrument_position_info;
};
#endif /* POSITION_MANAGER_H__ */
