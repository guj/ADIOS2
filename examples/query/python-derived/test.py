import adios2
import matplotlib.pyplot as plt
import numpy as np
import sys
from matplotlib.colors import LogNorm
#import mplcyberpunk


#plt.style.use("cyberpunk")

a2 = adios2.ADIOS()

#bpDir="/Users/junmin/software/W/test/BP-v/"
bpDir="./"
vFile = bpDir+"openpmd.bp"
print (vFile)

rank = 0

queryIO = a2.DeclareIO("query");
reader = queryIO.Open(vFile, adios2.Mode.Read)

queryFile="query.xml"

if (len(sys.argv) > 1):
    queryFile = sys.argv[1]

print("Using query file: ", queryFile);

varNames = {
        "Ux":"/data/particles/electrons/momentum/x/__data__",
        "Uy":"/data/particles/electrons/momentum/y/__data__",
        "Uz":"/data/particles/electrons/momentum/z/__data__",
        "w" :"/data/particles/electrons/weighting/__data__",
        "x" :"/data/particles/electrons/position/x/__data__",
        "y" :"/data/particles/electrons/position/y/__data__",
        "z" :"/data/particles/electrons/position/z/__data__"
}


varLabels = {
        "/data/particles/electrons/momentum/x/__data__":"Ux",
        "/data/particles/electrons/momentum/y/__data__":"Uy",
        "/data/particles/electrons/momentum/z/__data__":"Uz",
        "/data/particles/electrons/weighting/__data__":"w",
        "/data/particles/electrons/position/x/__data__":"x",
        "/data/particles/electrons/position/y/__data__":"y",
        "/data/particles/electrons/position/z/__data__":"z"
}


def runQuery(queryFile, varX, varY):
    w = adios2.Query(queryFile, reader);
    
    touched_blocks = []
    
    var = [ queryIO.InquireVariable(varNames[varX]),
            queryIO.InquireVariable(varNames[varY]),
            queryIO.InquireVariable(varNames["w"]) ]
    
    print ("Num steps: ", reader.Steps())
    stepCounter = 0;
    while (reader.BeginStep() == adios2.StepStatus.OK):
        # say only rank 0 wants to process result
        if (rank == 0):
            touched_blocks = w.GetResult();
            print ("Step: ", stepCounter, "  num blocks: ", len(touched_blocks))
            #doAnalysis(reader, touched_blocks, var, stepCounter);

        #plt.savefig('hist2d_'+varX+varY+str(stepCounter)+'.png')
        #plt.clf();

        reader.EndStep();
        stepCounter += 1
    reader.Close();




runQuery(queryFile, "z", "x");
