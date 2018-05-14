/* 046267 Computer Architecture - Spring 2016 - HW #2 */
/* This file should hold your implementation of the predictor simulator */

#include "bp_api.h"
#include <stdint.h>
#include <malloc.h>
#include <assert.h>
#include <stdio.h>

#define BHR_1BIT_MASK 1; // For BHR length 1 bit
#define BHR_2BIT_MASK 3; // For BHR length 2 bit
#define BHR_3BIT_MASK 7; // For BHR length 3 bit
#define BHR_4BIT_MASK 15; // For BHR length 4 bit
#define BHR_5BIT_MASK 31; // For BHR length 5 bit
#define BHR_6BIT_MASK 63; // For BHR length 6 bit
#define BHR_7BIT_MASK 127; // For BHR length 7 bit
#define BHR_8BIT_MASK 255; // For BHR length 8 bit

#define TARGET_SIZE 30

//// Type definitions ////
typedef enum state{SNT, WNT, WT, ST} Machine_State; // States of 2-bit state machine.
typedef enum prediction{PRED_TAKEN, PRED_NOT_TAKEN} Machine_Prediction;
typedef enum br_event{BR_TAKEN, BR_NOT_TAKEN} Branch_Event;
typedef enum pred_type{GHGT, GHLT, LHLT, LHGT} predType;
typedef enum shared_type{NOT_USING_SHARE, USING_SHARE_LSB, USING_SHARE_MID} sharedType;

typedef struct state_machines{
    Machine_State state;
} state_machine;


// 4 different types of predictors - sub-modules
typedef struct predictors_gg{
    int8_t BHR;
    state_machine* state_machines_array;
    int shared_type;
} GHGTPred;

// Wrapper for sm_arrays
typedef struct sm_arrays_table{
    state_machine* state_machines_array;
}sm_arrays_table;

typedef struct predictors_gl{
    uint8_t BHR;
    sm_arrays_table* sm_table;
} GHLTPred;

typedef struct predictors_ll{
    uint8_t* BHR;
    sm_arrays_table* sm_table;
} LHLTPred;

typedef struct predictors_lg{
    uint8_t* BHR;
    state_machine* state_machines_array;
    int shared_type;
} LHGTPred;


// The predictor module.
// It has 4 sub-modules for each predictor type
// i.e. Global/Local BHR and Global/Local Table
typedef struct pred_module{
    bool isGlobalHist;
    bool isGlobalTable;

    unsigned int btbsize; // Can be: 1,2,4,8,16,32
    unsigned int historySize; // Can be: 1-8 bit
    unsigned int tagSize; // Can be 0-30 bit
    int Shared; // Only when state machines table is global.

    //// BTB table ////
    uint32_t* tags;
    uint32_t* targets;

    //// BHR Mask ////
    int8_t BHR_mask;
    int32_t TAG_mask;

    //// Predicotr type ////
    predType my_predicotr_type;

    //// Predictors ////
    GHGTPred GHGT_pred;
    GHLTPred GHLT_pred;
    LHLTPred LHLT_pred;
    LHGTPred LHGT_pred;
} Predictor;
//// Type definitions end ////


//// Function declarations ////
void print_pred_table();
void vars_init(unsigned btbSize, unsigned historySize, unsigned tagSize,
               bool isGlobalHist, bool isGlobalTable, int Shared);
int two_in_power(int power);
uint8_t index_from_pc(uint32_t pc);
uint32_t tag_from_pc(uint32_t pc);
void set_tag_mask(unsigned int tag_size);
void GHGT_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst);
void GHLT_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst);
void LHLT_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst);
void LHGT_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst);
void update_state(state_machine* machine, bool taken);
Machine_Prediction get_prediction(state_machine* machine);
void select_BHR_mask(unsigned historySize);
//void print_local_pred_table(int btb_size);
void print_local_pred_table(int btb_size, int btb_entry);
uint8_t ms_entry_calc(uint32_t pc, uint8_t BHR, int Shared);
//// Function declarations END ////

//// Global vars ////
Predictor my_predictor; // Global variable.
unsigned int flush_num = 0; // actually its a number of mispredictions.
bool last_prediction_taken;
bool predictor_needs_reset;
unsigned int branch_num = 0;
unsigned int size = 0;
//// Global vars END////

//// Functions implementation ////

// Initialization functions for each prediction module
void GHGT_init(unsigned btbSize, unsigned historySize, unsigned tagSize,
            bool isGlobalHist, bool isGlobalTable, int Shared){

    my_predictor.GHGT_pred.shared_type = Shared;

    size = btbSize*(tagSize + TARGET_SIZE) + historySize + 2*two_in_power(historySize);

    my_predictor.GHGT_pred.BHR = 0;
    select_BHR_mask(historySize);

    //// Size of state machines array depends on History size.
    int sm_table_size = two_in_power(historySize);
    //printf("sm_table_size = %d\n", sm_table_size);
    my_predictor.GHGT_pred.state_machines_array = malloc(sizeof(state_machine)*sm_table_size);
    assert(my_predictor.GHGT_pred.state_machines_array != NULL);

    //// Init the machines for WNT state.
    for(int i=0;i<sm_table_size;i++){
        my_predictor.GHGT_pred.state_machines_array[i].state = WNT;
    }

}

void GHLT_init(unsigned btbSize, unsigned historySize, unsigned tagSize,
              bool isGlobalHist, bool isGlobalTable, int Shared){

    size = btbSize*(tagSize + TARGET_SIZE + 2*two_in_power(historySize)) + historySize;

    select_BHR_mask(historySize);

    // malloc and init BHRs array of btb size.
    my_predictor.GHLT_pred.BHR = 0;

    // malloc btb_size x ms_arrays of size 2^hist_size
    my_predictor.GHLT_pred.sm_table = malloc(sizeof(sm_arrays_table)*btbSize);
    assert(my_predictor.GHLT_pred.sm_table != NULL);
    int sm_array_size = two_in_power(historySize);
    for(int i=0;i<btbSize;i++){
        // In each row in sm table, create an array of sm's.
        my_predictor.GHLT_pred.sm_table[i].state_machines_array = malloc(sizeof(state_machine)*sm_array_size);
        assert(my_predictor.GHLT_pred.sm_table[i].state_machines_array != NULL);
    }

    // Init the machines for WNT state.
    for(int i=0;i<btbSize;i++){
        for (int j = 0; j < sm_array_size; ++j) {
            my_predictor.GHLT_pred.sm_table[i].state_machines_array[j].state = WNT;
        }
    }
}

void LHLT_init(unsigned btbSize, unsigned historySize, unsigned tagSize,
              bool isGlobalHist, bool isGlobalTable, int Shared){

    size = btbSize*(tagSize + TARGET_SIZE + historySize + 2*two_in_power(historySize));

    select_BHR_mask(historySize);

    // malloc and init BHRs array of btb size.
    my_predictor.LHLT_pred.BHR = malloc(sizeof(uint8_t)*btbSize);
    assert(my_predictor.LHLT_pred.BHR != NULL);
    for(int i=0;i<btbSize;i++){
        my_predictor.LHLT_pred.BHR[i] = 0;
    }

    // malloc btb_size x ms_arrays of size 2^hist_size
    my_predictor.LHLT_pred.sm_table = malloc(sizeof(sm_arrays_table)*btbSize);
    assert(my_predictor.LHLT_pred.sm_table != NULL);
    int sm_array_size = two_in_power(historySize);
    for(int i=0;i<btbSize;i++){
        // In each row in sm table, create an array of sm's.
        my_predictor.LHLT_pred.sm_table[i].state_machines_array = malloc(sizeof(state_machine)*sm_array_size);
        assert(my_predictor.LHLT_pred.sm_table[i].state_machines_array != NULL);
    }

    // Init the machines for WNT state.
    for(int i=0;i<btbSize;i++){
        for (int j = 0; j < sm_array_size; ++j) {
            my_predictor.LHLT_pred.sm_table[i].state_machines_array[j].state = WNT;
        }
    }
}

void LHGT_init(unsigned btbSize, unsigned historySize, unsigned tagSize,
              bool isGlobalHist, bool isGlobalTable, int Shared){

    my_predictor.LHGT_pred.shared_type = Shared;

    size = btbSize*(tagSize + TARGET_SIZE + historySize) + 2*two_in_power(historySize);

    select_BHR_mask(historySize);

    // malloc and init BHRs array of btb size.
    my_predictor.LHGT_pred.BHR = malloc(sizeof(uint8_t)*btbSize);
    assert(my_predictor.LHGT_pred.BHR != NULL);
    for(int i=0;i<btbSize;i++){
        my_predictor.LHGT_pred.BHR[i] = 0;
    }

    // Alloc and init ms array.
    int sm_array_size = two_in_power(historySize);
    //printf("sm_array_size = %d\n", sm_array_size);
    my_predictor.LHGT_pred.state_machines_array = malloc(sizeof(state_machine)*sm_array_size);
    assert(my_predictor.LHGT_pred.state_machines_array != NULL);

    // Init the machines for WNT state.
    for(int i=0;i<sm_array_size;i++){
        my_predictor.LHGT_pred.state_machines_array[i].state = WNT;
    }
}

// Init relevant prediction module according to given type
int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize,
             bool isGlobalHist, bool isGlobalTable, int Shared){

    // Inits the vars and allocate the btb - without the predictor.
    vars_init(btbSize, historySize, tagSize, isGlobalHist, isGlobalTable, Shared);
    set_tag_mask(tagSize);

    if(isGlobalHist == true && isGlobalTable == true){
        GHGT_init(btbSize, historySize, tagSize, isGlobalHist, isGlobalTable, Shared);
        my_predictor.my_predicotr_type = GHGT;
        return 0;
    }
    if(isGlobalHist == true && isGlobalTable == false){
        GHLT_init(btbSize, historySize, tagSize, isGlobalHist, isGlobalTable, Shared);
        my_predictor.my_predicotr_type = GHLT;
        return 0;
    }
    if(isGlobalHist == false && isGlobalTable == false){
        LHLT_init(btbSize, historySize, tagSize, isGlobalHist, isGlobalTable, Shared);
        my_predictor.my_predicotr_type = LHLT;
        return 0;
    }
    if(isGlobalHist == false && isGlobalTable == true){
        LHGT_init(btbSize, historySize, tagSize, isGlobalHist, isGlobalTable, Shared);
        my_predictor.my_predicotr_type = LHGT;
        return 0;
    }

	return -1;
}

// Prediction functions for each prediction module
bool GHGT_predict(uint32_t pc, uint32_t *dst){

    branch_num++;

    uint8_t btb_entry = index_from_pc(pc);

    //go to state machines table ang get the prediction.
    //int ms_array_entry = my_predictor.GHGT_pred.BHR;
    uint8_t sm_array_entry = ms_entry_calc(pc, my_predictor.GHGT_pred.BHR, my_predictor.GHGT_pred.shared_type);

    Machine_Prediction prediction = get_prediction(&my_predictor.GHGT_pred.state_machines_array[sm_array_entry]);
    //printf("\nPrediciotn: %d\n",prediction);

    // Save last prediction for flush management
    if(prediction == PRED_TAKEN){
        last_prediction_taken = true;
    } else {
        last_prediction_taken = false;
    }



    if(my_predictor.tags[btb_entry] != 0 || my_predictor.targets[btb_entry] != 0){
        if(my_predictor.tags[btb_entry] == tag_from_pc(pc)){

            if(prediction == PRED_TAKEN){
                *dst = my_predictor.targets[btb_entry];
            } else {
                *dst = pc+4;
            }

        } else {
            // Looks like in this case we just need return NOT_TAKEN and pc+4 and that's it.
            *dst = pc+4;
            last_prediction_taken = false;
            return false;

        }
    } else {
        *dst = pc+4;
        last_prediction_taken = false;
        return false;
    }


    if(prediction == PRED_TAKEN){
        return true;
    } else {
        return false;
    }
}
bool GHLT_predict(uint32_t pc, uint32_t *dst){

    branch_num++;

    //go to state machines table ang get the prediction.
    uint32_t btb_entry = index_from_pc(pc);
    uint32_t sm_array_enty = my_predictor.GHLT_pred.BHR;

    Machine_Prediction prediction = get_prediction(&my_predictor.GHLT_pred.sm_table[btb_entry].state_machines_array[sm_array_enty]);

    // Save last prediction for flush management
    if(prediction == PRED_TAKEN){
        last_prediction_taken = true;
    } else {
        last_prediction_taken = false;
    }

    if(my_predictor.tags[btb_entry] != 0 || my_predictor.targets[btb_entry] != 0){
        // we got some tag
        if(my_predictor.tags[btb_entry] == tag_from_pc(pc)){
            if(prediction == PRED_TAKEN){
                *dst = my_predictor.targets[index_from_pc(pc)];
            } else {
                *dst = pc+4;
            }
        } else {
            // In this case we just need return NOT_TAKEN and pc+4 and reset all ms_array in this row.
            *dst = pc+4;
            last_prediction_taken = false;
            predictor_needs_reset = true;
            //my_predictor.GHLT_pred.BHR = 0;
            for(int i=0;i<two_in_power(my_predictor.historySize);i++){
                my_predictor.GHLT_pred.sm_table[btb_entry].state_machines_array[i].state = WNT;
            }
            return false;
        }
    } else {
        *dst = pc+4;
        last_prediction_taken = false;
        return false;
    }

    if(prediction == PRED_TAKEN){
        return true;
    } else {
        return false;
    }
}
bool LHLT_predict(uint32_t pc, uint32_t *dst){

    branch_num++;

    //go to state machines table ang get the prediction.
    uint8_t btb_entry = index_from_pc(pc);
    uint8_t sm_array_entry = my_predictor.LHLT_pred.BHR[btb_entry];

    Machine_Prediction prediction = get_prediction(&my_predictor.LHLT_pred.sm_table[btb_entry].state_machines_array[sm_array_entry]);

    // Save last prediction for flush management
    if(prediction == PRED_TAKEN){
        last_prediction_taken = true;
    } else {
        last_prediction_taken = false;
    }

    //if(my_predictor.tags[btb_entry] != 0){
    if(my_predictor.tags[btb_entry] != 0 || my_predictor.targets[btb_entry] != 0){
        // we got some tag
        if(my_predictor.tags[btb_entry] == tag_from_pc(pc)){
            //printf("Here we found an exisiting tag.\n");
            if(prediction == PRED_TAKEN){
                //printf("tag exist AND prediction is TAKEN, then dst = target.\n");
                *dst = my_predictor.targets[index_from_pc(pc)];
            } else {
                //printf("If prediction is NOT_TAKEN, dst is pc+4.\n");
                *dst = pc+4;
            }
        } else {
            // In this case we just need return NOT_TAKEN and pc+4 and reset all ms_array in this row.
            *dst = pc+4;
            last_prediction_taken = false;
            predictor_needs_reset = true;
            my_predictor.LHLT_pred.BHR[btb_entry] = 0;
            for(int i=0;i<two_in_power(my_predictor.historySize);i++){
                my_predictor.LHLT_pred.sm_table[btb_entry].state_machines_array[i].state = WNT;
            }
            return false;
        }
    } else {
        *dst = pc+4;
        last_prediction_taken = false;
        return false;

    }

    if(prediction == PRED_TAKEN){
        return true;
    } else {
        return false;
    }
}
bool LHGT_predict(uint32_t pc, uint32_t *dst){

    branch_num++;

    //go to state machines table ang get the prediction.
    uint8_t btb_entry = index_from_pc(pc);
    //int sm_array_entry = my_predictor.LHGT_pred.BHR[btb_entry];
    uint8_t sm_array_entry = ms_entry_calc(pc, my_predictor.LHGT_pred.BHR[btb_entry], my_predictor.LHGT_pred.shared_type);

    Machine_Prediction prediction = get_prediction(&my_predictor.LHGT_pred.state_machines_array[sm_array_entry]);

    // Save last prediction for flush management
    if(prediction == PRED_TAKEN){
        last_prediction_taken = true;
    } else {
        last_prediction_taken = false;
    }

    if(my_predictor.tags[btb_entry] != 0 || my_predictor.targets[btb_entry] != 0){
        // we got some tag
        if(my_predictor.tags[index_from_pc(pc)] == tag_from_pc(pc)){
            if(prediction == PRED_TAKEN){
                *dst = my_predictor.targets[index_from_pc(pc)];
            } else {
                *dst = pc+4;
            }
        } else {
            // In this case we just need return NOT_TAKEN and pc+4 and that's it.
            *dst = pc+4;
            last_prediction_taken = false;
            my_predictor.LHGT_pred.BHR[btb_entry] = 0;
            return false;
        }
    } else {
        *dst = pc+4;
        last_prediction_taken = false;
        return false;
    }

    if(prediction == PRED_TAKEN){
        return true;
    } else {
        return false;
    }
}

// Calling for relevant prediction function
bool BP_predict(uint32_t pc, uint32_t *dst){

    //Choose preditor type
    switch (my_predictor.my_predicotr_type){
        case GHGT:
            return GHGT_predict(pc,dst);
        case GHLT:
            return GHLT_predict(pc,dst);
        case LHLT:
            return LHLT_predict(pc,dst);
        case LHGT:
            return LHGT_predict(pc,dst);
    }

	return false;
}

// Calling for relevant update function
void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst){

    if(my_predictor.my_predicotr_type == GHGT){
        GHGT_update(pc, targetPc, taken, pred_dst);
    }
    if(my_predictor.my_predicotr_type == GHLT){
        GHLT_update(pc, targetPc,taken, pred_dst);
    }
    if(my_predictor.my_predicotr_type == LHLT){
        LHLT_update(pc, targetPc, taken, pred_dst);
    }
    if(my_predictor.my_predicotr_type == LHGT){
        LHGT_update(pc, targetPc, taken, pred_dst);
    }
}

// Update function for each prediction module.
void GHGT_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst){
    // Update the state machine in relevant entry.

    //int8_t array_index = my_predictor.GHGT_pred.BHR;
    uint8_t sm_array_entry = ms_entry_calc(pc, my_predictor.GHGT_pred.BHR, my_predictor.GHGT_pred.shared_type);
    //printf("update machine in index: %d ...",array_index);
    update_state(&my_predictor.GHGT_pred.state_machines_array[sm_array_entry],taken);

    //update the relevant entry in btb table.
    my_predictor.tags[index_from_pc(pc)] = tag_from_pc(pc);
    my_predictor.targets[index_from_pc(pc)] = targetPc;

    //Calculate flush number
    if (taken != last_prediction_taken){
        // We had a misprediction, hence flush.
        flush_num++;
    } else if(last_prediction_taken && (targetPc != pred_dst)){
        flush_num++;
    }
    //printf("Flush value: %d\n",flush_num);

    // Update the BHR according to last prediction apply mask.
    my_predictor.GHGT_pred.BHR = my_predictor.GHGT_pred.BHR << 1;
    if(taken == true){
        my_predictor.GHGT_pred.BHR += 1;
    } else {
        my_predictor.GHGT_pred.BHR += 0; // Bitch please
    }
    my_predictor.GHGT_pred.BHR = my_predictor.GHGT_pred.BHR & my_predictor.BHR_mask;
    //printf("Now BHR = %d\n",my_predictor.GHGT_pred.BHR);


}
void GHLT_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst){
    uint8_t btb_entry = index_from_pc(pc);
    uint8_t sm_array_entry = my_predictor.GHLT_pred.BHR;

    // Update the state machine in relevant entry.
    update_state(&my_predictor.GHLT_pred.sm_table[btb_entry].state_machines_array[sm_array_entry],taken);

    //update the relevant entry in btb table.
    my_predictor.tags[btb_entry] = tag_from_pc(pc);
    my_predictor.targets[btb_entry] = targetPc;

    //Calculate flush number
    if (taken != last_prediction_taken){
        // We had a misprediction, hence flush.
        flush_num++;
    } else if(last_prediction_taken && (targetPc != pred_dst)){
        flush_num++;
    }

    // Update the BHR according to last prediction apply mask.
    my_predictor.GHLT_pred.BHR = my_predictor.GHLT_pred.BHR << 1;
    if(taken == true){
        my_predictor.GHLT_pred.BHR += 1;
    } else {
        my_predictor.GHLT_pred.BHR += 0; // Bitch please
    }
    my_predictor.GHLT_pred.BHR = my_predictor.GHLT_pred.BHR & my_predictor.BHR_mask;
}
void LHLT_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst){

    uint8_t btb_entry = index_from_pc(pc);
    uint8_t sm_array_entry = my_predictor.LHLT_pred.BHR[btb_entry];

    // Update the state machine in relevant entry.
    update_state(&my_predictor.LHLT_pred.sm_table[btb_entry].state_machines_array[sm_array_entry],taken);

    //update the relevant entry in btb table.
    my_predictor.tags[btb_entry] = tag_from_pc(pc);
    my_predictor.targets[btb_entry] = targetPc;

    //Calculate flush number
    if (taken != last_prediction_taken){
        // We had a misprediction, hence flush.
        flush_num++;
    } else if(last_prediction_taken && (targetPc != pred_dst)){
        flush_num++;
    }

        // Update the BHR according to last prediction apply mask.
    my_predictor.LHLT_pred.BHR[btb_entry] = my_predictor.LHLT_pred.BHR[btb_entry] << 1;
    if (taken == true) {
        my_predictor.LHLT_pred.BHR[btb_entry] += 1;
    } else {
        my_predictor.LHLT_pred.BHR[btb_entry] += 0; // Bitch please
    }

    my_predictor.LHLT_pred.BHR[btb_entry] = my_predictor.LHLT_pred.BHR[btb_entry] & my_predictor.BHR_mask;

}
void LHGT_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst){
    uint8_t btb_entry = index_from_pc(pc);
    //int sm_array_entry = my_predictor.LHGT_pred.BHR[btb_entry];
    uint8_t sm_array_entry = ms_entry_calc(pc, my_predictor.LHGT_pred.BHR[btb_entry], my_predictor.LHGT_pred.shared_type);
    // Update the state machine in relevant entry.
    update_state(&my_predictor.LHGT_pred.state_machines_array[sm_array_entry],taken);

    //update the relevant entry in btb table.
    my_predictor.tags[btb_entry] = tag_from_pc(pc);
    my_predictor.targets[btb_entry] = targetPc;

    //Calculate flush number
    if (taken != last_prediction_taken){
        // We had a misprediction, hence flush.
        flush_num++;
    } else if(last_prediction_taken && (targetPc != pred_dst)){
        flush_num++;
    }

    // Update the BHR according to last prediction apply mask.
    my_predictor.LHGT_pred.BHR[btb_entry] = my_predictor.LHGT_pred.BHR[btb_entry] << 1;
    if(taken == true){
        my_predictor.LHGT_pred.BHR[btb_entry] += 1;
    } else {
        my_predictor.LHGT_pred.BHR[btb_entry] += 0; // Bitch please
    }
    my_predictor.LHGT_pred.BHR[btb_entry] = my_predictor.LHGT_pred.BHR[btb_entry] & my_predictor.BHR_mask;
}

void BP_GetStats(SIM_stats *curStats) {
    curStats->flush_num = flush_num;
    curStats->br_num = branch_num;
    curStats->size = size;
}

///// Helper Functions ////

// Get prediction from state machine
Machine_Prediction get_prediction(state_machine* machine){
    //printf("Machine state is: %d\n",machine->state);
    if(machine->state == WT || machine->state == ST){
        return PRED_TAKEN;
    } else {
        return PRED_NOT_TAKEN;
    }
}

// Update state of given state machine
void update_state(state_machine* machine, bool taken){
    // State machine: enum state{SNT, WNT, WT, ST}
    if (taken == true){
        switch (machine->state){
            case SNT:
                machine->state = WNT;
                break;
            case WNT:
                machine->state = WT;
                break;
            case WT:
                machine->state = ST;
                break;
            case ST:
                machine->state = ST;
                break;
        }
    } else { // not taken
        switch (machine->state){
            case SNT:
                machine->state = SNT;
                break;
            case WNT:
                machine->state = SNT;
                break;
            case WT:
                machine->state = WNT;
                break;
            case ST:
                machine->state = WT;
                break;
        }
    }
}

// Helper function to init all basic vars of the predictor.
void vars_init(unsigned btbSize, unsigned historySize, unsigned tagSize,
               bool isGlobalHist, bool isGlobalTable, int Shared){
    my_predictor.isGlobalHist = isGlobalHist;
    my_predictor.isGlobalTable = isGlobalTable;
    my_predictor.btbsize = btbSize;
    my_predictor.historySize = historySize;
    my_predictor.tagSize = tagSize;
    my_predictor.Shared = Shared;

    //// Init BTB Table without the Predictor ////
    my_predictor.tags = malloc(sizeof(uint32_t)*btbSize);
    assert(my_predictor.tags != NULL);
    my_predictor.targets = malloc(sizeof(uint32_t)*btbSize);
    assert(my_predictor.targets != NULL);

    //// init the btb
    for(int i;i<btbSize;i++){
        my_predictor.tags[i] = 0;
        my_predictor.targets[i] = 0;
    }
}

// Define BHR MASK according to num of bits in BHR
void select_BHR_mask(unsigned historySize){
    switch (historySize){
        case 1:
            my_predictor.BHR_mask = BHR_1BIT_MASK;
            break;
        case 2:
            my_predictor.BHR_mask = BHR_2BIT_MASK;
            break;
        case 3:
            my_predictor.BHR_mask = BHR_3BIT_MASK;
            break;
        case 4:
            my_predictor.BHR_mask = BHR_4BIT_MASK;
            break;
        case 5:
            my_predictor.BHR_mask = BHR_5BIT_MASK;
            break;
        case 6:
            my_predictor.BHR_mask = BHR_6BIT_MASK;
            break;
        case 7:
            my_predictor.BHR_mask = BHR_7BIT_MASK;
            break;
        case 8:
            my_predictor.BHR_mask = BHR_8BIT_MASK;
            break;
        default:
            my_predictor.BHR_mask = BHR_8BIT_MASK;
            break;
    }
}

// Helper
int two_in_power(int power){
    int result = 1;
    for(int i=0;i<power;i++){
        result*=2;
    }
    return result;
}

// Define tag MASK according to num of bits in tag
void set_tag_mask(unsigned int tag_size){
        if(tag_size == 0)     my_predictor.TAG_mask = 0;
        if(tag_size == 1)     my_predictor.TAG_mask = 1;
        if(tag_size == 2)     my_predictor.TAG_mask = 3;
        if(tag_size == 3)     my_predictor.TAG_mask = 7;
        if(tag_size == 4)     my_predictor.TAG_mask = 15;
        if(tag_size == 5)     my_predictor.TAG_mask = 31;
        if(tag_size == 6)     my_predictor.TAG_mask = 63;
        if(tag_size == 7)     my_predictor.TAG_mask = 127;
        if(tag_size == 8)     my_predictor.TAG_mask = 255;
        if(tag_size == 9)     my_predictor.TAG_mask = 511;
        if(tag_size == 10)    my_predictor.TAG_mask = 1023;
        if(tag_size == 11)    my_predictor.TAG_mask = 2047;
        if(tag_size == 12)    my_predictor.TAG_mask = 4095;
        if(tag_size == 13)    my_predictor.TAG_mask = 8191;
        if(tag_size == 14)    my_predictor.TAG_mask = 16383;
        if(tag_size == 15)    my_predictor.TAG_mask = 32767;
        if(tag_size == 16)    my_predictor.TAG_mask = 65535;
        if(tag_size == 17)    my_predictor.TAG_mask = 131071;
        if(tag_size == 18)    my_predictor.TAG_mask = 262143;
        if(tag_size == 19)    my_predictor.TAG_mask = 524287;
        if(tag_size == 20)    my_predictor.TAG_mask = 1048575;
        if(tag_size == 21)    my_predictor.TAG_mask = 2097151;
        if(tag_size == 22)    my_predictor.TAG_mask = 4194303;
        if(tag_size == 23)    my_predictor.TAG_mask = 8388607;
        if(tag_size == 24)    my_predictor.TAG_mask = 16777215;
        if(tag_size == 25)    my_predictor.TAG_mask = 33554431;
        if(tag_size == 26)    my_predictor.TAG_mask = 67108863;
        if(tag_size == 27)    my_predictor.TAG_mask = 134217727;
        if(tag_size == 28)    my_predictor.TAG_mask = 268435455;
        if(tag_size == 29)    my_predictor.TAG_mask = 536870911;
        if(tag_size == 30)    my_predictor.TAG_mask = 1073741823;
}

// Calculate tag from given pc
uint32_t tag_from_pc(uint32_t pc){
    uint32_t shifted_pc = pc >> 2; // Shift two bits left.
    uint32_t tag = shifted_pc & my_predictor.TAG_mask; // AND operation.
    return tag;
}

// Calculate btb_entry from given pc
uint8_t index_from_pc(uint32_t pc){
    uint32_t shifted_pc = pc >> 2; // Shift two bits left.
    int bits_in_index;

    switch (my_predictor.btbsize){
        case 1:
            bits_in_index = 0;
            break;
        case 2:
            bits_in_index = 1;
            break;
        case 4:
            bits_in_index = 2;
            break;
        case 8:
            bits_in_index = 3;
            break;
        case 16:
            bits_in_index = 4;
            break;
        case 32:
            bits_in_index = 5;
            break;
        default:
            bits_in_index = 5;
            break;
    }

    uint32_t index_mask;
    switch (bits_in_index){
        case 0:
            index_mask = 0;
            break;
        case 1:
            index_mask = 1;
            break;
        case 2:
            index_mask = 3;
            break;
        case 3:
            index_mask = 7;
            break;
        case 4:
            index_mask = 15;
            break;
        case 5:
            index_mask = 31;
            break;
    }

    return (uint8_t)(shifted_pc & index_mask);
}

// Calculate entry to state_machines array.
uint8_t ms_entry_calc(uint32_t pc, uint8_t BHR, int Shared){

    uint8_t shifted_pc;

    switch (Shared){
        case NOT_USING_SHARE:
            return BHR;
        case USING_SHARE_LSB:
            shifted_pc = (uint8_t)(pc >> 2);
            return (BHR ^ shifted_pc) & my_predictor.BHR_mask;
        case USING_SHARE_MID:
            shifted_pc = (uint8_t)(pc >> 16);
            return (BHR ^ shifted_pc) & my_predictor.BHR_mask;
    }
    return  0;
}

// Debugging function.
void print_pred_table(){
    int size = two_in_power(my_predictor.historySize);
    printf("\nms:  ");
    for(int i=0;i<size;i++){
        int state = my_predictor.GHGT_pred.state_machines_array[i].state;
        switch (state){
            case 0:
                printf("_SNT_");
                break;
            case 1:
                printf("_WNT_");
                break;
            case 2:
                printf("_WT_");
                break;
            case 3:
                printf("_ST_");
                break;
        }
    }
    printf("\n");
}

// Debugging function.
void print_local_pred_table(int btb_size, int btb_entry){
    int array_size = two_in_power(my_predictor.historySize);
    if(btb_entry == -1){
        printf("ms:\n");
        for(int i=0;i<btb_size;i++){
            printf("sm %d:",i);
            for (int j = 0; j < array_size; ++j) {
                int sm_state;
                if(my_predictor.my_predicotr_type == LHLT){
                    sm_state = my_predictor.LHLT_pred.sm_table[i].state_machines_array[j].state;
                } else if(my_predictor.my_predicotr_type == GHLT){
                    sm_state = my_predictor.GHLT_pred.sm_table[i].state_machines_array[j].state;
                }
                switch (sm_state){
                    case 0:
                        printf("_SNT_");
                        break;
                    case 1:
                        printf("_WNT_");
                        break;
                    case 2:
                        printf("_WT_");
                        break;
                    case 3:
                        printf("_ST_");
                        break;
                }

            }
            printf("\n");
        }
    } else {
        printf("sm %d:",btb_entry);
        for (int j = 0; j < array_size; ++j) {
            int sm_state;
            if(my_predictor.my_predicotr_type == LHLT){
                sm_state = my_predictor.LHLT_pred.sm_table[btb_entry].state_machines_array[j].state;
            } else if(my_predictor.my_predicotr_type == GHLT){
                sm_state = my_predictor.GHLT_pred.sm_table[btb_entry].state_machines_array[j].state;
            }
            switch (sm_state){
                case 0:
                    printf("_SNT_");
                    break;
                case 1:
                    printf("_WNT_");
                    break;
                case 2:
                    printf("_WT_");
                    break;
                case 3:
                    printf("_ST_");
                    break;
            }

        }
        printf("\n");
    }

}