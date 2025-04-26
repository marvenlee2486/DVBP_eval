#include "schedulerClairvoyant.h"
#include <random>
#include <time.h>
#include <stdlib.h>
#include <algorithm>
#include <thread>


Node* LifetimeAlignment::request(VM* vm){
    Cluster* cluster = storage->getClusters()[0];
    spec vmSpec = vmType->getSpec(vm->vmTypeId, cluster->mid);
    double tend = vm->expectedEndTime;
    double tstart = vm->curTime;
    int category = (int)log2((tend - tstart) * 24 * 3600); 
    if(category < 0) category = 0;
    Node* best_same = NULL;
    Node* best_general = NULL;
    for(auto it = heuristics.begin(); it != heuristics.end(); it++){
        auto &[deactivatingTime, id] = *it;
        long double remainingTime = deactivatingTime - vm->curTime;
        
        Node* cur = cluster->machines[id];
        int cur_category;
        if(remainingTime < 0) cur_category = 0;    
        else cur_category = log2(remainingTime * 24 * 3600);
        if(cur_category < 0) cur_category = 0;

        if(Util::canFit(cur->sp, vmSpec)){
            if(best_general == NULL || Util::LinfMaxCompare(best_general->sp + vmSpec, cur->sp + vmSpec) > 0){
                best_general = cur;
            }

            if(category == cur_category){
                if(best_same == NULL || Util::LinfMaxCompare(best_same->sp + vmSpec, cur->sp + vmSpec) > 0 ){
                    best_same = cur;
                }
            }
        }
    } 
    assert(best_general != NULL || best_same == NULL);
    Node* choice = NULL;
    if(category != 0){
        choice = best_same;
    }
    if(choice == NULL) choice = best_general;

    if(choice == NULL){ // New machines added
        int retId = lastActive;
        insert(vm,lastActive);
        return cluster->machines[retId];
    }
    
    insert(vm, choice->pid);
    return choice;
}

Node* LifetimeAlignmentBinary::request(VM* vm){
    Cluster* cluster = storage->getClusters()[0];
    spec vmSpec = vmType->getSpec(vm->vmTypeId, cluster->mid);
    double tend = vm->expectedEndTime;
    double tstart = vm->curTime;
    int category = ((tend - tstart) * 24 * 3600 < 7200)?0:1; // mean

    Node* best_same = NULL;
    Node* best_general = NULL;
    for(auto it = heuristics.begin(); it != heuristics.end(); it++){
        auto& [deactivatingTime, id] = *it;
        long double remainingTime = deactivatingTime - vm->curTime;
        
        Node* cur = cluster->machines[id];
        int cur_category = ( remainingTime * 24 * 3600 < 7200)?0:1; // mean 

        if(Util::canFit(cur->sp, vmSpec)){
            if(best_general == NULL || Util::LinfMaxCompare(best_general->sp + vmSpec, cur->sp + vmSpec) > 0){
                best_general = cur;
            }

            if(category == cur_category){
                if(best_same == NULL || Util::LinfMaxCompare(best_same->sp + vmSpec, cur->sp + vmSpec) > 0 ){
                    best_same = cur;
                }
            }
        }
    } 
    assert(best_general != NULL || best_same == NULL);
    Node* choice = NULL;
    if(category != 0){
        choice = best_same;
    }
    if(choice == NULL) choice = best_general;

    if(choice == NULL){ // New machines added
        int retId = lastActive;
        insert(vm,lastActive);
        return cluster->machines[retId];
    }
    
    insert(vm,choice->pid);
    return choice;
}

