#ifndef UTIL_H
#define UTIL_H
// #include "logging/log.h"
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <unordered_map>
#include <iostream>
#include <algorithm>
#include <utility>
#include <queue>
#include <set>
#include <cassert>
using namespace std;

const double EPS = 1e-9;
const double PI = acos(-1);

/// @brief spec is the vector of resources. 
struct spec{
    int mId; // machine Id
    double hdd; // hdd or ssd
    double ssd;
    double core;
    double memory;
    double nic;
    spec(){}
    spec(int mid, double core, double memory, double hdd, double ssd, double nic):
    mId(mid),hdd(hdd),ssd(ssd),core(core),memory(memory),nic(nic){}

    spec operator+ (spec next){
        if(mId != next.mId){
            throw;
        }

        double hdd = this->hdd + next.hdd;
        double ssd = this->ssd + next.ssd;
        double core = this->core + next.core;
        double memory = this->memory + next.memory;
        double nic = this->nic + next.nic;
        return spec(this->mId,core,memory,hdd,ssd,nic);
        // if(storage > 1 || core > 1 || memory > 1 || nic > 1) throw;
    }

    spec operator- (spec next){
        if(mId != next.mId){
            throw;
        }

        double hdd = this->hdd - next.hdd;
        double ssd = this->ssd - next.ssd;
        double core = this->core - next.core;
        double memory = this->memory - next.memory;
        double nic = this->nic - next.nic;
        // cout << storage << " " << EPS <<'\n';
        if(hdd < EPS) hdd = 0;
        if(ssd < EPS) ssd = 0;
        if(core < EPS) core = 0;
        if(memory < EPS) memory = 0;
        if(nic < EPS) nic = 0;
        
        return spec(this->mId,core,memory,hdd,ssd,nic);

    }

    inline bool operator== (double x){
        if(hdd < EPS) hdd = 0;
        if(ssd < EPS) ssd = 0;
        if(core < EPS) core = 0;
        if(memory < EPS) memory = 0;
        if(nic < EPS) nic = 0;
        return (hdd == x) && (ssd == x) && (core == x) && (memory == x) && (nic == x);
    }
};


struct Request{
    int vmId;
    int tenantId;
    int vmTypeId;
    int priority;
    double startTime;
    double endTime;
    double predictedendTime;
    long double predictedlifetime;
    Request(){};
    Request(int vmId, int tenantId, int vmTypeId, int priority, double startTime, double endTime, double predictedendTime = -1, long double predictedlifetime = -1):
    vmId(vmId), tenantId(tenantId), vmTypeId(vmTypeId), priority(priority), startTime(startTime), endTime(endTime), predictedendTime(predictedendTime), predictedlifetime(predictedlifetime){
        if(predictedendTime == -1) this->predictedendTime = endTime;
    }
};

/// @brief Util contains the helper functions
class Util{
    public:
        static double timeCounter;
        static bool canFit(spec s1, spec s2);
        static void printSpec(spec s1);
        static double L1(spec s1);
        static double L2(spec s2);
        static double LinfMax(spec s3, int arg = 0);
        static int LinfMaxCompare(spec s1, spec s2);
        static double LMin(spec s3); 
        static double getCurrentTime();
        static int getMaxd(spec s1);
};

/// @brief The mode contains the global parameter from arguments of Main Function
class Mode{
    private:
        static Mode* instance;
        Mode();
        int mId = 0;
        double error = 1;
        int seed = 0;
    public:
        static Mode* getInstance();
        void setMachineId(int mId);
        void setError(double error);
        void setSeed(int seed);
        double getError();
        int getMachineId();
        int getSeed();

};

#endif