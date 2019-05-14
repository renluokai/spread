#include "position_manager.h"
void PositionManager::UpdatePosition(string instrument, EOpenClose oc, ELongShort ls, int volume, double price)
{
	
	if(instrument_position_info.count(instrument)==0){
		instrument_position_info.insert(make_pair(instrument, PositionInfo()));
	}
	int* p = (int*)&instrument_position_info[instrument].position;
	switch(oc){
		case E_OPEN:
			switch(ls){
				case E_LONG:
					p[P_TODAY_LONG] += volume;
					break;
				case E_SHORT:
					p[P_TODAY_SHORT] += volume;
					break;
			}
			break;
		case E_CLOSE:
		case E_CLOSE_T:
			switch(ls){
				case E_LONG:
					p[P_TODAY_LONG] -= volume;
					break;
				case E_SHORT:
					p[P_TODAY_SHORT] -= volume;
					break;
			}
			break;
		case E_CLOSE_Y:
			switch(ls){
				case E_LONG:
					p[P_YESTERDAY_LONG] -= volume;
					break;
				case E_SHORT:
					p[P_YESTERDAY_SHORT] -= volume;
					break;
			}
			break;
	}	
	return;
}

int PositionManager::GetPosition(const char* ins, EPositionType posType)
{
	if(instrument_position_info.count(ins) > 0 ){
		return instrument_position_info[ins].position[posType].totalPosition;
	}
	return 0;
}

void PositionManager::ShowPosition(const char* ins)
{
	printf("INSTRUMENT\tY-L\tY-S\tT-L\tT-S\n");
	if(ins == NULL){
		map<string, PositionInfo>::iterator iter = instrument_position_info.begin();
		for(; iter!= instrument_position_info.end(); iter++){
			printf("%s\t\t%d\t%d\t%d\t%d\n", iter->first.c_str(),
					iter->second.position[P_YESTERDAY_LONG].totalPosition,
					iter->second.position[P_YESTERDAY_SHORT].totalPosition,
					iter->second.position[P_TODAY_LONG].totalPosition,
					iter->second.position[P_TODAY_SHORT].totalPosition);
		}
	}else if(instrument_position_info.count(ins) > 0){
		PositionInfo *tmp = &instrument_position_info[ins];
		printf("%s\t\t%d\t%d\t%d\t%d\n", ins,
				tmp->position[P_YESTERDAY_LONG].totalPosition,
				tmp->position[P_YESTERDAY_SHORT].totalPosition,
				tmp->position[P_TODAY_LONG].totalPosition,
				tmp->position[P_TODAY_SHORT].totalPosition);
	}else{
		printf("%s\t\t%d\t%d\t%d\t%d\n", ins, 0, 0, 0, 0);
	}
}
