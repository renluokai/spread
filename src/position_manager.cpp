#include <algorithm>    // std::for_each
#include <iostream>
#include "position_manager.h"
void PositionManager::UpdatePosition(string instrument, EOpenClose oc, ELongShort ls, int volume, double price, EPositionType  pt)
{
//if openclose is open, add a new entry at positionlist tail
//if openclose is close, sub volume from positionlist head	
    cout<<__FUNCTION__<<endl<<instrument<<"\t"<<volume<<"\t"<<price<<endl;
	if(instrument_position_info.count(instrument)==0){
		instrument_position_info.insert(make_pair(instrument, PositionInfo()));
	}
	switch(oc){
		case E_OPEN:{
			PositionEntry pe;
			pe.instrument = instrument;
			pe.volume = volume;
			pe.price = price;
			pe.positionType = (pt==P_LONGSHORT) ? (ls==E_LONG ? P_TODAY_LONG : P_TODAY_SHORT) : pt;
			instrument_position_info[instrument].position[pe.positionType].positionList.push_back(pe);
			instrument_position_info[instrument].position[pe.positionType].totalPosition += volume;
		}
		break;
		case E_CLOSE:
		//TODO
		break;
		case E_CLOSE_T:
			if(ls==E_LONG){
				instrument_position_info[instrument].position[P_TODAY_LONG].positionList.sort();
				while(true){
					PositionEntry *pe = &instrument_position_info[instrument].position[P_TODAY_LONG].positionList.front();
					instrument_position_info[instrument].position[P_TODAY_LONG].totalPosition -= volume;
					if(pe->volume > volume){
						pe->volume -= volume;	
						break;
					}else{
						volume -= pe->volume;
					}
					
					instrument_position_info[instrument].position[P_TODAY_LONG].positionList.pop_front();
					if(volume == 0){
						break;
					}
				}
			}else{
				instrument_position_info[instrument].position[P_TODAY_SHORT].positionList.sort();
				while(true){
					PositionEntry *pe = &instrument_position_info[instrument].position[P_TODAY_SHORT].positionList.front();
					instrument_position_info[instrument].position[P_TODAY_SHORT].totalPosition -= volume;
					if(pe->volume > volume){
						pe->volume -= volume;	
						break;
					}else{
						volume -= pe->volume;
					}
					
					instrument_position_info[instrument].position[P_TODAY_SHORT].positionList.pop_front();
					if(volume == 0){
						break;
					}
				}
			}
		break;
		case E_CLOSE_Y:
			if(ls==E_LONG){
				instrument_position_info[instrument].position[P_YESTERDAY_LONG].positionList.sort();
				while(true){
					PositionEntry *pe = &instrument_position_info[instrument].position[P_YESTERDAY_LONG].positionList.front();
					instrument_position_info[instrument].position[P_YESTERDAY_LONG].totalPosition -= volume;
					if(pe->volume > volume){
						pe->volume -= volume;	
						break;
					}else{
						volume -= pe->volume;
					}
					
					instrument_position_info[instrument].position[P_YESTERDAY_LONG].positionList.pop_front();
					if(volume == 0){
						break;
					}
				}
			}else{
				instrument_position_info[instrument].position[P_YESTERDAY_SHORT].positionList.sort();
				while(true){
					PositionEntry *pe = &instrument_position_info[instrument].position[P_YESTERDAY_SHORT].positionList.front();
					instrument_position_info[instrument].position[P_YESTERDAY_SHORT].totalPosition -= volume;
					if(pe->volume > volume){
						pe->volume -= volume;	
						break;
					}else{
						volume -= pe->volume;
					}
					
					instrument_position_info[instrument].position[P_YESTERDAY_SHORT].positionList.pop_front();
					if(volume == 0){
						break;
					}
				}
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

void PositionManager::UpdateQryMatch(string instrument, EOpenClose oc, ELongShort ls, int volume, double price)
{
	const char *o=NULL;
	const char *l=NULL;
	switch(oc){
	case E_OPEN:
		o = "E_OPEN\t";break;
	case E_CLOSE:
		o = "E_CLOSE\t";break;
	case E_CLOSE_T:
		o = "E_CLOSE_T\t";break;
	case E_CLOSE_Y:
		o = "E_CLOSE_Y\t";break;
	}
	switch(ls){
	case E_LONG:
		l = "E_LONG";break;
	case E_SHORT:
		l = "E_SHORT";break;
	}
	cout<<instrument<<"\t"<<volume<<"\t"<<price<<o<<l<<endl;
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
			
	map<string, YesterdayPosition>::iterator iter;
	iter = instrument_yesterday_position.begin();
	for(;iter!=instrument_yesterday_position.end();iter++){
		UpdatePosition(iter->first, E_OPEN, E_LONG,
						iter->second.long_volume,
						iter->second.long_price,
						P_YESTERDAY_LONG);
		UpdatePosition(iter->first, E_OPEN, E_SHORT,
						iter->second.short_volume,
						iter->second.short_price,
						P_YESTERDAY_SHORT);
	}
	
	map<string, QryMatch>::iterator qmIter=instrument_qry_match.begin();
	for(;qmIter!=instrument_qry_match.end();qmIter++){
		list<PositionEntry>::iterator peIter;
		qmIter->second.qryMatchList[E_OPEN][E_LONG].sort();
		peIter = qmIter->second.qryMatchList[E_OPEN][E_LONG].begin();
		for(;peIter != qmIter->second.qryMatchList[E_OPEN][E_LONG].end();
			peIter++){
				UpdatePosition(qmIter->first, E_OPEN, E_LONG,
						peIter->volume,
						peIter->price,
						P_TODAY_LONG);
		}

		qmIter->second.qryMatchList[E_OPEN][E_SHORT].sort();
		peIter = qmIter->second.qryMatchList[E_OPEN][E_SHORT].begin();
		for(;peIter != qmIter->second.qryMatchList[E_OPEN][E_SHORT].end();
			peIter++){
				UpdatePosition(qmIter->first, E_OPEN, E_SHORT,
						peIter->volume,
						peIter->price,
						P_TODAY_SHORT);
		}

		qmIter->second.qryMatchList[E_CLOSE_Y][E_LONG].sort();
		peIter = qmIter->second.qryMatchList[E_CLOSE_Y][E_LONG].begin();
		for(;peIter != qmIter->second.qryMatchList[E_CLOSE_Y][E_LONG].end();
			peIter++){
				UpdatePosition(qmIter->first, E_CLOSE_Y, E_LONG,
						peIter->volume,
						peIter->price);
		}

		qmIter->second.qryMatchList[E_CLOSE_Y][E_SHORT].sort();
		peIter = qmIter->second.qryMatchList[E_CLOSE_Y][E_SHORT].begin();
		for(;peIter !=qmIter->second.qryMatchList[E_CLOSE_Y][E_SHORT].end();
			peIter++){
				UpdatePosition(qmIter->first, E_CLOSE_Y, E_SHORT,
						peIter->volume,
						peIter->price);
		}

		qmIter->second.qryMatchList[E_CLOSE_T][E_LONG].sort();
		peIter = qmIter->second.qryMatchList[E_CLOSE_T][E_LONG].begin();
		for(;peIter != qmIter->second.qryMatchList[E_CLOSE_T][E_LONG].end();
			peIter++){
				UpdatePosition(qmIter->first, E_CLOSE_T, E_LONG,
						peIter->volume,
						peIter->price);
		}

		qmIter->second.qryMatchList[E_CLOSE_T][E_SHORT].sort();
		peIter = qmIter->second.qryMatchList[E_CLOSE_T][E_SHORT].begin();
		for(;peIter !=qmIter->second.qryMatchList[E_CLOSE_T][E_SHORT].end();
			peIter++){
				UpdatePosition(qmIter->first, E_CLOSE_T, E_SHORT,
						peIter->volume,
						peIter->price);
		}
	}
}

void PositionManager::UpdateYesterdayPosition(string instrument, ELongShort ls, int volume, double price)
{
	if(instrument_yesterday_position.count(instrument)==0){
		instrument_yesterday_position.insert(make_pair(instrument, YesterdayPosition()));
	}
	if(ls == E_LONG){
		instrument_yesterday_position[instrument].long_volume = volume;
		instrument_yesterday_position[instrument].long_price = price;
	}else{
		instrument_yesterday_position[instrument].short_volume = volume;
		instrument_yesterday_position[instrument].short_price = price;
	}
}
void ShowMatchEntry(PositionManager::PositionEntry pe){
	cout<<pe.instrument<<"\t"<<pe.volume<<"\t"<<pe.price<<endl;
}

void PositionManager::ShowQryMatch()
{
	map<string, QryMatch>::iterator iter;
	iter = instrument_qry_match.begin();
	for(;iter!=instrument_qry_match.end(); iter++){
		if(iter->second.qryMatchList[E_OPEN][E_LONG].size()>0){
			cout<<"----------open long----------"<<endl;
			for_each((iter->second.qryMatchList)[E_OPEN][E_LONG].begin(),
				(iter->second.qryMatchList)[E_OPEN][E_LONG].end(),
				ShowMatchEntry);
		}
		if((iter->second.qryMatchList)[E_CLOSE][E_LONG].size()>0){
			cout<<"----------close long----------"<<endl;
			for_each((iter->second.qryMatchList)[E_CLOSE][E_LONG].begin(),
				(iter->second.qryMatchList)[E_CLOSE][E_LONG].end(),
				ShowMatchEntry);
		}
		if((iter->second.qryMatchList)[E_CLOSE_T][E_LONG].size()>0){
			cout<<"----------close today long----------"<<endl;
			for_each((iter->second.qryMatchList)[E_CLOSE_T][E_LONG].begin(),
				(iter->second.qryMatchList)[E_CLOSE_T][E_LONG].end(),
				ShowMatchEntry);
		}
		if((iter->second.qryMatchList)[E_CLOSE_Y][E_LONG].size()>0){
			cout<<"----------close yesterday long----------"<<endl;
			for_each((iter->second.qryMatchList)[E_CLOSE_Y][E_LONG].begin(),
				(iter->second.qryMatchList)[E_CLOSE_Y][E_LONG].end(),
				ShowMatchEntry);
		}

		if(iter->second.qryMatchList[E_OPEN][E_SHORT].size()>0){
			cout<<"----------open short----------"<<endl;
			for_each((iter->second.qryMatchList)[E_OPEN][E_SHORT].begin(),
				(iter->second.qryMatchList)[E_OPEN][E_SHORT].end(),
				ShowMatchEntry);
		}
		if((iter->second.qryMatchList)[E_CLOSE][E_SHORT].size()>0){
			cout<<"----------close short----------"<<endl;
			for_each((iter->second.qryMatchList)[E_CLOSE][E_SHORT].begin(),
				(iter->second.qryMatchList)[E_CLOSE][E_SHORT].end(),
				ShowMatchEntry);
		}
		if((iter->second.qryMatchList)[E_CLOSE_T][E_SHORT].size()>0){
			cout<<"----------close today long----------"<<endl;
			for_each((iter->second.qryMatchList)[E_CLOSE_T][E_SHORT].begin(),
				(iter->second.qryMatchList)[E_CLOSE_T][E_SHORT].end(),
				ShowMatchEntry);
		}
		if((iter->second.qryMatchList)[E_CLOSE_Y][E_SHORT].size()>0){
			cout<<"----------close yesterday long----------"<<endl;
			for_each((iter->second.qryMatchList)[E_CLOSE_Y][E_SHORT].begin(),
				(iter->second.qryMatchList)[E_CLOSE_Y][E_SHORT].end(),
				ShowMatchEntry);
		}
	}
}
