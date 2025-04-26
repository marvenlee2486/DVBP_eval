#include "request/request.h"
#include "scheduler/schedulerClairvoyant.h"
#include "storage/storage.h"
#include "storage/vmType.h"
#include "util.h"
#include <time.h>
#include <chrono>

const double INF = 1e9;
const double INF6 = 1e6;

/**
 * @brief Select the scheduling algorithms from argument
 * 
 * @param arg the argument of scheduling algorithms
 * @param parameter some scheduling algorithm required additional parameter to define the algorithm
 * 
 * @return The object of the scheduler from class in scheduler/ 
 */
Scheduler* select_scheduler_from_arg(int arg, double parameter){
    cout << "Select Algorithm - ";

    switch(arg){
        // Non-Clairvoyant Algorithm
        case 0: cout << "First Fit\n"; return new firstFit();
        case 1: cout << "Next Fit\n"; return new nextFit();
        case 2: cout << "Round Robin Next Fit\n"; return new RRnextFit();

        case 5: cout << "Most Recently Used\n"; return new MRUScheduler();
        case 6: cout << "Least Recently Used\n"; return new LRUScheduler();

        case 11: cout << "Best Fit L1\n"; return new SCL1BestFitScheduler();
        case 12: cout << "Best Fit L2\n"; return new SCL2BestFitScheduler();
        case 13: cout << "Best Fit Linf\n"; return new SCLinfBestFitScheduler();
        case 14: cout << "Worst Fit L1\n"; return new SCL1WorstFitScheduler();
        case 15: cout << "Worst Fit L2\n"; return new SCL2WorstFitScheduler();
        case 16: cout << "Worst Fit Linf\n"; return new SCLinfWorstFitScheduler();
        
        // Clairvoyant Algorithms
        case 120: cout << "Prioritized NRT\n"; return new StandardNearestRemainingTime();
        case 121: cout << "Standard NRT\n"; return new PrioritizedNearestRemainingTime();
        
        case 101: cout << "Classify-By-Duration\n"; if(parameter == 0) parameter = 2; return new ClassifyByDuration(parameter);
        case 102: cout << "Classify-By-Departure Time\n"; if(parameter == 0) parameter = 1; return new ClassifyByDepartureTime(parameter);
        case 103: cout << "Greedy Algorithm\n"; return new GreedyAlgorithm();
        
        case 104: cout << "Reduced Hybrid Algorithm\n"; return new ReducedHybridAlgorithm(parameter);
        case 105: cout << "Original Hybrid Algorithm\n"; return new OriginalHybridAlgorithm(parameter);
        case 111: cout << "Reduced Hybrid Algorithm - Direct Sum\n"; return new ReducedHybridAlgorithm_DirectSum(parameter);
        case 112: cout << "Original Hybrid Algorithm - Direct Sum\n"; return new OriginalHybridAlgorithm_DirectSum(parameter);

        case 106: cout << "(Geometric) Lifetime Alignment\n"; return new LifetimeAlignment();
        case 107: cout << "Lifetime Alignment Binary\n"; return new LifetimeAlignmentBinary();

        case 113: cout << "RCP Algorithm\n"; return new RCPAlgorithm();
        case 114: cout << "RCP No Large Machine Algorithn\n"; return new RCPNOLARGEAlgorithm();
        case 115: cout << "PPEAlgorithm\n"; return new PPEAlgorithm();
        case 116: cout << "PPE No Large Machine Algorithm\n"; return new PPENOLARGEAlgorithm();

        default: throw "Scheduler Not Found";
    }
}

/**
 * @brief Initialize the Simulator 
 * 
 * Create VM Type Schema, Storage Object, Request Handler, and scheduler in main memory.
 * Then, simulate the arriving (departing) of VM request and assigning the VM to a physical machine and physically allocate.
 * 
 * @param pathName the name of the file
 * @param schedulerId the scheduler algorithm
 * @param parameter The parameter for scheduler algorithm
 * 
 */
void initialize_simulation(string pathName, int schedulerId, double parameter){ 
    srand(time(NULL));
    
    cout << "START READING VM\n";
    VmType* vmType = VmType::getInstance();
    cout << "START READING Storage\n";
    Storage* storage = Storage::getInstance();
    cout << "START READING Request\n";
    RequestHandler* requestHandler = new RequestHandler();
    storage->setRequests(requestHandler->getRequests());
    cout << "START READING Schedule\n";
    if(schedulerId == 104 || schedulerId == 105 || schedulerId == 111 || schedulerId == 112){
        double min_lifetime = requestHandler->get_min_duration() *3600*24;
        parameter = (long long)(abs(floor(log2(min_lifetime))) + 3);
    }
    Scheduler* scheduler = select_scheduler_from_arg(schedulerId, parameter);
    fstream file(pathName, fstream::out);
    file << "timeStamp,nBin,nodeId,vmId\n";
    
    cout << "Initilization DONE\n";

    typeOfRequest type;
    Request req;
    tie(req, type) = requestHandler->next();
    int progress = 0;
    unordered_map<int, VM*> vms;
    double prev_t = 100;
    do{
        // cout << req.vmId << "\n";
        double t = 0; int n, id;
        if(progress % 1000000 == 0){
            cout << progress << " DONE \n";
        }

        if(type == typeOfRequest::CREATE){
            
            spec chosenSpec; 
            int pidx;
            VM* vm = new VM(req.vmId, req.vmTypeId, req.startTime, req.predictedendTime, req.endTime, req.predictedlifetime);
            Node* node = scheduler->request(vm);
            id = node->pid;
            t = req.startTime;
            storage->created(vm, node, req.startTime);
            vms[req.vmId] = vm;
        }
        else if(type == typeOfRequest::TERMINATE){
           
            t = req.endTime;
            VM* vm = vms[req.vmId];
            id = storage->getAllocation(vm)->pid;
            scheduler->terminate(vm);
            storage->terminated(vm, req.endTime);
            vms.erase(req.vmId);
        }else{
            cout << "ENDING";
            break;
        }
        n = storage->getActivePhy();
        
        file << t << "," << n  << "," << id << "," << req.vmId <<"\n";
        tie(req, type) = requestHandler->next();
        progress++;
    }while(type != typeOfRequest::ENDOFREQUEST);
    file.close();
}

/**
 * @brief Find the lower bound of a set of requests
 * 
 * If scheduler Id is 1000. This code is execute instead of void initialize_simulation(...)
 * 
 * @param pathName the name of the file
 */
void run_lower_bound_simulation(string pathName){
    srand(time(NULL));
    
    cout << "START READING VM\n";
    VmType* vmType = VmType::getInstance();
    cout << "START READING Storage\n";
    Storage* storage = Storage::getInstance();
    cout << "START READING Request\n";
    RequestHandler* requestHandler = new RequestHandler();
    storage->setRequests(requestHandler->getRequests());
    cout << "START READING Schedule\n";
    fstream file(pathName, fstream::out);
    file << "timeStamp,nBin,nodeId,vmId,predictedlen\n";
    
    cout << "Initilization DONE\n";
    typeOfRequest type;
    Request req;
    tie(req, type) = requestHandler->next();
    int progress = 0;
    unordered_map<int, spec> vms;

    int mId = Mode::getInstance()->getMachineId();
    spec cur = spec(mId, 0, 0, 0, 0, 0);
    do{
        // cout << req.vmId << "\n";
        auto start = chrono::high_resolution_clock::now();
        double t; int n;

        if(progress % 1000000 == 0){
            cout << progress << " DONE \n";
        }
        if(type == typeOfRequest::CREATE){   
            int pidx;
            spec chosenSpec = vmType->getSpec(req.vmTypeId, mId);
            t = req.startTime;
                
            vms[req.vmId] = chosenSpec;
            cur = cur + chosenSpec;
        }
        else if(type == typeOfRequest::TERMINATE){
            t = req.endTime;
            spec sp = vms[req.vmId];
            vms.erase(req.vmId);
            
            cur = cur - sp;
            
        }else{
            cout << "ENDING";
            break;
        }

        long double temp = max(cur.core, max(cur.memory, max(cur.nic, max(cur.hdd, cur.ssd))));
        if(temp - floor(temp) < 1e-6){
            n = floor(temp);
        }
        else{
            n = ceil(temp);
        }
        auto end = chrono::high_resolution_clock::now();
        double time_taken = chrono::duration_cast<chrono::nanoseconds>(end - start).count() / INF6;
        
        file << t << "," << n << "," << "0" << "," << req.vmId << "," << req.predictedendTime - req.startTime << "\n";
        
        tie(req, type) = requestHandler->next();
        progress++;
        
    }while(type != typeOfRequest::ENDOFREQUEST);
    
    file.close();
}


int main(int argc, char *argv[]){
  
    string status = "000"; 
    int mId = -1;
    int scheduler = -1;
    double error = 1;
    double parameter = 0;
    int seed = 0;
    string filename = "";

    for (int i = 1; i < argc; ++i) { // Start from index 1 to skip program name
        std::string arg(argv[i]);
        if ( (arg == "-m" || arg == "--machine" ) && i + 1 < argc) {
            mId = stoi(argv[++i]);
            status[0] = '1';
        } else if ( (arg == "-s" || arg == "--scheduler") && i + 1 < argc) {
            scheduler = std::stoi(argv[++i]);
            status[1] = '1';
        } else if ( (arg == "-o" || arg == "--output") && i + 1 < argc){
            filename = argv[++i];
            status[2] = '1';
        } else if ( (arg == "-e" || arg == "--error") && i + 1 < argc){
            error = std::stold(argv[++i]);
        } else if( (arg == "-p" || arg == "--parameter") && i + 1 < argc){
            parameter = std::stold(argv[++i]);
        } else if( (arg == "-r" || arg == "--seed") && i + 1 < argc){
            seed = std::stoi(argv[++i]);
        }
    }
    if(status != "111"){
        cout << "Not enought arguments\n";
        // TODO LOGGING
        return 0;
    }

    filename = "../" + filename + "_nbin.csv"; // TODO input argz

    Mode::getInstance()->setMachineId(mId);    
    Mode::getInstance()->setError(error);
    Mode::getInstance()->setSeed(seed);
    if(scheduler == 1000){
        run_lower_bound_simulation(filename);
        return 0;
    }
    initialize_simulation(filename, scheduler, parameter);
}
