#include "schedulerClairvoyant.h"


Node* RCPAlgorithm::request(VM* vm){
    spec vmSpec = vmType->getSpec(vm->vmTypeId, cluster->mid);

    double tend = vm->expectedEndTime;
    double tstart = vm->curTime;
    double duration = vm->predictedlifetime;
    int category = floor(log2(duration));
    if(seen_category.find(category) == seen_category.end()) {
        seen_category.emplace(category); 
        marked_job[category] = set<int>();
        GN_items[category] = set<int>();
        d[category] = spec(vmSpec.mId,0,0,0,0,0);
        x++;
    }

    // scheduler to large machines
    if(max(vmSpec.core, max(vmSpec.memory,  max(vmSpec.nic , max(vmSpec.hdd, vmSpec.ssd)))) > 0.5){
        largemachine.emplace(lastActive);
        return cluster->machines[lastActive++];
    }
    
    // line 13
    spec category_load = d[category];
    if( 1 - Util::LMin(category_load + vmSpec) <= 1.0 / sqrt(x)){
        int id = firstFit(vmSpec, &GN);
        GN_items[category].emplace(vm->vmId);
        d[category] = d[category] + vmSpec;
        return cluster->machines[id];
    }

    if(marked_job.find(category) == marked_job.end() || marked_job[category].size() == 0){

        if(base_machine == NULL){
            base_machine = cluster->machines[lastActive++];
        }
        
        base_items.emplace(vm);
        int pId = base_machine->pid;
        if(1 - Util::LMin(base_machine->sp + vmSpec) > 0.5){
            
            int largest_category = -1;
            double largest = -1;
            
            for(auto& it: base_items){
                long double dur = it->expectedEndTime - it->curTime;

                if(dur > largest){
                    largest_category = floor(log2(dur));
                    largest = dur;
                }
            }

            for(auto& it: base_items){
                marked_job[largest_category].emplace(it->vmId);
                item2CB[it->vmId] = largest_category;
            }
            
            category_bins[largest_category].emplace(base_machine->pid);
            bin2category[base_machine->pid] = largest_category;
            base_items.clear();
            base_machine = NULL;
        }
        return cluster->machines[pId];
    }
    
    int retId = firstFit(vmSpec, &category_bins[category]);
    bin2category[retId] = category;
    assert(cluster->machines[retId]->pid == retId);
    spec total_category = spec(vmSpec.mId,0,0,0,0,0);
    for(int pId: category_bins[category]){
        total_category = total_category + cluster->machines[pId]->sp;
    }
    category_items[category].emplace(vm->vmId);
    if(1 - Util::LMin(total_category) > 0.5){
        for(auto& id: category_items[category]){
            marked_job[category].emplace(id);
            item2CB[id] = category;
        }
        category_items.clear();
    }
    
    return cluster->machines[retId];
}

void RCPAlgorithm::terminate(VM* vm){
    Node* node = storage->getAllocation(vm);
    spec vmSpec = vmType->getSpec(vm->vmTypeId, node->sp.mId);
    spec nodeSpec = node->sp;
    
    int category = floor(log2(vm->predictedlifetime));
    if(item2CB.find(vm->vmId) != item2CB.end()){
        category = item2CB[vm->vmId];
    }

    // make sure all the jobs are properly deleted as they departed (real)
    if(GN_items[category].find(vm->vmId) != GN_items[category].end()){
        GN_items[category].erase(vm->vmId);
        d[category] = d[category] - vmSpec;
    }
    
    if(marked_job[category].find(vm->vmId) != marked_job[category].end()){
        marked_job[category].erase(vm->vmId);
    }

    if(base_items.find(vm) != base_items.end()){
        base_items.erase(vm);
    }

    if(category_items[category].find(vm->vmId) != category_items[category].end()){
        category_items[category].erase(vm->vmId);
    }

    if((nodeSpec - vmSpec) == 0){
        
        int nodeId = node->pid;
        
        if(GN.find(nodeId) != GN.end()){
            GN.erase(nodeId);
        }

        if(bin2category.find(nodeId) != bin2category.end()){
            category_bins[bin2category[nodeId]].erase(nodeId);
        }

        if(largemachine.find(nodeId) != largemachine.end()){
            largemachine.erase(nodeId);
        }
    } 
    
}


Node* PPEAlgorithm::request(VM* vm){
    spec vmSpec = vmType->getSpec(vm->vmTypeId, cluster->mid);

    double tend = vm->expectedEndTime;
    double tstart = vm->curTime;
    double duration = vm->predictedlifetime;
    int category = floor(log2(duration));
    if(seen_category.find(category) == seen_category.end()) {
        seen_category.emplace(category); 
        marked_job[category] = set<int>();
        GN_items[category] = set<int>();
        d[category] = spec(vmSpec.mId,0,0,0,0,0);
        x++;
    }

    // scheduler to large machines
    if(max(vmSpec.core, max(vmSpec.memory,  max(vmSpec.nic ,  max(vmSpec.hdd, vmSpec.ssd)))) > 0.5){
        largemachine.emplace(lastActive);
        return cluster->machines[lastActive++];
    }
    
    // line 13
    spec category_load = d[category];
    if( 1 - Util::LMin(category_load + vmSpec) <= alpha * 1.0 / sqrt(x)){
        int id = firstFit(vmSpec, &GN);
        GN_items[category].emplace(vm->vmId);
        d[category] = d[category] + vmSpec;
        return cluster->machines[id];
    }

    if(marked_job.find(category) == marked_job.end() || marked_job[category].size() == 0){

        if(base_machine == NULL){
            base_machine = cluster->machines[lastActive++];
        }
        
        base_items.emplace(vm);
        int pId = base_machine->pid;
        if(1 - Util::LMin(base_machine->sp + vmSpec) > 0.5){
            
            int largest_category = -1;
            double largest = -1;
            
            for(auto& it: base_items){
                long double dur = it->expectedEndTime - it->curTime;
                
                if(dur > largest){
                    largest_category = floor(log2(dur));
                    largest = dur;
                }
            }
            
            for(auto& it: base_items){
                marked_job[largest_category].emplace(it->vmId);
                item2CB[it->vmId] = largest_category;
            }
            
            category_bins[largest_category].emplace(base_machine->pid);
            bin2category[base_machine->pid] = largest_category;
            base_items.clear();
            base_machine = NULL;
        }
        return cluster->machines[pId];
    }
    
    int retId = firstFit(vmSpec, &category_bins[category]);
    bin2category[retId] = category;
    assert(cluster->machines[retId]->pid == retId);
    spec total_category = spec(vmSpec.mId,0,0,0,0,0);
    for(int pId: category_bins[category]){
        total_category = total_category + cluster->machines[pId]->sp;
    }
    category_items[category].emplace(vm->vmId);
    if(1 - Util::LMin(total_category) > 0.5){
        for(auto& id: category_items[category]){
            marked_job[category].emplace(id);
            item2CB[id] = category;
        }
        category_items.clear();
    }
    
    return cluster->machines[retId];
}

void PPEAlgorithm::terminate(VM* vm){
    Node* node = storage->getAllocation(vm);
    spec vmSpec = vmType->getSpec(vm->vmTypeId, node->sp.mId);
    spec nodeSpec = node->sp;
    
    int category = floor(log2(vm->predictedlifetime));
    if(item2CB.find(vm->vmId) != item2CB.end()){
        category = item2CB[vm->vmId];
    }

    // make sure all the jobs are properly deleted as they departed (real)
    if(GN_items[category].find(vm->vmId) != GN_items[category].end()){
        GN_items[category].erase(vm->vmId);
        d[category] = d[category] - vmSpec;
    }
    
    if(marked_job[category].find(vm->vmId) != marked_job[category].end()){
        marked_job[category].erase(vm->vmId);
    }

    if(base_items.find(vm) != base_items.end()){
        base_items.erase(vm);
    }

    if(category_items[category].find(vm->vmId) != category_items[category].end()){
        category_items[category].erase(vm->vmId);
    }

    if((nodeSpec - vmSpec) == 0){
        
        int nodeId = node->pid;
        
        if(GN.find(nodeId) != GN.end()){
            GN.erase(nodeId);
        }

        if(bin2category.find(nodeId) != bin2category.end()){
            category_bins[bin2category[nodeId]].erase(nodeId);
        }

        if(largemachine.find(nodeId) != largemachine.end()){
            largemachine.erase(nodeId);
        }
    } 

    // PPE
    if(vm->realEndTime / vm->expectedEndTime > alpha){
        int n = floor(log2(vm->realEndTime / vm->expectedEndTime));
        if( (pow(2,n) - vm->realEndTime / vm->expectedEndTime ) < EPS){ // equality with LHS
            alpha = pow(2,n); 
        }
        else{
            alpha = pow(2, n + 1);
        }
    }
    
}



Node* RCPNOLARGEAlgorithm::request(VM* vm){
    spec vmSpec = vmType->getSpec(vm->vmTypeId, cluster->mid);

    double tend = vm->expectedEndTime;
    double tstart = vm->curTime;
    double duration = vm->predictedlifetime;
    int category = floor(log2(duration));
    if(seen_category.find(category) == seen_category.end()) {
        seen_category.emplace(category); 
        marked_job[category] = set<int>();
        GN_items[category] = set<int>();
        d[category] = spec(vmSpec.mId,0,0,0,0,0);
        x++;
    }
    
    // line 13
    spec category_load = d[category];
    if( 1 - Util::LMin(category_load + vmSpec) <= 1.0 / sqrt(x)){
        int id = firstFit(vmSpec, &GN);
        GN_items[category].emplace(vm->vmId);
        d[category] = d[category] + vmSpec;
        return cluster->machines[id];
    }

    if((marked_job.find(category) == marked_job.end() || marked_job[category].size() == 0) && (base_machine == NULL || Util::canFit(vmSpec, base_machine->sp))){

        if(base_machine == NULL){
            base_machine = cluster->machines[lastActive++];
        }
        
        base_items.emplace(vm);
        int pId = base_machine->pid;
        if(1 - Util::LMin(base_machine->sp + vmSpec) > 0.5){
            
            int largest_category = -1;
            double largest = -1;
            
            for(auto& it: base_items){
                long double dur = it->expectedEndTime - it->curTime;
               
                if(dur > largest){
                    largest_category = floor(log2(dur));
                    largest = dur;
                }
            }
           
            for(auto& it: base_items){
                marked_job[largest_category].emplace(it->vmId);
                item2CB[it->vmId] = largest_category;
            }
            
            category_bins[largest_category].emplace(base_machine->pid);
            bin2category[base_machine->pid] = largest_category;
            base_items.clear();
            base_machine = NULL;
        }
        return cluster->machines[pId];
    }
    
    int retId = firstFit(vmSpec, &category_bins[category]);
    bin2category[retId] = category;
    assert(cluster->machines[retId]->pid == retId);
    spec total_category = spec(vmSpec.mId,0,0,0,0,0);
    for(int pId: category_bins[category]){
        total_category = total_category + cluster->machines[pId]->sp;
    }
    category_items[category].emplace(vm->vmId);
    if(1 - Util::LMin(total_category) > 0.5){
        for(auto& id: category_items[category]){
            marked_job[category].emplace(id);
            item2CB[id] = category; 
        }
        category_items.clear();
    }
    
    return cluster->machines[retId];
}

void RCPNOLARGEAlgorithm::terminate(VM* vm){
    Node* node = storage->getAllocation(vm);
    spec vmSpec = vmType->getSpec(vm->vmTypeId, node->sp.mId);
    spec nodeSpec = node->sp;
    
    int category = floor(log2(vm->predictedlifetime));
    if(item2CB.find(vm->vmId) != item2CB.end()){
        category = item2CB[vm->vmId];
    }

    // make sure all the jobs are properly deleted as they departed (real)
    if(GN_items[category].find(vm->vmId) != GN_items[category].end()){
        GN_items[category].erase(vm->vmId);
        d[category] = d[category] - vmSpec;
    }
    
    if(marked_job[category].find(vm->vmId) != marked_job[category].end()){
        marked_job[category].erase(vm->vmId);
    }

    if(base_items.find(vm) != base_items.end()){
        base_items.erase(vm);
    }

    if(category_items[category].find(vm->vmId) != category_items[category].end()){
        category_items[category].erase(vm->vmId);
    }

    if((nodeSpec - vmSpec) == 0){
        
        int nodeId = node->pid;
        if(GN.find(nodeId) != GN.end()){
            GN.erase(nodeId);
        }

        if(bin2category.find(nodeId) != bin2category.end()){
            category_bins[bin2category[nodeId]].erase(nodeId);
        }
    } 
    
}


Node* PPENOLARGEAlgorithm::request(VM* vm){
    spec vmSpec = vmType->getSpec(vm->vmTypeId, cluster->mid);

    double tend = vm->expectedEndTime;
    double tstart = vm->curTime;
    double duration = vm->predictedlifetime;
    int category = floor(log2(duration));
    if(seen_category.find(category) == seen_category.end()) {
        seen_category.emplace(category); 
        marked_job[category] = set<int>();
        GN_items[category] = set<int>();
        d[category] = spec(vmSpec.mId,0,0,0,0,0);
        x++;
    }

    // line 13
    spec category_load = d[category];
    if( 1 - Util::LMin(category_load + vmSpec) <= alpha * 1.0 / sqrt(x)){
        int id = firstFit(vmSpec, &GN);
        GN_items[category].emplace(vm->vmId);
        d[category] = d[category] + vmSpec;
        return cluster->machines[id];
    }

    if((marked_job.find(category) == marked_job.end() || marked_job[category].size() == 0) && (base_machine == NULL || Util::canFit(vmSpec, base_machine->sp)) ){

        if(base_machine == NULL){
            base_machine = cluster->machines[lastActive++];
        }
        
        base_items.emplace(vm);
        int pId = base_machine->pid;
        if(1 - Util::LMin(base_machine->sp + vmSpec) > 0.5){
            
            int largest_category = -1;
            double largest = -1;
            
            for(auto& it: base_items){
                long double dur = it->expectedEndTime - it->curTime;
                if(dur > largest){
                    largest_category = floor(log2(dur));
                    largest = dur;
                }
            }

            for(auto& it: base_items){
                marked_job[largest_category].emplace(it->vmId);
                item2CB[it->vmId] = largest_category;
            }
            
            category_bins[largest_category].emplace(base_machine->pid);
            bin2category[base_machine->pid] = largest_category;
            base_items.clear();
            base_machine = NULL;
        }
        return cluster->machines[pId];
    }
    
    int retId = firstFit(vmSpec, &category_bins[category]);
    bin2category[retId] = category;
    assert(cluster->machines[retId]->pid == retId);
    spec total_category = spec(vmSpec.mId,0,0,0,0,0);
    for(int pId: category_bins[category]){
        total_category = total_category + cluster->machines[pId]->sp;
    }
    category_items[category].emplace(vm->vmId);
    if(1 - Util::LMin(total_category) > 0.5){
        for(auto& id: category_items[category]){
            marked_job[category].emplace(id);
            item2CB[id] = category;
        }
        category_items.clear();
    }
    
    return cluster->machines[retId];
}

void PPENOLARGEAlgorithm::terminate(VM* vm){
    Node* node = storage->getAllocation(vm);
    spec vmSpec = vmType->getSpec(vm->vmTypeId, node->sp.mId);
    spec nodeSpec = node->sp;
    
    int category = floor(log2(vm->predictedlifetime));
    if(item2CB.find(vm->vmId) != item2CB.end()){
        category = item2CB[vm->vmId];
    }

    // make sure all the jobs are properly deleted as they departed (real)
    if(GN_items[category].find(vm->vmId) != GN_items[category].end()){
        GN_items[category].erase(vm->vmId);
        d[category] = d[category] - vmSpec;
    }
    
    if(marked_job[category].find(vm->vmId) != marked_job[category].end()){
        marked_job[category].erase(vm->vmId);
    }

    if(base_items.find(vm) != base_items.end()){
        base_items.erase(vm);
    }

    if(category_items[category].find(vm->vmId) != category_items[category].end()){
        category_items[category].erase(vm->vmId);
    }
    
    if((nodeSpec - vmSpec) == 0){
        
        int nodeId = node->pid;
        
        if(GN.find(nodeId) != GN.end()){
            GN.erase(nodeId);
        }

        if(bin2category.find(nodeId) != bin2category.end()){
            category_bins[bin2category[nodeId]].erase(nodeId);
        }

    } 

    // PPE
    if(vm->realEndTime / vm->expectedEndTime > alpha){
        int n = floor(log2(vm->realEndTime / vm->expectedEndTime));
        if( (pow(2,n) - vm->realEndTime / vm->expectedEndTime ) < EPS){ // equality with LHS
            alpha = pow(2,n); 
        }
        else{
            alpha = pow(2, n + 1);
        }
    }
    
}
