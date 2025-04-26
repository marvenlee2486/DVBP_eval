#include "schedulerClairvoyant.h"
#include <random>
#include <time.h>
#include <stdlib.h>
#include <algorithm>
#include <thread>

Node* ReducedHybridAlgorithm::request(VM* vm){
    cluster = storage->getClusters()[0];
    int retId = 0;
    spec vmSpec = vmType->getSpec(vm->vmTypeId, cluster->mid);

    long double lifetime = vm->predictedlifetime;
    
    lifetime *= 3600 * 24;
    long long i = floor(log2(lifetime)) + offset;
    if(i <= 0) i = 1;
    assert(i >= 1);
    
    if(d.find(i) == d.end())
        d[i] = spec(vmSpec.mId, 0, 0, 0, 0, 0);
    
    spec new_d = d[i] + vmSpec;
    double max_d = max(new_d.core, max(new_d.memory, max(new_d.nic, max(new_d.hdd, new_d.ssd))));
    
    if(max_d <= 1.0 / (2 * sqrt(abs(i)))){
        retId = firstFit(vmSpec, &GN);    
        allocation[retId] = &GN;
        d[i] = new_d;
    }
    else{
        retId = firstFit(vmSpec, &CD[i]);
        allocation[retId] = &CD[i];  
    }
    
    return cluster->machines[retId];
}

void ReducedHybridAlgorithm::terminate(VM* vm){
    Node* node = storage->getAllocation(vm);
    spec vmSpec = vmType->getSpec(vm->vmTypeId, node->sp.mId);
    spec nodeSpec = node->sp;
    int nodeId = node->pid;
    set<int>* setPtr = allocation[nodeId]; 
    
    if(setPtr == &GN){
        double lifetime = vm->predictedlifetime;
        lifetime *= 3600*24;
        int i = floor(log2(lifetime))+ offset;
        if(i <= 0) i = 1;
        assert(i >= 1);
        d[i] = d[i] - vmSpec;
    }
    
    if((nodeSpec - vmSpec) == 0){
        allocation.erase(nodeId);
        setPtr->erase(nodeId);
    }
}

Node* OriginalHybridAlgorithm::request(VM* vm){
    cluster = storage->getClusters()[0];
    int retId = 0;
    spec vmSpec = vmType->getSpec(vm->vmTypeId, cluster->mid);

    double lifetime = vm->expectedEndTime - vm->curTime;
    lifetime *= 3600*24;
    long long i = floor(log2(lifetime));
    long long c = vm->curTime * 3600 * 24 / pow(2,i) + 1; // get the floor is c - 1,

    i += offset;
    assert(i >= 1);
    if(d.find(pair<long long,long long>(i,c)) == d.end())
        d[pair<long long,long long>(i,c)] = spec(vmSpec.mId, 0, 0, 0, 0, 0);
    spec new_d = d[pair<long long,long long>(i,c)] + vmSpec;
    d[pair<long long,long long>(i,c)] = new_d;
    
    if(CD.find(pair<long long,long long>(i,c)) != CD.end() && CD[pair<long long,long long>(i,c)].size() != 0 ){
        retId = firstFit(vmSpec, &CD[pair<long long,long long>(i,c)]);
        allocation[retId] = &CD[pair<long long,long long>(i,c)];   
    }
    else{
        double max_d = max(new_d.core, max(new_d.memory, max(new_d.nic, max(new_d.hdd, new_d.ssd))));
        if(max_d <= 1.0 / (2 * sqrt(abs(i)))){
           retId = firstFit(vmSpec, &GN);    
           allocation[retId] = &GN;
        }
        else{
            retId = firstFit(vmSpec, &CD[pair<long long,long long>(i,c)]);
            allocation[retId] = &CD[pair<long long,long long>(i,c)];   
        }
    }
    return cluster->machines[retId];
}

void OriginalHybridAlgorithm::terminate(VM* vm){
    Node* node = storage->getAllocation(vm);
    spec vmSpec = vmType->getSpec(vm->vmTypeId, node->sp.mId);
    spec nodeSpec = node->sp;
    int nodeId = node->pid;
    set<int>* setPtr = allocation[nodeId]; 

    double lifetime = vm->expectedEndTime - vm->curTime;
    lifetime *= 3600 * 24;
    long long i = floor(log2(lifetime));
    long long c = vm->curTime * 3600 * 24 / pow(2,i) + 1;
    i += offset;
    assert(i >= 1);
    d[{i,c}] = d[{i,c}] - vmSpec;

    if((nodeSpec - vmSpec) == 0){
        allocation.erase(nodeId);
        setPtr->erase(nodeId);
    }
}


Node* OriginalHybridAlgorithm_DirectSum::request(VM* vm){
    cluster = storage->getClusters()[0];
    int retId = 0;
    spec vmSpec = vmType->getSpec(vm->vmTypeId, cluster->mid);

    double lifetime = vm->expectedEndTime - vm->curTime;
    lifetime *= 3600*24 ;
    long long i = floor(log2(lifetime));
    long long c = vm->curTime * 3600 * 24 / pow(2,i) + 1; // get the floor is c - 1,
    i += offset;
    assert(i >= 1);
    
    long long argd = Util::getMaxd(vmSpec);

    if(d.find(tuple<long long,long long,long long>(i,c,argd)) == d.end())
        d[tuple<long long,long long,long long>(i,c,argd)] = spec(vmSpec.mId, 0, 0, 0, 0, 0);
    spec new_d = d[tuple<long long,long long,long long>(i,c,argd)] + vmSpec;
    d[tuple<long long,long long,long long>(i,c,argd)] = new_d;
    if(CD.find(tuple<long long,long long,long long>(i,c,argd)) != CD.end() && CD[tuple<long long,long long,long long>(i,c,argd)].size() != 0 ){
        retId = firstFit(vmSpec, &CD[tuple<long long,long long,long long>(i,c,argd)]);
        allocation[retId] = &CD[tuple<long long,long long,long long>(i,c,argd)];   
    }
    else{
        double max_d = max(new_d.core, max(new_d.memory, max(new_d.nic, max(new_d.hdd, new_d.ssd))));
        if(max_d <= 1.0 / (2 * sqrt(abs(i)))){
           retId = firstFit(vmSpec, &GN);    
           allocation[retId] = &GN;
        }
        else{
            retId = firstFit(vmSpec, &CD[tuple<long long,long long,long long>(i,c,argd)]);
            allocation[retId] = &CD[tuple<long long,long long,long long>(i,c,argd)]; 
        }
    }
    return cluster->machines[retId];
}

void OriginalHybridAlgorithm_DirectSum::terminate(VM* vm){
    Node* node = storage->getAllocation(vm);
    spec vmSpec = vmType->getSpec(vm->vmTypeId, node->sp.mId);
    spec nodeSpec = node->sp;
    int nodeId = node->pid;
    set<int>* setPtr = allocation[nodeId]; 
    
 
    double lifetime = vm->expectedEndTime - vm->curTime;
    lifetime *= 3600*24;
    long long i = floor(log2(lifetime));
    long long c = vm->curTime * 3600 * 24 / pow(2,i) + 1;

    i += offset;
    assert(i >= 1);
    int argd = Util::getMaxd(vmSpec);     
    
    d[{i,c,argd}] = d[{i,c,argd}] - vmSpec;
    
    if((nodeSpec - vmSpec) == 0){
        allocation.erase(nodeId);
        setPtr->erase(nodeId);
    }
}


Node* ReducedHybridAlgorithm_DirectSum::request(VM* vm){
    cluster = storage->getClusters()[0];
    int retId = 0;
    spec vmSpec = vmType->getSpec(vm->vmTypeId, cluster->mid);

    double lifetime = vm->expectedEndTime - vm->curTime;
    lifetime *= 3600*24;
    int i = floor(log2(lifetime)) + offset;
    assert(i >= 1);
    int c = Util::getMaxd(vmSpec);
    
    if(d.find(pair<int,int>(i,c)) == d.end())
        d[pair<int,int>(i,c)] = spec(vmSpec.mId, 0, 0, 0, 0, 0);
    spec new_d = d[pair<int,int>(i,c)] + vmSpec;
    double max_d = max(new_d.core, max(new_d.memory, max(new_d.nic, max(new_d.hdd, new_d.ssd))));
    if(max_d <= 1.0 / (2 * sqrt(abs(i)))){
        retId = firstFit(vmSpec, &GN);    
        allocation[retId] = &GN;
        d[pair<int,int>(i,c)] = new_d;
    }
    else{
        retId = firstFit(vmSpec, &CD[pair<int,int>(i,c)]);
        allocation[retId] = &CD[pair<int,int>(i,c)];   
    }
    
    return cluster->machines[retId];
}

void ReducedHybridAlgorithm_DirectSum::terminate(VM* vm){
    Node* node = storage->getAllocation(vm);
    spec vmSpec = vmType->getSpec(vm->vmTypeId, node->sp.mId);
    spec nodeSpec = node->sp;
    int nodeId = node->pid;
    set<int>* setPtr = allocation[nodeId]; 
    
    if(setPtr == &GN){
        double lifetime = vm->expectedEndTime - vm->curTime;
        lifetime *= 3600*24;
        int i = floor(log2(lifetime)) + offset;
    assert(i >= 1);
        int c =  Util::getMaxd(vmSpec);
        d[{i,c}] = d[{i,c}] - vmSpec;
    }
    
    if((nodeSpec - vmSpec) == 0){
        allocation.erase(nodeId);
        setPtr->erase(nodeId);
    }
}
