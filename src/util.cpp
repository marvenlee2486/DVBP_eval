#include "util.h"

double Util::timeCounter = 0;

bool Util::canFit(spec s1, spec s2){
    // printSpec(s1);
    // printSpec(s2);
    spec total = s1 + s2;
    // printSpec(s1);
    // printSpec(s2);
    return (total.core <= 1 + EPS) && (total.memory <= 1 + EPS) && (total.nic <= 1 + EPS) && (total.hdd <= 1 + EPS) && (total.ssd <= 1 + EPS); 
}

/// @brief L1 Norm (Mahantthan)
/// @param sp The vector of requested resources
/// @return 
double Util::L1(spec sp){
    if(sp.mId == 2 || sp.mId == 5 || sp.mId == 6 || sp.mId == 7 || sp.mId == 14 || sp.mId == 26)
        return 5 - (sp.core + sp.memory + sp.nic + sp.hdd + sp.ssd);
    else
        return 4 - (sp.core + sp.memory + sp.nic + sp.ssd);
}

/// @brief L2 Norm (Euclidean)
/// @param sp The vector of requested resources
/// @return 
double Util::L2(spec sp){
    if(sp.mId == 2 || sp.mId == 5 || sp.mId == 6 || sp.mId == 7 || sp.mId == 14 || sp.mId == 26)
        return sqrt(pow(1 - sp.core, 2) + pow(1 - sp.memory, 2) + pow(1 - sp.nic, 2) + pow(1 - sp.ssd, 2) + pow(1 - sp.hdd, 2));
    else 
        return sqrt(pow(1 - sp.core, 2) + pow(1 - sp.memory, 2) + pow(1 - sp.nic, 2) + pow(1 - sp.ssd, 2));
}

/// @brief L_inf Norm (Chebyshev)
/// @param sp The vector of requested resources
/// @return
double Util::LinfMax(spec sp, int arg){
    if(arg == 1){
        return 1 - min(sp.core, min(sp.memory,  min(sp.nic ,sp.ssd)));
    }
    if(sp.mId == 2 || sp.mId == 5 || sp.mId == 6 || sp.mId == 7 || sp.mId == 14 || sp.mId == 26)
        return 1 - min(sp.core, min(sp.memory,  min(sp.nic , min(sp.hdd, sp.ssd))));
    else 
        return 1 - min(sp.core, min(sp.memory,  min(sp.nic ,sp.ssd)));
}

/// @brief MIN instead of Max for L_inf norm
/// @param sp The vector of requested resources
/// @return
double Util::LMin(spec sp){
    if(sp.mId == 2 || sp.mId == 5 || sp.mId == 6 || sp.mId == 7 || sp.mId == 14 || sp.mId == 26)
        return 1 - max(sp.core, max(sp.memory,  max(sp.nic , max(sp.hdd, sp.ssd)))); 
    else
        return 1 - max(sp.core, max(sp.memory,  max(sp.nic , sp.ssd))); 
}

/// @brief A more efficient way in comparing the Linf norm.
/// If the most dominant dimension is tie, it will check the second most dominant one, etc.
/// @param sp1 the first vector
/// @param sp2 the second vector
/// @return 1 if sp1 is less than sp2, 0 if same, -1 otherwise
int Util::LinfMaxCompare(spec sp1, spec sp2){
    
    double sp1max = Util::LinfMax(sp1, 0);
    double sp2max = Util::LinfMax(sp2, 0);

    if(sp1max < sp2max) return -1;
    else if(sp1max > sp2max) return 1;
 
    vector<double> sp1v, sp2v;
    sp1v.emplace_back(sp1.core);
    sp1v.emplace_back(sp1.memory);
    sp1v.emplace_back(sp1.nic);
    sp1v.emplace_back(sp1.ssd);

    sp2v.emplace_back(sp2.core);
    sp2v.emplace_back(sp2.memory);
    sp2v.emplace_back(sp2.nic);
    sp2v.emplace_back(sp2.ssd);
    spec sp = sp1;
    if(sp.mId == 2 || sp.mId == 5 || sp.mId == 6 || sp.mId == 7 || sp.mId == 14 || sp.mId == 26){
        sp1v.emplace_back(sp1.hdd);
        sp2v.emplace_back(sp2.hdd);
    }

    sort(sp1v.begin(), sp1v.end());
    sort(sp2v.begin(), sp2v.end());

    for(int i = 1; i < sp1v.size(); i++){
        if(sp1v[i] < sp2v[i]) return 1;
        else if(sp1v[i] > sp2v[i]) return -1;
    }
    return 0;
}

void Util::printSpec(spec s1){
    cout << "pId = " << s1.mId << ", core = " << s1.core << ", hdd = " << s1.hdd << ", ssd = " << s1.ssd << ", memory = " << s1.memory << ", nic = " << s1.nic << "\n";
}

double Util::getCurrentTime(){
    Util::timeCounter++;
    return Util::timeCounter;
}

Mode* Mode::instance = NULL;
Mode::Mode(){
    mId = -1;
}


Mode* Mode::getInstance(){
    if (Mode::instance == nullptr){
        Mode::instance = new Mode();
    }
    return Mode::instance;
}

void Mode::setMachineId(int mId){
    this->mId = mId;
}

int Mode::getMachineId(){
    return mId;
}

void Mode::setError(double e){
    error = e;
}

void Mode::setSeed(int seed){
    this->seed = seed;
}

double Mode::getError(){
    return error;
}

int Mode::getSeed(){
    return seed;
}

int Util::getMaxd(spec s1){
    long double maxv = max(s1.core, max(s1.hdd, max(s1.memory, max(s1.nic, s1.ssd))));
    if(s1.core == maxv) return 1;
    if(s1.memory == maxv) return 2;
    if(s1.nic == maxv) return 3;
    if(s1.ssd == maxv) return 4;
    if(s1.hdd == maxv) return 5;
    return -1;
}