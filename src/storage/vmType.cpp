#include "vmType.h"
#include <iostream>
#include <set>
#include <map>

VmType* VmType::instance = NULL;

VmType::VmType(){
    string path_used = path;
    fstream file(path_used);

    vector<string> row;
    string word, line;
    map<int, int> pId;
    int maxVmTypeId = 0;
    set<int> vmTypeSet;
    vmPairing.assign(35, set<int>());

    getline(file, line); // get header out
    while(getline(file, line)){
        stringstream str(line);
        row.clear();
        int x = 0;
        while(getline(str, word, ',')){
            x += 1;
            if(word == "\r" || word == "") word = "0";
            row.push_back(word);
        }
        while(x != 8) {
            row.emplace_back("0");
            x += 1;
        }
        int id, vmTypeId, machineId;
        double core, memory, hhd, ssd, sic;
       
        id = stoi(row[0]);
        vmTypeId = stoi(row[1]);
        machineId = stoi(row[2]);
        core = stod(row[3]);
        memory = stod(row[4]);
         
        try{
            hhd = stod(row[5]);
        }
        catch(const exception ex) {
            hhd = 0;
        }
        
        try{
            ssd = stod(row[6]);
        }
        catch(const exception ex){
            ssd = 0;
        }
        try{
            sic = stod(row[7]);
        }
        catch(const exception ex){
            sic = 0;
        }
        vmTypeSet.insert(vmTypeId);
        if(pId.find(machineId) == pId.end()) pId[machineId] = 0;
        
        pId[machineId]+= 1;
        maxVmTypeId = max(maxVmTypeId, vmTypeId);
        vmPairing[machineId].insert(vmTypeId);
        if(vmTypeId == 466 && machineId == 0){
            cout << "HEREHERE\n";
        }
        vmTypeId2specs[vmTypeId].emplace_back(machineId, core, memory, hhd, ssd, sic);
        
    }

    NPID = pId.size();//"TODO";
    for(auto[_,cnt]: pId){
        pDist.emplace_back(cnt);
    }
    cout << maxVmTypeId << "\n";
    vmTypeIdmId2spec.assign(maxVmTypeId + 1, unordered_map<int, spec>());
    for(auto&[vId, sps]:vmTypeId2specs){
        for(auto& sp: sps){
            if(vId == 466 && sp.mId == 0){
                cout << "HEREHEREAGAIN\n";
            }
            vmTypeIdmId2spec[vId][sp.mId] = sp;
        }
    }
}

vector<spec> VmType::getSpecs(int vmTypeId){
    return vmTypeId2specs[vmTypeId];
}

int VmType::totalVmType(){
    return vmTypeId2specs.size();
}

VmType* VmType::getInstance(){
    if (VmType::instance == nullptr)
        VmType::instance = new VmType();
    return VmType::instance;
}

int VmType::getDistP(int pId){
    return pDist[pId];
}

spec VmType::getSpec(int vmTypeId, int mId){
    return vmTypeIdmId2spec[vmTypeId][mId];
}

bool VmType::isValidSpec(int vmTypeId, int mId){
    return vmTypeIdmId2spec[vmTypeId].find(mId) != vmTypeIdmId2spec[vmTypeId].end();
}

set<int> VmType::getVmTypeformId(int pId){
    return vmPairing[pId];
}