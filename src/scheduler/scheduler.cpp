#include "scheduler.h"
#include <random>
#include <time.h>
#include <stdlib.h>
#include <algorithm>
#include <thread>


Scheduler::Scheduler(){
    storage = Storage::getInstance();
    vmType = VmType::getInstance();
}

void SCScheduler::terminate(VM* vm){
    Node* node = storage->getAllocation(vm);
    spec vmSpec = vmType->getSpec(vm->vmTypeId, node->sp.mId);
    double oldScore = this->score(node->sp);
    auto it = heuristics.find(pair<double, int>(oldScore, node->pid));
    assert(it != heuristics.end());
    heuristics.erase(it);

    if((node->sp - vmSpec) == 0) {
        return;
    }

    double newScore = this->score(node->sp - vmSpec);
    heuristics.emplace(newScore, node->pid);
}

Node* SCL1BestFitScheduler::request(VM* vm){ 
    Cluster* cluster = storage->getClusters()[0];
    
    Node* ret = NULL;
    spec vmSpec = vmType->getSpec(vm->vmTypeId, cluster->mid);
    spec used = spec(vmSpec.mId, 1 - vmSpec.core, 1 - vmSpec.memory, 1 - vmSpec.hdd, 1 - vmSpec.ssd, 1 - vmSpec.nic);
    auto it = heuristics.lower_bound(pair<double, int>(this->score(used), -1));
    while(it != heuristics.end()){
        if(Util::canFit(vmSpec, cluster->machines[it->second]->sp)){
            ret = cluster->machines[it->second];
            double sc = this->score(ret->sp + vmSpec);
            pair<double, int> nh = make_pair(sc, it->second);
            heuristics.erase(it);
            heuristics.insert(nh);
            return ret;
        }
        it++;
    }
    ret = cluster->machines[lastActive];
    heuristics.insert(pair<double, int>(score(vmSpec),lastActive));
    lastActive++;
    return ret;
}

Node* SCL2BestFitScheduler::request(VM* vm){ 
    Cluster* cluster = storage->getClusters()[0];
    
    Node* ret = NULL;
    spec vmSpec = vmType->getSpec(vm->vmTypeId, cluster->mid);
    double vmScore = score(spec(vmSpec.mId, 1 - vmSpec.core, 1 - vmSpec.memory, 1 - vmSpec.hdd, 1 - vmSpec.ssd, 1 - vmSpec.nic));
    double vmScoreMax = 1 - score(vmSpec);
    auto it = heuristics.lower_bound(pair<double, int>(vmScore, -1));
    double cache = 10; auto finalIt = heuristics.end();
    while(it != heuristics.end()){
        Node* cur = cluster->machines[it->second];
        if(Util::canFit(vmSpec, cur->sp)){            
            double sc = this->score(cur->sp + vmSpec);
            if(sc < cache){
                cache = sc;
                finalIt = it;
            }
        }
        if(score(cur->sp) - vmScore >= cache - EPS){ 
            break;
        }
        it++;
    }
    if(finalIt != heuristics.end()){
        ret = cluster->machines[finalIt->second];
        pair<double, int> nh = make_pair(cache, finalIt->second);
        heuristics.erase(finalIt);
        heuristics.insert(nh);
        return ret;
    }

    ret = cluster->machines[lastActive];
    heuristics.insert(pair<double, int>(score(vmSpec),lastActive));
    lastActive++;
    return ret;
}

Node* SCLinfBestFitScheduler::request(VM* vm){ 
    Cluster* cluster = storage->getClusters()[0];
    
    Node* ret = NULL;
    spec vmSpec = vmType->getSpec(vm->vmTypeId, cluster->mid);
    double vmScore = score(spec(vmSpec.mId, 1 - vmSpec.core, 1 - vmSpec.memory, 1 - vmSpec.hdd, 1 - vmSpec.ssd, 1 - vmSpec.nic));
    double vmScoreMax = 1 - score(vmSpec);
    auto it = heuristics.lower_bound(pair<double, int>(vmScore, -1));
    double cache = 7; auto finalIt = heuristics.end();
    while(it != heuristics.end()){
        Node* cur = cluster->machines[it->second];
        if(Util::canFit(vmSpec, cur->sp)){            
            if(finalIt == heuristics.end()){
                finalIt = it;
                it++;
                continue; 
            }
            if( Util::LinfMaxCompare(cluster->machines[finalIt->second]->sp + vmSpec, cur->sp + vmSpec) > 0){
                finalIt = it;
            }
        }
        it++;
    }
    if(finalIt != heuristics.end()){
        ret = cluster->machines[finalIt->second];
        pair<double, int> nh = make_pair(this->score(ret->sp + vmSpec), finalIt->second);
        heuristics.erase(finalIt);
        heuristics.insert(nh);
        return ret;
    }

    ret = cluster->machines[lastActive];
    heuristics.insert(pair<double, int>(score(vmSpec),lastActive));
    lastActive++;
    return ret;
}

Node* SCL1WorstFitScheduler::request(VM* vm){ 
    Cluster* cluster = storage->getClusters()[0];
    
    Node* ret = NULL;
    spec vmSpec = vmType->getSpec(vm->vmTypeId, cluster->mid);
    
    if(heuristics.size() == 0){ 
        ret = cluster->machines[lastActive];
        heuristics.emplace(score(vmSpec),lastActive);
        lastActive++;
        return ret;
    }   

    auto it = heuristics.end();
    it--;
    if(!Util::canFit(vmSpec, cluster->machines[it->second]->sp)){ 
        ret = cluster->machines[lastActive];
        heuristics.emplace(score(vmSpec),lastActive);
        lastActive++;
        return ret;
    }   
    
    ret = cluster->machines[it->second];
    double sc = this->score(ret->sp + vmSpec);
    int id =  it->second;
    heuristics.erase(it);
    heuristics.emplace(sc, id);
    return ret;
}

double SCL2WorstFitScheduler::angle(spec vmSpec, int id){
    if(mapping.find(id) == mapping.end()) return mapping[id];
    
    double vmScore = score(spec(vmSpec.mId, 1 - vmSpec.core, 1 - vmSpec.memory, 1 - vmSpec.hdd, 1 - vmSpec.ssd, 1 - vmSpec.nic));
    vector<pair<double, double>> pairs;
    pairs.emplace_back(vmSpec.core, 1 - vmSpec.core);
    pairs.emplace_back(vmSpec.nic, 1 - vmSpec.nic);
    pairs.emplace_back(vmSpec.hdd, 1 - vmSpec.hdd);
    pairs.emplace_back(vmSpec.ssd, 1 - vmSpec.ssd);
    pairs.emplace_back(vmSpec.memory, 1 - vmSpec.memory);
    sort(pairs.begin(),pairs.end(), [](auto l, auto r)->bool {
        return (l.first < r.first) || (l.first == r.first && r.second > l.second);
    });
    double temp = 0;
    for(int i = 4; i >= 1; i--){
        temp += pow(pairs[i].first, 2);
    }
    temp = sqrt(temp);
    double theta = acos(temp / vmScore);
    assert(temp / vmScore < 1);

    mapping[id] = theta;
    return theta;
}

Node* SCL2WorstFitScheduler::request(VM* vm){ 
    Cluster* cluster = storage->getClusters()[0];
    
    Node* ret = NULL;
    spec vmSpec = vmType->getSpec(vm->vmTypeId, cluster->mid);

    auto it = heuristics.end();
    double cache = 0; auto finalIt = heuristics.end();
    while(it != heuristics.begin()){
        it--;
        
        Node* cur = cluster->machines[it->second];
        if(Util::canFit(vmSpec, cur->sp)){   
            double sc = this->score(cur->sp + vmSpec);         
           
            if(sc > cache){
                cache = sc;
                finalIt = it;
            }
        }
    }
    if(finalIt != heuristics.end()){
        ret = cluster->machines[finalIt->second];
        pair<double, int> nh = make_pair(cache, finalIt->second);
        heuristics.erase(finalIt);
        heuristics.insert(nh);
        return ret;
    }

    ret = cluster->machines[lastActive];
    heuristics.insert(pair<double, int>(score(vmSpec),lastActive));
    lastActive++;
    return ret;
}

// Tested and Linf
Node* SCLinfWorstFitScheduler::request(VM* vm){ 
    Cluster* cluster = storage->getClusters()[0];
    
    Node* ret = NULL;
    spec vmSpec = vmType->getSpec(vm->vmTypeId, cluster->mid);
    double vmScore = score(spec(vmSpec.mId, 1 - vmSpec.core, 1 - vmSpec.memory, 1 - vmSpec.hdd, 1 - vmSpec.ssd, 1 - vmSpec.nic));
    double minvmScore = 1 - score(vmSpec);
    auto it = heuristics.end();
    double cache = 0; auto finalIt = heuristics.end();
    while(it != heuristics.begin()){
        it--;
        
        Node* cur = cluster->machines[it->second];
        double nodeScore = score(cur->sp);
        if(Util::canFit(vmSpec, cur->sp)){    
            if(finalIt == heuristics.end()){
                finalIt = it;
                continue; 
            }   
            if(Util::LinfMaxCompare(cluster->machines[finalIt->second]->sp + vmSpec, cur->sp + vmSpec) < 0){
                finalIt = it;
            }
        }
        if(nodeScore - minvmScore - EPS <= cache || nodeScore < vmScore){
            break;
        }
    }
    if(finalIt != heuristics.end()){
        ret = cluster->machines[finalIt->second];
        pair<double, int> nh = make_pair(this->score(ret->sp + vmSpec), finalIt->second);
        heuristics.erase(finalIt);
        heuristics.insert(nh);
        return ret;
    }

    ret = cluster->machines[lastActive];
    heuristics.insert(pair<double, int>(score(vmSpec),lastActive));
    lastActive++;
    return ret;
}

Node* MRUScheduler::request(VM* vm){
    Cluster* cluster = storage->getClusters()[0];

    Node* ret = NULL;
    spec vmSpec = vmType->getSpec(vm->vmTypeId, cluster->mid);
    auto it = heuristics.begin(); 
    while(it != heuristics.end()){ 
        Node* cur = cluster->machines[it->second];
        if(Util::canFit(vmSpec, cur->sp)){            
            ret = cluster->machines[it->second];
            double newScore = score();
            pair<double, int> nh = make_pair(newScore , it->second);
            heuristics.erase(it);
            heuristics.insert(nh);
            mapping[nh.second] = newScore;
            return ret;
        }
        it++;
    }
    ret = cluster->machines[lastActive];
    double newScore = score();
    heuristics.insert(pair<double, int>(newScore, lastActive));
    mapping[lastActive] = newScore;
    lastActive++;
    return ret; 
}

void MRUScheduler::terminate(VM* vm){
    Node* node = storage->getAllocation(vm);
    spec vmSpec = vmType->getSpec(vm->vmTypeId, node->sp.mId);
    
    if((node->sp - vmSpec) == 0) {
        double oldScore = mapping[node->pid];
        auto it = heuristics.find(pair<double, int>(oldScore, node->pid));
        heuristics.erase(it);
    }
}

Node* LRUScheduler::request(VM* vm){
    Cluster* cluster = storage->getClusters()[0];

    Node* ret = NULL;
    spec vmSpec = vmType->getSpec(vm->vmTypeId, cluster->mid);
    auto it = heuristics.begin(); 
    while(it != heuristics.end()){ 
        Node* cur = cluster->machines[it->second];
        if(Util::canFit(vmSpec, cur->sp)){            
            ret = cluster->machines[it->second];
            double newScore = score();
            pair<double, int> nh = make_pair(newScore , it->second);
            heuristics.erase(it);
            heuristics.insert(nh);
            mapping[nh.second] = newScore;
            return ret;
        }
        it++;
    }
    ret = cluster->machines[lastActive];
    double newScore = score();
    heuristics.insert(pair<double, int>(newScore, lastActive));
    mapping[lastActive] = newScore;
    lastActive++;
    return ret; 
}

void LRUScheduler::terminate(VM* vm){
    Node* node = storage->getAllocation(vm);
    spec vmSpec = vmType->getSpec(vm->vmTypeId, node->sp.mId);
    
    if((node->sp - vmSpec) == 0) {
        double oldScore = mapping[node->pid];
        auto it = heuristics.find(pair<double, int>(oldScore, node->pid));
        heuristics.erase(it);
    }
}
