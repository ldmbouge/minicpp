#!/usr/local/bin/python3
# Filename bench.py
import json
from subprocess import Popen, PIPE
import os
import sys
import csv

def readRecordFromLineArray(al):
    keep = ""
    cat = False
    for l in al:
        if l.startswith("{ \"JSON\""):
            cat = True
        if cat:
            keep = keep + l
    return json.loads(keep)

class Runner:
    def __init__(self,bin):
        self.home = os.environ['HOME']
        self.path = os.environ['PWD']
        self.bin = bin
        self.pwd = os.getcwd()

    def run(self,size,width,model):
        os.chdir(self.path)
        full = './' + self.bin
        flags = (full,'-n{0}'.format(size),'-w{0}'.format(width),'-m{0}'.format(model))
        h = Popen(flags,stdout=PIPE,stderr=PIPE)
        allLines = h.communicate()[0].strip().decode('ascii').splitlines()
        rec = readRecordFromLineArray(allLines)
        return rec
            

name = "allInterval"
r = Runner('build/'+name)

ar = []

for s in range(8,12):                # This is the instance size (-n<x>)
    for m in range(0,4):             # This is the models to cover (-m<x>)
        for i in range(0,7):        # This is the width to consider 2^0 .. 2^k (-w<x>)
            w = 2**i
            rec = r.run(s,w,m)
            rec = rec['JSON'][name]
            rec['time'] = rec['time'] / 1000.0
            ar.append(rec)
            print(rec)


jsonObject = json.dumps(ar,indent=4)
with open("/tmp/saved.json","w") as outfile:
    outfile.write(jsonObject)

with open("/tmp/saved.csv","w") as f:
    w = csv.writer(f,delimiter=",")
    w.writerow(ar[0])
    for row in ar:
        w.writerow([row[k] for k in row])
