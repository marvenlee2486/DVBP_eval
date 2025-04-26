/**
 * @file legacy_tests.cpp
 * @brief Legacy Unit Tests
 * 
 * These tests were created during early development to verify correctness of some easy-to-make-mistake scheduler
 *  */

#include "request/request.h"
#include "rule/schedulerOPT.h"
#include "storage/storage.h"
#include "storage/vmType.h"
#include "util.h"
#include <time.h>
#include <chrono>

const double INF = 1e9;
const double INF6 = 1e6;

Scheduler* select_scheduler_from_arg(int arg){
    cout << "Select Algorithm - ";
    if(arg == 103){
        cout << "Greedy Algorithm\n";
        return new GreedyAlgorithm();
    }
    if(arg == 104) {
        cout << "Hybrid Algorithm\n"; 
        return new HybridAlgorithm();
    }
    else if(arg == 105){
        cout << "RCP Algorithm\n";
        return new RCPAlgorithm();
    }
    else if(arg == 106){
        cout << "Lifetime Alignment\n";
        return new LifetimeAlignment();
    }
    return new ModifiedAnyFitClosestRemainingTime();
}


void greedy_algorithm(){
    Storage* storage = Storage::getInstance();
    Scheduler* scheduler = new GreedyAlgorithm();
    Node* ret = NULL;

    VM* vm1 = new VM(0,7,0,1);
    ret = scheduler->request(vm1);
    storage->created(vm1, ret, 0);
    assert(ret->pid == 0);

    VM* vm2 = new VM(1,7,0,2);
    ret = scheduler->request(vm2);
    storage->created(vm2, ret, 0);
    assert(ret->pid == 0); 
    
    VM* vm3 = new VM(2,11,0,0.9);
    ret = scheduler->request(vm3);
    storage->created(vm3, ret, 0);
    assert(ret->pid == 0); 

    VM* vm4 = new VM(3,11,0.5,1.5);
    ret = scheduler->request(vm4);
    storage->created(vm4, ret, 0.5);
    assert(ret->pid == 1); 

    VM* vm5 = new VM(4,5,0.6,0.8);
    ret = scheduler->request(vm5);
    storage->created(vm5, ret, 0.6);
    assert(ret->pid == 0);

    VM* vm6 = new VM(4,10,0.7,1);
    ret = scheduler->request(vm6);
    storage->created(vm6, ret, 0.7);
    assert(ret->pid == 1);

    scheduler->terminate(vm5);
    storage->terminated(vm5,0.8);
    scheduler->terminate(vm3);
    storage->terminated(vm3,0.9);
    scheduler->terminate(vm1);
    storage->terminated(vm1,1);
    scheduler->terminate(vm6);
    storage->terminated(vm6,1);
    assert(Storage::getInstance()->getActivePhy() == 2);
    scheduler->terminate(vm4);
    storage->terminated(vm4,1.5);
    assert(storage->getCluster(0)->machines[1]->sp  == 0);
    assert(Storage::getInstance()->getActivePhy() == 1);
    scheduler->terminate(vm2);
    storage->terminated(vm2,2);
    assert(storage->getCluster(0)->machines[0]->sp  == 0);
    assert(Storage::getInstance()->getActivePhy() == 0);
}

void hybrid_algorithm(){
    Storage* storage = Storage::getInstance();
    Scheduler* scheduler = new HybridAlgorithm();
    Node* ret = NULL;

    VM* vmg = new VM(0,5,1,2);
    ret = scheduler->request(vmg);
    storage->created(vmg, ret, 1);
    assert(ret->pid == 0);
    
    VM* vmg2 = new VM(0,5,1.9,2.9); // different category
    ret = scheduler->request(vmg2);
    storage->created(vmg2, ret, 1.9);
    assert(ret->pid == 0);

    VM* vm1 = new VM(0,12,1,2);
    ret = scheduler->request(vm1);
    storage->created(vm1, ret, 1);
    assert(ret->pid == 1);

    VM* vm2 = new  VM(0,11,1,2);
    ret = scheduler->request(vm2);
    storage->created(vm2, ret, 1);
    assert(ret->pid == 2); 
    assert(storage->getActivePhy() == 3);

    scheduler->terminate(vmg);
    storage->terminated(vmg,2);
    scheduler->terminate(vm1);
    storage->terminated(vm1,2);
    scheduler->terminate(vm2);
    storage->terminated(vm2,2);
    scheduler->terminate(vmg2);
    storage->terminated(vmg2,2.9);
    assert(storage->getActivePhy() == 0 );
}

void DDFF(){
    Storage* storage = Storage::getInstance();
    Node* ret = NULL;
    vector<Request> reqs;
    // vmid, tenantid, vmTypeid, priority, start, end
    reqs.emplace_back(0,0,11,0,0.1,14); // 0
    reqs.emplace_back(1,0,10,0,0.1,7); // 0
    reqs.emplace_back(2,0,10,0,7,14); // 0
    reqs.emplace_back(3,0,10,0,6,12); // this 1
    reqs.emplace_back(4,0,11,0,1,6); // this will be 1
    reqs.emplace_back(5,0,11,0,1,2); // this will be 2
    reqs.emplace_back(6,0,5,0,1,2); // this will be 0

    int ans[] = {0,0,0,1,1,2,0};
    storage->setRequests(reqs);
    Scheduler* scheduler = new ApproximateDurationDecreasingFirstFit();
    sort(reqs.begin(),reqs.end(), [](Request l, Request r)->bool {
        return (l.startTime < r.startTime);
    });

    for(Request req: reqs){
        ret = scheduler->request(new VM(req.vmId, req.vmTypeId, req.startTime, req.endTime));
        Util::printSpec(VmType::getInstance()->getSpec(req.vmTypeId, 0));
        // cout << req.vmId << " " << ret->pid << " " << ans[req.vmId] << "\n";
        assert(ret->pid == ans[req.vmId]);
    }

}

void lifetime_alignment(){
    Storage* storage = Storage::getInstance();
    Node* ret = NULL;
    Scheduler* scheduler = new LifetimeAlignment();
    // log2 
    // check category first
    double second = 1.0/(24 * 60 * 60);
    VM* vm1 = new VM(0,10,0,3 * second);
    ret = scheduler->request(vm1);
    storage->created(vm1, ret, 0);
    assert(ret->pid == 0);

    VM* vm2 = new VM(0,10,0,2 * second);
    ret = scheduler->request(vm2);
    storage->created(vm2, ret, 0);
    assert(ret->pid == 0);


    VM* vm3 = new VM(0,10,0,4 * second);
    ret = scheduler->request(vm3);
    storage->created(vm3, ret, 0);
    assert(ret->pid == 1);

    VM* vm4 = new VM(0,8,0,4 * second);
    ret = scheduler->request(vm4);
    storage->created(vm4, ret, 0);
    assert(ret->pid == 1);

    // check for class 0
    VM* vm5 = new VM(0,8,0,1 * second);
    ret = scheduler->request(vm5);
    storage->created(vm5, ret, 0);
    assert(ret->pid == 0);

    VM* vm6 = new VM(0,10,0,4 * second);
    ret = scheduler->request(vm6);
    storage->created(vm6, ret, 0);
    assert(ret->pid == 1);

    scheduler->terminate(vm5);
    storage->terminated(vm5, 1 * second);

    // check for dynamic
    VM* vm7 = new VM(0,8,1 * second, 4 * second);
    ret = scheduler->request(vm7);
    storage->created(vm7, ret, 0);
    assert(ret->pid == 1);


    scheduler->terminate(vm2);
    storage->terminated(vm2, 2 * second);

    scheduler->terminate(vm1);
    storage->terminated(vm1, 3 * second);

    scheduler->terminate(vm3);
    storage->terminated(vm3, 4 * second);

    scheduler->terminate(vm4);
    storage->terminated(vm4, 4 * second);

    scheduler->terminate(vm6);
    storage->terminated(vm6, 4 * second);

    scheduler->terminate(vm7);
    storage->terminated(vm7, 4 * second);

}

// void lifetime_alignment_binary(){
//     Storage* storage = Storage::getInstance();
//     Node* ret = NULL;
//     Scheduler* scheduler = new LifetimeAlignmentBinary()

//     VM* vm1 = new VM(0,10)
// }

void init_run(){ 
    // vector<Scheduler*> schedulers;
    // schedulers.emplace_back
    
    VmType* vmType = VmType::getInstance();
    Storage* storage = Storage::getInstance();
    
    greedy_algorithm();
    hybrid_algorithm();
    DDFF();
    // lifetime_alignment();
}

int main(int argc, char *argv[]){

    Mode::getInstance()->setMachineId(0);  
    init_run();
}
