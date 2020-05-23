#!/usr/local/bin/python3
# Filename bench.py
import json
from subprocess import Popen, PIPE
import os
import sys
import csv
from collections import namedtuple

Test = namedtuple('Test', ('name', 'constraints', 'parameters', 'nonMDDModels'))
Constraint = namedtuple("Constraint", "checkFunction parameters")
PartialConstraint = namedtuple("PartialConstraint", "checkFunction parameters subsetIndices")
AllDiffParameters = namedtuple("AllDiffParameters", "")
SumParameters = namedtuple("SumParameters", "weights lower upper")
SequenceParameters = namedtuple("SequenceParameters", "length lower upper values")
AmongParameters = namedtuple("AmongParameters", "lower upper values")
EqualsAbsDiffParameters = namedtuple("EqualsDiffParameters", "")

numRuns = 1


def checkAllDiff(solution,parameters):
  for i in range(len(solution)-1):
    for j in range(i+1,len(solution)):
      if solution[i] == solution[j]:
        return False
  return True

def checkSum(solution,parameters):
  weights = parameters.weights
  lower = parameters.lower
  upper = parameters.upper
  sum = 0
  for i in range(len(solution)):
    sum += solution[i] * weights[i]
  return sum >= lower and sum <= upper

def checkSequence(solution,parameters):
  length = parameters.length
  lower = parameters.lower
  upper = parameters.upper
  values = parameters.values
  for firstIndex in range(0,len(solution)-length):
    count = 0
    subsolution = solution[firstIndex:firstIndex+length]
    for value in values:
      count += subsolution.count(value)
    if count < lower or count > upper:
      return False
  return True

def checkAmong(solution,parameters):
  lower = parameters.lower
  upper = parameters.upper
  values = parameters.values
  count = 0
  for value in values:
    count += solution.count(value)
  return count >= lower and count <= upper

def checkEqualsAbsDiff(solution, parameters):
  left = solution[0]
  rightA = solution[1]
  rightB = solution[2]
  return left == abs(rightA - rightB)

amongNurseConstraints = [Constraint(checkSequence, SequenceParameters(length=8, lower=0, upper=6, values=[1])),
                         Constraint(checkSequence, SequenceParameters(length=30, lower=22, upper=30, values=[1])),
                         PartialConstraint(checkAmong, AmongParameters(lower = 4, upper = 5, values=[1]), [0,1,2,3,4,5,6]),
                         PartialConstraint(checkAmong, AmongParameters(lower = 4, upper = 5, values=[1]), [7,8,9,10,11,12,13]),
                         PartialConstraint(checkAmong, AmongParameters(lower = 4, upper = 5, values=[1]), [14,15,16,17,18,19,20]),
                         PartialConstraint(checkAmong, AmongParameters(lower = 4, upper = 5, values=[1]), [21,22,23,24,25,26,27]),
                         PartialConstraint(checkAmong, AmongParameters(lower = 4, upper = 5, values=[1]), [28,29,30,31,32,33,34])]
allInterval4Constraints = [PartialConstraint(checkAllDiff, AllDiffParameters(), [0,1,2,3]),
                          PartialConstraint(checkAllDiff, AllDiffParameters(), [4,5,6]),
                          PartialConstraint(checkEqualsAbsDiff, EqualsAbsDiffParameters(), [4,0,1]),
                          PartialConstraint(checkEqualsAbsDiff, EqualsAbsDiffParameters(), [5,1,2]),
                          PartialConstraint(checkEqualsAbsDiff, EqualsAbsDiffParameters(), [6,2,3])]
allInterval8Constraints = [PartialConstraint(checkAllDiff, AllDiffParameters(), [0,1,2,3,4,5,6,7]),
                          PartialConstraint(checkAllDiff, AllDiffParameters(), [8,9,10,11,12,13,14]),
                          PartialConstraint(checkEqualsAbsDiff, EqualsAbsDiffParameters(), [8,0,1]),
                          PartialConstraint(checkEqualsAbsDiff, EqualsAbsDiffParameters(), [9,1,2]),
                          PartialConstraint(checkEqualsAbsDiff, EqualsAbsDiffParameters(), [10,2,3]),
                          PartialConstraint(checkEqualsAbsDiff, EqualsAbsDiffParameters(), [11,3,4]),
                          PartialConstraint(checkEqualsAbsDiff, EqualsAbsDiffParameters(), [12,4,5]),
                          PartialConstraint(checkEqualsAbsDiff, EqualsAbsDiffParameters(), [13,5,6]),
                          PartialConstraint(checkEqualsAbsDiff, EqualsAbsDiffParameters(), [14,6,7])]
allInterval12Constraints = [PartialConstraint(checkAllDiff, AllDiffParameters(), [0,1,2,3,4,5,6,7,8,9,10,11]),
                          PartialConstraint(checkAllDiff, AllDiffParameters(), [12,13,14,15,16,17,18,19,20,21,22]),
                          PartialConstraint(checkEqualsAbsDiff, EqualsAbsDiffParameters(), [12,0,1]),
                          PartialConstraint(checkEqualsAbsDiff, EqualsAbsDiffParameters(), [13,1,2]),
                          PartialConstraint(checkEqualsAbsDiff, EqualsAbsDiffParameters(), [14,2,3]),
                          PartialConstraint(checkEqualsAbsDiff, EqualsAbsDiffParameters(), [15,3,4]),
                          PartialConstraint(checkEqualsAbsDiff, EqualsAbsDiffParameters(), [16,4,5]),
                          PartialConstraint(checkEqualsAbsDiff, EqualsAbsDiffParameters(), [17,5,6]),
                          PartialConstraint(checkEqualsAbsDiff, EqualsAbsDiffParameters(), [18,6,7]),
                          PartialConstraint(checkEqualsAbsDiff, EqualsAbsDiffParameters(), [19,7,8]),
                          PartialConstraint(checkEqualsAbsDiff, EqualsAbsDiffParameters(), [20,8,9]),
                          PartialConstraint(checkEqualsAbsDiff, EqualsAbsDiffParameters(), [21,9,10]),
                          PartialConstraint(checkEqualsAbsDiff, EqualsAbsDiffParameters(), [22,10,11])]
sequenceNurseConstraints = [Constraint(checkSequence, SequenceParameters(length=14, lower=4, upper=14, values=[0])),
                            Constraint(checkSequence, SequenceParameters(length=28, lower=20, upper=28, values=[1,2,3])),
                            Constraint(checkSequence, SequenceParameters(length=14, lower=1, upper=4, values=[3])),
                            Constraint(checkSequence, SequenceParameters(length=14, lower=4, upper=8, values=[2])),
                            Constraint(checkSequence, SequenceParameters(length=2, lower=0, upper=1, values=[3])),
                            Constraint(checkSequence, SequenceParameters(length=7, lower=2, upper=4, values=[2,3])),
                            Constraint(checkSequence, SequenceParameters(length=7, lower=0, upper=6, values=[1,2,3]))]
workForceConstraints = []
file = open("build/data/workForce100-jobs.csv", "r")
file.readline()
startTimes = []
endTimes = []
for line in file:
  values = line.split(',')
  startTimes.append(values[1])
  endTimes.append(values[2])
pt = []
for j in range(len(startTimes)):
  pt.append([int(startTimes[j]),True,j])
  pt.append([int(endTimes[j]),False,j])
pt.sort(key = lambda x: (x[0], not x[1]))
clique = []
added = False
for p in pt:
  if p[1]:
    clique.append(p[2])
  else:
    if added:
      workForceConstraints.append(PartialConstraint(checkAllDiff, AllDiffParameters(), clique))
    clique.remove(p[2])
  added = p[1]

tests = [
         Test("amongNurse", amongNurseConstraints, [('w', [1,2,4,8]), ('m', range(6)), ('r', [0,1,2,4,8]), ('i', [1,5,10,20])], [0,4]),
         #Test("allInterval", allInterval4Constraints, [('n', [4]), ('w', [1,2,4,8]), ('m', range(4)), ('r', [0,1,2,4])], [0,1]),
         #Test("allInterval", allInterval8Constraints, [('n', [8]), ('w', [1,2,4,8]), ('m', range(4)), ('r', [0,1,2,4,8])], [0,1]),
         #Test("allInterval", allInterval12Constraints, [('n', [12]), ('w', [1,2,4,8]), ('m', range(4)), ('r', [0,1,2,4,8,12])], [0,1])],
         #Test("workForce", workForceConstraints, [('w', [1,2,4,8]), ('o', [60])], []),
        ]


def checkConstraints(solution, constraints):
  for constraint in constraints:
    if isinstance(constraint, PartialConstraint):
      if not constraint.checkFunction([solution[i] for i in constraint.subsetIndices],constraint.parameters):
        return False
    else:
      if not constraint.checkFunction(solution,constraint.parameters):
        return False
  return True

def findSolution(resultText):
  indexOfSolIs = resultText.find('Assignment: [')
  solution = resultText[indexOfSolIs+13:]
  endIndex = solution.find(']')
  solution = solution[:endIndex]
  return [int(s) for s in solution.split(',')],resultText[indexOfSolIs+13+endIndex+1:]

#Confirms all found solutions are feasible
def checkAllSolutions(constraints,resultText):
  numFeasibleSolutions = 0
  numInfeasibleSolutions = 0
  while resultText.find('Assignment: [') != -1:
    solution,resultText = findSolution(resultText)
    if checkConstraints(solution,constraints):
      numFeasibleSolutions += 1
    else:
      numInfeasibleSolutions += 1
  return numFeasibleSolutions,numInfeasibleSolutions


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

    def runEachParameter(self,name,width,model,reboot,iter,parameters,constraints,flags):
        solns = []
        if len(parameters) > 0:
          parameter = parameters.pop(0)
          if parameter[0] == 'm':
            solns += self.runEachParameter(name, width, model, reboot, iter, parameters, constraints, flags+('-m{0}'.format(model),))
          elif parameter[0] == 'w':
            solns += self.runEachParameter(name, width, model, reboot, iter, parameters, constraints, flags+('-w{0}'.format(width),))
          elif parameter[0] == 'r':
            solns += self.runEachParameter(name, width, model, reboot, iter, parameters, constraints, flags+('-r{0}'.format(reboot),))
          elif parameter[0] == 'i':
            solns += self.runEachParameter(name, width, model, reboot, iter, parameters, constraints, flags+('-i{0}'.format(iter),))
          else:
            options = parameter[1]
            for option in options:
              solns += self.runEachParameter(name, width, model, reboot, iter, parameters, constraints, flags+('-' + parameter[0] + str(option),))
          parameters.insert(0,parameter)
          return solns
        else:
          h = Popen(flags,stdout=PIPE,stderr=PIPE)
          output = h.communicate()[0].strip().decode('ascii')
          numFeasible, numInfeasible = checkAllSolutions(constraints,output)
          if numInfeasible > 0:
            print("Found " + str(numFeasible) + " infeasible solution" + ('' if numInfeasible == 1 else 's') + " when running " + str(flags))
          allLines = output.splitlines()
          rec = readRecordFromLineArray(allLines)
          rec['JSON'][name]['solns'] = str(numFeasible+numInfeasible)
          solns += [rec]
          return solns
          

    def run(self,name,width,model,reboot,iter,parameters,constraints):
        os.chdir(self.path)
        full = './' + self.bin
        flags = (full,)
        return self.runEachParameter(name,width,model,reboot,iter,parameters,constraints,flags)

allResults = []

for test in tests:
  runner = Runner('build/' + test.name)
  results = []
  parameters = test.parameters
  models = [0]
  for parameter in parameters:
    if parameter[0] == 'm':
      models = parameter[1]
      break
  for model in models:
    if model in test.nonMDDModels:
      miniResults = runner.run(test.name,0,model,0,0,parameters,test.constraints)
      for result in miniResults:
        result = result['JSON'][test.name]
        result['time'] = result['time'] / 1000.0
        results.append(result)
        print(result)
    else:
      widths = []
      for parameter in parameters:
        if parameter[0] == 'w':
          widths = parameter[1]
          break
      reboots = []
      for parameter in parameters:
        if parameter[0] == 'r':
          reboots = parameter[1]
          break
      iters = []
      for parameter in parameters:
        if parameter[0] == 'i':
          iters = parameter[1]
          break
      for width in widths:
        if width == 1:   #If width is 1, reboot distance doesn't matter
          miniResults = runner.run(test.name,width,model,0,0,parameters,test.constraints)
          for result in miniResults:
            result = result['JSON'][test.name]
            result['time'] = result['time'] / 1000.0
            results.append(result)
            print(result)
        else:
          for reboot in reboots:
            for iter in iters:
              miniResults = runner.run(test.name,width,model,reboot,iter,parameters,test.constraints)
              for result in miniResults:
                result = result['JSON'][test.name]
                result['time'] = result['time'] / 1000.0
                results.append(result)
                print(result)
  allResults.append(results)

usedNames = []
for resultsIndex in range(len(allResults)):
  results = allResults[resultsIndex]
  name = tests[resultsIndex].name
  if name in usedNames:
    jsonObject = json.dumps(results,indent=4)
    with open("/tmp/" + name + ".json","a") as outfile:
        outfile.write(jsonObject)
    
    with open("/tmp/" + name + ".csv","a") as f:
        w = csv.writer(f,delimiter=",")
        for row in results:
            w.writerow([row[k] for k in row])
  else:
    usedNames.append(name)
    jsonObject = json.dumps(results,indent=4)
    with open("/tmp/" + name + ".json","w") as outfile:
        outfile.write(jsonObject)
    
    with open("/tmp/" + name + ".csv","w") as f:
        w = csv.writer(f,delimiter=",")
        w.writerow(results[0])
        for row in results:
            w.writerow([row[k] for k in row])
