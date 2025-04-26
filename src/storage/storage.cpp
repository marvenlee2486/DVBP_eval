#include "storage.h"
#include <tuple>

bool Node::isActive(){
    return st == state::RUNNING;
}

Cluster::Cluster(int cid, int mid, int limit):mid(mid),nPhy(limit),cid(cid){
    int idx = 0;
    machines.assign(limit, NULL);
    for(auto& node: machines){
        node = new Node(mid,idx, cid);
        idx++;
    }
}

int Cluster::getCache(){
    if (cache < machines.size() && machines[cache]->isActive()) cache++;
    return cache;
}

Storage* Storage::instance = NULL;

Storage::Storage(int nPid){
    
    int nMachines = 10000000;
    int total = 1;
   
    cout << total << " Clusters are created each with " << nMachines << " machines\n"; 
    clusters.assign(total,  NULL);
    clusters[0] = new Cluster(0, Mode::getInstance()->getMachineId(), nMachines);
          
    vmType = VmType::getInstance();

    recentTime.assign(total, vector<double>(nMachines, -10000000));
}

Storage* Storage::getInstance(){
    if( Storage::instance == NULL){
        Storage::instance = new Storage(VmType::getInstance()->NPID);
    }

    return Storage::instance;
}

/**
 * @brief Create a VM and allocate it to the node
 * @param vm The VM Created
 * @param node The node VM is allocated to 
 * @param timeStamp at which time it is allocated
 */
void Storage::created(VM* vm, Node* node, double timeStamp){
    
    if(!node->isActive()){
        // Manage the count
        node->st = state::RUNNING;
        nActivePhy++;
        clusters[node->cid]->nActivePhy++;
    }

    node->sp = node->sp + vmType->getSpec(vm->vmTypeId, node->sp.mId);
   
    assert(node->sp.core <= 1.001 && node->sp.memory <= 1.001 && node->sp.nic <= 1.001 && node->sp.hdd <= 1.001 && node->sp.ssd <= 1.001);
    
    // change requests
    allocation.emplace(vm, node);

    // change history
    history.emplace_back(timeStamp, nActivePhy);

    // update caches
    recentTime[node->cid][node->pid] = timeStamp;
}

/**
 * @brief Terminate VM and remove it from the node
 * @param vm The VM that is terminated
 * @param timeStamp at which time it is allocated
 */
void Storage::terminated(VM* vm, double timeStamp){
    // change request
    Node* node = allocation[vm];
    allocation.erase(vm);

    node->sp = node->sp - vmType->getSpec(vm->vmTypeId, node->sp.mId);;

    if(node->sp == 0){
        node->st = state::TERMINATED;
        nActivePhy--;
        clusters[node->cid]->nActivePhy--;
    }

    // change history
    history.emplace_back(timeStamp, nActivePhy);

    // update caches
    recentTime[node->cid][node->pid] = timeStamp;
}

Node* Storage::getAllocation(VM* vm){
    return allocation[vm];
}

int Storage::getActivePhy(){
    return nActivePhy;
}

vector<pair<double, int> > Storage::getHistory(){
    return history;
}

Cluster* Storage::getCluster(int cid){
    return clusters[cid];
}

vector<Cluster*> Storage::getClusters(){
    return clusters;
}


double Storage::getRecentTime(int cId, int pId){
    //cout << cId << " " << pId << "\n";
    return recentTime[cId][pId];
}
void Storage::setRecentTime(int cId, int pId, double t){
    recentTime[cId][pId] = t;
}