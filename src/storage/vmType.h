#ifndef VMTYPE_H
#define VMTYPE_H
#include "../util.h"
#include <string>
#include <fstream>
#include <vector>
#include <sstream>
#include <unordered_map>
#include <iostream>
using namespace std;


class VmType{
    private:
        VmType();   
        static VmType* instance;
        const string path = "../data/vmType.csv";
        unordered_map<int,vector<spec> > vmTypeId2specs;
        vector< unordered_map<int, spec> >vmTypeIdmId2spec;
        vector<int> pDist;
        vector< set<int> > vmPairing;
    public:
        int NPID;
        static VmType* getInstance();
        vector<spec> getSpecs(int vmTypeId);
        spec getSpec(int vmTypeId, int mId);
        bool isValidSpec(int vmTypeId, int mId);
        int getDistP(int pId);
        int totalVmType();
        set<int> getVmTypeformId(int pId);
};

#endif
