/**
 * @file The scheduler.h file contains the header template for Clairvoyant Scheduler Algorithms
 * 
 * 'schedulerClarivoyant.cpp' contains the implementation codes for the following algorithms.
 *  - Standard NRT
 *  - Prioritize NRT
 *  - Greedy
 *  - Classify By Duration
 *  - Classify By Departure Time
 *  
 * 'HA.cpp' contains the implementation codes for the following algorithms.
 *  - HA
 *  - Reduced HA
 *  - HA (Direct Sum)
 *  - Reduced HA (Direct Sum) 
 * 
 * 'LA.cpp' contains the implementation codes for the following algorithms.
 *  - LA (Geometric Ranges)
 *  - LA (Binary)
 * 
 * 'RCP.cpp' contains the implementation codes for the following algortihms.
 *  - RCP
 *  - PPE
 *  - RCP (without large bin)
 *  - PPE (without large bin)
 */
#ifndef CLAIRSCHEDULER_H
#define CLAIRSCHEDULER_H

#include "scheduler.h"
#include <unordered_set>
#include <math.h>
#include <cmath>

/// @brief The abstract class with implemented terminate logic that is reusable for NRT, LA, Greedy.
class DynamicClairvoyant: public Scheduler{
    public:
    int lastActive = 0;
    Cluster* cluster;
    set <pair<double, int>, decltype(bestFitcmp) > heuristics;
    unordered_map<int, set<pair<double,int> > > mapping;
    DynamicClairvoyant(): Scheduler(){cluster = storage->getClusters()[0];};
    virtual void insert(VM* vm, int nodeId);
    virtual void terminate(VM* vm);
};

/// @brief Standard NRT - Place VM to the nearest Node in term of time left to depart
class StandardNearestRemainingTime : public DynamicClairvoyant{
    public:
        StandardNearestRemainingTime(): DynamicClairvoyant(){};
        Node* request(VM* req);
};

/// @brief Prioritize NRT - Similar to Standard NRT but prioritize the node that have greater time left than the VM.
class PrioritizedNearestRemainingTime : public DynamicClairvoyant{        
    public:
        PrioritizedNearestRemainingTime(): DynamicClairvoyant(){};
        Node* request(VM* req);
};

/// @brief Greedy - Place VM to the bin with latest time left to depart
class GreedyAlgorithm : public DynamicClairvoyant{
    public:
        GreedyAlgorithm(): DynamicClairvoyant(){};
        Node* request(VM *req);
};

/// @brief LA (Geoemtric Ranges) - See paper
class LifetimeAlignment : public DynamicClairvoyant{
    public:
        LifetimeAlignment(): DynamicClairvoyant(){};
        Node* request(VM* req);
};

/// @brief LA (Binary) - See paper
class LifetimeAlignmentBinary : public DynamicClairvoyant{
    public:
        LifetimeAlignmentBinary(): DynamicClairvoyant(){};
        Node* request(VM* req);
};

/// @brief Create a new Abstract class for algorithms that categorize nodes and required a first fit algortihm for each category
class Clarivoyant: public Scheduler{
    public:
        Cluster* cluster;
        Clarivoyant(): Scheduler(){ cluster = storage->getClusters()[0];};
        
        int lastActive = 0;
        int firstFit(spec vmSpec, set<int>* binType);
};

/// @brief Classify By Duration - Classify the nodes with the duration
class ClassifyByDuration : public Clarivoyant{
    private: 
        map<int, set<int> > CD;  // Classify by duration
        map<int, int> allocation;
        int lastActive = 0;
        long double alpha;
    public:
        ClassifyByDuration(double alpha = 2): Clarivoyant(), alpha(alpha){};
        Node* request(VM* req);
        void terminate(VM* vm);
};

/// @brief Classify By Departure Time 
class ClassifyByDepartureTime : public Clarivoyant{
    private:
        double t;
        map<int,set<int>> dBin;
    public:
        ClassifyByDepartureTime(double t = 1): Clarivoyant(), t(t){};
        Node* request(VM *req);
        void terminate(VM* vm);
};

/// @brief Reduced HA
class ReducedHybridAlgorithm : public Clarivoyant{
    private:
        vector<double> pow2;
        map<int, set<int>* > allocation;
        set<int> GN; // General-type Bins
        map<int, set<int> > CD; // CD-type  Bins
        map<int, spec> d; 
        int offset = 0;
    public:
        ReducedHybridAlgorithm(int offset): offset(offset), Clarivoyant(){}; 
        Node* request(VM* req);
        void terminate(VM* vm);

};

/// @brief HA
class OriginalHybridAlgorithm : public Clarivoyant{
    private:
        map<int, set<int>* > allocation;
        set<int> GN; // General-type Bins
        map<pair<long long,long long>, set<int> > CD; // CD-type  Bins
        map<pair<long long,long long>, spec> d; 
        int offset = 0;
    public:
        OriginalHybridAlgorithm(int offset): offset(offset), Clarivoyant(){}; 
        Node* request(VM* req);
        void terminate(VM* vm);

};

/// @brief HA (Direct Sum)
class OriginalHybridAlgorithm_DirectSum : public Clarivoyant{
    private:
      
        map<int, set<int>* > allocation;
        set<int> GN; // General-type Bins
        map<tuple<long long,long long, long long>, set<int> > CD; // CD-type  Bins
        map<tuple<long long,long long, long long>, spec> d; 
        int offset = 0;
    public:
        OriginalHybridAlgorithm_DirectSum(int offset): offset(offset), Clarivoyant(){}; 
        
        Node* request(VM* req);
        void terminate(VM* vm);

};

/// @brief Reduced HA (Direct Sum)
class ReducedHybridAlgorithm_DirectSum : public Clarivoyant{
    private:
        map<int, set<int>* > allocation;
        set<int> GN; // General-type Bins
        map<pair<int,int>, set<int> > CD; // CD-type  Bins
        map<pair<int,int>, spec> d; 
        int offset = 0;
    public:
        ReducedHybridAlgorithm_DirectSum(int offset): offset(offset), Clarivoyant(){};
        Node* request(VM* req);
        void terminate(VM* vm);

};

/// @brief RCP
class RCPAlgorithm : public Clarivoyant{
    private:
        int x = 0;
        set<int> largemachine;
        unordered_set<int> seen_category;
        
        unordered_map<int, set<int> > marked_job; // categoryId to set of marked items
        unordered_map<int, set<int> > category_bins; // categoryId to the set of bins opened
        unordered_map<int, int> bin2category; 

        set<int> GN; // general machines
        map<int, spec> d; // categoryId to the total spec;
        map<int, set<int> > GN_items; // categoryId to set of items;
        map<int, int> item2CB; // item to category in marked job

        Node* base_machine = NULL;
        set<VM*> base_items;
        unordered_map<int, set<int> > category_items;
        
    public:
        RCPAlgorithm(): Clarivoyant(){};
        Node* request(VM* req);
        void terminate(VM* vm);
};

/// @brief PPE
class PPEAlgorithm : public Clarivoyant{
    private:
        int x = 0;
        set<int> largemachine;
        unordered_set<int> seen_category;
        
        unordered_map<int, set<int> > marked_job; // categoryId to set of marked items
        unordered_map<int, set<int> > category_bins; // categoryId to the set of bins opened
        unordered_map<int, int> bin2category; 

        set<int> GN; // general machines
        map<int, spec> d; // categoryId to the total spec;
        map<int, set<int> > GN_items; // categoryId to set of items;
        map<int, int> item2CB; // item to category in marked job

        Node* base_machine = NULL;
        set<VM*> base_items;
        unordered_map<int, set<int> > category_items;
        long double alpha = 1;
        
    public:
        PPEAlgorithm(): Clarivoyant(){};
        Node* request(VM* req);
        void terminate(VM* vm);
};

/// @brief RCP (without large bin)
class RCPNOLARGEAlgorithm : public Clarivoyant{
    private:
        int x = 0;
        unordered_set<int> seen_category;
        
        unordered_map<int, set<int> > marked_job; // categoryId to set of marked items
        unordered_map<int, set<int> > category_bins; // categoryId to the set of bins opened
        unordered_map<int, int> bin2category; 

        set<int> GN; // general machines
        map<int, spec> d; // categoryId to the total spec;
        map<int, set<int> > GN_items; // categoryId to set of items;
        map<int, int> item2CB; // item to category in marked job

        Node* base_machine = NULL;
        set<VM*> base_items;
        unordered_map<int, set<int> > category_items;
        
    public:
        RCPNOLARGEAlgorithm(): Clarivoyant(){};
        Node* request(VM* req);
        void terminate(VM* vm);
};

/// @brief PPE (without large bin)
class PPENOLARGEAlgorithm : public Clarivoyant{
    private:
        int x = 0;
        unordered_set<int> seen_category;
        
        unordered_map<int, set<int> > marked_job; // categoryId to set of marked items
        unordered_map<int, set<int> > category_bins; // categoryId to the set of bins opened
        unordered_map<int, int> bin2category; 
        
        set<int> GN; // general machines
        map<int, spec> d; // categoryId to the total spec;
        map<int, set<int> > GN_items; // categoryId to set of items;
        map<int, int> item2CB; // item to category in marked job

        Node* base_machine = NULL;
        set<VM*> base_items;
        unordered_map<int, set<int> > category_items;
        long double alpha = 1;
        
    public:
        PPENOLARGEAlgorithm(): Clarivoyant(){};
        Node* request(VM* req);
        void terminate(VM* vm);
};

#endif