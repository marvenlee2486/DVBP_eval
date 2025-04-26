/**
 * @file The scheduler.h file contains the header template and some implementation code for Non-Clairvoyant Scheduler Algorithms
 * 
 * The algorithms that also have the implementation codes are as follows:
 *  - First Fit
 *  - Next Fit
 *  - Round Robin Next Fit
 * 
 * 'scheduler.cpp' contains the implementation of the following algortihms.
 *  - L1, L2, Linf Best Fit
 *  - L1, L2, Linf Worst Fit
 *  - MRU
 *  - LRU
 * 
 */
#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "../storage/storage.h"
#include "../storage/vmType.h"
#include "../util.h"
#include <numeric>
#include <random>

/// @brief Abstract class for Scheduler
class Scheduler{
    public:
        Storage* storage;
        VmType* vmType;

        Scheduler();
        /// @brief a VM request comes, the scheduler is expect to allocate it to a node
        virtual Node* request(VM* vm) = 0; // return pId and pidx
        /// @brief when a VM is terminated, change the internal state of Scheduler
        virtual void terminate(VM* vm){};
};

/// @brief custom best fit comparator (i.e., the lower score first)
auto bestFitcmp = [](pair<double, int> left, pair<double,int> right){
    return (left.first < right.first) || (left.first == right.first && left.second < right.second);
};

/// @brief Implement Scheduler that allows algorithm which required to sort VM by a score.
class SCScheduler : public Scheduler{
    public:
        SCScheduler(): Scheduler(){};
        set <pair<double, int>, decltype(bestFitcmp) > heuristics;
        virtual double score(spec machine) = 0;
        int lastActive = 0;
        virtual void terminate(VM* vm);
};

/// @brief First Fit Algorithm - Allocate VM to the first node that can receive the VM
class firstFit : public Scheduler{
    public:
        int lastActive = 0;
        Cluster* cluster;
        firstFit(): Scheduler(){
            cluster = storage->getClusters()[0];
        }

        Node* request(VM* vm){
            spec vmSpec = vmType->getSpec(vm->vmTypeId, cluster->mid);
            Node* machine; spec nodeSpec;
            for(int i = 0; i < lastActive; i++){
                machine = cluster->machines[i];
                nodeSpec = machine->sp;
                if(nodeSpec == 0){
                    continue;
                }
                if(Util::canFit(nodeSpec, vmSpec)){
                    return machine;
                }
            }
            lastActive++;
            
            return cluster->machines[lastActive - 1];
        }
};

/// @brief Next Fit Algorithm - Maintain only a node, place the VM on that node until it cannot be placed anymore. The Node is permanently closed for receiving VM.
class nextFit : public Scheduler{
    public:
        int lastActive = 0;
        spec currentSpec;
        Cluster* cluster;
        nextFit(): Scheduler(){
            cluster = storage->getClusters()[0]; 
            currentSpec = spec(cluster->mid,0,0,0,0,0);
        };
        Node* request(VM* vm){
            spec vmSpec = vmType->getSpec(vm->vmTypeId, cluster->mid);

            if(!Util::canFit(currentSpec, vmSpec)){
                currentSpec = spec(cluster->mid,0,0,0,0,0);
                lastActive++;
            }
            currentSpec = currentSpec + vmSpec;
            return cluster->machines[lastActive];
        };
};

/// @brief Round Robin Next Fit Algorithm - Similar to Next Fit, but do not permanently closed the Node. Instead do a Round Robin search.
class RRnextFit : public Scheduler{
    public:
        int lastActive = 0;
        int lastAccess = 0;
        Cluster* cluster;
        RRnextFit(): Scheduler(){
            cluster = storage->getClusters()[0]; 
        };
        Node* request(VM* vm){
            if(lastActive == 0){
                lastActive++;
                lastAccess = 0;
                return cluster->machines[0]; 
            }
            
            spec vmSpec = vmType->getSpec(vm->vmTypeId, cluster->mid);
            
            Node* machine = cluster->machines[lastAccess];
            spec nodeSpec = machine->sp;
            
            if(Util::canFit(nodeSpec, vmSpec)){ 
                return machine;
            }
            
            for(int i = lastAccess + 1; i != lastAccess; i++){
                if(i >= lastActive){
                    i -= lastActive;
                }
                if(i == lastAccess) break;
                machine = cluster->machines[i];
                nodeSpec = machine->sp;
                if(nodeSpec == 0){
                    continue;
                }
                if(Util::canFit(nodeSpec, vmSpec)){
                    lastAccess = i;
                    return machine;
                }
            
            }
            lastAccess = lastActive;
            lastActive++;
            
            return cluster->machines[lastActive - 1];
        };
};

/// @brief L1 Best Fit - Place VM to a node that result in the least amount of space defined by L1-Norm
class SCL1BestFitScheduler : public SCScheduler{
    public:
        double score(spec machine){return Util::L1(machine);};
        SCL1BestFitScheduler(): SCScheduler(){};
        Node* request(VM* req);
};

/// @brief L2 Best Fit - Place VM to a node that result in the least amount of space defined by L2-Norm
class SCL2BestFitScheduler : public SCScheduler{
    public:
        double score(spec machine){return Util::L2(machine);};
        SCL2BestFitScheduler(): SCScheduler(){};
        Node* request(VM* req);
};

/// @brief L_inf Best Fit - Place VM to a node that result in the least amount of space defined by L_inf-Norm
class SCLinfBestFitScheduler : public SCScheduler{
    public:
        double score(spec machine){return Util::LinfMax(machine);};
        SCLinfBestFitScheduler(): SCScheduler(){};
        Node* request(VM* req);
};

/// @brief L1 Worst Fit - Place VM to a node that result in the most amount of space defined by L1-Norm
class SCL1WorstFitScheduler : public SCScheduler{
    public:
        double score(spec machine){return Util::L1(machine);};
        SCL1WorstFitScheduler(): SCScheduler(){};
        Node* request(VM* req);
};

/// @brief L2 Worst Fit - Place VM to a node that result in the most amount of space defined by L2-Norm
class SCL2WorstFitScheduler : public SCScheduler{
    private: 
        unordered_map<int, double> mapping;
        double angle(spec VM, int id);
    public:
        double score(spec machine){return Util::L2(machine);};
        SCL2WorstFitScheduler(): SCScheduler(){};
        Node* request(VM* req);
};

/// @brief Linf Worst Fit - Place VM to a node that result in the most amount of space defined by L_inf-Norm
class SCLinfWorstFitScheduler : public SCScheduler{
    public:
        double score(spec machine){return Util::LinfMax(machine);};
        SCLinfWorstFitScheduler(): SCScheduler(){};
        Node* request(VM* req);
};


/// @brief Most Recently Used - Place VM into the most recently used Node  
class MRUScheduler : public SCScheduler{
    public:
        unordered_map<int, double> mapping;
        double score(spec machine = spec()){return -1 * Util::getCurrentTime();};
        MRUScheduler(): SCScheduler(){};
        Node* request(VM* req);
        virtual void terminate(VM* vm);
};

/// @brief Least Recently Used - Place VM into the least recently used Node
class LRUScheduler : public SCScheduler{
    public:
        unordered_map<int, double> mapping;
        double score(spec machine = spec()){return Util::getCurrentTime();};
        LRUScheduler(): SCScheduler(){};
        Node* request(VM* req);
        virtual void terminate(VM* vm);
};

#endif