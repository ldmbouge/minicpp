#!/usr/local/bin/python3
# Filename bench.py

import json
from subprocess import Popen, PIPE
import os
import sys
import json
import calendar
import time
import datetime
import platform, re
import csv

def readRecord(fName):
    with open(fName) as f:
        al = f.readlines()
        keep = ""
        cat = False
        for l in al:
            if l.startswith("{ \"JSON\""):
                cat = True
            if cat:
                keep = keep + l
        return json.loads(keep)


def readRecordFromLineArray(al):
    keep = ""
    cat = False
    for l in al:
        if l.startswith("{ \"JSON\""):
            cat = True
        if cat:
            keep = keep + l
    return json.loads(keep)

# rec = readRecord('test.json')
    
# run = rec['JSON']['amongNurse']
# for fields in run:
#     print(fields)
#     print(run[fields])
    

class Runner:
    def __init__(self,bin):
        self.home = os.environ['HOME']
        self.path = os.environ['PWD']
        self.bin = bin
        self.pwd = os.getcwd()
        if os.path.exists('results.json'):
            self.all = open('results.json','r')
            self.ab  = json.loads(self.all.read())
            self.all.close()
        else:
            self.ab = {}
            print("CWD is:" + self.pwd)
            os.environ['DYLD_FRAMEWORK_PATH'] = self.path

    def run(self,width,model):
        os.chdir(self.path)
        full = './' + self.bin
        flags = (full,'-w{0}'.format(width),'-m{0}'.format(model))
        #print(flags)
        h = Popen(flags,stdout=PIPE,stderr=PIPE)
        allLines = h.communicate()[0].strip().decode('ascii').splitlines()
        #rc = h.wait()
        #print('Return code {0}'.format(rc))
        #print("BACK")
        rec = {}
        fulltext = []
        for line in allLines:
            fulltext.append(line)
        rec = readRecordFromLineArray(fulltext)
        return rec
            

nbRun = 2
#r = Runner('build/among4Relax')
r = Runner('build/amongNurse')

for m in range(1,4):
    for i in range(0,6):
        w = 2**i
        rec = r.run(w,m)
        rec = rec['JSON']['amongNurse']
        rec['time'] = rec['time'] / 1000.0
        print(rec)
