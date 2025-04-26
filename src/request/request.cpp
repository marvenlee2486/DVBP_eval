#include "request.h"
#include <random>

/**
 * @brief Initialize Request Handler by reading the request from the vm.csv
 */
RequestHandler::RequestHandler(){
    string path_used = path;
    fstream file(path_used);

    vector<string> row;
    string word, line;
    getline(file, line); // get header out
    int seed_modified = Mode::getInstance()->getSeed();
    int mId = Mode::getInstance()->getMachineId();
    set<int> vmId;
    if(mId != -1) vmId = VmType::getInstance()->getVmTypeformId(mId);
    cout << mId << "\n";
    long double min_duration = 14;
    long double max_duration = 0;
    long long cnt = 0;
    unsigned int seed = (unsigned int) mId * 31 + seed_modified;
    srand(seed);

    std::mt19937 engine; // uniform random bit engine

    // seed the URBG
    std::random_device dev{};
    engine.seed(seed);

    // setup a distribution:
    double mu    = 0;
    double sigma = Mode::getInstance()->getError();
    std::lognormal_distribution<double> dist;
    if(sigma != 0)
       dist =  std::lognormal_distribution<double>(mu, sigma);    
    
    const int MOD = 1e3;
    long double minimum_expected_duration = 10000;
    while(getline(file, line)){
        stringstream str(line);
        row.clear();
        int x = 0;
        while(getline(str, word, ',')){
            x += 1;
            row.push_back(word);
        }

        if(x == 5) row.push_back("100");
        else if(row[5] == "" || row[5] == "\r" || row.size() <= 4) row[5] = "100";
        
        assert(row.size() == 6);

        int id = stoi(row[0]);
        int tenantId = stoi(row[1]);
        int vmTypeId = stoi(row[2]);
        int priority = stoi(row[3]);
        double starttime = stod(row[4]);
        double endtime = stod(row[5]);

        if(mId != -1){
            assert(starttime < endtime);
            
            if(starttime < 0 || endtime > 14 || !binary_search(vmId.begin(),vmId.end(), vmTypeId)) {
                continue;
            }
            assert(endtime <= 14);
        }
        double predictedlifetime, predictedendTime;
        if(sigma == 0){
            predictedlifetime = (endtime - starttime);
             predictedendTime = endtime;
        }else{
            predictedlifetime = (endtime - starttime) * dist(engine);
            predictedendTime = starttime + predictedlifetime;
        }
        
        requests.emplace_back(id, tenantId, vmTypeId, priority, starttime, endtime, predictedendTime, predictedlifetime);
        min_duration = min(min_duration, (long double) endtime - starttime);
        max_duration = max(max_duration, (long double) endtime - starttime);
        minimum_expected_duration = min(minimum_expected_duration, (long double) predictedlifetime);
        cnt++;
    }
  
    sort(requests.begin(), requests.end(), [](Request l, Request r)-> bool {
        return (l.startTime < r.startTime) || (fabs(l.startTime - r.startTime) < EPS && l.vmId < r.vmId);
    });

    this->minimum_duration = minimum_expected_duration;

    if(mId != -1){
        cout << "Request Cleaned with Following Parameter\n";
        cout << "\t- Negative start Time and No end time removed\n";
        cout << "\t- Only considered vmType that is support for physical machine of type " << mId << " which are :\n";
        cout << "\t";
        for(auto& v: vmId){
            cout << v << " ";
        }
        cout << "\n";
        cout << "Number of Request Left " << requests.size() << "\n";
    }
}

/**
 * @brief Calling next() will return the next request
 * 
 * This simulates the behavior of the request is make sequentially. 
 * Type of Request have two type - Create a new VM (Create) and terminate an existing VM (TERMINATE)
 */
pair<Request, typeOfRequest> RequestHandler::next(){
    if(idx == requests.size() && Mode::getInstance()->getMachineId() == -1) return {Request(0,0,0,0,0,0,0),typeOfRequest::ENDOFREQUEST};
    else if(idx == requests.size() && Mode::getInstance()->getMachineId() != -1 && pq.empty()) return {Request(0,0,0,0,0,0,0),typeOfRequest::ENDOFREQUEST};
    else if(idx == requests.size()  && Mode::getInstance()->getMachineId() != -1){
        Request ret = pq.top();
        pq.pop();
        return {ret,typeOfRequest::TERMINATE};
    }
    Request ret = requests[idx];
    
    if(!pq.empty() && (pq.top().endTime < ret.startTime || fabs(pq.top().endTime - ret.startTime) <= EPS) ){
        ret = pq.top();
        pq.pop();
        return {ret,typeOfRequest::TERMINATE};
    }
    // CREATE Request
    pq.emplace(ret);
    idx++;
    return {ret,typeOfRequest::CREATE};
}