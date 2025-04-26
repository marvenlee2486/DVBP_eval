#ifndef REQUEST_H
#define REQUEST_H

#include "../util.h"
#include "../storage/vmType.h"
#include <queue>
#include <string>
#include <fstream>
#include <vector>
#include <sstream>
#include <unordered_map>
#include <iostream>
#include <utility>

/// @brief  Types of VM requests
enum typeOfRequest {CREATE, TERMINATE, ENDOFREQUEST};

/// @brief Comparator for sorting requests by end time (min-heap)
class endTimeComparator{
    public:
        bool operator()(Request below, Request above){
            return below.endTime > above.endTime;
        }
};

/// @brief Handles reading, storing, and providing VM requests
class RequestHandler{
    private:
        string path = "../data/vm.csv";
        priority_queue<Request, vector<Request>, endTimeComparator > pq;
        vector<Request> requests;
        long double minimum_duration;
        int idx = 0;
    public:
        /// @brief The constructor reads vm.csv and construct the requests
        RequestHandler(); 
        /// @brief Calling next will return the next request
        pair<Request,typeOfRequest> next();
        vector<Request> getRequests(){
            return requests;
        };

        long double get_min_duration(){
            return this->minimum_duration;
        }
};

#endif