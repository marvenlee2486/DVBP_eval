# %%
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
from textwrap import wrap
import concurrent

# %% [markdown]
# # Read Required files

# %%
vm = pd.read_csv("data/vm.csv")
vmType = pd.read_csv("data/vmType.csv")

# %%
def calculate_active_time(data):
    duplicate = data['timeStamp'] 
    duplicate = pd.concat([duplicate[1:], pd.Series([14])]).reset_index(drop = True)
    data['afterTime'] = duplicate
    data['activeTime'] = (data['afterTime'] - data['timeStamp']) * data['nBin']
    total = data['activeTime'].sum()
    return total

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
def valid(data):
    # if data[(data.timeStamp < 0) | (data.timeStamp > 14)].shape[0] != 0:
    #     return False
    if data.iloc[0,1] != 1 or data.iloc[data.shape[0] - 1, 1] != 0:
        return False
    return True

# %%
nclairalgo = ["FirstFit", "BestFit", "WorstFit", "NextFit", "RRNextFit", "MRU", "LRU"]
bestfit = ["BestFit_L1", "BestFit_L2", "BestFit_Linf"]
worstfit = ["WorstFit_L1", "WorstFit_L2", "WorstFit_Linf"]
LA = ["LA_binary", "LA_logsecond"]
HA = ["HA", "Reduced_HA", "HA_DirectSum","Reduced_HA_DirectSum"]
RCP =["RCP", "RCP_NOLARGE", "PPE", "PPE_NOLARGE"]
CRT = ["CRT", "BestCRT"]
clairalgo = ["Classifybydeparture", "Classifybyduration","Reduced_HA", "LA", "Greedy", "BestCRT"]
learningalgo = ["RCP", "LA",  "Greedy", "BestCRT"]
lower = ["lowerbound"]
CBDU = ["Classifybyduration_p_" + str(p) for p in [1.25, 1.5, 2, 3, 4, 5, 8]]
CBDE = ["Classifybydeparture_p_" + str(p) for p in [0.0625, 0.125, 0.25, 0.5, 1, 2, 4, 8]]

# %%
## Read CBD two.

activeTimeCB = {}
need = []
for algo in CBDE + CBDU:
    if activeTimeCB.get(algo) != None:
        continue
    
    activeTimeCB[algo] = [i for i in validIds]
    need.append(algo)

def analyse(algo, machineId, idx):
    try:
        data = pd.read_csv("generated/Results/" + algo + "_pId_" + str(machineId) + "_nbin.csv")   
        if not valid(data):
            print(machineId, algo, "Validate Failed")
            return
    except:
        print(machineId, algo, "Got Problem")
        return 
    # print(algo)
    activeTimeCB[algo][idx] = calculate_active_time(data)
    del data

max_concurrent_processes = 80

idx = 0
futures = []
with concurrent.futures.ThreadPoolExecutor(max_workers=max_concurrent_processes) as executor:
    for machineId in validIds:
        for algo in need:
            futures.append(executor.submit(analyse(algo, machineId, idx)))        
        idx += 1
    for future in concurrent.futures.as_completed(futures):
    # You can handle the result or exceptions here if needed
        future.result()

# %%
## Read No Error

nclairalgo = ["FirstFit", "BestFit", "WorstFit", "NextFit", "RRNextFit", "MRU", "LRU"]
algorun = bestfit + worstfit + ["FirstFit", "NextFit", "RRNextFit", "MRU", "LRU"]
algorun += LA + HA + RCP + CRT + ["Classifybydeparture", "Classifybyduration", "Greedy"] + lower
activeTimenormal = {}

for algo in algorun:
    activeTimenormal[algo] = [0 for i in validIds]


def read_input(algo, idx, machineId):
    try:
        data = pd.read_csv("generated/Results/" + algo + "_pId_" + str(machineId) + "_nbin.csv")   
        if not valid(data):
            print(machineId, algo, "Validate Failed")
            return
    except Exception as exp:
        print(machineId, algo, "Got Problem", exp)
        return
    activeTimenormal[algo][idx] = calculate_active_time(data)
    del data
    
max_concurrent_processes = 80

with concurrent.futures.ThreadPoolExecutor(max_workers=max_concurrent_processes) as executor:
    idx = 0
    futures = []
    for machineId in validIds:
        futures += [executor.submit(read_input, algo, idx, machineId) for algo in algorun] 
        idx += 1
    for future in concurrent.futures.as_completed(futures):
        # You can handle the result or exceptions here if needed
        future.result()


# %%
error_log = [0.5, 1, 1.5, 2, 2.5, 3, 4, 5, 7.5, 10, 12.5, 15]

# %%
## Log Normal error 
algorun_error = ["LA_binary", "LA_logsecond", 'Classifybydeparture', 'Classifybyduration', 'HA', 'Reduced_HA', 'HA_DirectSum', 'Reduced_HA_DirectSum', 'CRT',  'Greedy', 'BestCRT', 'RCP', 'RCP_NOLARGE', 'PPE', 'PPE_NOLARGE']

activeTimeerror = {}
for e in error_log:
    activeTimeerror[e] = {}
    for algo in algorun_error:
        activeTimeerror[e][algo] = [0 for i in validIds]


def read_input(e, algo, idx, machineId):
    try:
        data = pd.read_csv("generated/Results_e_log" + str(e) + "/" + algo + "_pId_" + str(machineId) + "_nbin.csv")   
        if not valid(data):
            print(machineId, algo, e, "Validate Failed")
            return
    except Exception as exp:
        print(machineId, algo, e, "Got Problem", exp)
        return
    activeTimeerror[e][algo][idx] = calculate_active_time(data)
    del data
    
max_concurrent_processes = 80

with concurrent.futures.ThreadPoolExecutor(max_workers=max_concurrent_processes) as executor:
    idx = 0
    futures = []
    for machineId in validIds:
        futures += [executor.submit(read_input, e, algo, idx, machineId) for e in error_log for algo in algorun_error] 
        idx += 1
    for future in concurrent.futures.as_completed(futures):
        # You can handle the result or exceptions here if needed
        future.result()

# %% [markdown]
# ## Compile all result to a single csv

# %%
activeTime = pd.DataFrame(activeTimenormal)
activeTime['error'] = [0 for i in validIds]
activeTime['pId'] = validIds 

# %%
for e in error_log:
    temp = pd.DataFrame(activeTimeerror[e])
    temp['error'] = [e for i in validIds]
    temp['pId'] = validIds 
    activeTime = pd.concat([activeTime, temp]).reset_index(drop = True)
activeTime

# %%
activeTimetemp = activeTime.copy()
activeTimetemp['pId'] = 1
for column in activeTimetemp.columns:
    if column == 'pId':
        continue
    if column == 'error':
        continue
    activeTimetemp[column] = activeTimetemp[column] - [1,1]
activeTime = pd.concat([activeTime, activeTimetemp]).reset_index(drop = True)

# %%
activeTimeCB_df = pd.DataFrame(activeTimeCB)
lowerbound = activeTime[(activeTime.error == 0) & (activeTime.pId != 18)]['lowerbound']

# %% [markdown]
# # Analyse Performance Ratio (Box Plot)

# %%
lower_base = activeTime[(activeTime.error == 0) & (activeTime.pId != 18)].lowerbound

# %%
plt.rcParams.update({'font.size': 12, "text.usetex": False, "font.family": "Helvetica"})
meanprops = {
    "marker": "s",       # Shape of the mean marker
    "markerfacecolor": "red",  # Color of the mean marker
    "markeredgecolor": "black",
    "markersize": 10      # Size of the mean marker
}

# %%
plot_df = activeTime[(activeTime.error == 0) & (activeTime.pId != 18)][bestfit]

plot_df = plot_df.divide(lower_base, axis = 0)
plot_df.plot(kind='box',figsize = (7, 5), widths = 0.6, showmeans = True, meanprops = meanprops)# , meanline = True)
labels = plot_df.columns
labels = ["BestFit L1", "BestFit L2", "BestFit " + r"$\\ell_\\infty$"]
labels = ['\n'.join(wrap(l, 12)) for l in labels]
plt.ylabel("Performance Ratio")
plt.xticks([i for i in range(1, 1 + len(labels))],labels)
plt.savefig("analyzer/bestfitbox.png", bbox_inches='tight')
plt.show()
plt.clf()

# %%
plot_df = activeTime[(activeTime.error == 0) & (activeTime.pId != 18)][worstfit]
plot_df = plot_df.divide(lower_base, axis = 0)

plot_df.plot(kind='box',figsize = (7, 5), title='BoxPlot of the distribution of Worst Fit Algorithms', widths = 0.6, showmeans = True, meanprops=meanprops)
labels = ["WorstFit L1", "WorstFit L2", "WorstFit Linf (Max)", "WorstFit Linf (Min)"]
labels = ['\n'.join(wrap(l, 14)) for l in labels]
plt.xlabel("Algorithm (algo)") 
plt.ylabel("Ratio of algo/lowerbound")
plt.xticks([i for i in range(1, 1 + len(labels))],labels)

plt.savefig("analyzer/worstfitbox.png", bbox_inches='tight')
plt.show()
plt.clf()

# %%
activeTime["BestFit"] = activeTime["BestFit_Linf"]
activeTime["WorstFit"] = activeTime["WorstFit_Linf"]
nclair = activeTime[activeTime.pId != 18][nclairalgo]
nclair = nclair.divide(activeTime['lowerbound'], axis = 0)
nclair = nclair.rename(columns={"RRNextFit": "Round Robin NextFit"})

nclair.plot(kind='box',figsize = (12, 5), widths = 0.6, showmeans = True, meanprops=meanprops)
labels = nclair.columns

labels = ['\n'.join(wrap(l, 12)) for l in labels]
plt.title('BoxPlot of the distribution of algorithm in non-clarivoyant settings')
plt.xlabel("Algorithm (algo)") 
plt.ylabel("Ratio of algo/lowerbound")
plt.xticks([i for i in range(1, 1 + len(labels))],labels)
plt.ylim(bottom = 1, top = 2)
plt.savefig("analyzer/nclairboxlim.png", bbox_inches='tight')
plt.show()
plt.clf()

# %%

duration = activeTime[(activeTime.error == 0) & (activeTime.pId != 18)]
#print(non_clair)
CBDU2 = [ 'Classifybyduration_p_1.25',
 'Classifybyduration_p_1.5',
 'Classifybyduration_p_1.75',
 'Classifybyduration_p_2',
 'Classifybyduration_p_3',
 'Classifybyduration_p_4',
 'Classifybyduration_p_5',
 'Classifybyduration_p_8'
]
duration = duration[CBDU2]
duration = duration.divide(lowerbound, axis = 0)
duration.plot(kind='box',figsize = (12, 5), title='BoxPlot of the distribution of Classify By Duration', showmeans=True, meanprops=meanprops)
labels = [name.split("_")[-1] for name in CBDU2]
labels = ['\n'.join(wrap(l, 12)) for l in labels]
plt.xlabel("max/min ratio for each category (days)") 
plt.ylabel("Ratio of algo/lowerbound")
plt.xticks([i for i in range(1, 1 + len(labels))],labels)
plt.ylim((1.05,1.2))
plt.savefig("analyzer/Durationbox.png", bbox_inches='tight')

plt.show()
plt.clf()


# %%
depart = activeTime[(activeTime.error == 0) & (activeTime.pId != 18)]
CBDE2 = ['Classifybydeparture_p_0.0625',
 'Classifybydeparture_p_0.125',
 'Classifybydeparture_p_0.25',
 'Classifybydeparture_p_0.5',
  'Classifybydeparture_p_1',
 'Classifybydeparture_p_2',
 'Classifybydeparture_p_4',
 'Classifybydeparture_p_8']
#print(non_clair)
depart = depart[CBDE2]
depart = depart.divide(lowerbound, axis = 0)
depart.plot(kind='box',figsize = (12, 5), title='BoxPlot of the distribution of Classify By Departure Time', showmeans=True, meanprops=meanprops)
labels = [name.split("_")[-1] for name in CBDE2]
labels = ['\n'.join(wrap(l, 12)) for l in labels]
print(labels)
plt.xlabel("Width of Departure Time (Day)") 
plt.ylabel("Ratio of algo/lowerbound")
plt.xticks([i for i in range(1, 1 + len(labels))],labels)
plt.ylim((1,1.2))
plt.savefig("analyzer/Departurebox.png", bbox_inches='tight')
plt.show()
plt.clf()


# %%
clair = activeTime[(activeTime.error == 0) & (activeTime.pId != 18)][["CRT", "BestCRT"]]
clair = clair.divide(lower_base, axis = 0)
clair.rename(columns={
    "CRT" : "Standard NRT",
    "ModifiedCRT" : "Fixed deactivating time",
    "BestCRT" : "Prioritized NRT"
}, inplace = True)
clair.plot(kind='box',figsize = (8, 5), title='BoxPlot of the distribution of Nearest Remaining Time Algorithms', widths=0.6, showmeans=True, meanprops=meanprops)
labels = clair.columns
labels = ['\n'.join(wrap(l, 24)) for l in labels]
plt.xlabel("Algorithm (algo)") 
plt.ylabel("Ratio of algo/lowerbound")
plt.xticks([i for i in range(1, 1 + len(labels))],labels)
plt.ylim(bottom = 1, top = 1.1)
# plt.savefig("analyzer/CRTbox.png", bbox_inches='tight')
plt.show()
plt.clf()

# %%
clair = activeTime[(activeTime.error == 0) & (activeTime.pId != 18)][HA]
#print(non_clair)
clair = clair.divide(lower_base, axis = 0)
print(clair)
clair.plot(kind='box',figsize = (10, 5), title='BoxPlot of the distribution of Hybrid Algorithms', showmeans = True, meanprops=meanprops)
labels = clair.columns
labels = ['\n'.join(wrap(l, 12)) for l in labels]
plt.xlabel("Algorithm (algo)") 
plt.ylabel("Ratio of algo/lowerbound")
plt.xticks([i for i in range(1, 1 + len(labels))],labels)

# plt.savefig("analyzer/HAbox.png", bbox_inches='tight')
plt.ylim(bottom = 1.05, top = 1.25)
plt.show()
plt.clf()

# %%
clair = activeTime[(activeTime.pId != 18) & (activeTime.error == 0)][[
    'Classifybydeparture',
    'Classifybyduration',
    'Reduced_HA',
    'Greedy',
    'BestCRT']]


clair = clair.divide(lower_base, axis = 0)
clair = clair.rename( columns = {
    "BestCRT":"Prioritized NRT",
    "Reduced_HA":  "Reduced Hybrid"
})

clair.plot(kind='box',figsize = (10, 5), showmeans = True, meanprops=meanprops)
labels = clair.columns
labels = ['\n'.join(wrap(l, 12)) for l in labels]
plt.title('BoxPlot of the distribution of algorithm in online clarivoyant settings')
plt.xlabel("Algorithm (algo)") 
plt.ylabel("Ratio of algo/lowerbound")
plt.xticks([i for i in range(1, 1 + len(labels))],labels)
plt.ylim(bottom = 1, top = 1.2)
plt.savefig("analyzer/clairboxelim.png", bbox_inches='tight')
plt.show()
plt.clf()

# %% [markdown]
# ## Learning Augmented Settings

# %%
algorithm_colors = {
    "First Fit": "black",
    "RCP": "blue",
    "RCP_NOLARGE": "dodgerblue",
    "PPE": "green",
    "PPE_NOLARGE": "darkolivegreen",
    "LA_binary": "gold",
    "LA_logsecond": "brown",
    "Classifybyduration": "red",
    "Classifybydeparture": "darkgreen",
    "Greedy": "purple",
    "BestCRT": "magenta",
    "Reduced_HA": "teal",
}

present_name = {
    "FirstFit": "FirstFit",
    "RCP": "RCP",
    "RCP_NOLARGE": "RCP Without Large Bin",
    "PPE": "PPE",
    "PPE_NOLARGE": "PPE Without Large Bin",
    "LA_binary": "LA (Binary Classification)",
    "LA_logsecond": "LA (Geometric Ranges)",
    "Classifybyduration": "Classify By Duration",
    "Classifybydeparture": "Classify By Departure Time",
    "Greedy": "Greedy",
    "BestCRT": "Prioritized NRT",
    "Reduced_HA": "Reduced Hybrid",
}

# %%
imptalgo = ["Classifybyduration", "Classifybydeparture", "Greedy", "BestCRT", "Reduced_HA",
            "LA_binary", "LA_logsecond", "RCP", "RCP_NOLARGE", "PPE", "PPE_NOLARGE"]

mean_df_s0 = {}
for e in [0] + error_log:
    temp = activeTime[(activeTime.error == e) & (activeTime.pId != 18)][imptalgo].reset_index(drop = True)
    temp = temp.divide(lower_base, axis = 0)
    mean_df_s0[e] = temp.mean()
mean_df_s0 = pd.DataFrame(mean_df_s0).T.reset_index().rename(columns={'index':'error'})
firstfitmean = (activeTime[(activeTime.error == 0) & (activeTime.pId != 18)]['FirstFit'].reset_index(drop=True)/lower_base).mean()

# %%
plt.figure(figsize=(10,5))
# plt.xscale('log', basex = 10)
plot_df = mean_df_s0[["LA_binary", "LA_logsecond", 'error']]#.drop(["Classifybydeparture"], axis = 1)[RCP +['error']]
for k in plot_df.columns:
    if k == 'error':
        continue
    plt.plot(plot_df['error'], plot_df[k], 'x-', label = present_name[k], color = algorithm_colors[k])
plt.plot(plot_df['error'], [firstfitmean for _ in plot_df['error']], '--', label='FirstFit')
plt.title("Results of Clairvoyant Algorithm across multiplicative error (Seed = 0)")
plt.legend()
plt.xlabel("Maximum Multiplicative Error")
plt.ylabel("Mean Ratio")
# plt.savefig("analyzer/errorplot.png")

plt.show()


# %%
plt.figure(figsize=(10,5))
# plt.xscale('log', basex = 10)
plot_df = mean_df_s0[["RCP", "RCP_NOLARGE", "PPE", "PPE_NOLARGE",'error']]#.drop(["Classifybydeparture"], axis = 1)[RCP +['error']]
for k in plot_df.columns:
    if k == 'error':
        continue
    plt.plot(plot_df['error'], plot_df[k], 'x-', label = present_name[k], color = algorithm_colors[k])
plt.plot(plot_df['error'], [firstfitmean for _ in plot_df['error']], '--', label='FirstFit')
plt.title("Results of Clairvoyant Algorithm across multiplicative error (Seed = 40)")
plt.legend()
plt.xlabel("Maximum Multiplicative Error")
plt.ylabel("Mean Ratio")
# plt.savefig("analyzer/errorplot.png")

plt.show()


# %%
plt.figure(figsize=(10,5))
# plt.xscale('log', basex = 10)
plot_df = mean_df_s0[["Classifybydeparture","Classifybyduration",'error']]#.drop(["Classifybydeparture"], axis = 1)[RCP +['error']]
for k in plot_df.columns:
    if k == 'error':
        continue
    plt.plot(plot_df['error'], plot_df[k], 'x-', label = present_name[k], color = algorithm_colors[k])
plt.plot(plot_df['error'], [firstfitmean for _ in plot_df['error']], '--', label='FirstFit')
plt.title("Results of Clairvoyant Algorithm across multiplicative error (Seed = 40)")
plt.legend()
plt.xlabel("Maximum Multiplicative Error")
plt.ylabel("Mean Ratio")
# plt.savefig("analyzer/errorplot.png")

plt.show()


# %%
plt.figure(figsize=(10,5))
# plt.xscale('log', basex = 10)
plot_df = mean_df_s0[["PPE_NOLARGE", "Greedy", "BestCRT", "LA_binary","LA_logsecond", 'Reduced_HA', 'error']]#.drop(["Classifybydeparture"], axis = 1)[RCP +['error']]
for k in plot_df.columns:
    if k == 'error':
        continue
    plt.plot(plot_df['error'], plot_df[k], 'x-', label = present_name[k], color = algorithm_colors[k])
plt.plot(plot_df['error'], [firstfitmean for _ in plot_df['error']], '--', label='FirstFit')
plt.title("Results of Clairvoyant Algorithm across multiplicative error (Seed = 40)")
plt.legend()
plt.xlabel("Maximum Multiplicative Error")
plt.ylabel("Mean Ratio")
# plt.savefig("analyzer/errorplot.png")

plt.show()



