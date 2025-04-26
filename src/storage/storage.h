#ifndef STORAGE_H
#define STORAGE_H

#include "vmType.h"
#include <map>
#include <tuple>
#include <utility>

/// @brief Enum of state of a physical machine (Bin)
enum state {RUNNING, TERMINATED};

/// @brief Class for a Virtual machine (VM) (Item)
class VM{
    public:
        int vmId;
        int vmTypeId;
        double curTime;
        double expectedEndTime;
        double realEndTime;
        long double predictedlifetime;
        VM(int vmId, int vmTypeId, double curTime, double expectedEndTime = 0, double realEndTime = 0, long double predictedlifetime = 0):vmId(vmId), vmTypeId(vmTypeId), curTime(curTime), expectedEndTime(expectedEndTime), realEndTime(realEndTime), predictedlifetime(predictedlifetime){}
};

/// @brief Class for a Node (Physical Machine)
class Node{
    public:
        int pid;
        int cid;
        spec sp;
        state st;
        Node(){}
        Node(int mid, int pid, int cid):pid(pid), cid(cid){
            sp.mId = mid, sp.core = 0, sp.memory = 0, sp.hdd = 0, sp.ssd = 0, sp.nic = 0; st = state::TERMINATED;
        }
        bool isActive();

};

/// @brief A Cluster refer to a set of nodes. (There may be multiple cluster, but as of now, there is only 1 cluster)
class Cluster{
    private:
        
        int cache = 1;
    public:
        int nActivePhy = 0;
        int nPhy = 4000;
        int mid;
        int cid;
        vector< Node* > machines;
        
        Cluster(int cid, int mid, int limit = 4000);
        int getCache();
        
};

/// @brief A singleton that simulate a physical storage
class Storage{
    private:
        Storage(int nPid);
        static Storage* instance;

        VmType* vmType;

        map<VM*, Node*> allocation; 
        vector<Cluster*> clusters;
        vector< vector<double> > recentTime;
        // Outer array means pId, inner array means the number of that types, storing requestId (rId) and total used spec
        int nActivePhy = 0;
        
        vector<pair<double,int> > history; 
        vector<Request> requests;

    public:
        static Storage* getInstance();
        
        /// @brief Create a VM and allocate it to a physical Node
        void created(VM* vm, Node* node, double timeStamp);

        /// @brief When a VM is terminate, remove it from the node
        void terminated(VM* vm, double timeStamp);  

        // Getter and Setter
        Cluster* getCluster(int mid);
        vector<Cluster*> getClusters();
        Node* getAllocation(VM* vm);
        int getActivePhy();
        vector<pair<double,int> > getHistory();
        double getRecentTime(int cId, int pId);
        void setRecentTime(int cId, int pId, double t);
        void setRequests(vector<Request> requests){ this->requests = requests;};
        vector<Request> getRequests(){ return requests;};
};  

#endif