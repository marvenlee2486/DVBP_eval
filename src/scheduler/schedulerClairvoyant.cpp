#include "schedulerClairvoyant.h"
#include <random>
#include <time.h>
#include <stdlib.h>
#include <algorithm>
#include <thread>

Node* StandardNearestRemainingTime::request(VM* vm){
    Node* ret = NULL;
    double expectedEndTime = vm->expectedEndTime;
    spec vmSpec = vmType->getSpec(vm->vmTypeId, cluster->mid);

    auto itp = heuristics.lower_bound(pair<double, int>(expectedEndTime, -1));
    auto itn = itp;

    double cache = -1;
    auto it = heuristics.end();
    while(itn != heuristics.begin()){
        itn--; 
        Node* curn = cluster->machines[itn->second];
        spec specn = curn->sp;
        if(Util::canFit(vmSpec, specn)){
            cache = abs(expectedEndTime - itn->first);
            it = itn;
            break;
        }          
    }

    while(itp != heuristics.end()){
        Node* curp = cluster->machines[itp->second];
        spec specp = curp->sp; 
        if(cache != -1 && abs(itp->first - expectedEndTime) > cache){
            break;
        }

        if(Util::canFit(vmSpec, specp)){
            it = itp;
            break;
        }   
    
        itp++;
    }
    
    if(it == heuristics.end()){
        ret = cluster->machines[lastActive];
        insert(vm, lastActive);
        return ret;
    }


    ret = cluster->machines[it->second];
    insert(vm, it->second);
    return ret;
}

Node* PrioritizedNearestRemainingTime::request(VM* vm){
    Node* ret = NULL;
    double expectedEndTime = vm->expectedEndTime;
    spec vmSpec = vmType->getSpec(vm->vmTypeId, cluster->mid);

    auto itp = heuristics.lower_bound(pair<double, int>(expectedEndTime, -1));
 
    auto itn = itp;
    if(heuristics.size() == 0){
        ret = cluster->machines[lastActive];
        insert(vm,lastActive);
        return ret;
    }
    
    auto answer = heuristics.end();
    while(itp != heuristics.end()){
        Node* curp = cluster->machines[itp->second];
        spec specp = curp->sp; 
        if(Util::canFit(vmSpec, specp)){
            answer = itp;
            itp = heuristics.end();
        }   
        else itp++;
    }

    while(answer == heuristics.end() && itn != heuristics.begin()){
        itn--; 
        Node* curn = cluster->machines[itn->second];
        spec specn = curn->sp;
        if(Util::canFit(vmSpec, specn)){
            answer = itn;
            itn = heuristics.begin();
        }          
    }
   
    if(answer == heuristics.end()){
        ret = cluster->machines[lastActive];
        insert(vm,lastActive);
        return ret;
    }
    
    ret = cluster->machines[answer->second];
    insert(vm,answer->second);
    return ret;
}

Node* ClassifyByDuration::request(VM* vm){
    spec vmSpec = vmType->getSpec(vm->vmTypeId, cluster->mid);
    double lifetime = vm->predictedlifetime;

    int i = floor(log2(lifetime) / log2(alpha));
    
    set<int>* bin = &CD[i];
    int retId = firstFit(vmSpec, bin);
    return cluster->machines[retId];
}

void ClassifyByDuration::terminate(VM* vm){  
    Node* node = storage->getAllocation(vm);
    spec vmSpec = vmType->getSpec(vm->vmTypeId, node->sp.mId);
    spec nodeSpec = node->sp;
    
    if((nodeSpec - vmSpec) == 0){
        int nodeId = node->pid;

        double lifetime = vm->predictedlifetime;
        int i = floor(log2(lifetime) / log2(alpha));
        set<int>* setPtr = &CD[i];
        assert(setPtr->find(nodeId) != setPtr->end());
        setPtr->erase(nodeId);
    }
}

int Clarivoyant::firstFit(spec vmSpec, set<int>* binType){
    for(auto it = binType->begin(); it != binType->end(); it++){
        if( Util::canFit(vmSpec, cluster->machines[*it]->sp)){
            return *it;
        }
    }
    binType->insert(lastActive);
    int n = lastActive;
    lastActive++;
    return n;
}

Node* ClassifyByDepartureTime::request(VM* vm){
    spec vmSpec = vmType->getSpec(vm->vmTypeId, cluster->mid);
    int idx = vm->expectedEndTime / t;
    int retId = firstFit(vmSpec,&dBin[idx]);
    return cluster->machines[retId];
}

void ClassifyByDepartureTime::terminate(VM* vm){
    Node* node = storage->getAllocation(vm);
    spec vmSpec = vmType->getSpec(vm->vmTypeId, node->sp.mId);
    spec nodeSpec = node->sp;
    
    if((nodeSpec - vmSpec) == 0){
        int nodeId = node->pid;
        int idx = vm->expectedEndTime / t;
        assert(dBin[idx].find(nodeId) != dBin[idx].end());
        dBin[idx].erase(nodeId);
    }
}

Node* GreedyAlgorithm::request(VM* vm){  
    spec vmSpec = vmType->getSpec(vm->vmTypeId, cluster->mid);
    
    auto it = heuristics.end();
    if(heuristics.size() == 0){
        int retId = lastActive;
        insert(vm, lastActive);
        return cluster->machines[retId];
    }
    do{
        it--;
        int id = it->second;
        Node* node = cluster->machines[id];
        if(Util::canFit(vmSpec, node->sp)){
            insert(vm, id);
            return node;
        }
    }while(it != heuristics.begin());
    
    int retId = lastActive;
    insert(vm, lastActive);
    return cluster->machines[retId]; 
}

void DynamicClairvoyant::insert(VM* vm, int nodeId){
    bool newly = false;
    if(nodeId == lastActive) newly = true;
    pair<double,int> answer;
    if(!newly) answer = {mapping[nodeId].rbegin()->first, nodeId};
    mapping[nodeId].emplace(vm->expectedEndTime, vm->vmId);
    pair<double,int> nh = {mapping[nodeId].rbegin()->first, nodeId};
    if(!newly) {
        assert(heuristics.find(answer) != heuristics.end());
        heuristics.erase(answer);
    }
    heuristics.emplace(nh);
    if(newly) lastActive++;
}

void DynamicClairvoyant::terminate(VM* vm){
    Node* node = storage->getAllocation(vm);
    spec vmSpec = vmType->getSpec(vm->vmTypeId, node->sp.mId);
    spec nodeSpec = node->sp;

    assert(mapping[node->pid].find(pair<double,int>(vm->expectedEndTime, vm->vmId)) != mapping[node->pid].end());
    mapping[node->pid].erase(pair<double,int>(vm->expectedEndTime, vm->vmId));

    if(mapping[node->pid].size() == 0){
        assert(heuristics.find(pair<double, int>(vm->expectedEndTime, node->pid)) != heuristics.end());
        heuristics.erase(pair<double,int>(vm->expectedEndTime, node->pid));
        return;
    }

    double max_time = mapping[node->pid].rbegin()->first;
    if(heuristics.find(pair<double,int>(max_time, node->pid)) == heuristics.end()){
        assert(heuristics.find(pair<double, int>(vm->expectedEndTime, node->pid)) != heuristics.end());
        heuristics.erase(pair<double,int>(vm->expectedEndTime, node->pid));
        heuristics.emplace(pair<double,int>(max_time, node->pid));
    }
}
