import csv
import math

modeColIndex = 0
widthColIndex = 1
rebootColIndex = 2
iterColIndex = 3
constraintsColIndex = 4
failsColIndex = 7
timeColIndex = 14

#modeColIndex = 8
#widthColIndex = 14
#rebootColIndex = 13
#iterColIndex = 4
#constraintsColIndex = 1
#failsColIndex = 9
#timeColIndex = 15



timeDict = {}
failDict = {}

with open("/tmp/amongNurse.csv") as csvfile:
  reader = csv.reader(csvfile)
  next(reader)
  for row in reader:
    mode = row[modeColIndex]
    width = int(row[widthColIndex])
    widthIndex = 0
    if width > 0:
      widthIndex = int(math.log(width,2))
    reboot = row[rebootColIndex]
    if reboot == '1000000':
      reboot = 'MAX'
    iter = row[iterColIndex]
    if iter == '1000000':
      iter = 'MAX'
    elif iter == '0':
      iter = '1'
    constraints = row[constraintsColIndex]
    fails = row[failsColIndex]
    time = row[timeColIndex]

    key = 'M' + mode + 'R' + reboot + 'I' + iter + 'C' + constraints

    if key in timeDict:
      failDict[key][widthIndex] = fails
      timeDict[key][widthIndex] = time
    else:
      failArray = [0,0,0,0,0,0,0]
      timeArray = [0,0,0,0,0,0,0]
      failArray[widthIndex] = fails
      timeArray[widthIndex] = time
      failDict[key] = failArray
      timeDict[key] = timeArray

for key in timeDict:
  timePrintArray = '[' + str(timeDict[key][0])
  for time in timeDict[key][1:]:
    timePrintArray += ',' + str(time)
  timePrintArray += ']'
  failPrintArray = '[' + str(failDict[key][0])
  for fail in failDict[key][1:]:
    failPrintArray += ',' + str(fail)
  failPrintArray += ']'
  print(key + 'Time = ' + timePrintArray)
  print(key + 'Fail = ' + failPrintArray)
