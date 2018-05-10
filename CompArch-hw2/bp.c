/* 046267 Computer Architecture - Spring 2016 - HW #2 */
/* This file should hold your implementation of the predictor simulator */

#include "bp_api.h"

int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize,
             bool isGlobalHist, bool isGlobalTable, int Shared){
	/*
	
				BTB: Local History
	 _______________________________________
	| tag |    target    |  History - BHR   |
	|_____|______________|__________________|
	|_____|______________|__________________| |  
	|_____|______________|__________________| |
	|_____|______________|__________________| btbSize -> from this size derives the branch IP "window".
	|_____|______________|__________________| |
	|_____|______________|__________________| |
	|_____|______________|__________________| |
		|                         |
        |	                      |
	tagSize                    historySize
	
	Table is local or global.
	
	
*/	
	return -1;
}

bool BP_predict(uint32_t pc, uint32_t *dst){
	return false;
}

void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst){
	return;
}

void BP_GetStats(SIM_stats *curStats) {
	return;
}
