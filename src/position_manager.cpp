#include "position_manager.h"
void PositionManager::UpdatePosition(string instrument, EOpenClose oc, ELongShort ls, int volume, double price, EPositionType  pt)
{
//if openclose is open, add a new entry at positionlist tail
//if openclose is close, sub volume from positionlist head	
	if(instrument_position_info.count(instrument)==0){
		instrument_position_info.insert(make_pair(instrument, PositionInfo()));
	}
	switch(oc){
		case E_OPEN:{
			PositionEntry pe;
			pe.instrument = instrument;
			pe.volume = volume;
			pe.price = price;
			pe.positionType = (pt==P_LONGSHORT) ? (ls==E_LONG ? P_LONG : P_SHORT) : pt;
			instrument_position_info[instrument].position[pe.positionType].positionList.push_back(pe);
			instrument_position_info[instrument].position[pe.positionType].totalPosition += volume;
		}
		break;
		case E_CLOSE:
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
					iter->second.position[P_LONG].totalPosition,
					iter->second.position[P_SHORT].totalPosition);
		}
	}else if(instrument_position_info.count(ins) > 0){
		PositionInfo *tmp = &instrument_position_info[ins];
		printf("%s\t\t%d\t%d\t%d\t%d\n", ins,
				tmp->position[P_YESTERDAY_LONG].totalPosition,
				tmp->position[P_YESTERDAY_SHORT].totalPosition,
				tmp->position[P_LONG].totalPosition,
				tmp->position[P_SHORT].totalPosition);
	}else{
		printf("%s\t\t%d\t%d\t%d\t%d\n", ins, 0, 0, 0, 0);
	}
}

void PositionManager::UpdateQryMatch(string instrument, EOpenClose oc, ELongShort ls, int volume, double price)
{
	if(instrument_qry_match.count(instrument)==0){
		instrument_qry_match.insert(make_pair(instrument, QryMatch()));
	}
	PositionEntry positionEntry;
	positionEntry.instrument = instrument;
	positionEntry.volume = volume;
	positionEntry.price = price;
	positionEntry.positionType = P_LONGSHORT;
	instrument_qry_match[instrument].qryMatchList[oc][ls].push_back(positionEntry);
}

void PositionManager::GeneratePositionFromQryMatch()
{


}

void PositionManager::UpdateYesterdayPosition(string instrument, ELongShort ls, int volume, double price)
{
	if(instrument_yesterday_position.count(instrument)==0){
		instrument_qry_match.insert(make_pair(instrument, YesterdayPosition()));
	}
	if(ls == E_LONG){
		instrument_yesterday_position[instrument].long_volume = volume;
		instrument_yesterday_position[instrument].long_price = price;
	}else{
		instrument_yesterday_position[instrument].short_volume = volume;
		instrument_yesterday_position[instrument].short_price = price;
	}
}
