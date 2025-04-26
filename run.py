# %%
import pandas as pd
import numpy as np
import subprocess
import concurrent.futures
import os 

vmType = pd.read_csv("data/vmType.csv")

# %%
mapping = {}
for index, df in vmType.groupby('machineId'):
    if index == 17: # empty requests
        continue
    if index == 20: # empty requests
        continue
    if index == 33: # crash 
        continue
    
    df_list = df.vmTypeId.to_list()
    yes = True
    
    for k,v in mapping.items():
        if(index == 30):
            break
        if(set(v) == set(df_list)):
            print(index, k)
            yes = False
            break
        
    if yes:
        mapping[index] = df_list
validIds = list(mapping.keys())
print(validIds)
print(len(validIds))

# %%
## Can choose to use this sequence to reduce generating time
validIds = [12, 31, 8, 16, 11, 23, 6, 34, 3, 22, 1, 9, 30, 24, 27, 26, 25, 2, 7, 15, 14, 5, 28, 19, 32, 0, 4, 10, 18]

# %%
## Normal + error (log normal)
processes = []

dict = {
    11: "BestFit_L1",
    12: "BestFit_L2",
    13: "BestFit_Linf",
    14: "WorstFit_L1",
    15: "WorstFit_L2",
    16: "WorstFit_Linf",

    0: "FirstFit",
    1: "NextFit",
    2: "RRNextFit",

    5: "MRU",
    6: "LRU",
    
    120: "BestCRT",
    121: "CRT",

    101: "Classifybyduration",
    102: "Classifybydeparture",
    103: "Greedy",
    104: "Reduced_HA",
    105: "HA",
    111: "Reduced_HA_DirectSum",
    112: "HA_DirectSum",

    106: "LA_logsecond",
    107: "LA_binary",

    113: "RCP",
    114: "RCP_NOLARGE",
    115: "PPE",
    116: "PPE_NOLARGE",

    1000: "lowerbound"
}

def run_process(machineId, sch, error = 0, seed = 0):
    path = "generated/Results"
    if error != 0:
        path += "_e_log" + str(error)
    if seed != 0:
        path += "_s" + str(seed)
    path += "/"
    
    if not os.path.isdir("../generated"):
        os.mkdir("../generated")
    if not os.path.isdir("../" + path):
        os.mkdir("../" + path)
    
    fileName = path + dict[sch] + "_pId_" + str(machineId)
    
    print("For Physical Machine Id ", machineId, "Run", dict[sch], seed)
    command = ["./dvbp", "-o", fileName, "-m", str(machineId), "-s", str(sch), "-e", str(error), "-r",  str(seed)]
    subprocess.run(command)
    
max_concurrent_processes = 100

with concurrent.futures.ThreadPoolExecutor(max_workers=max_concurrent_processes) as executor:
    futures = []

    #  Non-Clarivoyant
    futures += [executor.submit(run_process, machineId, sch) for sch in [11, 12, 13, 14, 15, 16, 0, 1, 2, 5, 6] for machineId in validIds]  

    # Clairvoyant 
    futures += [executor.submit(run_process, machineId, sch, error) for sch in [106, 107, 120, 121, 101, 102, 103, 104, 105, 111, 112, 113, 114, 115, 116] for machineId in validIds for error in [0, 0.5, 1, 1.5, 2, 2.5, 3, 4, 5, 7.5, 10, 12.5, 15]]  

    # lower bound
    futures += [executor.submit(run_process, machineId, sch) for sch in [1000] for machineId in validIds]  

    for future in concurrent.futures.as_completed(futures):
        future.result()

# %%
def run_process_with_parameter(machineId, sch, parameter, error = 0, seed = 0):
    path = "generated/Results"
    if error != 0:
        path += "_e_log" + str(error)
    if seed != 0:
        path += "_s" + str(seed)
    path += "/"
    
    if not os.path.isdir("../generated"):
        os.mkdir("../generated")
    if not os.path.isdir("../" + path):
        os.mkdir("../" + path)
    
    fileName = path + dict[sch] + "_p" + str(parameter) + "_pId_" + str(machineId)
    
    print("For Physical Machine Id ", machineId, "Run", dict[sch], seed)
    command = ["./dvbp", "-o", fileName, "-m", str(machineId), "-s", str(sch), "-e", str(error), "-r",  str(seed), "-p", str(parameter)]
    subprocess.run(command)


with concurrent.futures.ThreadPoolExecutor(max_workers=max_concurrent_processes) as executor:
    futures = []

    # Classify By Duration
    futures += [executor.submit(run_process, machineId, 101, p) for p in [1.25, 1.5, 2, 3, 4, 5, 8] for machineId in validIds]  

    # Classify By Departure 
    futures += [executor.submit(run_process, machineId, 102, p) for p in [0.0625, 0.125, 0.25, 0.5, 1, 2, 4, 8] for machineId in validIds]  

    for future in concurrent.futures.as_completed(futures):
        future.result()
 


