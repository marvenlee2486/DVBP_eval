# Step 1: Compile the C++ Code
```
mkdir build
cd build
cmake ../CMakeLists.txt
cmake --build .
cd ..
```

# Step 2: Execute the .ipynb code or convert it to .py code with the dependency in requirements.txt
## Download the dataset
```
wget https://azurepublicdatasettraces.blob.core.windows.net/azurepublicdatasetv2/azurevmallocation_dataset2020/AzurePackingTraceV1.zip 
unzip AzurePackingTraceV1.zip -d data
cd data
sqlite3 -header -csv packing_trace_zone_a_v1.sqlite "select * from vm;" > vm.csv
sqlite3 -header -csv packing_trace_zone_a_v1.sqlite "select * from vmType;" > vmType.csv
```
'vm.csv' and 'vmType.csv' should be found under data/ directory.

## Install the environment
```
python3 -m venv venv
source venv/bin/activate
pip install -r requirements.txt
```

## Run the python code
```
python3 run.py
```
The generated csv can be found at directory generated/

# Step 3: Analyze the csv file
```
python3 analyzer.py
```
The figures can be found at analyzer/
