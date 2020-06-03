import numpy as np
import matplotlib.pyplot as plt

allSoln = 1


allIntervalReboot0Fail = np.array([10062, 9459, 7815, 6196, 4027, 2368, 1511])
allIntervalReboot2Fail = np.array([10062, 9332, 7522, 6191, 3796, 2264, 1470])
allIntervalReboot4Fail = np.array([10062, 9069, 6972, 5279, 3282, 2100, 1357])
allIntervalReboot8Fail = np.array([10062, 5698, 3277, 2283, 1626, 1839, 316])
allIntervalRebootMAXFail = np.array([10062, 1155, 140, 40, 34, 23, 6])
allIntervalReboot0Time = np.array([1.136, 2.905, 4.626, 8.031, 11.093, 16.06, 28.00])
allIntervalReboot2Time = np.array([1.136, 2.906, 5.116, 7.403, 12.25, 17.77, 29.22])
allIntervalReboot4Time = np.array([1.136, 4.170, 6.147, 9.665, 16.25, 27.55, 39.63])
allIntervalReboot8Time = np.array([1.136, 5.995, 11.53, 14.27, 23.70, 29.55, 68.53])
allIntervalRebootMAXTime = np.array([1.136, 14.93, 19.12, 20.16, 26.12, 34.22, 35.87])



plt.figure(figsize=(8,5))
plt.plot([1,2,4,8,16,32,64], allIntervalReboot0Fail+1, marker='o', markersize=6, color='#4477AA', label='Reboot=0')
plt.plot([1,2,4,8,16,32,64], allIntervalReboot2Fail+1, marker='^', markersize=6, color='#228833', label='Reboot=2', linestyle='dashed')
plt.plot([1,2,4,8,16,32,64], allIntervalReboot4Fail+1, marker='s', markersize=6, color='#CCBB44', label='Reboot=4', linestyle='dotted')
plt.plot([1,2,4,8,16,32,64], allIntervalReboot8Fail+1, marker='*', markersize=6, color='#EE6677', label='Reboot=8', linestyle='dashdot')
plt.plot([1,2,4,8,16,32,64], allIntervalRebootMAXFail+1, marker='x', markersize=6, color='#AA3377', label='Reboot=MAX', linestyle=(0,(3,5,1,10)))

plt.xlabel("Width")
plt.ylabel("Backtracks")
plt.yscale("log")
plt.title("Impact of Reboot on Backtracks")
#plt.legend(loc=1)
plt.savefig('allIntervalBTGraph.png')


plt.figure(figsize=(8,5))
plt.plot([1,2,4,8,16,32,64], allIntervalReboot0Time, marker='o', markersize=6, color='#4477AA', label='Reboot=0')
plt.plot([1,2,4,8,16,32,64], allIntervalReboot2Time, marker='^', markersize=6, color='#228833', label='Reboot=2', linestyle='dashed')
plt.plot([1,2,4,8,16,32,64], allIntervalReboot4Time, marker='s', markersize=6, color='#CCBB44', label='Reboot=4', linestyle='dotted')
plt.plot([1,2,4,8,16,32,64], allIntervalReboot8Time, marker='*', markersize=6, color='#EE6677', label='Reboot=8', linestyle='dashdot')
plt.plot([1,2,4,8,16,32,64], allIntervalRebootMAXTime, marker='x', markersize=6, color='#AA3377', label='Reboot=MAX', linestyle=(0,(3,5,1,10)))

plt.xlabel("Width")
plt.ylabel("CPU Time (seconds)")
plt.yscale("log")
plt.title("Impact of Reboot on Time")
plt.legend(loc=2)
plt.savefig('allIntervalTimeGraph.png')




thesisTimeToMyComputerRatio = 2.49360613811
def convertTimes(originalTimeArray):
  return np.array(originalTimeArray) / thesisTimeToMyComputerRatio

dC1 = [np.array([175175,64969,5025,893,46,11,12]),np.array([273187,111769,9887,1621,133,33,28])]
dC2 = [np.array([179743,74331,8747,1577,3,2,2]),np.array([518529,219855,25057,3580,31,28,27])]
dC3 = [np.array([882640,423053,33379,4236,680,55,32]),np.array([4201778,2191133,185755,22488,1835,834,81])]
dC1TimeOriginal = [[], [699.72,501.30,80.71,26.15,5.41,4.72,5.24]]
dC2TimeOriginal = [[], [1312.85,1023.10,230.46,63.89,0.78,1.18,1.78]]
dC3TimeOriginal = [[], [12042.30,8574.52,1228.34,302.61,115.38,94.62,95.88]]
dC1Time = [convertTimes(dC1TimeOriginal[0]), convertTimes(dC1TimeOriginal[1])]
dC2Time = [convertTimes(dC2TimeOriginal[0]), convertTimes(dC2TimeOriginal[1])]
dC3Time = [convertTimes(dC3TimeOriginal[0]), convertTimes(dC3TimeOriginal[1])]
classicC1Time = [0.192,6]
classicC2Time = [1.381,11.546]
classicC3Time = [0.359,51.567]
classicC1Fail = [5784,198091]
classicC2Fail = [47525,393748]
classicC3Fail = [11405,1847335]



dC1H40 = [np.array([]),np.array([230550,77941,17175,6741,106,99,90])]
dC2H40 = [np.array([]),np.array([518489,182106,40279,8933,37,40,35])]
dC3H40 = [np.array([]),np.array([455495,157984,25071,24319,2039,454,74])]
dC1H40TimeOriginal = [[], [212.01,106.28,30.79,17.09,3.11,3.10,3.15]]
dC2H40TimeOriginal = [[], [469.32,247.56,75.36,22.87,0.24,0.32,0.41]]
dC3H40TimeOriginal = [[], [563.99,363.10,199.13,206.70,161.81,159.34,163.14]]
dC1H40Time = [convertTimes(dC1H40TimeOriginal[0]), convertTimes(dC1H40TimeOriginal[1])]
dC2H40Time = [convertTimes(dC2H40TimeOriginal[0]), convertTimes(dC2H40TimeOriginal[1])]
dC3H40Time = [convertTimes(dC3H40TimeOriginal[0]), convertTimes(dC3H40TimeOriginal[1])]
classicC1H40Time = [0,2.936]
classicC2H40Time = [0,5.456]
classicC3H40Time = [0,5.676]
classicC1H40Fail = [0,185287]
classicC2H40Fail = [0,393748]
classicC3H40Fail = [0,328376]



#print("Dedicated C1 (H=40): " + str(dC1H40Time[1]))
#print("Dedicated C2 (H=40): " + str(dC2H40Time[1]))
#print("Dedicated C3 (H=40): " + str(dC3H40Time[1]))
#print("Dedicated C1 (H=80): " + str(dC1Time[1]))
#print("Dedicated C2 (H=80): " + str(dC2Time[1]))
#print("Dedicated C3 (H=80): " + str(dC3Time[1]))




M1R0I1C1Time = [np.array([2.374,1.182,2.633,2.476,0.894,1.032,0.953]),]
M1R0I1C1Fail = [np.array([5784,1193,880,505,71,26,11]),]
M1R0I1C2Time = [np.array([19.5,13.963,36.454,18.613,14.329,9.863,10.215]),]
M1R0I1C2Fail = [np.array([47525,13404,8866,3651,1344,436,198]),]
M1R0I1C3Time = [np.array([4.542,2.26,6.604,5.304,1.975,2.09,3.18]),]
M1R0I1C3Fail = [np.array([11405,2061,2466,953,175,78,52]),]
M1RMAXIMAXC1Time = [np.array([2.374,13.264,10.643,13.888,13.482,24.446,16.448]),]
M1RMAXIMAXC1Fail = [np.array([5784,16,8,0,4,0,0]),]
M1RMAXIMAXC2Time = [np.array([19.5,13.463,21.869,46.959,38.914,76.032,85.766]),]
M1RMAXIMAXC2Fail = [np.array([47525,16,0,0,0,0,0]),]
M1RMAXIMAXC3Time = [np.array([4.542,149.394,54.097,57.045,106.487,158.961,82.75]),]
M1RMAXIMAXC3Fail = [np.array([11405,15,13,9,6,2,5]),]
M2R0I1C1Time = [np.array([0.578,0.253,0.714,0.709,0.528,0.625,0.725]),]
M2R0I1C1Fail = [np.array([5784,1203,912,523,187,89,45]),]
M2R0I1C2Time = [np.array([5.029,2.488,8.722,7.895,7.401,6.906,10.709]),]
M2R0I1C2Fail = [np.array([47525,13406,12799,6597,2829,1431,881]),]
M2R0I1C3Time = [np.array([1.131,0.44,1.836,1.371,0.849,1.093,1.458]),]
M2R0I1C3Fail = [np.array([11405,2061,2583,956,314,167,82]),]
M2RMAXIMAXC1Time = [np.array([0.0578,2.717,0.791,1.233,0.732,13.466,31.986]),]
M2RMAXIMAXC1Fail = [np.array([5784,16,5,5,3,0,0]),]
M2RMAXIMAXC2Time = [np.array([5.029,2.169,4.913,4.334,2.521,4.04,9.522]),]
M2RMAXIMAXC2Fail = [np.array([47525,16,2,0,3,1,0]),]
M2RMAXIMAXC3Time = [np.array([1.131,42.798,0.464,1.699,0.877,2.779,96.688]),]
M2RMAXIMAXC3Fail = [np.array([11405,15,7,10,9,5,8]),]
M3R0I1C1Time = [np.array([0.957,0.467,1.262,0.538,0.36,0.464,0.563]),]
M3R0I1C1Fail = [np.array([5784,1193,1254,264,84,43,19]),]
M3R0I1C2Time = [np.array([7.931,5.703,12.252,9.309,6.71,7.101,8.036]),]
M3R0I1C2Fail = [np.array([47525,12883,9635,4616,1857,860,412]),]
M3R0I1C3Time = [np.array([1.939,0.803,1.73,1.93,1.65,2.306,2.556]),]
M3R0I1C3Fail = [np.array([11405,2061,1969,1070,447,246,118]),]
M3RMAXIMAXC1Time = [np.array([0.957,3.735,0.324,1.11,0.711,17.756,67.07]),]
M3RMAXIMAXC1Fail = [np.array([5784,16,5,4,4,0,0]),]
M3RMAXIMAXC2Time = [np.array([7.931,4.364,9.05,8.252,3.393,3.214,6.403]),]
M3RMAXIMAXC2Fail = [np.array([47525,16,0,346,4,3,4]),]
M3RMAXIMAXC3Time = [np.array([1.939,44.916,0.509,2.158,1.294,2.093,404.663]),]
M3RMAXIMAXC3Fail = [np.array([11405,15,11,10,9,5,7]),]

M1RMAXI100C1Time = [np.array([2.374,13.945,10.825,14.07,8.951,19.107,16.59]),]
M1RMAXI100C1Fail = [np.array([5784,16,8,0,4,0,0]),]
M1RMAXI100C3Time = [np.array([19.5,112.004,39.722,43.718,46.437,74.951,66.003]),]
M1RMAXI100C3Fail = [np.array([47525,15,9,5,9,3,5]),]
M1RMAXI100C2Time = [np.array([4.542,15.45,22.26,27.445,45.063,51.388,76.761]),]
M1RMAXI100C2Fail = [np.array([11405,16,0,0,0,0,0]),]
M2RMAXI100C2Time = [np.array([0.578,1.998,3.863,4.494,2.536,4.341,9.594]),]
M2RMAXI100C2Fail = [np.array([5784,16,2,0,3,1,0]),]
M2RMAXI100C3Time = [np.array([5.029,19.016,0.455,1.774,0.939,2.898,1.725]),]
M2RMAXI100C3Fail = [np.array([47525,15,7,10,9,5,8]),]
M2RMAXI100C1Time = [np.array([1.131,2.684,0.471,1.219,0.787,2.322,2.24]),]
M2RMAXI100C1Fail = [np.array([11405,16,5,5,3,0,0]),]
M3RMAXI100C3Time = [np.array([0.957,27.142,0.513,2.184,1.286,2.304,2.323]),]
M3RMAXI100C3Fail = [np.array([5784,15,11,10,9,5,7]),]
M3RMAXI100C2Time = [np.array([7.931,4.456,9.003,8.46,3.412,3.187,6.738]),]
M3RMAXI100C2Fail = [np.array([47525,16,0,346,4,3,4]),]
M3RMAXI100C1Time = [np.array([1.939,3.809,0.343,1.098,0.709,2.685,2.415]),]
M3RMAXI100C1Fail = [np.array([11405,16,5,4,4,0,0]),]

M1RMAXI5WC1Time = [np.array([2.374,7.501,7.926,12.504,8.857,17.807,15.94]),]
M1RMAXI5WC1Fail = [np.array([5784,16,4,2,4,0,0]),]
M1RMAXI5WC2Time = [np.array([19.5,11.161,14.194,25.965,35.485,45.836,72.398]),]
M1RMAXI5WC2Fail = [np.array([47525,16,0,0,0,0,0]),]
M1RMAXI5WC3Time = [np.array([4.542,32.058,29.979,37.529,90.696,133.419,65.35]),]
M1RMAXI5WC3Fail = [np.array([11405,15,8,9,3,3,5]),]
M2RMAXI5WC1Time = [np.array([0.578,1.632,0.478,1.212,0.768,2.258,2.206]),]
M2RMAXI5WC1Fail = [np.array([5784,16,5,5,3,0,0]),]
M2RMAXI5WC2Time = [np.array([5.029,1.599,2.815,5.262,2.534,4.067,9.353]),]
M2RMAXI5WC2Fail = [np.array([47525,16,0,0,3,1,0]),]
M2RMAXI5WC3Time = [np.array([1.131,7.491,0.431,1.696,0.882,2.747,1.71]),]
M2RMAXI5WC3Fail = [np.array([11405,15,7,10,9,5,8]),]
M3RMAXI5WC1Time = [np.array([0.957,2.446,0.33,1.116,0.718,2.756,2.356]),]
M3RMAXI5WC1Fail = [np.array([5784,16,5,4,4,0,0]),]
M3RMAXI5WC2Time = [np.array([7.931,3.568,4.238,7.203,3.497,3.035,6.224]),]
M3RMAXI5WC2Fail = [np.array([47525,16,1,0,4,3,4]),]
M3RMAXI5WC3Time = [np.array([1.939,10.069,0.511,2.159,1.402,2.003,2.278]),]
M3RMAXI5WC3Fail = [np.array([11405,15,11,10,9,5,7]),]

M1R0IHalfC1Time = [np.array([2.374,1.221,2.082,2.151,0.938,1.004,0.846]),]
M1R0IHalfC1Fail = [np.array([5784,1193,546,252,37,5,5]),]
M1R0IHalfC2Time = [np.array([19.5,12.507,22.512,16.509,12.864,9.705,6.414]),]
M1R0IHalfC2Fail = [np.array([47525,13404,6579,2292,532,121,15]),]
M1R0IHalfC3Time = [np.array([4.542,2.073,5.904,4.371,2.421,2.173,2.841]),]
M1R0IHalfC3Fail = [np.array([11405,2061,1957,483,107,37,14]),]
M1R0I1C1Time = [np.array([2.374,1.234,1.974,2.089,0.908,0.944,0.834]),]
M1R0I1C1Fail = [np.array([5784,1188,528,257,37,5,5]),]
M1R0I1C2Time = [np.array([19.5,12.098,21.323,16.012,12.472,9.193,6.131]),]
M1R0I1C2Fail = [np.array([47525,13332,4948,2166,478,111,11]),]
M1R0I1C3Time = [np.array([4.542,2.133,5.802,4.355,2.355,2.087,2.931]),]
M1R0I1C3Fail = [np.array([11405,2061,1898,484,101,37,14]),]
M1R0I2C1Time = [np.array([2.374,1.254,1.986,2.085,0.901,0.954,0.832]),]
M1R0I2C1Fail = [np.array([5784,1188,528,252,37,5,5]),]
M1R0I2C2Time = [np.array([19.5,12.095,21.059,16.118,12.324,8.977,5.837]),]
M1R0I2C2Fail = [np.array([47525,13332,4714,2141,453,106,9]),]
M1R0I2C3Time = [np.array([4.542,2.13,5.735,4.298,2.354,2.097,3.077]),]
M1R0I2C3Fail = [np.array([11405,2061,1902,485,101,37,15]),]
M1R0I4C1Time = [np.array([2.374,1.216,2.167,2.231,0.903,0.978,0.847]),]
M1R0I4C1Fail = [np.array([5784,1188,528,252,37,5,5]),]
M1R0I4C2Time = [np.array([19.5,12.395,22.673,16.881,12.368,8.911,5.891]),]
M1R0I4C2Fail = [np.array([47525,13332,4701,2140,446,104,8]),]
M1R0I4C3Time = [np.array([4.542,2.161,6.053,4.379,2.33,2.124,3.099]),]
M1R0I4C3Fail = [np.array([11405,2061,1902,485,101,37,15]),]
M1R0I5C1Time = [np.array([2.374,1.237,1.96,2.085,0.912,0.945,0.835]),np.array([97.423,40.417,65.15,56.406,32.334,29.082,10.723])]
M1R0I5C1Fail = [np.array([5784,1188,528,252,37,5,5]),np.array([198091,38859,18315,5699,1683,496,86])]
M1R0I5C2Time = [np.array([19.5,12.27,20.886,16.223,12.652,8.826,5.897]),np.array([158.473,114.257,150.066,133.532,89.241,48.072,44.769])]
M1R0I5C2Fail = [np.array([47525,13332,4701,2140,445,104,8]),np.array([393748,101850,34755,15600,3886,880,324])]
M1R0I5C3Time = [np.array([4.542,2.178,5.759,4.272,2.353,2.127,3.054]),np.array([938.842,583.394,671.812,536.999,594.458,473.377,429.896])]
M1R0I5C3Fail = [np.array([11405,2061,1902,485,101,37,15]),np.array([1847335,632422,243138,74082,30507,12911,5361])]
M1R0I8C1Time = [np.array([2.374,1.225,2.083,2.084,0.953,0.958,0.828]),]
M1R0I8C1Fail = [np.array([5784,1188,528,252,37,5,5]),]
M1R0I8C2Time = [np.array([19.5,12.775,21.112,16.169,12.354,8.858,5.935]),]
M1R0I8C2Fail = [np.array([47525,13332,4701,2140,445,103,8]),]
M1R0I8C3Time = [np.array([4.542,2.387,5.832,4.275,2.393,2.14,3.089]),]
M1R0I8C3Fail = [np.array([11405,2061,1902,485,101,37,15]),]
M1R0I10C1Time = [np.array([2.374,1.239,1.971,2.12,0.934,1.214,0.877]),]
M1R0I10C1Fail = [np.array([5784,1188,528,252,37,5,5]),]
M1R0I10C2Time = [np.array([19.5,12.263,21.132,16.794,13.046,9.616,6.185]),]
M1R0I10C2Fail = [np.array([47525,13332,4701,2140,445,103,8]),]
M1R0I10C3Time = [np.array([4.542,2.145,6.384,4.523,2.511,2.216,3.232]),]
M1R0I10C3Fail = [np.array([11405,2061,1902,485,101,37,15]),]
M3R0IHalfC1Time = [np.array([0.957,0.41,0.973,0.473,0.428,0.461,0.749]),]
M3R0IHalfC1Fail = [np.array([5784,1193,1071,169,59,15,9]),]
M3R0IHalfC2Time = [np.array([7.931,4.252,8.568,7.209,4.132,3.494,3.414]),]
M3R0IHalfC2Fail = [np.array([47525,12883,7445,2349,505,101,26]),]
M3R0IHalfC3Time = [np.array([1.939,0.7,1.599,1.481,1.021,1.162,1.567]),]
M3R0IHalfC3Fail = [np.array([11405,2061,1601,482,76,29,10]),]
M3R0I1C1Time = [np.array([0.957,0.417,0.938,0.48,0.425,0.441,0.714]),]
M3R0I1C1Fail = [np.array([5784,1188,984,150,58,15,9]),]
M3R0I1C2Time = [np.array([7.931,4.51,7.997,7.021,3.696,3.087,5.124]),]
M3R0I1C2Fail = [np.array([47525,12675,5589,1992,409,72,17]),]
M3R0I1C3Time = [np.array([1.939,0.739,1.484,1.501,0.792,0.944,1.434]),]
M3R0I1C3Fail = [np.array([11405,2061,1481,482,43,17,10]),]
M3R0I2C1Time = [np.array([0.957,0.42,0.926,0.506,0.425,0.414,0.949]),]
M3R0I2C1Fail = [np.array([5784,1188,965,150,58,15,9]),]
M3R0I2C2Time = [np.array([7.931,4.538,7.976,6.622,3.592,2.763,2.908]),]
M3R0I2C2Fail = [np.array([47525,12654,4940,1870,348,59,14]),]
M3R0I2C3Time = [np.array([1.939,0.733,1.464,1.617,0.633,0.849,1.359]),]
M3R0I2C3Fail = [np.array([11405,2061,1432,477,25,12,6]),]
M3R0I4C1Time = [np.array([0.957,0.431,0.92,0.538,0.422,0.436,0.743]),]
M3R0I4C1Fail = [np.array([5784,1188,965,150,58,15,9]),]
M3R0I4C2Time = [np.array([7.931,4.486,8.092,6.383,3.361,2.614,2.637]),]
M3R0I4C2Fail = [np.array([47525,12654,4973,1836,317,52,12]),]
M3R0I4C3Time = [np.array([1.939,0.759,1.466,1.453,0.63,0.859,1.31]),]
M3R0I4C3Fail = [np.array([11405,2061,1432,475,16,8,6]),]
M3R0I5C1Time = [np.array([0.957,0.425,0.93,0.619,0.44,0.459,0.745]),np.array([32.831,15.814,30.904,24.851,20.93,19.638,26.535])]
M3R0I5C1Fail = [np.array([5784,1188,965,150,58,15,9]),np.array([198091,43566,26977,7595,2581,997,400])]
M3R0I5C2Time = [np.array([7.931,4.552,8.059,6.467,3.282,2.55,2.725]),np.array([64.445,39.608,66.323,54.643,44.841,35.11,33.152])]
M3R0I5C2Fail = [np.array([47525,12654,4974,1826,314,52,12]),np.array([393748,102571,46398,21497,6290,1334,597])]
M3R0I5C3Time = [np.array([1.939,0.738,1.473,1.487,0.599,0.832,1.489]),np.array([360.193,192.505,199.862,192.132,182.926,164.912,221.936])]
M3R0I5C3Fail = [np.array([11405,2061,1432,475,16,8,6]),np.array([1847335,636680,257988,115434,45068,19190,8749])]
M3R0I8C1Time = [np.array([0.957,0.446,0.927,0.625,0.436,0.424,0.713]),]
M3R0I8C1Fail = [np.array([5784,1188,965,150,58,15,9]),]
M3R0I8C2Time = [np.array([7.931,4.541,7.977,6.381,3.324,2.552,2.603]),]
M3R0I8C2Fail = [np.array([47525,12654,4974,1817,311,50,11]),]
M3R0I8C3Time = [np.array([1.939,0.737,1.448,1.443,0.591,0.858,1.328]),]
M3R0I8C3Fail = [np.array([11405,2061,1432,475,12,8,6]),]
M3R0I10C1Time = [np.array([0.957,0.511,0.927,0.666,0.46,0.445,0.725]),]
M3R0I10C1Fail = [np.array([5784,1188,965,150,58,15,9]),]
M3R0I10C2Time = [np.array([7.931,4.58,8.176,6.441,3.349,2.705,2.744]),]
M3R0I10C2Fail = [np.array([47525,12654,4974,1817,309,50,11]),]
M3R0I10C3Time = [np.array([1.939,0.733,1.434,1.469,0.606,0.908,1.329]),]
M3R0I10C3Fail = [np.array([11405,2061,1432,475,11,8,6]),]





M1R0I5C1H40Time = [0,np.array([21.552,9.338,13.344,9.506,5.238,4.443,1.996])]
M1R0I5C1H40Fail = [0,np.array([185287,33802,16115,4696,1261,429,70])]
M1R0I5C2H40Time = [0,np.array([46.01,38.77,28.835,23.106,15.171,9.702,9.26])]
M1R0I5C2H40Fail = [0,np.array([393748,101850,34738,15596,3889,880,324])]
M1R0I5C3H40Time = [0,np.array([57.559,35.557,45.467,41.85,36.694,30.473,28.338])]
M1R0I5C3H40Fail = [0,np.array([328376,61023,38061,17341,4421,1255,248])]
M3R0I5C1H40Time = [0,np.array([16.278,6.45,10.24,6.124,4.697,3.987,4.697])]
M3R0I5C1H40Fail = [0,np.array([185287,38522,24989,6697,2138,770,305])]
M3R0I5C2H40Time = [0,np.array([36.177,17.427,20.705,16.143,12.331,10.988,10.889])]
M3R0I5C2H40Fail = [0,np.array([393748,102571,46417,21495,6288,1334,597])]
M3R0I5C3H40Time = [0,np.array([41.451,21.799,28.965,28.194,24.571,25.479,28.172])]
M3R0I5C3H40Fail = [0,np.array([328376,64932,43228,21028,4114,1614,1135])]

M1RMAXI5C1H40Time = [0,np.array([21.552,9.239,6.129,7.532,9.252,14.781,24.841])]
M1RMAXI5C1H40Fail = [0,np.array([185287,1477,302,109,56,27,14])]
M1RMAXI5C2H40Time = [0,np.array([46.01,8.752,9.742,11.874,16.88,20.731,32.771])]
M1RMAXI5C2H40Fail = [0,np.array([393748,52,3,0,0,0,0])]
M1RMAXI5C3H40Time = [0,np.array([57.559,30.102,30.362,33.626,36.733,50.321,70.733])]
M1RMAXI5C3H40Fail = [0,np.array([328376,7835,2982,1403,377,263,189])]


M1R0I5C1H40Slack0Time = [0,np.array([21.552,9.274,13.231,10.365,7.373,5.292,3.012])]
M1R0I5C1H40Slack0Fail = [0,np.array([185287,33802,18452,8160,1503,369,84])]
M1R0I5C2H40Slack0Time = [0,np.array([46.01,26.726,29.879,21.355,17.168,13.037,7.458])]
M1R0I5C2H40Slack0Fail = [0,np.array([393748,101850,32696,13056,3862,1164,279])]
M1R0I5C3H40Slack0Time = [0,np.array([57.559,34.447,46.874,45.009,37.582,30.694,27.446])]
M1R0I5C3H40Slack0Fail = [0,np.array([328376,61023,36631,17906,3745,1039,236])]
M1R0I5C1H40Slack1Time = [0,np.array([21.552,10.631,11.38,13.935,7.329,5.288,3.251])]
M1R0I5C1H40Slack1Fail = [0,np.array([185287,33802,11668,3286,845,151,40])]
M1R0I5C2H40Slack1Time = [0,np.array([46.01,28.937,28.693,30.652,16.342,14.477,8.016])]
M1R0I5C2H40Slack1Fail = [0,np.array([393748,101850,31551,7961,1174,502,103])]
M1R0I5C3H40Slack1Time = [0,np.array([57.559,36.764,45.514,45.048,35.28,36.291,36.585])]
M1R0I5C3H40Slack1Fail = [0,np.array([328376,61023,30931,12295,2228,961,389])]
M1R5I5C1H40Slack1Time = [0,np.array([21.552,5.197,9.873,6.174,3.397,3.094,3.13])]
M1R5I5C1H40Slack1Fail = [0,np.array([185287,17133,11371,3819,530,141,40])]
M1R5I5C2H40Slack1Time = [0,np.array([46.01,21.44,15.653,17.428,9.762,6.443,7.314])]
M1R5I5C2H40Slack1Fail = [0,np.array([393748,55967,24573,8864,2685,857,226])]
M1R5I5C3H40Slack1Time = [0,np.array([57.559,29.04,32.614,31.91,26.289,29.073,37.005])]
M1R5I5C3H40Slack1Fail = [0,np.array([328376,31112,26676,9904,1069,480,278])]
M1R0I5C1H40Slack2Time = [0,np.array([21.552,9.734,16.389,10.994,6.209,4.086,2.464])]
M1R0I5C1H40Slack2Fail = [0,np.array([185287,33802,18238,4284,1044,268,57])]
M1R0I5C2H40Slack2Time = [0,np.array([46.01,32.479,31.13,24.695,18.339,10.888,10.932])]
M1R0I5C2H40Slack2Fail = [0,np.array([393748,101850,33366,14343,3838,879,302])]
M1R0I5C3H40Slack2Time = [0,np.array([57.559,41.62,52.172,51.082,39.359,32.724,30.915])]
M1R0I5C3H40Slack2Fail = [0,np.array([328376,61023,36576,17294,3733,1023,206])]


#M1R0I5C1H40Split1Time = [0,np.array([21.552,17.815,13.54,9.157,3.509,2.85,1.705])]
#M1R0I5C1H40Split1Fail = [0,np.array([185287,61709,16251,4912,483,140,6])]
#M1R0I5C2H40Split1Time = [0,np.array([46.01,45.391,32.299,21.139,10.959,11.217,6.737])]
#M1R0I5C2H40Split1Fail = [0,np.array([393748,176989,36952,10330,1960,695,142])]
#M1R0I5C3H40Split1Time = [0,np.array([57.559,55.724,51.349,45.161,35.453,31.974,29.763])]
#M1R0I5C3H40Split1Fail = [0,np.array([328376,143931,40571,13311,2436,500,81])]
#M1R10I5C1H40Split1Time = [0,np.array([21.552,11.544,14.468,9.115,3.512,2.771,1.811])]
#M1R10I5C1H40Split1Fail = [0,np.array([185278,33460,16060,4894,471,151,5])]
#M1R10I5C2H40Split1Time = [0,np.array([46.01,30.106,33.779,20.838,10.485,11.092,3.442])]
#M1R10I5C2H40Split1Fail = [0,np.array([393748,92178,36916,10290,1961,734,67])]
#M1R10I5C3H40Split1Time = [0,np.array([57.559,45.681,51.588,45.036,34.644,30.909,27.959])]
#M1R10I5C3H40Split1Fail = [0,np.array([328376,86823,39798,13354,2490,484,70])]


M1R0I5C1H40Split1Time = [0,np.array([21.552,16.455,13.794,9.241,4.138,1.608,1.562])]
M1R0I5C1H40Split1Fail = [0,np.array([185287,61709,17070,5031,539,53,4])]
M1R0I5C2H40Split1Time = [0,np.array([46.01,43.885,30.849,17.276,10.021,8.875,4.532])]
M1R0I5C2H40Split1Fail = [0,np.array([393748,176989,36331,8322,1762,620,85])]
M1R0I5C3H40Split1Time = [0,np.array([57.559,53.653,47.845,44.029,35.43,24.105,23.602])]
M1R0I5C3H40Split1Fail = [0,np.array([328376,143931,40397,13222,2825,280,44])]
M1R10I5C1H40Split1Time = [0,np.array([21.552,11.068,13.462,9.269,4.103,2.019,1.972])]
M1R10I5C1H40Split1Fail = [0,np.array([185278,33460,16873,5014,540,58,5])]
M1R10I5C2H40Split1Time = [0,np.array([46.01,29.094,30.441,17.331,9.953,8.852,4.612])]
M1R10I5C2H40Split1Fail = [0,np.array([393748,92178,36308,8276,1768,607,84])]
M1R10I5C3H40Split1Time = [0,np.array([57.559,42.861,47.519,44.126,35.642,26.373,22.559])]
M1R10I5C3H40Split1Fail = [0,np.array([328376,86823,39652,13297,2833,452,23])]
M1RMAXI5C1H40Split1Time = [0,np.array([21.552,11.984,13.526,9.209,4.132,2.0,1.935])]
M1RMAXI5C1H40Split1Fail = [0,np.array([185287,23996,16873,5014,540,58,5])]
M1RMAXI5C2H40Split1Time = [0,np.array([46.01,25.319,31.184,17.232,9.965,8.774,4.574])]
M1RMAXI5C2H40Split1Fail = [0,np.array([393748,59785,36308,8276,1768,607,84])]
M1RMAXI5C3H40Split1Time = [0,np.array([57.559,43.602,48.266,43.861,35.629,26.461,22.533])]
M1RMAXI5C3H40Split1Fail = [0,np.array([328376,64394,39652,13297,2833,452,23])]


M1R0I5C1H40Split1FixedTime = [0,np.array([21.552,16.934,15.215,4.735,2.381,2.049,1.365])]
M1R0I5C1H40Split1FixedFail = [0,np.array([185287,35637,15100,1343,172,39,1])]
M1R0I5C2H40Split1FixedTime = [0,np.array([46.01,48.469,23.112,16.078,8.939,3.886,1.564])]
M1R0I5C2H40Split1FixedFail = [0,np.array([393748,119100,20450,4691,1189,160,19])]
M1R0I5C3H40Split1FixedTime = [0,np.array([57.559,47.36,46.679,38.127,29.103,29.368,27.51])]
M1R0I5C3H40Split1FixedFail = [0,np.array([328376,68000,32612,7905,1097,354,73])]
M1R10I5C1H40Split1FixedTime = [0,np.array([21.552,15.84,15.872,4.935,2.391,1.294,1.354])]
M1R10I5C1H40Split1FixedFail = [0,np.array([185287,32862,16162,1414,172,15,1])]
M1R10I5C2H40Split1FixedTime = [0,np.array([46.01,44.443,22.825,15.948,10.409,3.1,2.705])]
M1R10I5C2H40Split1FixedFail = [0,np.array([393748,109240,20241,4694,1355,134,39])]
M1R10I5C3H40Split1FixedTime = [0,np.array([57.559,48.038,53.098,37.893,29.744,28.967,27.138])]
M1R10I5C3H40Split1FixedFail = [0,np.array([328376,64731,40397,7121,1110,364,73])]



M3R0I5C1H40Split1Time = [0,np.array([0,8.326,13.257,9.792,11.625,12.988,12.884])]
M3R0I5C1H40Split1Fail = [0,np.array([0,48758,26473,9924,3533,1205,290])]
M3R0I5C2H40Split1Time = [0,np.array([0,22.218,31.708,21.785,23.239,24.496,29.803])]
M3R0I5C2H40Split1Fail = [0,np.array([0,129033,63596,20284,6755,2297,1113])]
M3R0I5C3H40Split1Time = [0,np.array([0,28.921,36.104,31.233,35.209,41.42,44.508])]
M3R0I5C3H40Split1Fail = [0,np.array([0,106464,46636,23835,7056,2309,815])]
M3R10I5C1H40Split1Time = [0,np.array([0,8.061,13.322,9.687,11.466,13.004,12.834])]
M3R10I5C1H40Split1Fail = [0,np.array([0,47761,26473,9925,3555,1206,295])]
M3R10I5C2H40Split1Time = [0,np.array([0,21.817,31.701,21.843,22.683,24.695,29.345])]
M3R10I5C2H40Split1Fail = [0,np.array([0,128998,63574,20278,6759,2296,1122])]
M3R10I5C3H40Split1Time = [0,np.array([0,27.791,36.232,31.195,35.068,40.6,43.535])]
M3R10I5C3H40Split1Fail = [0,np.array([0,99040,46636,23835,7074,2307,802])]

M3R10I5C1H40Split1FixedTime = [0,np.array([16.278,6.978,7.302,8.228,7.223,8.349,7.045])]
M3R10I5C1H40Split1FixedFail = [0,np.array([185287,29373,17784,6065,1645,699,105])]
M3R10I5C2H40Split1FixedTime = [0,np.array([36.177,23.99,23.446,20.17,19.49,18.13,22.759])]
M3R10I5C2H40Split1FixedFail = [0,np.array([393748,99226,43157,16374,5131,1676,656])]
M3R10I5C3H40Split1FixedTime = [0,np.array([41.451,23.891,26.973,27.37,30.035,30.183,28.516])]
M3R10I5C3H40Split1FixedFail = [0,np.array([328376,53278,35456,17721,3806,1050,274])]



M1R5I5C1H40Time =  [np.array([]), np.array([21.552,4.877,9.125,7.876,5.625,4.235,4.098])]
M1R5I5C1H40Fail =  [np.array([]), np.array([185287,17133,10644,3307,853,191,68])]
M1R5I5C2H40Time =  [np.array([]), np.array([46.01,15.861,18.181,13.618,9.546,8.222,7.993])]
M1R5I5C2H40Fail =  [np.array([]), np.array([393748,55967,19992,5948,1814,943,176])]
M1R5I5C3H40Time =  [np.array([]), np.array([57.559,26.88,39.736,37.128,33.137,36.216,33.731])]
M1R5I5C3H40Fail =  [np.array([]), np.array([328376,31112,30394,9542,2806,1075,315])]
M1R10I5C1H40Time = [np.array([]), np.array([21.552,4.141,6.322,6.795,6.123,5.539,24.494])]
M1R10I5C1H40Fail = [np.array([]), np.array([185287,1729,2594,1711,1196,362,16])]
M1R10I5C2H40Time = [np.array([]), np.array([46.01,14.139,12.908,11.207,7.38,7.048,22.577])]
M1R10I5C2H40Fail = [np.array([]), np.array([393748,14788,4444,3432,367,144,37])]
M1R10I5C3H40Time = [np.array([]), np.array([57.559,31.183,29.919,30.489,32.465,44.703,72.263])]
M1R10I5C3H40Fail = [np.array([]), np.array([328376,22213,8080,3200,1138,281,179])]
M1R20I5C1H40Time = [np.array([]), np.array([21.552,6.232,6.124,7.193,9.145,15.059,29.045])]
M1R20I5C1H40Fail = [np.array([]), np.array([185287,1477,302,109,57,25,13])]
M1R20I5C2H40Time = [np.array([]), np.array([46.01,8.158,9.639,11.726,15.336,20.449,35.26])]
M1R20I5C2H40Fail = [np.array([]), np.array([393748,52,3,0,0,0,0])]
M1R20I5C3H40Time = [np.array([]), np.array([57.559,29.806,30.09,32.18,36.378,56.614,75.937])]
M1R20I5C3H40Fail = [np.array([]), np.array([328376,7835,2982,1403,377,257,190])]
M3R5I5C1H40Time =  [np.array([]), np.array([16.278,2.494,7.335,5.267,4.926,5.038,4.702])]
M3R5I5C1H40Fail =  [np.array([]), np.array([185287,14249,14487,4431,1574,787,268])]
M3R5I5C2H40Time =  [np.array([]), np.array([36.177,8.191,13.899,14.816,10.226,10.301,8.416])]
M3R5I5C2H40Fail =  [np.array([]), np.array([393748,45586,30090,20108,4152,2178,652])]
M3R5I5C3H40Time =  [np.array([]), np.array([41.451,15.086,25.094,23.11,22.454,21.622,20.591])]
M3R5I5C3H40Fail =  [np.array([]), np.array([328376,25077,31897,11367,4426,1360,348])]
M3R10I5C1H40Time = [np.array([]), np.array([16.278,3.629,4.789,4.704,4.208,3.148,10.655])]
M3R10I5C1H40Fail = [np.array([]), np.array([185287,12581,7829,2366,1420,218,18])]
M3R10I5C2H40Time = [np.array([]), np.array([36.177,9.29,11.09,9.453,11.659,9.696,17.305])]
M3R10I5C2H40Fail = [np.array([]), np.array([393748,34183,12006,6946,1319,1644,368])]
M3R10I5C3H40Time = [np.array([]), np.array([41.451,16.505,21.887,19.146,20.551,19.721,35.166])]
M3R10I5C3H40Fail = [np.array([]), np.array([328376,23391,20498,6076,1560,335,190])]
M3R20I5C1H40Time = [np.array([]), np.array([16.278,3.113,2.925,3.165,4.338,6.366,12.382])]
M3R20I5C1H40Fail = [np.array([]), np.array([185287,1477,225,144,61,25,11])]
M3R20I5C2H40Time = [np.array([]), np.array([36.177,4.283,5.116,6.268,7.047,10.973,13.855])]
M3R20I5C2H40Fail = [np.array([]), np.array([393748,71,1119,81,500,1005,120])]
M3R20I5C3H40Time = [np.array([]), np.array([41.451,16.835,17.457,18.769,20.923,25.756,37.267])]
M3R20I5C3H40Fail = [np.array([]), np.array([328376,7836,2977,1421,358,272,203])]





#plt.plot([1,2,4,8,16,32,64], dC1[allSoln]+1, marker='o', markersize=4, linestyle='dashed', color='#F5793A', label='Dedicated, C-I')
#plt.plot([1,2,4,8,16,32,64], dC2[allSoln]+1, marker='^', markersize=4, linestyle='dashed', color='#A95AA1', label='Dedicated, C-II')
#plt.plot([1,2,4,8,16,32,64], dC3[allSoln]+1, marker='s', markersize=4, linestyle='dashed', color='#85C0F9', label='Dedicated, C-III')
#plt.plot([1,2,4,8,16,32,64], M1R0I1C1Fail+1, marker='o', markersize=4, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M1R0I1C2Fail+1, marker='^', markersize=4, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M1R0I1C3Fail+1, marker='s', markersize=4, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("Backtracks")
#plt.yscale("log")
#plt.title("Dedicated vs HADDOCK (Mode 1) for AmongNurse")
#plt.legend(loc=1)
#plt.savefig('amongNurseBTGraphM1R0I1.pdf')
#
#
#plt.plot([0,64],[classicC1,classicC1], color='#F5793A', linestyle='dashed', label='Classic, C-I')
#plt.plot([0,64],[classicC2,classicC2], color='#A95AA1', linestyle='dashed', label='Classic, C-II')
#plt.plot([0,64],[classicC3,classicC3], color='#85C0F9', linestyle='dashed', label='Classic, C-III')
#plt.plot([1,2,4,8,16,32,64], M1R0I1C1Time, marker='o', markersize=4, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M1R0I1C2Time, marker='^', markersize=4, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M1R0I1C3Time, marker='s', markersize=4, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("CPU Time (seconds)")
#plt.title("Classic vs HADDOCK (Mode 1) for AmongNurse")
#plt.legend(loc=1)
#plt.savefig('amongNurseTimeGraphM1R0I1.pdf')
#
#
#
#plt.plot([1,2,4,8,16,32,64], dC1[allSoln]+1, marker='o', markersize=4, linestyle='dashed', color='#F5793A', label='Dedicated, C-I')
#plt.plot([1,2,4,8,16,32,64], dC2[allSoln]+1, marker='^', markersize=4, linestyle='dashed', color='#A95AA1', label='Dedicated, C-II')
#plt.plot([1,2,4,8,16,32,64], dC3[allSoln]+1, marker='s', markersize=4, linestyle='dashed', color='#85C0F9', label='Dedicated, C-III')
#plt.plot([1,2,4,8,16,32,64], M2R0I1C1Fail+1, marker='o', markersize=4, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M2R0I1C2Fail+1, marker='^', markersize=4, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M2R0I1C3Fail+1, marker='s', markersize=4, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("Backtracks")
#plt.yscale("log")
#plt.title("Dedicated vs HADDOCK (Mode 2) for AmongNurse")
#plt.legend(loc=1)
#plt.savefig('amongNurseBTGraphM2R0I1.pdf')
#
#
#plt.plot([0,64],[classicC1,classicC1], color='#F5793A', linestyle='dashed', label='Classic, C-I')
#plt.plot([0,64],[classicC2,classicC2], color='#A95AA1', linestyle='dashed', label='Classic, C-II')
#plt.plot([0,64],[classicC3,classicC3], color='#85C0F9', linestyle='dashed', label='Classic, C-III')
#plt.plot([1,2,4,8,16,32,64], M2R0I1C1Time, marker='o', markersize=4, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M2R0I1C2Time, marker='^', markersize=4, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M2R0I1C3Time, marker='s', markersize=4, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("CPU Time (seconds)")
#plt.title("Classic vs HADDOCK (Mode 2) for AmongNurse")
#plt.legend(loc=1)
#plt.savefig('amongNurseTimeGraphM2R0I1.pdf')
#
#
#
#plt.plot([1,2,4,8,16,32,64], dC1[allSoln]+1, marker='o', markersize=4, linestyle='dashed', color='#F5793A', label='Dedicated, C-I')
#plt.plot([1,2,4,8,16,32,64], dC2[allSoln]+1, marker='^', markersize=4, linestyle='dashed', color='#A95AA1', label='Dedicated, C-II')
#plt.plot([1,2,4,8,16,32,64], dC3[allSoln]+1, marker='s', markersize=4, linestyle='dashed', color='#85C0F9', label='Dedicated, C-III')
#plt.plot([1,2,4,8,16,32,64], M3R0I1C1Fail+1, marker='o', markersize=4, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M3R0I1C2Fail+1, marker='^', markersize=4, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M3R0I1C3Fail+1, marker='s', markersize=4, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("Backtracks")
#plt.yscale("log")
#plt.title("Dedicated vs HADDOCK (Mode 3) for AmongNurse")
#plt.legend(loc=1)
#plt.savefig('amongNurseBTGraphM3R0I1.pdf')
#
#
#plt.plot([0,64],[classicC1,classicC1], color='#F5793A', linestyle='dashed', label='Classic, C-I')
#plt.plot([0,64],[classicC2,classicC2], color='#A95AA1', linestyle='dashed', label='Classic, C-II')
#plt.plot([0,64],[classicC3,classicC3], color='#85C0F9', linestyle='dashed', label='Classic, C-III')
#plt.plot([1,2,4,8,16,32,64], M3R0I1C1Time, marker='o', markersize=4, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M3R0I1C2Time, marker='^', markersize=4, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M3R0I1C3Time, marker='s', markersize=4, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("CPU Time (seconds)")
#plt.title("Classic vs HADDOCK (Mode 3) for AmongNurse")
#plt.legend(loc=1)
#plt.savefig('amongNurseTimeGraphM3R0I1.pdf')
#
#
#
#
#
#plt.plot([1,2,4,8,16,32,64], dC1[allSoln]+1, marker='o', markersize=4, linestyle='dashed', color='#F5793A', label='Dedicated, C-I')
#plt.plot([1,2,4,8,16,32,64], dC2[allSoln]+1, marker='^', markersize=4, linestyle='dashed', color='#A95AA1', label='Dedicated, C-II')
#plt.plot([1,2,4,8,16,32,64], dC3[allSoln]+1, marker='s', markersize=4, linestyle='dashed', color='#85C0F9', label='Dedicated, C-III')
#plt.plot([1,2,4,8,16,32,64], M1RMAXIMAXC1Fail+1, marker='o', markersize=4, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M1RMAXIMAXC2Fail+1, marker='^', markersize=4, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M1RMAXIMAXC3Fail+1, marker='s', markersize=4, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("Backtracks")
#plt.yscale("log")
#plt.title("Dedicated vs HADDOCK (Mode 1) for AmongNurse")
#plt.legend(loc=1)
#plt.savefig('amongNurseBTGraphM1RMAXIMAX.pdf')
#
#
#plt.plot([0,64],[classicC1,classicC1], color='#F5793A', linestyle='dashed', label='Classic, C-I')
#plt.plot([0,64],[classicC2,classicC2], color='#A95AA1', linestyle='dashed', label='Classic, C-II')
#plt.plot([0,64],[classicC3,classicC3], color='#85C0F9', linestyle='dashed', label='Classic, C-III')
#plt.plot([1,2,4,8,16,32,64], M1RMAXIMAXC1Time, marker='o', markersize=4, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M1RMAXIMAXC2Time, marker='^', markersize=4, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M1RMAXIMAXC3Time, marker='s', markersize=4, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("CPU Time (seconds)")
#plt.title("Classic vs HADDOCK (Mode 1) for AmongNurse")
#plt.legend(loc=1)
#plt.savefig('amongNurseTimeGraphM1RMAXIMAX.pdf')
#
#
#
#plt.plot([1,2,4,8,16,32,64], dC1[allSoln]+1, marker='o', markersize=4, linestyle='dashed', color='#F5793A', label='Dedicated, C-I')
#plt.plot([1,2,4,8,16,32,64], dC2[allSoln]+1, marker='^', markersize=4, linestyle='dashed', color='#A95AA1', label='Dedicated, C-II')
#plt.plot([1,2,4,8,16,32,64], dC3[allSoln]+1, marker='s', markersize=4, linestyle='dashed', color='#85C0F9', label='Dedicated, C-III')
#plt.plot([1,2,4,8,16,32,64], M2RMAXIMAXC1Fail+1, marker='o', markersize=4, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M2RMAXIMAXC2Fail+1, marker='^', markersize=4, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M2RMAXIMAXC3Fail+1, marker='s', markersize=4, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("Backtracks")
#plt.yscale("log")
#plt.title("Dedicated vs HADDOCK (Mode 2) for AmongNurse")
#plt.legend(loc=1)
#plt.savefig('amongNurseBTGraphM2RMAXIMAX.pdf')
#
#
#plt.plot([0,64],[classicC1,classicC1], color='#F5793A', linestyle='dashed', label='Classic, C-I')
#plt.plot([0,64],[classicC2,classicC2], color='#A95AA1', linestyle='dashed', label='Classic, C-II')
#plt.plot([0,64],[classicC3,classicC3], color='#85C0F9', linestyle='dashed', label='Classic, C-III')
#plt.plot([1,2,4,8,16,32,64], M2RMAXIMAXC1Time, marker='o', markersize=4, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M2RMAXIMAXC2Time, marker='^', markersize=4, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M2RMAXIMAXC3Time, marker='s', markersize=4, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("CPU Time (seconds)")
#plt.title("Classic vs HADDOCK (Mode 2) for AmongNurse")
#plt.legend(loc=1)
#plt.savefig('amongNurseTimeGraphM2RMAXIMAX.pdf')
#
#
#
#plt.plot([1,2,4,8,16,32,64], dC1[allSoln]+1, marker='o', markersize=4, linestyle='dashed', color='#F5793A', label='Dedicated, C-I')
#plt.plot([1,2,4,8,16,32,64], dC2[allSoln]+1, marker='^', markersize=4, linestyle='dashed', color='#A95AA1', label='Dedicated, C-II')
#plt.plot([1,2,4,8,16,32,64], dC3[allSoln]+1, marker='s', markersize=4, linestyle='dashed', color='#85C0F9', label='Dedicated, C-III')
#plt.plot([1,2,4,8,16,32,64], M3RMAXIMAXC1Fail+1, marker='o', markersize=4, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M3RMAXIMAXC2Fail+1, marker='^', markersize=4, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M3RMAXIMAXC3Fail+1, marker='s', markersize=4, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("Backtracks")
#plt.yscale("log")
#plt.title("Dedicated vs HADDOCK (Mode 3) for AmongNurse")
#plt.legend(loc=1)
#plt.savefig('amongNurseBTGraphM3RMAXIMAX.pdf')
#
#
#plt.plot([0,64],[classicC1,classicC1], color='#F5793A', linestyle='dashed', label='Classic, C-I')
#plt.plot([0,64],[classicC2,classicC2], color='#A95AA1', linestyle='dashed', label='Classic, C-II')
#plt.plot([0,64],[classicC3,classicC3], color='#85C0F9', linestyle='dashed', label='Classic, C-III')
#plt.plot([1,2,4,8,16,32,64], M3RMAXIMAXC1Time, marker='o', markersize=4, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M3RMAXIMAXC2Time, marker='^', markersize=4, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M3RMAXIMAXC3Time, marker='s', markersize=4, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("CPU Time (seconds)")
#plt.title("Classic vs HADDOCK (Mode 3) for AmongNurse")
#plt.legend(loc=1)
#plt.savefig('amongNurseTimeGraphM3RMAXIMAX.pdf')
#
#
#
#
#
#plt.plot([1,2,4,8,16,32,64], dC1[allSoln]+1, marker='o', markersize=4, linestyle='dashed', color='#F5793A', label='Dedicated, C-I')
#plt.plot([1,2,4,8,16,32,64], dC2[allSoln]+1, marker='^', markersize=4, linestyle='dashed', color='#A95AA1', label='Dedicated, C-II')
#plt.plot([1,2,4,8,16,32,64], dC3[allSoln]+1, marker='s', markersize=4, linestyle='dashed', color='#85C0F9', label='Dedicated, C-III')
#plt.plot([1,2,4,8,16,32,64], M1RMAXI100C1Fail+1, marker='o', markersize=4, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M1RMAXI100C2Fail+1, marker='^', markersize=4, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M1RMAXI100C3Fail+1, marker='s', markersize=4, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("Backtracks")
#plt.yscale("log")
#plt.title("Dedicated vs HADDOCK (Mode 1) for AmongNurse")
#plt.legend(loc=1)
#plt.savefig('amongNurseBTGraphM1RMAXI100.pdf')
#
#
#plt.plot([0,64],[classicC1,classicC1], color='#F5793A', linestyle='dashed', label='Classic, C-I')
#plt.plot([0,64],[classicC2,classicC2], color='#A95AA1', linestyle='dashed', label='Classic, C-II')
#plt.plot([0,64],[classicC3,classicC3], color='#85C0F9', linestyle='dashed', label='Classic, C-III')
#plt.plot([1,2,4,8,16,32,64], M1RMAXI100C1Time, marker='o', markersize=4, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M1RMAXI100C2Time, marker='^', markersize=4, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M1RMAXI100C3Time, marker='s', markersize=4, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("CPU Time (seconds)")
#plt.title("Classic vs HADDOCK (Mode 1) for AmongNurse")
#plt.legend(loc=1)
#plt.savefig('amongNurseTimeGraphM1RMAXI100.pdf')
#
#
#
#plt.plot([1,2,4,8,16,32,64], dC1[allSoln]+1, marker='o', markersize=4, linestyle='dashed', color='#F5793A', label='Dedicated, C-I')
#plt.plot([1,2,4,8,16,32,64], dC2[allSoln]+1, marker='^', markersize=4, linestyle='dashed', color='#A95AA1', label='Dedicated, C-II')
#plt.plot([1,2,4,8,16,32,64], dC3[allSoln]+1, marker='s', markersize=4, linestyle='dashed', color='#85C0F9', label='Dedicated, C-III')
#plt.plot([1,2,4,8,16,32,64], M2RMAXI100C1Fail+1, marker='o', markersize=4, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M2RMAXI100C2Fail+1, marker='^', markersize=4, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M2RMAXI100C3Fail+1, marker='s', markersize=4, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("Backtracks")
#plt.yscale("log")
#plt.title("Dedicated vs HADDOCK (Mode 2) for AmongNurse")
#plt.legend(loc=1)
#plt.savefig('amongNurseBTGraphM2RMAXI100.pdf')
#
#
#plt.plot([0,64],[classicC1,classicC1], color='#F5793A', linestyle='dashed', label='Classic, C-I')
#plt.plot([0,64],[classicC2,classicC2], color='#A95AA1', linestyle='dashed', label='Classic, C-II')
#plt.plot([0,64],[classicC3,classicC3], color='#85C0F9', linestyle='dashed', label='Classic, C-III')
#plt.plot([1,2,4,8,16,32,64], M2RMAXI100C1Time, marker='o', markersize=4, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M2RMAXI100C2Time, marker='^', markersize=4, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M2RMAXI100C3Time, marker='s', markersize=4, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("CPU Time (seconds)")
#plt.title("Classic vs HADDOCK (Mode 2) for AmongNurse")
#plt.legend(loc=1)
#plt.savefig('amongNurseTimeGraphM2RMAXI100.pdf')
#
#
#
#plt.plot([1,2,4,8,16,32,64], dC1[allSoln]+1, marker='o', markersize=4, linestyle='dashed', color='#F5793A', label='Dedicated, C-I')
#plt.plot([1,2,4,8,16,32,64], dC2[allSoln]+1, marker='^', markersize=4, linestyle='dashed', color='#A95AA1', label='Dedicated, C-II')
#plt.plot([1,2,4,8,16,32,64], dC3[allSoln]+1, marker='s', markersize=4, linestyle='dashed', color='#85C0F9', label='Dedicated, C-III')
#plt.plot([1,2,4,8,16,32,64], M3RMAXI100C1Fail+1, marker='o', markersize=4, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M3RMAXI100C2Fail+1, marker='^', markersize=4, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M3RMAXI100C3Fail+1, marker='s', markersize=4, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("Backtracks")
#plt.yscale("log")
#plt.title("Dedicated vs HADDOCK (Mode 3) for AmongNurse")
#plt.legend(loc=1)
#plt.savefig('amongNurseBTGraphM3RMAXI100.pdf')
#
#
#plt.plot([0,64],[classicC1,classicC1], color='#F5793A', linestyle='dashed', label='Classic, C-I')
#plt.plot([0,64],[classicC2,classicC2], color='#A95AA1', linestyle='dashed', label='Classic, C-II')
#plt.plot([0,64],[classicC3,classicC3], color='#85C0F9', linestyle='dashed', label='Classic, C-III')
#plt.plot([1,2,4,8,16,32,64], M3RMAXI100C1Time, marker='o', markersize=4, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M3RMAXI100C2Time, marker='^', markersize=4, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M3RMAXI100C3Time, marker='s', markersize=4, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("CPU Time (seconds)")
#plt.title("Classic vs HADDOCK (Mode 3) for AmongNurse")
#plt.legend(loc=1)
#plt.savefig('amongNurseTimeGraphM3RMAXI100.pdf')
#
#
#
#
#
#
#plt.plot([1,2,4,8,16,32,64], dC1[allSoln]+1, marker='o', markersize=4, linestyle='dashed', color='#F5793A', label='Dedicated, C-I')
#plt.plot([1,2,4,8,16,32,64], dC2[allSoln]+1, marker='^', markersize=4, linestyle='dashed', color='#A95AA1', label='Dedicated, C-II')
#plt.plot([1,2,4,8,16,32,64], dC3[allSoln]+1, marker='s', markersize=4, linestyle='dashed', color='#85C0F9', label='Dedicated, C-III')
#plt.plot([1,2,4,8,16,32,64], M1RMAXI5WC1Fail+1, marker='o', markersize=4, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M1RMAXI5WC2Fail+1, marker='^', markersize=4, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M1RMAXI5WC3Fail+1, marker='s', markersize=4, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("Backtracks")
#plt.yscale("log")
#plt.title("Dedicated vs HADDOCK (Mode 1) for AmongNurse")
#plt.legend(loc=1)
#plt.savefig('amongNurseBTGraphM1RMAXI5W.pdf')
#
#
#plt.plot([0,64],[classicC1,classicC1], color='#F5793A', linestyle='dashed', label='Classic, C-I')
#plt.plot([0,64],[classicC2,classicC2], color='#A95AA1', linestyle='dashed', label='Classic, C-II')
#plt.plot([0,64],[classicC3,classicC3], color='#85C0F9', linestyle='dashed', label='Classic, C-III')
#plt.plot([1,2,4,8,16,32,64], M1RMAXI5WC1Time, marker='o', markersize=4, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M1RMAXI5WC2Time, marker='^', markersize=4, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M1RMAXI5WC3Time, marker='s', markersize=4, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("CPU Time (seconds)")
#plt.title("Classic vs HADDOCK (Mode 1) for AmongNurse")
#plt.legend(loc=1)
#plt.savefig('amongNurseTimeGraphM1RMAXI5W.pdf')
#
#
#
#plt.plot([1,2,4,8,16,32,64], dC1[allSoln]+1, marker='o', markersize=4, linestyle='dashed', color='#F5793A', label='Dedicated, C-I')
#plt.plot([1,2,4,8,16,32,64], dC2[allSoln]+1, marker='^', markersize=4, linestyle='dashed', color='#A95AA1', label='Dedicated, C-II')
#plt.plot([1,2,4,8,16,32,64], dC3[allSoln]+1, marker='s', markersize=4, linestyle='dashed', color='#85C0F9', label='Dedicated, C-III')
#plt.plot([1,2,4,8,16,32,64], M2RMAXI5WC1Fail+1, marker='o', markersize=4, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M2RMAXI5WC2Fail+1, marker='^', markersize=4, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M2RMAXI5WC3Fail+1, marker='s', markersize=4, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("Backtracks")
#plt.yscale("log")
#plt.title("Dedicated vs HADDOCK (Mode 2) for AmongNurse")
#plt.legend(loc=1)
#plt.savefig('amongNurseBTGraphM2RMAXI5W.pdf')
#
#
#plt.plot([0,64],[classicC1,classicC1], color='#F5793A', linestyle='dashed', label='Classic, C-I')
#plt.plot([0,64],[classicC2,classicC2], color='#A95AA1', linestyle='dashed', label='Classic, C-II')
#plt.plot([0,64],[classicC3,classicC3], color='#85C0F9', linestyle='dashed', label='Classic, C-III')
#plt.plot([1,2,4,8,16,32,64], M2RMAXI5WC1Time, marker='o', markersize=4, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M2RMAXI5WC2Time, marker='^', markersize=4, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M2RMAXI5WC3Time, marker='s', markersize=4, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("CPU Time (seconds)")
#plt.title("Classic vs HADDOCK (Mode 2) for AmongNurse")
#plt.legend(loc=1)
#plt.savefig('amongNurseTimeGraphM2RMAXI5W.pdf')
#
#
#
#plt.plot([1,2,4,8,16,32,64], dC1[allSoln]+1, marker='o', markersize=4, linestyle='dashed', color='#F5793A', label='Dedicated, C-I')
#plt.plot([1,2,4,8,16,32,64], dC2[allSoln]+1, marker='^', markersize=4, linestyle='dashed', color='#A95AA1', label='Dedicated, C-II')
#plt.plot([1,2,4,8,16,32,64], dC3[allSoln]+1, marker='s', markersize=4, linestyle='dashed', color='#85C0F9', label='Dedicated, C-III')
#plt.plot([1,2,4,8,16,32,64], M3RMAXI5WC1Fail+1, marker='o', markersize=4, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M3RMAXI5WC2Fail+1, marker='^', markersize=4, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M3RMAXI5WC3Fail+1, marker='s', markersize=4, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("Backtracks")
#plt.yscale("log")
#plt.title("Dedicated vs HADDOCK (Mode 3) for AmongNurse")
#plt.legend(loc=1)
#plt.savefig('amongNurseBTGraphM3RMAXI5W.pdf')
#
#
#plt.plot([0,64],[classicC1,classicC1], color='#F5793A', linestyle='dashed', label='Classic, C-I')
#plt.plot([0,64],[classicC2,classicC2], color='#A95AA1', linestyle='dashed', label='Classic, C-II')
#plt.plot([0,64],[classicC3,classicC3], color='#85C0F9', linestyle='dashed', label='Classic, C-III')
#plt.plot([1,2,4,8,16,32,64], M3RMAXI5WC1Time, marker='o', markersize=4, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M3RMAXI5WC2Time, marker='^', markersize=4, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M3RMAXI5WC3Time, marker='s', markersize=4, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("CPU Time (seconds)")
#plt.title("Classic vs HADDOCK (Mode 3) for AmongNurse")
#plt.legend(loc=1)
#plt.savefig('amongNurseTimeGraphM3RMAXI5W.pdf')
#





#plt.plot([0,64],[classicC1Fail[allSoln]+1,classicC1Fail[allSoln]+1], color='#F5793A', linestyle='dotted', label='Classic, C-I')
#plt.plot([0,64],[classicC2Fail[allSoln]+1,classicC2Fail[allSoln]+1], color='#A95AA1', linestyle='dotted', label='Classic, C-II')
#plt.plot([0,64],[classicC3Fail[allSoln]+1,classicC3Fail[allSoln]+1], color='#85C0F9', linestyle='dotted', label='Classic, C-III')
#plt.plot([1,2,4,8,16,32,64], dC1[allSoln]+1, marker='o', markersize=4, linestyle='dashed', color='#F5793A', label='Dedicated, C-I')
#plt.plot([1,2,4,8,16,32,64], dC2[allSoln]+1, marker='^', markersize=4, linestyle='dashed', color='#A95AA1', label='Dedicated, C-II')
#plt.plot([1,2,4,8,16,32,64], dC3[allSoln]+1, marker='s', markersize=4, linestyle='dashed', color='#85C0F9', label='Dedicated, C-III')
#plt.plot([1,2,4,8,16,32,64], M1R0IHalfC1Fail[allSoln]+1, marker='o', markersize=4, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M1R0IHalfC2Fail[allSoln]+1, marker='^', markersize=4, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M1R0IHalfC3Fail[allSoln]+1, marker='s', markersize=4, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("Backtracks")
#plt.yscale("log")
#plt.title("Dedicated vs HADDOCK (Mode 1) for AmongNurse")
#plt.legend(loc=1)
#plt.savefig('amongNurseBTGraphM1R0IHalf' + ('AllSoln' if allSoln else 'OneSoln') + '.pdf')
#
#
#plt.plot([0,64],[classicC1Time[allSoln],classicC1Time[allSoln]], color='#F5793A', linestyle='dotted', label='Classic, C-I')
#plt.plot([0,64],[classicC2Time[allSoln],classicC2Time[allSoln]], color='#A95AA1', linestyle='dotted', label='Classic, C-II')
#plt.plot([0,64],[classicC3Time[allSoln],classicC3Time[allSoln]], color='#85C0F9', linestyle='dotted', label='Classic, C-III')
#plt.plot([1,2,4,8,16,32,64], M1R0IHalfC1Time[allSoln], marker='o', markersize=4, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M1R0IHalfC2Time[allSoln], marker='^', markersize=4, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M1R0IHalfC3Time[allSoln], marker='s', markersize=4, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("CPU Time (seconds)")
#plt.title("Classic vs HADDOCK (Mode 1) for AmongNurse")
#plt.legend(loc=1)
#plt.savefig('amongNurseTimeGraphM1R0IHalf' + ('AllSoln' if allSoln else 'OneSoln') + '.pdf')
#
#
#
#plt.plot([0,64],[classicC1Fail[allSoln]+1,classicC1Fail[allSoln]+1], color='#F5793A', linestyle='dotted', label='Classic, C-I')
#plt.plot([0,64],[classicC2Fail[allSoln]+1,classicC2Fail[allSoln]+1], color='#A95AA1', linestyle='dotted', label='Classic, C-II')
#plt.plot([0,64],[classicC3Fail[allSoln]+1,classicC3Fail[allSoln]+1], color='#85C0F9', linestyle='dotted', label='Classic, C-III')
#plt.plot([1,2,4,8,16,32,64], dC1[allSoln]+1, marker='o', markersize=4, linestyle='dashed', color='#F5793A', label='Dedicated, C-I')
#plt.plot([1,2,4,8,16,32,64], dC2[allSoln]+1, marker='^', markersize=4, linestyle='dashed', color='#A95AA1', label='Dedicated, C-II')
#plt.plot([1,2,4,8,16,32,64], dC3[allSoln]+1, marker='s', markersize=4, linestyle='dashed', color='#85C0F9', label='Dedicated, C-III')
#plt.plot([1,2,4,8,16,32,64], M3R0IHalfC1Fail[allSoln]+1, marker='o', markersize=4, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M3R0IHalfC2Fail[allSoln]+1, marker='^', markersize=4, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M3R0IHalfC3Fail[allSoln]+1, marker='s', markersize=4, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("Backtracks")
#plt.yscale("log")
#plt.title("Dedicated vs HADDOCK (Mode 3) for AmongNurse")
#plt.legend(loc=1)
#plt.savefig('amongNurseBTGraphM3R0IHalf' + ('AllSoln' if allSoln else 'OneSoln') + '.pdf')
#
#
#plt.plot([0,64],[classicC1Time[allSoln],classicC1Time[allSoln]], color='#F5793A', linestyle='dotted', label='Classic, C-I')
#plt.plot([0,64],[classicC2Time[allSoln],classicC2Time[allSoln]], color='#A95AA1', linestyle='dotted', label='Classic, C-II')
#plt.plot([0,64],[classicC3Time[allSoln],classicC3Time[allSoln]], color='#85C0F9', linestyle='dotted', label='Classic, C-III')
#plt.plot([1,2,4,8,16,32,64], M3R0IHalfC1Time[allSoln], marker='o', markersize=4, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M3R0IHalfC2Time[allSoln], marker='^', markersize=4, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M3R0IHalfC3Time[allSoln], marker='s', markersize=4, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("CPU Time (seconds)")
#plt.title("Classic vs HADDOCK (Mode 3) for AmongNurse")
#plt.legend(loc=1)
#plt.savefig('amongNurseTimeGraphM3R0IHalf' + ('AllSoln' if allSoln else 'OneSoln') + '.pdf')
#
#
#
#plt.plot([0,64],[classicC1Fail[allSoln]+1,classicC1Fail[allSoln]+1], color='#F5793A', linestyle='dotted', label='Classic, C-I')
#plt.plot([0,64],[classicC2Fail[allSoln]+1,classicC2Fail[allSoln]+1], color='#A95AA1', linestyle='dotted', label='Classic, C-II')
#plt.plot([0,64],[classicC3Fail[allSoln]+1,classicC3Fail[allSoln]+1], color='#85C0F9', linestyle='dotted', label='Classic, C-III')
#plt.plot([1,2,4,8,16,32,64], dC1[allSoln]+1, marker='o', markersize=4, linestyle='dashed', color='#F5793A', label='Dedicated, C-I')
#plt.plot([1,2,4,8,16,32,64], dC2[allSoln]+1, marker='^', markersize=4, linestyle='dashed', color='#A95AA1', label='Dedicated, C-II')
#plt.plot([1,2,4,8,16,32,64], dC3[allSoln]+1, marker='s', markersize=4, linestyle='dashed', color='#85C0F9', label='Dedicated, C-III')
#plt.plot([1,2,4,8,16,32,64], M1R0I1C1Fail[allSoln]+1, marker='o', markersize=4, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M1R0I1C2Fail[allSoln]+1, marker='^', markersize=4, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M1R0I1C3Fail[allSoln]+1, marker='s', markersize=4, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("Backtracks")
#plt.yscale("log")
#plt.title("Dedicated vs HADDOCK (Mode 1) for AmongNurse")
#plt.legend(loc=1)
#plt.savefig('amongNurseBTGraphM1R0I1' + ('AllSoln' if allSoln else 'OneSoln') + '.pdf')
#
#
#plt.plot([0,64],[classicC1Time[allSoln],classicC1Time[allSoln]], color='#F5793A', linestyle='dotted', label='Classic, C-I')
#plt.plot([0,64],[classicC2Time[allSoln],classicC2Time[allSoln]], color='#A95AA1', linestyle='dotted', label='Classic, C-II')
#plt.plot([0,64],[classicC3Time[allSoln],classicC3Time[allSoln]], color='#85C0F9', linestyle='dotted', label='Classic, C-III')
#plt.plot([1,2,4,8,16,32,64], M1R0I1C1Time[allSoln], marker='o', markersize=4, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M1R0I1C2Time[allSoln], marker='^', markersize=4, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M1R0I1C3Time[allSoln], marker='s', markersize=4, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("CPU Time (seconds)")
#plt.title("Classic vs HADDOCK (Mode 1) for AmongNurse")
#plt.legend(loc=1)
#plt.savefig('amongNurseTimeGraphM1R0I1' + ('AllSoln' if allSoln else 'OneSoln') + '.pdf')
#
#
#
#plt.plot([0,64],[classicC1Fail[allSoln]+1,classicC1Fail[allSoln]+1], color='#F5793A', linestyle='dotted', label='Classic, C-I')
#plt.plot([0,64],[classicC2Fail[allSoln]+1,classicC2Fail[allSoln]+1], color='#A95AA1', linestyle='dotted', label='Classic, C-II')
#plt.plot([0,64],[classicC3Fail[allSoln]+1,classicC3Fail[allSoln]+1], color='#85C0F9', linestyle='dotted', label='Classic, C-III')
#plt.plot([1,2,4,8,16,32,64], dC1[allSoln]+1, marker='o', markersize=4, linestyle='dashed', color='#F5793A', label='Dedicated, C-I')
#plt.plot([1,2,4,8,16,32,64], dC2[allSoln]+1, marker='^', markersize=4, linestyle='dashed', color='#A95AA1', label='Dedicated, C-II')
#plt.plot([1,2,4,8,16,32,64], dC3[allSoln]+1, marker='s', markersize=4, linestyle='dashed', color='#85C0F9', label='Dedicated, C-III')
#plt.plot([1,2,4,8,16,32,64], M3R0I1C1Fail[allSoln]+1, marker='o', markersize=4, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M3R0I1C2Fail[allSoln]+1, marker='^', markersize=4, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M3R0I1C3Fail[allSoln]+1, marker='s', markersize=4, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("Backtracks")
#plt.yscale("log")
#plt.title("Dedicated vs HADDOCK (Mode 3) for AmongNurse")
#plt.legend(loc=1)
#plt.savefig('amongNurseBTGraphM3R0I1' + ('AllSoln' if allSoln else 'OneSoln') + '.pdf')
#
#
#plt.plot([0,64],[classicC1Time[allSoln],classicC1Time[allSoln]], color='#F5793A', linestyle='dotted', label='Classic, C-I')
#plt.plot([0,64],[classicC2Time[allSoln],classicC2Time[allSoln]], color='#A95AA1', linestyle='dotted', label='Classic, C-II')
#plt.plot([0,64],[classicC3Time[allSoln],classicC3Time[allSoln]], color='#85C0F9', linestyle='dotted', label='Classic, C-III')
#plt.plot([1,2,4,8,16,32,64], M3R0I1C1Time[allSoln], marker='o', markersize=4, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M3R0I1C2Time[allSoln], marker='^', markersize=4, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M3R0I1C3Time[allSoln], marker='s', markersize=4, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("CPU Time (seconds)")
#plt.title("Classic vs HADDOCK (Mode 3) for AmongNurse")
#plt.legend(loc=1)
#plt.savefig('amongNurseTimeGraphM3R0I1' + ('AllSoln' if allSoln else 'OneSoln') + '.pdf')
#
#
#
#
#plt.plot([0,64],[classicC1Fail[allSoln]+1,classicC1Fail[allSoln]+1], color='#F5793A', linestyle='dotted', label='Classic, C-I')
#plt.plot([0,64],[classicC2Fail[allSoln]+1,classicC2Fail[allSoln]+1], color='#A95AA1', linestyle='dotted', label='Classic, C-II')
#plt.plot([0,64],[classicC3Fail[allSoln]+1,classicC3Fail[allSoln]+1], color='#85C0F9', linestyle='dotted', label='Classic, C-III')
#plt.plot([1,2,4,8,16,32,64], dC1[allSoln]+1, marker='o', markersize=4, linestyle='dashed', color='#F5793A', label='Dedicated, C-I')
#plt.plot([1,2,4,8,16,32,64], dC2[allSoln]+1, marker='^', markersize=4, linestyle='dashed', color='#A95AA1', label='Dedicated, C-II')
#plt.plot([1,2,4,8,16,32,64], dC3[allSoln]+1, marker='s', markersize=4, linestyle='dashed', color='#85C0F9', label='Dedicated, C-III')
#plt.plot([1,2,4,8,16,32,64], M1R0I2C1Fail[allSoln]+1, marker='o', markersize=4, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M1R0I2C2Fail[allSoln]+1, marker='^', markersize=4, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M1R0I2C3Fail[allSoln]+1, marker='s', markersize=4, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("Backtracks")
#plt.yscale("log")
#plt.title("Dedicated vs HADDOCK (Mode 1) for AmongNurse")
#plt.legend(loc=1)
#plt.savefig('amongNurseBTGraphM1R0I2' + ('AllSoln' if allSoln else 'OneSoln') + '.pdf')
#
#
#plt.plot([0,64],[classicC1Time[allSoln],classicC1Time[allSoln]], color='#F5793A', linestyle='dotted', label='Classic, C-I')
#plt.plot([0,64],[classicC2Time[allSoln],classicC2Time[allSoln]], color='#A95AA1', linestyle='dotted', label='Classic, C-II')
#plt.plot([0,64],[classicC3Time[allSoln],classicC3Time[allSoln]], color='#85C0F9', linestyle='dotted', label='Classic, C-III')
#plt.plot([1,2,4,8,16,32,64], M1R0I2C1Time[allSoln], marker='o', markersize=4, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M1R0I2C2Time[allSoln], marker='^', markersize=4, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M1R0I2C3Time[allSoln], marker='s', markersize=4, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("CPU Time (seconds)")
#plt.title("Classic vs HADDOCK (Mode 1) for AmongNurse")
#plt.legend(loc=1)
#plt.savefig('amongNurseTimeGraphM1R0I2' + ('AllSoln' if allSoln else 'OneSoln') + '.pdf')
#
#
#
#plt.plot([0,64],[classicC1Fail[allSoln]+1,classicC1Fail[allSoln]+1], color='#F5793A', linestyle='dotted', label='Classic, C-I')
#plt.plot([0,64],[classicC2Fail[allSoln]+1,classicC2Fail[allSoln]+1], color='#A95AA1', linestyle='dotted', label='Classic, C-II')
#plt.plot([0,64],[classicC3Fail[allSoln]+1,classicC3Fail[allSoln]+1], color='#85C0F9', linestyle='dotted', label='Classic, C-III')
#plt.plot([1,2,4,8,16,32,64], dC1[allSoln]+1, marker='o', markersize=4, linestyle='dashed', color='#F5793A', label='Dedicated, C-I')
#plt.plot([1,2,4,8,16,32,64], dC2[allSoln]+1, marker='^', markersize=4, linestyle='dashed', color='#A95AA1', label='Dedicated, C-II')
#plt.plot([1,2,4,8,16,32,64], dC3[allSoln]+1, marker='s', markersize=4, linestyle='dashed', color='#85C0F9', label='Dedicated, C-III')
#plt.plot([1,2,4,8,16,32,64], M3R0I2C1Fail[allSoln]+1, marker='o', markersize=4, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M3R0I2C2Fail[allSoln]+1, marker='^', markersize=4, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M3R0I2C3Fail[allSoln]+1, marker='s', markersize=4, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("Backtracks")
#plt.yscale("log")
#plt.title("Dedicated vs HADDOCK (Mode 3) for AmongNurse")
#plt.legend(loc=1)
#plt.savefig('amongNurseBTGraphM3R0I2' + ('AllSoln' if allSoln else 'OneSoln') + '.pdf')
#
#
#plt.plot([0,64],[classicC1Time[allSoln],classicC1Time[allSoln]], color='#F5793A', linestyle='dotted', label='Classic, C-I')
#plt.plot([0,64],[classicC2Time[allSoln],classicC2Time[allSoln]], color='#A95AA1', linestyle='dotted', label='Classic, C-II')
#plt.plot([0,64],[classicC3Time[allSoln],classicC3Time[allSoln]], color='#85C0F9', linestyle='dotted', label='Classic, C-III')
#plt.plot([1,2,4,8,16,32,64], M3R0I2C1Time[allSoln], marker='o', markersize=4, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M3R0I2C2Time[allSoln], marker='^', markersize=4, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M3R0I2C3Time[allSoln], marker='s', markersize=4, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("CPU Time (seconds)")
#plt.title("Classic vs HADDOCK (Mode 3) for AmongNurse")
#plt.legend(loc=1)
#plt.savefig('amongNurseTimeGraphM3R0I2' + ('AllSoln' if allSoln else 'OneSoln') + '.pdf')
#
#
#
#
#
#plt.plot([0,64],[classicC1Fail[allSoln]+1,classicC1Fail[allSoln]+1], color='#F5793A', linestyle='dotted', label='Classic, C-I')
#plt.plot([0,64],[classicC2Fail[allSoln]+1,classicC2Fail[allSoln]+1], color='#A95AA1', linestyle='dotted', label='Classic, C-II')
#plt.plot([0,64],[classicC3Fail[allSoln]+1,classicC3Fail[allSoln]+1], color='#85C0F9', linestyle='dotted', label='Classic, C-III')
#plt.plot([1,2,4,8,16,32,64], dC1[allSoln]+1, marker='o', markersize=4, linestyle='dashed', color='#F5793A', label='Dedicated, C-I')
#plt.plot([1,2,4,8,16,32,64], dC2[allSoln]+1, marker='^', markersize=4, linestyle='dashed', color='#A95AA1', label='Dedicated, C-II')
#plt.plot([1,2,4,8,16,32,64], dC3[allSoln]+1, marker='s', markersize=4, linestyle='dashed', color='#85C0F9', label='Dedicated, C-III')
#plt.plot([1,2,4,8,16,32,64], M1R0I4C1Fail[allSoln]+1, marker='o', markersize=4, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M1R0I4C2Fail[allSoln]+1, marker='^', markersize=4, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M1R0I4C3Fail[allSoln]+1, marker='s', markersize=4, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("Backtracks")
#plt.yscale("log")
#plt.title("Dedicated vs HADDOCK (Mode 1) for AmongNurse")
#plt.legend(loc=1)
#plt.savefig('amongNurseBTGraphM1R0I4' + ('AllSoln' if allSoln else 'OneSoln') + '.pdf')
#
#
#plt.plot([0,64],[classicC1Time[allSoln],classicC1Time[allSoln]], color='#F5793A', linestyle='dotted', label='Classic, C-I')
#plt.plot([0,64],[classicC2Time[allSoln],classicC2Time[allSoln]], color='#A95AA1', linestyle='dotted', label='Classic, C-II')
#plt.plot([0,64],[classicC3Time[allSoln],classicC3Time[allSoln]], color='#85C0F9', linestyle='dotted', label='Classic, C-III')
#plt.plot([1,2,4,8,16,32,64], M1R0I4C1Time[allSoln], marker='o', markersize=4, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M1R0I4C2Time[allSoln], marker='^', markersize=4, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M1R0I4C3Time[allSoln], marker='s', markersize=4, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("CPU Time (seconds)")
#plt.title("Classic vs HADDOCK (Mode 1) for AmongNurse")
#plt.legend(loc=1)
#plt.savefig('amongNurseTimeGraphM1R0I4' + ('AllSoln' if allSoln else 'OneSoln') + '.pdf')
#
#
#
#plt.plot([0,64],[classicC1Fail[allSoln]+1,classicC1Fail[allSoln]+1], color='#F5793A', linestyle='dotted', label='Classic, C-I')
#plt.plot([0,64],[classicC2Fail[allSoln]+1,classicC2Fail[allSoln]+1], color='#A95AA1', linestyle='dotted', label='Classic, C-II')
#plt.plot([0,64],[classicC3Fail[allSoln]+1,classicC3Fail[allSoln]+1], color='#85C0F9', linestyle='dotted', label='Classic, C-III')
#plt.plot([1,2,4,8,16,32,64], dC1[allSoln]+1, marker='o', markersize=4, linestyle='dashed', color='#F5793A', label='Dedicated, C-I')
#plt.plot([1,2,4,8,16,32,64], dC2[allSoln]+1, marker='^', markersize=4, linestyle='dashed', color='#A95AA1', label='Dedicated, C-II')
#plt.plot([1,2,4,8,16,32,64], dC3[allSoln]+1, marker='s', markersize=4, linestyle='dashed', color='#85C0F9', label='Dedicated, C-III')
#plt.plot([1,2,4,8,16,32,64], M3R0I4C1Fail[allSoln]+1, marker='o', markersize=4, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M3R0I4C2Fail[allSoln]+1, marker='^', markersize=4, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M3R0I4C3Fail[allSoln]+1, marker='s', markersize=4, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("Backtracks")
#plt.yscale("log")
#plt.title("Dedicated vs HADDOCK (Mode 3) for AmongNurse")
#plt.legend(loc=1)
#plt.savefig('amongNurseBTGraphM3R0I4' + ('AllSoln' if allSoln else 'OneSoln') + '.pdf')
#
#
#plt.plot([0,64],[classicC1Time[allSoln],classicC1Time[allSoln]], color='#F5793A', linestyle='dotted', label='Classic, C-I')
#plt.plot([0,64],[classicC2Time[allSoln],classicC2Time[allSoln]], color='#A95AA1', linestyle='dotted', label='Classic, C-II')
#plt.plot([0,64],[classicC3Time[allSoln],classicC3Time[allSoln]], color='#85C0F9', linestyle='dotted', label='Classic, C-III')
#plt.plot([1,2,4,8,16,32,64], M3R0I4C1Time[allSoln], marker='o', markersize=4, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M3R0I4C2Time[allSoln], marker='^', markersize=4, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M3R0I4C3Time[allSoln], marker='s', markersize=4, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("CPU Time (seconds)")
#plt.title("Classic vs HADDOCK (Mode 3) for AmongNurse")
#plt.legend(loc=1)
#plt.savefig('amongNurseTimeGraphM3R0I4' + ('AllSoln' if allSoln else 'OneSoln') + '.pdf')
#
#
#
#
#plt.figure(figsize=(8,5))
#plt.plot([0,64],[classicC1Fail[allSoln]+1,classicC1Fail[allSoln]+1], color='#F5793A', linestyle='dotted', label='Classic, C-I')
#plt.plot([0,64],[classicC2Fail[allSoln]+1,classicC2Fail[allSoln]+1], color='#A95AA1', linestyle='dotted', label='Classic, C-II')
#plt.plot([0,64],[classicC3Fail[allSoln]+1,classicC3Fail[allSoln]+1], color='#85C0F9', linestyle='dotted', label='Classic, C-III')
#plt.plot([1,2,4,8,16,32,64], dC1[allSoln]+1, marker='o', markersize=4, linestyle='dashed', color='#F5793A', label='Dedicated, C-I')
#plt.plot([1,2,4,8,16,32,64], dC2[allSoln]+1, marker='^', markersize=4, linestyle='dashed', color='#A95AA1', label='Dedicated, C-II')
#plt.plot([1,2,4,8,16,32,64], dC3[allSoln]+1, marker='s', markersize=4, linestyle='dashed', color='#85C0F9', label='Dedicated, C-III')
#plt.plot([1,2,4,8,16,32,64], M1R0I5C1Fail[allSoln]+1, marker='o', markersize=4, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M1R0I5C2Fail[allSoln]+1, marker='^', markersize=4, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M1R0I5C3Fail[allSoln]+1, marker='s', markersize=4, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("Backtracks")
#plt.yscale("log")
#plt.title("Dedicated vs HADDOCK (Mode 1) for AmongNurse")
#plt.legend(loc=1)
#plt.savefig('amongNurseBTGraphM1R0I5' + ('AllSoln' if allSoln else 'OneSoln') + '.pdf')
#
#
#plt.figure(figsize=(8,5))
#plt.plot([0,64],[classicC1Time[allSoln],classicC1Time[allSoln]], color='#F5793A', linestyle='dotted', label='Classic, C-I')
#plt.plot([0,64],[classicC2Time[allSoln],classicC2Time[allSoln]], color='#A95AA1', linestyle='dotted', label='Classic, C-II')
#plt.plot([0,64],[classicC3Time[allSoln],classicC3Time[allSoln]], color='#85C0F9', linestyle='dotted', label='Classic, C-III')
#plt.plot([1,2,4,8,16,32,64], dC1Time[allSoln], marker='o', markersize=4, linestyle='dashed', color='#F5793A', label='Dedicated, C-I')
#plt.plot([1,2,4,8,16,32,64], dC2Time[allSoln], marker='^', markersize=4, linestyle='dashed', color='#A95AA1', label='Dedicated, C-II')
#plt.plot([1,2,4,8,16,32,64], dC3Time[allSoln], marker='s', markersize=4, linestyle='dashed', color='#85C0F9', label='Dedicated, C-III')
#plt.plot([1,2,4,8,16,32,64], M1R0I5C1Time[allSoln], marker='o', markersize=4, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M1R0I5C2Time[allSoln], marker='^', markersize=4, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M1R0I5C3Time[allSoln], marker='s', markersize=4, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("CPU Time (seconds)")
#plt.yscale("log")
#plt.title("Classic vs HADDOCK (Mode 1) for AmongNurse")
##plt.legend(loc=1)
#plt.savefig('amongNurseTimeGraphM1R0I5' + ('AllSoln' if allSoln else 'OneSoln') + '.pdf')
#
#
#
#plt.figure(figsize=(8,5))
#plt.plot([0,64],[classicC1Fail[allSoln]+1,classicC1Fail[allSoln]+1], color='#F5793A', linestyle='dotted', label='Classic, C-I')
#plt.plot([0,64],[classicC2Fail[allSoln]+1,classicC2Fail[allSoln]+1], color='#A95AA1', linestyle='dotted', label='Classic, C-II')
#plt.plot([0,64],[classicC3Fail[allSoln]+1,classicC3Fail[allSoln]+1], color='#85C0F9', linestyle='dotted', label='Classic, C-III')
#plt.plot([1,2,4,8,16,32,64], dC1[allSoln]+1, marker='o', markersize=4, linestyle='dashed', color='#F5793A', label='Dedicated, C-I')
#plt.plot([1,2,4,8,16,32,64], dC2[allSoln]+1, marker='^', markersize=4, linestyle='dashed', color='#A95AA1', label='Dedicated, C-II')
#plt.plot([1,2,4,8,16,32,64], dC3[allSoln]+1, marker='s', markersize=4, linestyle='dashed', color='#85C0F9', label='Dedicated, C-III')
#plt.plot([1,2,4,8,16,32,64], M3R0I5C1Fail[allSoln]+1, marker='o', markersize=4, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M3R0I5C2Fail[allSoln]+1, marker='^', markersize=4, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M3R0I5C3Fail[allSoln]+1, marker='s', markersize=4, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("Backtracks")
#plt.yscale("log")
#plt.title("Dedicated vs HADDOCK (Mode 3) for AmongNurse")
#plt.legend(loc=1)
#plt.savefig('amongNurseBTGraphM3R0I5' + ('AllSoln' if allSoln else 'OneSoln') + '.pdf')
#
#
#plt.figure(figsize=(8,5))
#plt.plot([0,64],[classicC1Time[allSoln],classicC1Time[allSoln]], color='#F5793A', linestyle='dotted', label='Classic, C-I')
#plt.plot([0,64],[classicC2Time[allSoln],classicC2Time[allSoln]], color='#A95AA1', linestyle='dotted', label='Classic, C-II')
#plt.plot([0,64],[classicC3Time[allSoln],classicC3Time[allSoln]], color='#85C0F9', linestyle='dotted', label='Classic, C-III')
#plt.plot([1,2,4,8,16,32,64], dC1Time[allSoln], marker='o', markersize=4, linestyle='dashed', color='#F5793A', label='Dedicated, C-I')
#plt.plot([1,2,4,8,16,32,64], dC2Time[allSoln], marker='^', markersize=4, linestyle='dashed', color='#A95AA1', label='Dedicated, C-II')
#plt.plot([1,2,4,8,16,32,64], dC3Time[allSoln], marker='s', markersize=4, linestyle='dashed', color='#85C0F9', label='Dedicated, C-III')
#plt.plot([1,2,4,8,16,32,64], M3R0I5C1Time[allSoln], marker='o', markersize=4, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M3R0I5C2Time[allSoln], marker='^', markersize=4, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M3R0I5C3Time[allSoln], marker='s', markersize=4, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("CPU Time (seconds)")
#plt.yscale("log")
#plt.title("Classic vs HADDOCK (Mode 3) for AmongNurse")
##plt.legend(loc=1)
#plt.savefig('amongNurseTimeGraphM3R0I5' + ('AllSoln' if allSoln else 'OneSoln') + '.pdf')
#
#
#
#
#plt.figure(figsize=(8,5))
#plt.plot([0,64],[classicC1Fail[allSoln]+1,classicC1Fail[allSoln]+1], color='#F5793A', linestyle='dotted', label='Classic, C-I')
#plt.plot([0,64],[classicC2Fail[allSoln]+1,classicC2Fail[allSoln]+1], color='#A95AA1', linestyle='dotted', label='Classic, C-II')
#plt.plot([0,64],[classicC3Fail[allSoln]+1,classicC3Fail[allSoln]+1], color='#85C0F9', linestyle='dotted', label='Classic, C-III')
#plt.plot([1,2,4,8,16,32,64], dC1[allSoln]+1, marker='o', markersize=4, linestyle='dashed', color='#F5793A', label='Dedicated, C-I')
#plt.plot([1,2,4,8,16,32,64], dC2[allSoln]+1, marker='^', markersize=4, linestyle='dashed', color='#A95AA1', label='Dedicated, C-II')
#plt.plot([1,2,4,8,16,32,64], dC3[allSoln]+1, marker='s', markersize=4, linestyle='dashed', color='#85C0F9', label='Dedicated, C-III')
#plt.plot([1,2,4,8,16,32,64], M1R0I8C1Fail[allSoln]+1, marker='o', markersize=4, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M1R0I8C2Fail[allSoln]+1, marker='^', markersize=4, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M1R0I8C3Fail[allSoln]+1, marker='s', markersize=4, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("Backtracks")
#plt.yscale("log")
#plt.title("Dedicated vs HADDOCK (Mode 1) for AmongNurse")
#plt.legend(loc=1)
#plt.savefig('amongNurseBTGraphM1R0I8' + ('AllSoln' if allSoln else 'OneSoln') + '.pdf')
#
#
#plt.figure(figsize=(8,5))
#plt.plot([0,64],[classicC1Time[allSoln],classicC1Time[allSoln]], color='#F5793A', linestyle='dotted', label='Classic, C-I')
#plt.plot([0,64],[classicC2Time[allSoln],classicC2Time[allSoln]], color='#A95AA1', linestyle='dotted', label='Classic, C-II')
#plt.plot([0,64],[classicC3Time[allSoln],classicC3Time[allSoln]], color='#85C0F9', linestyle='dotted', label='Classic, C-III')
#plt.plot([1,2,4,8,16,32,64], M1R0I8C1Time[allSoln], marker='o', markersize=4, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M1R0I8C2Time[allSoln], marker='^', markersize=4, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M1R0I8C3Time[allSoln], marker='s', markersize=4, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("CPU Time (seconds)")
#plt.title("Classic vs HADDOCK (Mode 1) for AmongNurse")
#plt.legend(loc=1)
#plt.savefig('amongNurseTimeGraphM1R0I8' + ('AllSoln' if allSoln else 'OneSoln') + '.pdf')
#
#
#
#plt.figure(figsize=(8,5))
#plt.plot([0,64],[classicC1Fail[allSoln]+1,classicC1Fail[allSoln]+1], color='#F5793A', linestyle='dotted', label='Classic, C-I')
#plt.plot([0,64],[classicC2Fail[allSoln]+1,classicC2Fail[allSoln]+1], color='#A95AA1', linestyle='dotted', label='Classic, C-II')
#plt.plot([0,64],[classicC3Fail[allSoln]+1,classicC3Fail[allSoln]+1], color='#85C0F9', linestyle='dotted', label='Classic, C-III')
#plt.plot([1,2,4,8,16,32,64], dC1[allSoln]+1, marker='o', markersize=4, linestyle='dashed', color='#F5793A', label='Dedicated, C-I')
#plt.plot([1,2,4,8,16,32,64], dC2[allSoln]+1, marker='^', markersize=4, linestyle='dashed', color='#A95AA1', label='Dedicated, C-II')
#plt.plot([1,2,4,8,16,32,64], dC3[allSoln]+1, marker='s', markersize=4, linestyle='dashed', color='#85C0F9', label='Dedicated, C-III')
#plt.plot([1,2,4,8,16,32,64], M3R0I8C1Fail[allSoln]+1, marker='o', markersize=4, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M3R0I8C2Fail[allSoln]+1, marker='^', markersize=4, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M3R0I8C3Fail[allSoln]+1, marker='s', markersize=4, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("Backtracks")
#plt.yscale("log")
#plt.title("Dedicated vs HADDOCK (Mode 3) for AmongNurse")
#plt.legend(loc=1)
#plt.savefig('amongNurseBTGraphM3R0I8' + ('AllSoln' if allSoln else 'OneSoln') + '.pdf')
#
#
#plt.figure(figsize=(8,5))
#plt.plot([0,64],[classicC1Time[allSoln],classicC1Time[allSoln]], color='#F5793A', linestyle='dotted', label='Classic, C-I')
#plt.plot([0,64],[classicC2Time[allSoln],classicC2Time[allSoln]], color='#A95AA1', linestyle='dotted', label='Classic, C-II')
#plt.plot([0,64],[classicC3Time[allSoln],classicC3Time[allSoln]], color='#85C0F9', linestyle='dotted', label='Classic, C-III')
#plt.plot([1,2,4,8,16,32,64], M3R0I8C1Time[allSoln], marker='o', markersize=4, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M3R0I8C2Time[allSoln], marker='^', markersize=4, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M3R0I8C3Time[allSoln], marker='s', markersize=4, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("CPU Time (seconds)")
#plt.title("Classic vs HADDOCK (Mode 3) for AmongNurse")
#plt.legend(loc=1)
#plt.savefig('amongNurseTimeGraphM3R0I8' + ('AllSoln' if allSoln else 'OneSoln') + '.pdf')
#
#
#
#
#
#plt.figure(figsize=(8,5))
#plt.plot([0,64],[classicC1Fail[allSoln]+1,classicC1Fail[allSoln]+1], color='#F5793A', linestyle='dotted', label='Classic, C-I')
#plt.plot([0,64],[classicC2Fail[allSoln]+1,classicC2Fail[allSoln]+1], color='#A95AA1', linestyle='dotted', label='Classic, C-II')
#plt.plot([0,64],[classicC3Fail[allSoln]+1,classicC3Fail[allSoln]+1], color='#85C0F9', linestyle='dotted', label='Classic, C-III')
#plt.plot([1,2,4,8,16,32,64], dC1[allSoln]+1, marker='o', markersize=4, linestyle='dashed', color='#F5793A', label='Dedicated, C-I')
#plt.plot([1,2,4,8,16,32,64], dC2[allSoln]+1, marker='^', markersize=4, linestyle='dashed', color='#A95AA1', label='Dedicated, C-II')
#plt.plot([1,2,4,8,16,32,64], dC3[allSoln]+1, marker='s', markersize=4, linestyle='dashed', color='#85C0F9', label='Dedicated, C-III')
#plt.plot([1,2,4,8,16,32,64], M1R0I10C1Fail[allSoln]+1, marker='o', markersize=4, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M1R0I10C2Fail[allSoln]+1, marker='^', markersize=4, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M1R0I10C3Fail[allSoln]+1, marker='s', markersize=4, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("Backtracks")
#plt.yscale("log")
#plt.title("Dedicated vs HADDOCK (Mode 1) for AmongNurse")
#plt.legend(loc=1)
#plt.savefig('amongNurseBTGraphM1R0I10' + ('AllSoln' if allSoln else 'OneSoln') + '.pdf')
#
#
#
#plt.figure(figsize=(8,5))
#plt.plot([0,64],[classicC1Time[allSoln],classicC1Time[allSoln]], color='#F5793A', linestyle='dotted', label='Classic, C-I')
#plt.plot([0,64],[classicC2Time[allSoln],classicC2Time[allSoln]], color='#A95AA1', linestyle='dotted', label='Classic, C-II')
#plt.plot([0,64],[classicC3Time[allSoln],classicC3Time[allSoln]], color='#85C0F9', linestyle='dotted', label='Classic, C-III')
#plt.plot([1,2,4,8,16,32,64], M1R0I10C1Time[allSoln], marker='o', markersize=4, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M1R0I10C2Time[allSoln], marker='^', markersize=4, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M1R0I10C3Time[allSoln], marker='s', markersize=4, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("CPU Time (seconds)")
#plt.title("Classic vs HADDOCK (Mode 1) for AmongNurse")
#plt.legend(loc=1)
#plt.savefig('amongNurseTimeGraphM1R0I10' + ('AllSoln' if allSoln else 'OneSoln') + '.pdf')
#
#
#
#plt.figure(figsize=(8,5))
#plt.plot([0,64],[classicC1Fail[allSoln]+1,classicC1Fail[allSoln]+1], color='#F5793A', linestyle='dotted', label='Classic, C-I')
#plt.plot([0,64],[classicC2Fail[allSoln]+1,classicC2Fail[allSoln]+1], color='#A95AA1', linestyle='dotted', label='Classic, C-II')
#plt.plot([0,64],[classicC3Fail[allSoln]+1,classicC3Fail[allSoln]+1], color='#85C0F9', linestyle='dotted', label='Classic, C-III')
#plt.plot([1,2,4,8,16,32,64], dC1[allSoln]+1, marker='o', markersize=4, linestyle='dashed', color='#F5793A', label='Dedicated, C-I')
#plt.plot([1,2,4,8,16,32,64], dC2[allSoln]+1, marker='^', markersize=4, linestyle='dashed', color='#A95AA1', label='Dedicated, C-II')
#plt.plot([1,2,4,8,16,32,64], dC3[allSoln]+1, marker='s', markersize=4, linestyle='dashed', color='#85C0F9', label='Dedicated, C-III')
#plt.plot([1,2,4,8,16,32,64], M3R0I10C1Fail[allSoln]+1, marker='o', markersize=4, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M3R0I10C2Fail[allSoln]+1, marker='^', markersize=4, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M3R0I10C3Fail[allSoln]+1, marker='s', markersize=4, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("Backtracks")
#plt.yscale("log")
#plt.title("Dedicated vs HADDOCK (Mode 3) for AmongNurse")
#plt.legend(loc=1)
#plt.savefig('amongNurseBTGraphM3R0I10' + ('AllSoln' if allSoln else 'OneSoln') + '.pdf')
#
#
#plt.figure(figsize=(8,5))
#plt.plot([0,64],[classicC1Time[allSoln],classicC1Time[allSoln]], color='#F5793A', linestyle='dotted', label='Classic, C-I')
#plt.plot([0,64],[classicC2Time[allSoln],classicC2Time[allSoln]], color='#A95AA1', linestyle='dotted', label='Classic, C-II')
#plt.plot([0,64],[classicC3Time[allSoln],classicC3Time[allSoln]], color='#85C0F9', linestyle='dotted', label='Classic, C-III')
#plt.plot([1,2,4,8,16,32,64], M3R0I10C1Time[allSoln], marker='o', markersize=4, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M3R0I10C2Time[allSoln], marker='^', markersize=4, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M3R0I10C3Time[allSoln], marker='s', markersize=4, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("CPU Time (seconds)")
#plt.title("Classic vs HADDOCK (Mode 3) for AmongNurse")
#plt.legend(loc=1)
#plt.savefig('amongNurseTimeGraphM3R0I10' + ('AllSoln' if allSoln else 'OneSoln') + '.pdf')
#
#
#
#
#
#
#plt.figure(figsize=(8,5))
##plt.plot([0,64],[classicC1H40Fail[allSoln]+1,classicC1H40Fail[allSoln]+1], color='#F5793A', linestyle='dotted', label='Classic, C-I')
##plt.plot([0,64],[classicC2H40Fail[allSoln]+1,classicC2H40Fail[allSoln]+1], color='#A95AA1', linestyle='dotted', label='Classic, C-II')
##plt.plot([0,64],[classicC3H40Fail[allSoln]+1,classicC3H40Fail[allSoln]+1], color='#85C0F9', linestyle='dotted', label='Classic, C-III')
#plt.plot([1,2,4,8,16,32,64], dC1H40[allSoln]+1, marker='o', markersize=6, linestyle='dashed', color='#F5793A', label='Dedicated, C-I')
#plt.plot([1,2,4,8,16,32,64], dC2H40[allSoln]+1, marker='^', markersize=6, linestyle='dashed', color='#A95AA1', label='Dedicated, C-II')
#plt.plot([1,2,4,8,16,32,64], dC3H40[allSoln]+1, marker='s', markersize=6, linestyle='dashed', color='#85C0F9', label='Dedicated, C-III')
#plt.plot([1,2,4,8,16,32,64], M1R0I5C1H40Fail[allSoln]+1, marker='o', markersize=6, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M1R0I5C2H40Fail[allSoln]+1, marker='^', markersize=8, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M1R0I5C3H40Fail[allSoln]+1, marker='s', markersize=8, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("Backtracks")
#plt.yscale("log")
#plt.title("Dedicated vs HADDOCK")
#plt.legend(loc=1)
#plt.savefig('amongNurseBTGraphM1R0I5' + ('AllSoln' if allSoln else 'OneSoln') + 'H40NoClassic.pdf')
#
#
#plt.figure(figsize=(8,5))
##plt.plot([0,64],[classicC1H40Fail[allSoln]+1,classicC1H40Fail[allSoln]+1], color='#F5793A', linestyle='dotted', label='Classic, C-I')
##plt.plot([0,64],[classicC2H40Fail[allSoln]+1,classicC2H40Fail[allSoln]+1], color='#A95AA1', linestyle='dotted', label='Classic, C-II')
##plt.plot([0,64],[classicC3H40Fail[allSoln]+1,classicC3H40Fail[allSoln]+1], color='#85C0F9', linestyle='dotted', label='Classic, C-III')
#plt.plot([1,2,4,8,16,32,64], dC1H40[allSoln]+1, marker='o', markersize=6, linestyle='dashed', color='#F5793A', label='Dedicated, C-I')
#plt.plot([1,2,4,8,16,32,64], dC2H40[allSoln]+1, marker='^', markersize=6, linestyle='dashed', color='#A95AA1', label='Dedicated, C-II')
#plt.plot([1,2,4,8,16,32,64], dC3H40[allSoln]+1, marker='s', markersize=6, linestyle='dashed', color='#85C0F9', label='Dedicated, C-III')
#plt.plot([1,2,4,8,16,32,64], M1RMAXI5C1H40Fail[allSoln]+1, marker='o', markersize=6, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M1RMAXI5C2H40Fail[allSoln]+1, marker='^', markersize=6, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M1RMAXI5C3H40Fail[allSoln]+1, marker='s', markersize=6, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("Backtracks")
#plt.yscale("log")
#plt.title("Dedicated vs HADDOCK")
#plt.legend(loc=1)
#plt.savefig('amongNurseBTGraphM1RMAXI5' + ('AllSoln' if allSoln else 'OneSoln') + 'H40NoClassic.pdf')
#
#
#
#plt.figure(figsize=(8,5))
#plt.plot([0,64],[classicC1H40Time[allSoln],classicC1H40Time[allSoln]], color='#F5793A', linestyle='dotted', label='Classic, C-I')
#plt.plot([0,64],[classicC2H40Time[allSoln],classicC2H40Time[allSoln]], color='#A95AA1', linestyle='dotted', label='Classic, C-II')
#plt.plot([0,64],[classicC3H40Time[allSoln],classicC3H40Time[allSoln]], color='#85C0F9', linestyle='dotted', label='Classic, C-III')
#plt.plot([1,2,4,8,16,32,64], dC1H40Time[allSoln], marker='o', markersize=8, linestyle='dashed', color='#F5793A', label='Dedicated, C-I')
#plt.plot([1,2,4,8,16,32,64], dC2H40Time[allSoln], marker='^', markersize=8, linestyle='dashed', color='#A95AA1', label='Dedicated, C-II')
#plt.plot([1,2,4,8,16,32,64], dC3H40Time[allSoln], marker='s', markersize=8, linestyle='dashed', color='#85C0F9', label='Dedicated, C-III')
#plt.plot([1,2,4,8,16,32,64], M1R0I5C1H40Time[allSoln], marker='o', markersize=8, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M1R0I5C2H40Time[allSoln], marker='^', markersize=8, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M1R0I5C3H40Time[allSoln], marker='s', markersize=8, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("CPU Time (seconds)")
#plt.yscale("log")
#plt.title("Classic vs HADDOCK (Mode 1) for AmongNurse")
##plt.legend(loc=1)
#plt.savefig('amongNurseTimeGraphM1R0I5' + ('AllSoln' if allSoln else 'OneSoln') + 'H40.png')
#
#
#
#
#
#plt.figure(figsize=(8,5))
#plt.plot([0,64],[classicC1H40Fail[allSoln]+1,classicC1H40Fail[allSoln]+1], color='#F5793A', linestyle='dotted', label='Classic, C-I')
#plt.plot([0,64],[classicC2H40Fail[allSoln]+1,classicC2H40Fail[allSoln]+1], color='#A95AA1', linestyle='dotted', label='Classic, C-II')
#plt.plot([0,64],[classicC3H40Fail[allSoln]+1,classicC3H40Fail[allSoln]+1], color='#85C0F9', linestyle='dotted', label='Classic, C-III')
#plt.plot([1,2,4,8,16,32,64], dC1H40[allSoln]+1, marker='o', markersize=4, linestyle='dashed', color='#F5793A', label='Dedicated, C-I')
#plt.plot([1,2,4,8,16,32,64], dC2H40[allSoln]+1, marker='^', markersize=4, linestyle='dashed', color='#A95AA1', label='Dedicated, C-II')
#plt.plot([1,2,4,8,16,32,64], dC3H40[allSoln]+1, marker='s', markersize=4, linestyle='dashed', color='#85C0F9', label='Dedicated, C-III')
#plt.plot([1,2,4,8,16,32,64], M3R0I5C1H40Fail[allSoln]+1, marker='o', markersize=4, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M3R0I5C2H40Fail[allSoln]+1, marker='^', markersize=4, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M3R0I5C3H40Fail[allSoln]+1, marker='s', markersize=4, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("Backtracks")
#plt.yscale("log")
#plt.title("Dedicated vs HADDOCK (Mode 3) for AmongNurse")
#plt.legend(loc=1)
#plt.savefig('amongNurseBTGraphM3R0I5' + ('AllSoln' if allSoln else 'OneSoln') + 'H40.pdf')
#
#plt.figure(figsize=(8,5))
#plt.plot([0,64],[classicC1H40Time[allSoln],classicC1H40Time[allSoln]], color='#F5793A', linestyle='dotted', label='Classic, C-I')
#plt.plot([0,64],[classicC2H40Time[allSoln],classicC2H40Time[allSoln]], color='#A95AA1', linestyle='dotted', label='Classic, C-II')
#plt.plot([0,64],[classicC3H40Time[allSoln],classicC3H40Time[allSoln]], color='#85C0F9', linestyle='dotted', label='Classic, C-III')
#plt.plot([1,2,4,8,16,32,64], dC1H40Time[allSoln], marker='o', markersize=4, linestyle='dashed', color='#F5793A', label='Dedicated, C-I')
#plt.plot([1,2,4,8,16,32,64], dC2H40Time[allSoln], marker='^', markersize=4, linestyle='dashed', color='#A95AA1', label='Dedicated, C-II')
#plt.plot([1,2,4,8,16,32,64], dC3H40Time[allSoln], marker='s', markersize=4, linestyle='dashed', color='#85C0F9', label='Dedicated, C-III')
#plt.plot([1,2,4,8,16,32,64], M3R0I5C1H40Time[allSoln], marker='o', markersize=4, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M3R0I5C2H40Time[allSoln], marker='^', markersize=4, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M3R0I5C3H40Time[allSoln], marker='s', markersize=4, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("CPU Time (seconds)")
#plt.yscale("log")
#plt.title("Classic vs HADDOCK (Mode 3) for AmongNurse")
##plt.legend(loc=1)
#plt.savefig('amongNurseTimeGraphM3R0I5' + ('AllSoln' if allSoln else 'OneSoln') + 'H40.pdf')
#
#
#
#
#
#plt.figure(figsize=(8,5))
#plt.plot([0,64],[classicC1H40Fail[allSoln]+1,classicC1H40Fail[allSoln]+1], color='#F5793A', linestyle='dotted', label='Classic, C-I')
#plt.plot([0,64],[classicC2H40Fail[allSoln]+1,classicC2H40Fail[allSoln]+1], color='#A95AA1', linestyle='dotted', label='Classic, C-II')
#plt.plot([0,64],[classicC3H40Fail[allSoln]+1,classicC3H40Fail[allSoln]+1], color='#85C0F9', linestyle='dotted', label='Classic, C-III')
#plt.plot([1,2,4,8,16,32,64], dC1H40[allSoln]+1, marker='o', markersize=4, linestyle='dashed', color='#F5793A', label='Dedicated, C-I')
#plt.plot([1,2,4,8,16,32,64], dC2H40[allSoln]+1, marker='^', markersize=4, linestyle='dashed', color='#A95AA1', label='Dedicated, C-II')
#plt.plot([1,2,4,8,16,32,64], dC3H40[allSoln]+1, marker='s', markersize=4, linestyle='dashed', color='#85C0F9', label='Dedicated, C-III')
#plt.plot([1,2,4,8,16,32,64], M1R5I5C1H40Fail[allSoln]+1, marker='o', markersize=4, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M1R5I5C2H40Fail[allSoln]+1, marker='^', markersize=4, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M1R5I5C3H40Fail[allSoln]+1, marker='s', markersize=4, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("Backtracks")
#plt.yscale("log")
#plt.title("Dedicated vs HADDOCK (Reboot 5, Mode 1) for AmongNurse")
#plt.legend(loc=1)
#plt.savefig('amongNurseBTGraphM1R5I5' + ('AllSoln' if allSoln else 'OneSoln') + 'H40.pdf')
#
#
#plt.figure(figsize=(8,5))
#plt.plot([0,64],[classicC1H40Time[allSoln],classicC1H40Time[allSoln]], color='#F5793A', linestyle='dotted', label='Classic, C-I')
#plt.plot([0,64],[classicC2H40Time[allSoln],classicC2H40Time[allSoln]], color='#A95AA1', linestyle='dotted', label='Classic, C-II')
#plt.plot([0,64],[classicC3H40Time[allSoln],classicC3H40Time[allSoln]], color='#85C0F9', linestyle='dotted', label='Classic, C-III')
#plt.plot([1,2,4,8,16,32,64], dC1H40Time[allSoln], marker='o', markersize=4, linestyle='dashed', color='#F5793A', label='Dedicated, C-I')
#plt.plot([1,2,4,8,16,32,64], dC2H40Time[allSoln], marker='^', markersize=4, linestyle='dashed', color='#A95AA1', label='Dedicated, C-II')
#plt.plot([1,2,4,8,16,32,64], dC3H40Time[allSoln], marker='s', markersize=4, linestyle='dashed', color='#85C0F9', label='Dedicated, C-III')
#plt.plot([1,2,4,8,16,32,64], M1R5I5C1H40Time[allSoln], marker='o', markersize=4, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M1R5I5C2H40Time[allSoln], marker='^', markersize=4, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M1R5I5C3H40Time[allSoln], marker='s', markersize=4, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("CPU Time (seconds)")
#plt.yscale("log")
#plt.title("Classic vs HADDOCK (Reboot 5, Mode 1) for AmongNurse")
##plt.legend(loc=1)
#plt.savefig('amongNurseTimeGraphM1R5I5' + ('AllSoln' if allSoln else 'OneSoln') + 'H40.pdf')
#
#
#
#plt.figure(figsize=(8,5))
#plt.plot([0,64],[classicC1H40Fail[allSoln]+1,classicC1H40Fail[allSoln]+1], color='#F5793A', linestyle='dotted', label='Classic, C-I')
#plt.plot([0,64],[classicC2H40Fail[allSoln]+1,classicC2H40Fail[allSoln]+1], color='#A95AA1', linestyle='dotted', label='Classic, C-II')
#plt.plot([0,64],[classicC3H40Fail[allSoln]+1,classicC3H40Fail[allSoln]+1], color='#85C0F9', linestyle='dotted', label='Classic, C-III')
#plt.plot([1,2,4,8,16,32,64], dC1H40[allSoln]+1, marker='o', markersize=4, linestyle='dashed', color='#F5793A', label='Dedicated, C-I')
#plt.plot([1,2,4,8,16,32,64], dC2H40[allSoln]+1, marker='^', markersize=4, linestyle='dashed', color='#A95AA1', label='Dedicated, C-II')
#plt.plot([1,2,4,8,16,32,64], dC3H40[allSoln]+1, marker='s', markersize=4, linestyle='dashed', color='#85C0F9', label='Dedicated, C-III')
#plt.plot([1,2,4,8,16,32,64], M3R5I5C1H40Fail[allSoln]+1, marker='o', markersize=4, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M3R5I5C2H40Fail[allSoln]+1, marker='^', markersize=4, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M3R5I5C3H40Fail[allSoln]+1, marker='s', markersize=4, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("Backtracks")
#plt.yscale("log")
#plt.title("Dedicated vs HADDOCK (Reboot 5, Mode 3) for AmongNurse")
#plt.legend(loc=1)
#plt.savefig('amongNurseBTGraphM3R5I5' + ('AllSoln' if allSoln else 'OneSoln') + 'H40.pdf')
#
#plt.figure(figsize=(8,5))
#plt.plot([0,64],[classicC1H40Time[allSoln],classicC1H40Time[allSoln]], color='#F5793A', linestyle='dotted', label='Classic, C-I')
#plt.plot([0,64],[classicC2H40Time[allSoln],classicC2H40Time[allSoln]], color='#A95AA1', linestyle='dotted', label='Classic, C-II')
#plt.plot([0,64],[classicC3H40Time[allSoln],classicC3H40Time[allSoln]], color='#85C0F9', linestyle='dotted', label='Classic, C-III')
#plt.plot([1,2,4,8,16,32,64], dC1H40Time[allSoln], marker='o', markersize=4, linestyle='dashed', color='#F5793A', label='Dedicated, C-I')
#plt.plot([1,2,4,8,16,32,64], dC2H40Time[allSoln], marker='^', markersize=4, linestyle='dashed', color='#A95AA1', label='Dedicated, C-II')
#plt.plot([1,2,4,8,16,32,64], dC3H40Time[allSoln], marker='s', markersize=4, linestyle='dashed', color='#85C0F9', label='Dedicated, C-III')
#plt.plot([1,2,4,8,16,32,64], M3R5I5C1H40Time[allSoln], marker='o', markersize=4, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M3R5I5C2H40Time[allSoln], marker='^', markersize=4, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M3R5I5C3H40Time[allSoln], marker='s', markersize=4, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("CPU Time (seconds)")
#plt.yscale("log")
#plt.title("Classic vs HADDOCK (Reboot 5, Mode 3) for AmongNurse")
##plt.legend(loc=1)
#plt.savefig('amongNurseTimeGraphM3R5I5' + ('AllSoln' if allSoln else 'OneSoln') + 'H40.pdf')
#
#
#
#
#
#
#
#
#plt.figure(figsize=(8,5))
#plt.plot([0,64],[classicC1H40Fail[allSoln]+1,classicC1H40Fail[allSoln]+1], color='#F5793A', linestyle=(0, (1, 10)), label='Classic, C-I')
#plt.plot([0,64],[classicC2H40Fail[allSoln]+1,classicC2H40Fail[allSoln]+1], color='#A95AA1', linestyle=(0, (3, 10, 1, 10)), label='Classic, C-II')
#plt.plot([0,64],[classicC3H40Fail[allSoln]+1,classicC3H40Fail[allSoln]+1], color='#85C0F9', linestyle=(0, (3, 5, 1, 5)), label='Classic, C-III')
#plt.plot([1,2,4,8,16,32,64], dC1H40[allSoln]+1, marker='o', markersize=6, linestyle='dashed', color='#F5793A', label='Dedicated, C-I')
#plt.plot([1,2,4,8,16,32,64], dC2H40[allSoln]+1, marker='^', markersize=6, linestyle='dashed', color='#A95AA1', label='Dedicated, C-II')
#plt.plot([1,2,4,8,16,32,64], dC3H40[allSoln]+1, marker='s', markersize=6, linestyle='dashed', color='#85C0F9', label='Dedicated, C-III')
#plt.plot([1,2,4,8,16,32,64], M1R10I5C1H40Fail[allSoln]+1, marker='o', markersize=6, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M1R10I5C2H40Fail[allSoln]+1, marker='^', markersize=6, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M1R10I5C3H40Fail[allSoln]+1, marker='s', markersize=6, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("Backtracks")
#plt.yscale("log")
#plt.title("Classic vs Dedicated vs HADDOCK")
#plt.legend(loc=1)
#plt.savefig('amongNurseBTGraphM1R10I5' + ('AllSoln' if allSoln else 'OneSoln') + 'H40.pdf')
#
#
#plt.figure(figsize=(8,5))
#plt.plot([0,64],[classicC1H40Time[allSoln],classicC1H40Time[allSoln]], color='#F5793A', linestyle=(0, (1, 10)), label='Classic, C-I')
#plt.plot([0,64],[classicC2H40Time[allSoln],classicC2H40Time[allSoln]], color='#A95AA1', linestyle=(0, (3, 10, 1, 10)), label='Classic, C-II')
#plt.plot([0,64],[classicC3H40Time[allSoln],classicC3H40Time[allSoln]], color='#85C0F9', linestyle=(0, (3, 5, 1, 5)), label='Classic, C-III')
#plt.plot([1,2,4,8,16,32,64], dC1H40Time[allSoln], marker='o', markersize=6, linestyle='dashed', color='#F5793A', label='Dedicated, C-I')
#plt.plot([1,2,4,8,16,32,64], dC2H40Time[allSoln], marker='^', markersize=6, linestyle='dashed', color='#A95AA1', label='Dedicated, C-II')
#plt.plot([1,2,4,8,16,32,64], dC3H40Time[allSoln], marker='s', markersize=6, linestyle='dashed', color='#85C0F9', label='Dedicated, C-III')
#plt.plot([1,2,4,8,16,32,64], M1R10I5C1H40Time[allSoln], marker='o', markersize=6, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M1R10I5C2H40Time[allSoln], marker='^', markersize=6, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M1R10I5C3H40Time[allSoln], marker='s', markersize=6, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("CPU Time (seconds)")
#plt.yscale("log")
#plt.title("Classic vs Dedicated vs HADDOCK")
##plt.legend(loc=1)
#plt.savefig('amongNurseTimeGraphM1R10I5' + ('AllSoln' if allSoln else 'OneSoln') + 'H40.pdf')
#
#
#
#plt.figure(figsize=(8,5))
#plt.plot([0,64],[classicC1H40Fail[allSoln]+1,classicC1H40Fail[allSoln]+1], color='#F5793A', linestyle=(0, (1, 10)), label='Classic, C-I')
#plt.plot([0,64],[classicC2H40Fail[allSoln]+1,classicC2H40Fail[allSoln]+1], color='#A95AA1', linestyle=(0, (3, 10, 1, 10)), label='Classic, C-II')
#plt.plot([0,64],[classicC3H40Fail[allSoln]+1,classicC3H40Fail[allSoln]+1], color='#85C0F9', linestyle=(0, (3, 5, 1, 5)), label='Classic, C-III')
#plt.plot([1,2,4,8,16,32,64], dC1H40[allSoln]+1, marker='o', markersize=6, linestyle='dashed', color='#F5793A', label='Dedicated, C-I')
#plt.plot([1,2,4,8,16,32,64], dC2H40[allSoln]+1, marker='^', markersize=6, linestyle='dashed', color='#A95AA1', label='Dedicated, C-II')
#plt.plot([1,2,4,8,16,32,64], dC3H40[allSoln]+1, marker='s', markersize=6, linestyle='dashed', color='#85C0F9', label='Dedicated, C-III')
#plt.plot([1,2,4,8,16,32,64], M3R10I5C1H40Fail[allSoln]+1, marker='o', markersize=6, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M3R10I5C2H40Fail[allSoln]+1, marker='^', markersize=6, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M3R10I5C3H40Fail[allSoln]+1, marker='s', markersize=6, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("Backtracks")
#plt.yscale("log")
#plt.title("Classic vs Dedicated vs HADDOCK")
#plt.legend(loc=1)
#plt.savefig('amongNurseBTGraphM3R10I5' + ('AllSoln' if allSoln else 'OneSoln') + 'H40.pdf')
#
#plt.figure(figsize=(8,5))
#plt.plot([0,64],[classicC1H40Time[allSoln],classicC1H40Time[allSoln]], color='#F5793A', linestyle=(0, (1, 10)), label='Classic, C-I')
#plt.plot([0,64],[classicC2H40Time[allSoln],classicC2H40Time[allSoln]], color='#A95AA1', linestyle=(0, (3, 10, 1, 10)), label='Classic, C-II')
#plt.plot([0,64],[classicC3H40Time[allSoln],classicC3H40Time[allSoln]], color='#85C0F9', linestyle=(0, (3, 5, 1, 5)), label='Classic, C-III')
#plt.plot([1,2,4,8,16,32,64], dC1H40Time[allSoln], marker='o', markersize=6, linestyle='dashed', color='#F5793A', label='Dedicated, C-I')
#plt.plot([1,2,4,8,16,32,64], dC2H40Time[allSoln], marker='^', markersize=6, linestyle='dashed', color='#A95AA1', label='Dedicated, C-II')
#plt.plot([1,2,4,8,16,32,64], dC3H40Time[allSoln], marker='s', markersize=6, linestyle='dashed', color='#85C0F9', label='Dedicated, C-III')
#plt.plot([1,2,4,8,16,32,64], M3R10I5C1H40Time[allSoln], marker='o', markersize=6, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M3R10I5C2H40Time[allSoln], marker='^', markersize=6, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M3R10I5C3H40Time[allSoln], marker='s', markersize=6, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("CPU Time (seconds)")
#plt.yscale("log")
#plt.title("Classic vs Dedicated vs HADDOCK")
##plt.legend(loc=1)
#plt.savefig('amongNurseTimeGraphM3R10I5' + ('AllSoln' if allSoln else 'OneSoln') + 'H40.pdf')
#
#
#
#
#
#
#
#plt.figure(figsize=(8,5))
#plt.plot([0,64],[classicC1H40Fail[allSoln]+1,classicC1H40Fail[allSoln]+1], color='#F5793A', linestyle='dotted', label='Classic, C-I')
#plt.plot([0,64],[classicC2H40Fail[allSoln]+1,classicC2H40Fail[allSoln]+1], color='#A95AA1', linestyle='dotted', label='Classic, C-II')
#plt.plot([0,64],[classicC3H40Fail[allSoln]+1,classicC3H40Fail[allSoln]+1], color='#85C0F9', linestyle='dotted', label='Classic, C-III')
#plt.plot([1,2,4,8,16,32,64], dC1H40[allSoln]+1, marker='o', markersize=4, linestyle='dashed', color='#F5793A', label='Dedicated, C-I')
#plt.plot([1,2,4,8,16,32,64], dC2H40[allSoln]+1, marker='^', markersize=4, linestyle='dashed', color='#A95AA1', label='Dedicated, C-II')
#plt.plot([1,2,4,8,16,32,64], dC3H40[allSoln]+1, marker='s', markersize=4, linestyle='dashed', color='#85C0F9', label='Dedicated, C-III')
#plt.plot([1,2,4,8,16,32,64], M1R20I5C1H40Fail[allSoln]+1, marker='o', markersize=4, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M1R20I5C2H40Fail[allSoln]+1, marker='^', markersize=4, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M1R20I5C3H40Fail[allSoln]+1, marker='s', markersize=4, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("Backtracks")
#plt.yscale("log")
#plt.title("Dedicated vs HADDOCK (Reboot 20, Mode 1) for AmongNurse")
#plt.legend(loc=1)
#plt.savefig('amongNurseBTGraphM1R20I5' + ('AllSoln' if allSoln else 'OneSoln') + 'H40.pdf')
#
#
#plt.figure(figsize=(8,5))
#plt.plot([0,64],[classicC1H40Time[allSoln],classicC1H40Time[allSoln]], color='#F5793A', linestyle='dotted', label='Classic, C-I')
#plt.plot([0,64],[classicC2H40Time[allSoln],classicC2H40Time[allSoln]], color='#A95AA1', linestyle='dotted', label='Classic, C-II')
#plt.plot([0,64],[classicC3H40Time[allSoln],classicC3H40Time[allSoln]], color='#85C0F9', linestyle='dotted', label='Classic, C-III')
#plt.plot([1,2,4,8,16,32,64], dC1H40Time[allSoln], marker='o', markersize=4, linestyle='dashed', color='#F5793A', label='Dedicated, C-I')
#plt.plot([1,2,4,8,16,32,64], dC2H40Time[allSoln], marker='^', markersize=4, linestyle='dashed', color='#A95AA1', label='Dedicated, C-II')
#plt.plot([1,2,4,8,16,32,64], dC3H40Time[allSoln], marker='s', markersize=4, linestyle='dashed', color='#85C0F9', label='Dedicated, C-III')
#plt.plot([1,2,4,8,16,32,64], M1R20I5C1H40Time[allSoln], marker='o', markersize=4, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M1R20I5C2H40Time[allSoln], marker='^', markersize=4, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M1R20I5C3H40Time[allSoln], marker='s', markersize=4, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("CPU Time (seconds)")
#plt.yscale("log")
#plt.title("Classic vs HADDOCK (Reboot 20, Mode 1) for AmongNurse")
##plt.legend(loc=1)
#plt.savefig('amongNurseTimeGraphM1R20I5' + ('AllSoln' if allSoln else 'OneSoln') + 'H40.pdf')
#
#
#
#plt.figure(figsize=(8,5))
#plt.plot([0,64],[classicC1H40Fail[allSoln]+1,classicC1H40Fail[allSoln]+1], color='#F5793A', linestyle='dotted', label='Classic, C-I')
#plt.plot([0,64],[classicC2H40Fail[allSoln]+1,classicC2H40Fail[allSoln]+1], color='#A95AA1', linestyle='dotted', label='Classic, C-II')
#plt.plot([0,64],[classicC3H40Fail[allSoln]+1,classicC3H40Fail[allSoln]+1], color='#85C0F9', linestyle='dotted', label='Classic, C-III')
#plt.plot([1,2,4,8,16,32,64], dC1H40[allSoln]+1, marker='o', markersize=4, linestyle='dashed', color='#F5793A', label='Dedicated, C-I')
#plt.plot([1,2,4,8,16,32,64], dC2H40[allSoln]+1, marker='^', markersize=4, linestyle='dashed', color='#A95AA1', label='Dedicated, C-II')
#plt.plot([1,2,4,8,16,32,64], dC3H40[allSoln]+1, marker='s', markersize=4, linestyle='dashed', color='#85C0F9', label='Dedicated, C-III')
#plt.plot([1,2,4,8,16,32,64], M3R20I5C1H40Fail[allSoln]+1, marker='o', markersize=4, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M3R20I5C2H40Fail[allSoln]+1, marker='^', markersize=4, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M3R20I5C3H40Fail[allSoln]+1, marker='s', markersize=4, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("Backtracks")
#plt.yscale("log")
#plt.title("Dedicated vs HADDOCK (Reboot 20, Mode 3) for AmongNurse")
#plt.legend(loc=1)
#plt.savefig('amongNurseBTGraphM3R20I5' + ('AllSoln' if allSoln else 'OneSoln') + 'H40.pdf')
#
#plt.figure(figsize=(8,5))
#plt.plot([0,64],[classicC1H40Time[allSoln],classicC1H40Time[allSoln]], color='#F5793A', linestyle='dotted', label='Classic, C-I')
#plt.plot([0,64],[classicC2H40Time[allSoln],classicC2H40Time[allSoln]], color='#A95AA1', linestyle='dotted', label='Classic, C-II')
#plt.plot([0,64],[classicC3H40Time[allSoln],classicC3H40Time[allSoln]], color='#85C0F9', linestyle='dotted', label='Classic, C-III')
#plt.plot([1,2,4,8,16,32,64], dC1H40Time[allSoln], marker='o', markersize=4, linestyle='dashed', color='#F5793A', label='Dedicated, C-I')
#plt.plot([1,2,4,8,16,32,64], dC2H40Time[allSoln], marker='^', markersize=4, linestyle='dashed', color='#A95AA1', label='Dedicated, C-II')
#plt.plot([1,2,4,8,16,32,64], dC3H40Time[allSoln], marker='s', markersize=4, linestyle='dashed', color='#85C0F9', label='Dedicated, C-III')
#plt.plot([1,2,4,8,16,32,64], M3R20I5C1H40Time[allSoln], marker='o', markersize=4, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M3R20I5C2H40Time[allSoln], marker='^', markersize=4, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M3R20I5C3H40Time[allSoln], marker='s', markersize=4, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("CPU Time (seconds)")
#plt.yscale("log")
#plt.title("Classic vs HADDOCK (Reboot 20, Mode 3) for AmongNurse")
##plt.legend(loc=1)
#plt.savefig('amongNurseTimeGraphM3R20I5' + ('AllSoln' if allSoln else 'OneSoln') + 'H40.pdf')





#plt.figure(figsize=(8,5))
#plt.plot([0,64],[classicC1H40Fail[allSoln]+1,classicC1H40Fail[allSoln]+1], color='#F5793A', linestyle='dotted', label='Classic, C-I')
#plt.plot([0,64],[classicC2H40Fail[allSoln]+1,classicC2H40Fail[allSoln]+1], color='#A95AA1', linestyle='dotted', label='Classic, C-II')
#plt.plot([0,64],[classicC3H40Fail[allSoln]+1,classicC3H40Fail[allSoln]+1], color='#85C0F9', linestyle='dotted', label='Classic, C-III')
#plt.plot([1,2,4,8,16,32,64], dC1H40[allSoln]+1, marker='o', markersize=4, linestyle='dashed', color='#F5793A', label='Dedicated, C-I')
#plt.plot([1,2,4,8,16,32,64], dC2H40[allSoln]+1, marker='^', markersize=4, linestyle='dashed', color='#A95AA1', label='Dedicated, C-II')
#plt.plot([1,2,4,8,16,32,64], dC3H40[allSoln]+1, marker='s', markersize=4, linestyle='dashed', color='#85C0F9', label='Dedicated, C-III')
#plt.plot([1,2,4,8,16,32,64], M1R0I5C1H40Slack0Fail[allSoln]+1, marker='o', markersize=4, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M1R0I5C2H40Slack0Fail[allSoln]+1, marker='^', markersize=4, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M1R0I5C3H40Slack0Fail[allSoln]+1, marker='s', markersize=4, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("Backtracks")
#plt.yscale("log")
#plt.title("Dedicated vs HADDOCK (Reboot 0, Mode 1) for AmongNurse")
#plt.legend(loc=1)
#plt.savefig('amongNurseBTGraphM1R0I5Slack0' + ('AllSoln' if allSoln else 'OneSoln') + 'H40.pdf')
#
#
#plt.figure(figsize=(8,5))
#plt.plot([0,64],[classicC1H40Time[allSoln],classicC1H40Time[allSoln]], color='#F5793A', linestyle='dotted', label='Classic, C-I')
#plt.plot([0,64],[classicC2H40Time[allSoln],classicC2H40Time[allSoln]], color='#A95AA1', linestyle='dotted', label='Classic, C-II')
#plt.plot([0,64],[classicC3H40Time[allSoln],classicC3H40Time[allSoln]], color='#85C0F9', linestyle='dotted', label='Classic, C-III')
#plt.plot([1,2,4,8,16,32,64], dC1H40Time[allSoln], marker='o', markersize=4, linestyle='dashed', color='#F5793A', label='Dedicated, C-I')
#plt.plot([1,2,4,8,16,32,64], dC2H40Time[allSoln], marker='^', markersize=4, linestyle='dashed', color='#A95AA1', label='Dedicated, C-II')
#plt.plot([1,2,4,8,16,32,64], dC3H40Time[allSoln], marker='s', markersize=4, linestyle='dashed', color='#85C0F9', label='Dedicated, C-III')
#plt.plot([1,2,4,8,16,32,64], M1R0I5C1H40Slack0Time[allSoln], marker='o', markersize=4, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M1R0I5C2H40Slack0Time[allSoln], marker='^', markersize=4, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M1R0I5C3H40Slack0Time[allSoln], marker='s', markersize=4, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("CPU Time (seconds)")
#plt.yscale("log")
#plt.title("Classic vs HADDOCK (Reboot 0, Mode 1) for AmongNurse")
##plt.legend(loc=1)
#plt.savefig('amongNurseTimeGraphM1R0I5Slack0' + ('AllSoln' if allSoln else 'OneSoln') + 'H40.pdf')
#
#
#
#
#
#
#
#plt.figure(figsize=(8,5))
#plt.plot([0,64],[classicC1H40Fail[allSoln]+1,classicC1H40Fail[allSoln]+1], color='#F5793A', linestyle='dotted', label='Classic, C-I')
#plt.plot([0,64],[classicC2H40Fail[allSoln]+1,classicC2H40Fail[allSoln]+1], color='#A95AA1', linestyle='dotted', label='Classic, C-II')
#plt.plot([0,64],[classicC3H40Fail[allSoln]+1,classicC3H40Fail[allSoln]+1], color='#85C0F9', linestyle='dotted', label='Classic, C-III')
#plt.plot([1,2,4,8,16,32,64], dC1H40[allSoln]+1, marker='o', markersize=4, linestyle='dashed', color='#F5793A', label='Dedicated, C-I')
#plt.plot([1,2,4,8,16,32,64], dC2H40[allSoln]+1, marker='^', markersize=4, linestyle='dashed', color='#A95AA1', label='Dedicated, C-II')
#plt.plot([1,2,4,8,16,32,64], dC3H40[allSoln]+1, marker='s', markersize=4, linestyle='dashed', color='#85C0F9', label='Dedicated, C-III')
#plt.plot([1,2,4,8,16,32,64], M1R0I5C1H40Slack1Fail[allSoln]+1, marker='o', markersize=4, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M1R0I5C2H40Slack1Fail[allSoln]+1, marker='^', markersize=4, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M1R0I5C3H40Slack1Fail[allSoln]+1, marker='s', markersize=4, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("Backtracks")
#plt.yscale("log")
#plt.title("Dedicated vs HADDOCK (Reboot 0, Mode 1) for AmongNurse")
#plt.legend(loc=1)
#plt.savefig('amongNurseBTGraphM1R0I5Slack1' + ('AllSoln' if allSoln else 'OneSoln') + 'H40.pdf')
#
#
#plt.figure(figsize=(8,5))
#plt.plot([0,64],[classicC1H40Time[allSoln],classicC1H40Time[allSoln]], color='#F5793A', linestyle='dotted', label='Classic, C-I')
#plt.plot([0,64],[classicC2H40Time[allSoln],classicC2H40Time[allSoln]], color='#A95AA1', linestyle='dotted', label='Classic, C-II')
#plt.plot([0,64],[classicC3H40Time[allSoln],classicC3H40Time[allSoln]], color='#85C0F9', linestyle='dotted', label='Classic, C-III')
#plt.plot([1,2,4,8,16,32,64], dC1H40Time[allSoln], marker='o', markersize=4, linestyle='dashed', color='#F5793A', label='Dedicated, C-I')
#plt.plot([1,2,4,8,16,32,64], dC2H40Time[allSoln], marker='^', markersize=4, linestyle='dashed', color='#A95AA1', label='Dedicated, C-II')
#plt.plot([1,2,4,8,16,32,64], dC3H40Time[allSoln], marker='s', markersize=4, linestyle='dashed', color='#85C0F9', label='Dedicated, C-III')
#plt.plot([1,2,4,8,16,32,64], M1R0I5C1H40Slack1Time[allSoln], marker='o', markersize=4, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M1R0I5C2H40Slack1Time[allSoln], marker='^', markersize=4, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M1R0I5C3H40Slack1Time[allSoln], marker='s', markersize=4, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("CPU Time (seconds)")
#plt.yscale("log")
#plt.title("Classic vs HADDOCK (Reboot 0, Mode 1) for AmongNurse")
##plt.legend(loc=1)
#plt.savefig('amongNurseTimeGraphM1R0I5Slack1' + ('AllSoln' if allSoln else 'OneSoln') + 'H40.pdf')
#
#
#
#plt.figure(figsize=(8,5))
#plt.plot([0,64],[classicC1H40Fail[allSoln]+1,classicC1H40Fail[allSoln]+1], color='#F5793A', linestyle='dotted', label='Classic, C-I')
#plt.plot([0,64],[classicC2H40Fail[allSoln]+1,classicC2H40Fail[allSoln]+1], color='#A95AA1', linestyle='dotted', label='Classic, C-II')
#plt.plot([0,64],[classicC3H40Fail[allSoln]+1,classicC3H40Fail[allSoln]+1], color='#85C0F9', linestyle='dotted', label='Classic, C-III')
#plt.plot([1,2,4,8,16,32,64], dC1H40[allSoln]+1, marker='o', markersize=4, linestyle='dashed', color='#F5793A', label='Dedicated, C-I')
#plt.plot([1,2,4,8,16,32,64], dC2H40[allSoln]+1, marker='^', markersize=4, linestyle='dashed', color='#A95AA1', label='Dedicated, C-II')
#plt.plot([1,2,4,8,16,32,64], dC3H40[allSoln]+1, marker='s', markersize=4, linestyle='dashed', color='#85C0F9', label='Dedicated, C-III')
#plt.plot([1,2,4,8,16,32,64], M1R5I5C1H40Slack1Fail[allSoln]+1, marker='o', markersize=4, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M1R5I5C2H40Slack1Fail[allSoln]+1, marker='^', markersize=4, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M1R5I5C3H40Slack1Fail[allSoln]+1, marker='s', markersize=4, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("Backtracks")
#plt.yscale("log")
#plt.title("Dedicated vs HADDOCK (Reboot 5, Mode 1) for AmongNurse")
#plt.legend(loc=1)
#plt.savefig('amongNurseBTGraphM1R5I5Slack1' + ('AllSoln' if allSoln else 'OneSoln') + 'H40.pdf')
#
#
#plt.figure(figsize=(8,5))
#plt.plot([0,64],[classicC1H40Time[allSoln],classicC1H40Time[allSoln]], color='#F5793A', linestyle='dotted', label='Classic, C-I')
#plt.plot([0,64],[classicC2H40Time[allSoln],classicC2H40Time[allSoln]], color='#A95AA1', linestyle='dotted', label='Classic, C-II')
#plt.plot([0,64],[classicC3H40Time[allSoln],classicC3H40Time[allSoln]], color='#85C0F9', linestyle='dotted', label='Classic, C-III')
#plt.plot([1,2,4,8,16,32,64], dC1H40Time[allSoln], marker='o', markersize=4, linestyle='dashed', color='#F5793A', label='Dedicated, C-I')
#plt.plot([1,2,4,8,16,32,64], dC2H40Time[allSoln], marker='^', markersize=4, linestyle='dashed', color='#A95AA1', label='Dedicated, C-II')
#plt.plot([1,2,4,8,16,32,64], dC3H40Time[allSoln], marker='s', markersize=4, linestyle='dashed', color='#85C0F9', label='Dedicated, C-III')
#plt.plot([1,2,4,8,16,32,64], M1R5I5C1H40Slack1Time[allSoln], marker='o', markersize=4, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M1R5I5C2H40Slack1Time[allSoln], marker='^', markersize=4, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M1R5I5C3H40Slack1Time[allSoln], marker='s', markersize=4, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("CPU Time (seconds)")
#plt.yscale("log")
#plt.title("Classic vs HADDOCK (Reboot 5, Mode 1) for AmongNurse")
##plt.legend(loc=1)
#plt.savefig('amongNurseTimeGraphM1R5I5Slack1' + ('AllSoln' if allSoln else 'OneSoln') + 'H40.pdf')
#
#
#
#
#plt.figure(figsize=(8,5))
#plt.plot([0,64],[classicC1H40Fail[allSoln]+1,classicC1H40Fail[allSoln]+1], color='#F5793A', linestyle='dotted', label='Classic, C-I')
#plt.plot([0,64],[classicC2H40Fail[allSoln]+1,classicC2H40Fail[allSoln]+1], color='#A95AA1', linestyle='dotted', label='Classic, C-II')
#plt.plot([0,64],[classicC3H40Fail[allSoln]+1,classicC3H40Fail[allSoln]+1], color='#85C0F9', linestyle='dotted', label='Classic, C-III')
#plt.plot([1,2,4,8,16,32,64], dC1H40[allSoln]+1, marker='o', markersize=4, linestyle='dashed', color='#F5793A', label='Dedicated, C-I')
#plt.plot([1,2,4,8,16,32,64], dC2H40[allSoln]+1, marker='^', markersize=4, linestyle='dashed', color='#A95AA1', label='Dedicated, C-II')
#plt.plot([1,2,4,8,16,32,64], dC3H40[allSoln]+1, marker='s', markersize=4, linestyle='dashed', color='#85C0F9', label='Dedicated, C-III')
#plt.plot([1,2,4,8,16,32,64], M1R0I5C1H40Slack2Fail[allSoln]+1, marker='o', markersize=4, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M1R0I5C2H40Slack2Fail[allSoln]+1, marker='^', markersize=4, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M1R0I5C3H40Slack2Fail[allSoln]+1, marker='s', markersize=4, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("Backtracks")
#plt.yscale("log")
#plt.title("Dedicated vs HADDOCK (Reboot 0, Mode 1) for AmongNurse")
#plt.legend(loc=1)
#plt.savefig('amongNurseBTGraphM1R0I5Slack2' + ('AllSoln' if allSoln else 'OneSoln') + 'H40.pdf')
#
#
#plt.figure(figsize=(8,5))
#plt.plot([0,64],[classicC1H40Time[allSoln],classicC1H40Time[allSoln]], color='#F5793A', linestyle='dotted', label='Classic, C-I')
#plt.plot([0,64],[classicC2H40Time[allSoln],classicC2H40Time[allSoln]], color='#A95AA1', linestyle='dotted', label='Classic, C-II')
#plt.plot([0,64],[classicC3H40Time[allSoln],classicC3H40Time[allSoln]], color='#85C0F9', linestyle='dotted', label='Classic, C-III')
#plt.plot([1,2,4,8,16,32,64], dC1H40Time[allSoln], marker='o', markersize=4, linestyle='dashed', color='#F5793A', label='Dedicated, C-I')
#plt.plot([1,2,4,8,16,32,64], dC2H40Time[allSoln], marker='^', markersize=4, linestyle='dashed', color='#A95AA1', label='Dedicated, C-II')
#plt.plot([1,2,4,8,16,32,64], dC3H40Time[allSoln], marker='s', markersize=4, linestyle='dashed', color='#85C0F9', label='Dedicated, C-III')
#plt.plot([1,2,4,8,16,32,64], M1R0I5C1H40Slack2Time[allSoln], marker='o', markersize=4, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M1R0I5C2H40Slack2Time[allSoln], marker='^', markersize=4, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M1R0I5C3H40Slack2Time[allSoln], marker='s', markersize=4, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("CPU Time (seconds)")
#plt.yscale("log")
#plt.title("Classic vs HADDOCK (Reboot 0, Mode 1) for AmongNurse")
##plt.legend(loc=1)
#plt.savefig('amongNurseTimeGraphM1R0I5Slack2' + ('AllSoln' if allSoln else 'OneSoln') + 'H40.pdf')





#plt.figure(figsize=(8,5))
#plt.plot([0,64],[classicC1H40Fail[allSoln]+1,classicC1H40Fail[allSoln]+1], color='#F5793A', linestyle=(0, (1, 10)), label='Classic, C-I')
#plt.plot([0,64],[classicC2H40Fail[allSoln]+1,classicC2H40Fail[allSoln]+1], color='#A95AA1', linestyle=(0, (3, 10, 1, 10)), label='Classic, C-II')
#plt.plot([0,64],[classicC3H40Fail[allSoln]+1,classicC3H40Fail[allSoln]+1], color='#85C0F9', linestyle=(0, (3, 5, 1, 5)), label='Classic, C-III')
#plt.plot([1,2,4,8,16,32,64], dC1H40[allSoln]+1, marker='o', markersize=6, linestyle='dashed', color='#F5793A', label='Dedicated, C-I')
#plt.plot([1,2,4,8,16,32,64], dC2H40[allSoln]+1, marker='^', markersize=6, linestyle='dashed', color='#A95AA1', label='Dedicated, C-II')
#plt.plot([1,2,4,8,16,32,64], dC3H40[allSoln]+1, marker='s', markersize=6, linestyle='dashed', color='#85C0F9', label='Dedicated, C-III')
#plt.plot([1,2,4,8,16,32,64], M1R0I5C1H40Split1Fail[allSoln]+1, marker='o', markersize=6, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M1R0I5C2H40Split1Fail[allSoln]+1, marker='^', markersize=6, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M1R0I5C3H40Split1Fail[allSoln]+1, marker='s', markersize=6, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("Backtracks")
#plt.yscale("log")
#plt.title("Classic vs Dedicated vs HADDOCK")
#plt.legend(loc=1)
#plt.savefig('amongNurseBTGraphM1R0I5Split1' + ('AllSoln' if allSoln else 'OneSoln') + 'H40.png')
#
#
#plt.figure(figsize=(8,5))
#plt.plot([0,64],[classicC1H40Time[allSoln],classicC1H40Time[allSoln]], color='#F5793A', linestyle=(0, (1, 10)), label='Classic, C-I')
#plt.plot([0,64],[classicC2H40Time[allSoln],classicC2H40Time[allSoln]], color='#A95AA1', linestyle=(0, (3, 10, 1, 10)), label='Classic, C-II')
#plt.plot([0,64],[classicC3H40Time[allSoln],classicC3H40Time[allSoln]], color='#85C0F9', linestyle=(0, (3, 5, 1, 5)), label='Classic, C-III')
#plt.plot([1,2,4,8,16,32,64], dC1H40Time[allSoln], marker='o', markersize=6, linestyle='dashed', color='#F5793A', label='Dedicated, C-I')
#plt.plot([1,2,4,8,16,32,64], dC2H40Time[allSoln], marker='^', markersize=6, linestyle='dashed', color='#A95AA1', label='Dedicated, C-II')
#plt.plot([1,2,4,8,16,32,64], dC3H40Time[allSoln], marker='s', markersize=6, linestyle='dashed', color='#85C0F9', label='Dedicated, C-III')
#plt.plot([1,2,4,8,16,32,64], M1R0I5C1H40Split1Time[allSoln], marker='o', markersize=6, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M1R0I5C2H40Split1Time[allSoln], marker='^', markersize=6, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M1R0I5C3H40Split1Time[allSoln], marker='s', markersize=6, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("CPU Time (seconds)")
#plt.yscale("log")
#plt.title("Classic vs Dedicated vs HADDOCK")
##plt.legend(loc=1)
#plt.savefig('amongNurseTimeGraphM1R0I5Split1' + ('AllSoln' if allSoln else 'OneSoln') + 'H40.png')
#
#
#
#plt.figure(figsize=(8,5))
#plt.plot([0,64],[classicC1H40Fail[allSoln]+1,classicC1H40Fail[allSoln]+1], color='#F5793A', linestyle=(0, (1, 10)), label='Classic, C-I')
#plt.plot([0,64],[classicC2H40Fail[allSoln]+1,classicC2H40Fail[allSoln]+1], color='#A95AA1', linestyle=(0, (3, 10, 1, 10)), label='Classic, C-II')
#plt.plot([0,64],[classicC3H40Fail[allSoln]+1,classicC3H40Fail[allSoln]+1], color='#85C0F9', linestyle=(0, (3, 5, 1, 5)), label='Classic, C-III')
#plt.plot([1,2,4,8,16,32,64], dC1H40[allSoln]+1, marker='o', markersize=6, linestyle='dashed', color='#F5793A', label='Dedicated, C-I')
#plt.plot([1,2,4,8,16,32,64], dC2H40[allSoln]+1, marker='^', markersize=6, linestyle='dashed', color='#A95AA1', label='Dedicated, C-II')
#plt.plot([1,2,4,8,16,32,64], dC3H40[allSoln]+1, marker='s', markersize=6, linestyle='dashed', color='#85C0F9', label='Dedicated, C-III')
#plt.plot([1,2,4,8,16,32,64], M1R10I5C1H40Split1Fail[allSoln]+1, marker='o', markersize=6, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M1R10I5C2H40Split1Fail[allSoln]+1, marker='^', markersize=6, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M1R10I5C3H40Split1Fail[allSoln]+1, marker='s', markersize=6, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("Backtracks")
#plt.yscale("log")
#plt.title("Classic vs Dedicated vs HADDOCK")
#plt.legend(loc=1)
#plt.savefig('amongNurseBTGraphM1R10I5Split1' + ('AllSoln' if allSoln else 'OneSoln') + 'H40.png')
#
#
#plt.figure(figsize=(8,5))
#plt.plot([0,64],[classicC1H40Time[allSoln],classicC1H40Time[allSoln]], color='#F5793A', linestyle=(0, (1, 10)), label='Classic, C-I')
#plt.plot([0,64],[classicC2H40Time[allSoln],classicC2H40Time[allSoln]], color='#A95AA1', linestyle=(0, (3, 10, 1, 10)), label='Classic, C-II')
#plt.plot([0,64],[classicC3H40Time[allSoln],classicC3H40Time[allSoln]], color='#85C0F9', linestyle=(0, (3, 5, 1, 5)), label='Classic, C-III')
#plt.plot([1,2,4,8,16,32,64], dC1H40Time[allSoln], marker='o', markersize=6, linestyle='dashed', color='#F5793A', label='Dedicated, C-I')
#plt.plot([1,2,4,8,16,32,64], dC2H40Time[allSoln], marker='^', markersize=6, linestyle='dashed', color='#A95AA1', label='Dedicated, C-II')
#plt.plot([1,2,4,8,16,32,64], dC3H40Time[allSoln], marker='s', markersize=6, linestyle='dashed', color='#85C0F9', label='Dedicated, C-III')
#plt.plot([1,2,4,8,16,32,64], M1R10I5C1H40Split1Time[allSoln], marker='o', markersize=6, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M1R10I5C2H40Split1Time[allSoln], marker='^', markersize=6, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M1R10I5C3H40Split1Time[allSoln], marker='s', markersize=6, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("CPU Time (seconds)")
#plt.yscale("log")
#plt.title("Classic vs Dedicated vs HADDOCK")
##plt.legend(loc=1)
#plt.savefig('amongNurseTimeGraphM1R10I5Split1' + ('AllSoln' if allSoln else 'OneSoln') + 'H40.png')




#plt.figure(figsize=(8,5))
#plt.plot([0,64],[classicC1H40Fail[allSoln]+1,classicC1H40Fail[allSoln]+1], color='#F5793A', linestyle=(0, (1, 10)), label='Classic, C-I')
#plt.plot([0,64],[classicC2H40Fail[allSoln]+1,classicC2H40Fail[allSoln]+1], color='#A95AA1', linestyle=(0, (3, 10, 1, 10)), label='Classic, C-II')
#plt.plot([0,64],[classicC3H40Fail[allSoln]+1,classicC3H40Fail[allSoln]+1], color='#85C0F9', linestyle=(0, (3, 5, 1, 5)), label='Classic, C-III')
#plt.plot([1,2,4,8,16,32,64], dC1H40[allSoln]+1, marker='o', markersize=6, linestyle='dashed', color='#F5793A', label='Dedicated, C-I')
#plt.plot([1,2,4,8,16,32,64], dC2H40[allSoln]+1, marker='^', markersize=6, linestyle='dashed', color='#A95AA1', label='Dedicated, C-II')
#plt.plot([1,2,4,8,16,32,64], dC3H40[allSoln]+1, marker='s', markersize=6, linestyle='dashed', color='#85C0F9', label='Dedicated, C-III')
#plt.plot([1,2,4,8,16,32,64], M1R0I5C1H40Slack1Fail[allSoln]+1, marker='o', markersize=6, linestyle="dotted", color='#F5793A', label='HADDOCK Old Split, C-I')
#plt.plot([1,2,4,8,16,32,64], M1R0I5C2H40Slack1Fail[allSoln]+1, marker='^', markersize=6, linestyle="dotted", color='#A95AA1', label='HADDOCK Old Split, C-II')
#plt.plot([1,2,4,8,16,32,64], M1R0I5C3H40Slack1Fail[allSoln]+1, marker='s', markersize=6, linestyle="dotted", color='#85C0F9', label='HADDOCK Old Split, C-III')
#plt.plot([1,2,4,8,16,32,64], M1R0I5C1H40Split1Fail[allSoln]+1, marker='o', markersize=6, color='#F5793A', label='HADDOCK New Split, C-I')
#plt.plot([1,2,4,8,16,32,64], M1R0I5C2H40Split1Fail[allSoln]+1, marker='^', markersize=6, color='#A95AA1', label='HADDOCK New Split, C-II')
#plt.plot([1,2,4,8,16,32,64], M1R0I5C3H40Split1Fail[allSoln]+1, marker='s', markersize=6, color='#85C0F9', label='HADDOCK New Split, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("Backtracks")
#plt.yscale("log")
#plt.title("Classic vs Dedicated vs HADDOCK")
#plt.legend(loc=1)
#plt.savefig('amongNurseBTGraphM1R0I5Comparison' + ('AllSoln' if allSoln else 'OneSoln') + 'H40.png')
#
#
#plt.figure(figsize=(8,5))
#plt.plot([0,64],[classicC1H40Time[allSoln],classicC1H40Time[allSoln]], color='#F5793A', linestyle=(0, (1, 10)), label='Classic, C-I')
#plt.plot([0,64],[classicC2H40Time[allSoln],classicC2H40Time[allSoln]], color='#A95AA1', linestyle=(0, (3, 10, 1, 10)), label='Classic, C-II')
#plt.plot([0,64],[classicC3H40Time[allSoln],classicC3H40Time[allSoln]], color='#85C0F9', linestyle=(0, (3, 5, 1, 5)), label='Classic, C-III')
#plt.plot([1,2,4,8,16,32,64], dC1H40Time[allSoln], marker='o', markersize=6, linestyle='dashed', color='#F5793A', label='Dedicated, C-I')
#plt.plot([1,2,4,8,16,32,64], dC2H40Time[allSoln], marker='^', markersize=6, linestyle='dashed', color='#A95AA1', label='Dedicated, C-II')
#plt.plot([1,2,4,8,16,32,64], dC3H40Time[allSoln], marker='s', markersize=6, linestyle='dashed', color='#85C0F9', label='Dedicated, C-III')
#plt.plot([1,2,4,8,16,32,64], M1R0I5C1H40Slack1Time[allSoln], marker='o', markersize=6, linestyle="dotted", color='#F5793A', label='HADDOCK Old Split, C-I')
#plt.plot([1,2,4,8,16,32,64], M1R0I5C2H40Slack1Time[allSoln], marker='^', markersize=6, linestyle="dotted", color='#A95AA1', label='HADDOCK Old Split, C-II')
#plt.plot([1,2,4,8,16,32,64], M1R0I5C3H40Slack1Time[allSoln], marker='s', markersize=6, linestyle="dotted", color='#85C0F9', label='HADDOCK Old Split, C-III')
#plt.plot([1,2,4,8,16,32,64], M1R0I5C1H40Split1Time[allSoln], marker='o', markersize=6, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M1R0I5C2H40Split1Time[allSoln], marker='^', markersize=6, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M1R0I5C3H40Split1Time[allSoln], marker='s', markersize=6, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("CPU Time (seconds)")
#plt.yscale("log")
#plt.title("Classic vs Dedicated vs HADDOCK")
##plt.legend(loc=1)
#plt.savefig('amongNurseTimeGraphM1R0I5Comparison' + ('AllSoln' if allSoln else 'OneSoln') + 'H40.png')





plt.figure(figsize=(8,5))
plt.plot([0,64],[classicC1H40Fail[allSoln]+1,classicC1H40Fail[allSoln]+1], color='#F5793A', linestyle=(0, (1, 10)), label='Classic, C-I')
plt.plot([0,64],[classicC2H40Fail[allSoln]+1,classicC2H40Fail[allSoln]+1], color='#A95AA1', linestyle=(0, (3, 10, 1, 10)), label='Classic, C-II')
plt.plot([0,64],[classicC3H40Fail[allSoln]+1,classicC3H40Fail[allSoln]+1], color='#85C0F9', linestyle=(0, (3, 5, 1, 5)), label='Classic, C-III')
plt.plot([1,2,4,8,16,32,64], dC1H40[allSoln]+1, marker='o', markersize=6, linestyle='dashed', color='#F5793A', label='Dedicated, C-I')
plt.plot([1,2,4,8,16,32,64], dC2H40[allSoln]+1, marker='^', markersize=6, linestyle='dashed', color='#A95AA1', label='Dedicated, C-II')
plt.plot([1,2,4,8,16,32,64], dC3H40[allSoln]+1, marker='s', markersize=6, linestyle='dashed', color='#85C0F9', label='Dedicated, C-III')
#plt.plot([1,2,4,8,16,32,64], M1R10I5C1H40Fail[allSoln]+1, marker='o', markersize=6, linestyle="dotted", color='#F5793A', label='HADDOCK Old Split, C-I')
#plt.plot([1,2,4,8,16,32,64], M1R10I5C2H40Fail[allSoln]+1, marker='^', markersize=6, linestyle="dotted", color='#A95AA1', label='HADDOCK Old Split, C-II')
#plt.plot([1,2,4,8,16,32,64], M1R10I5C3H40Fail[allSoln]+1, marker='s', markersize=6, linestyle="dotted", color='#85C0F9', label='HADDOCK Old Split, C-III')
plt.plot([1,2,4,8,16,32,64], M1R0I5C1H40Split1FixedFail[allSoln]+1, marker='o', markersize=6, color='#F5793A', label='HADDOCK, C-I')
plt.plot([1,2,4,8,16,32,64], M1R0I5C2H40Split1FixedFail[allSoln]+1, marker='^', markersize=6, color='#A95AA1', label='HADDOCK, C-II')
plt.plot([1,2,4,8,16,32,64], M1R0I5C3H40Split1FixedFail[allSoln]+1, marker='s', markersize=6, color='#85C0F9', label='HADDOCK, C-III')

plt.xlabel("Width")
plt.ylabel("Backtracks")
plt.yscale("log")
plt.title("Classic vs Dedicated vs HADDOCK")
plt.legend(loc=1)
plt.savefig('amongNurseBTGraphM1R0I5' + ('AllSoln' if allSoln else 'OneSoln') + 'H40.pdf')


plt.figure(figsize=(8,5))
plt.plot([0,64],[classicC1H40Time[allSoln],classicC1H40Time[allSoln]], color='#F5793A', linestyle=(0, (1, 10)), label='Classic, C-I')
plt.plot([0,64],[classicC2H40Time[allSoln],classicC2H40Time[allSoln]], color='#A95AA1', linestyle=(0, (3, 10, 1, 10)), label='Classic, C-II')
plt.plot([0,64],[classicC3H40Time[allSoln],classicC3H40Time[allSoln]], color='#85C0F9', linestyle=(0, (3, 5, 1, 5)), label='Classic, C-III')
plt.plot([1,2,4,8,16,32,64], dC1H40Time[allSoln], marker='o', markersize=6, linestyle='dashed', color='#F5793A', label='Dedicated, C-I')
plt.plot([1,2,4,8,16,32,64], dC2H40Time[allSoln], marker='^', markersize=6, linestyle='dashed', color='#A95AA1', label='Dedicated, C-II')
plt.plot([1,2,4,8,16,32,64], dC3H40Time[allSoln], marker='s', markersize=6, linestyle='dashed', color='#85C0F9', label='Dedicated, C-III')
#plt.plot([1,2,4,8,16,32,64], M1R10I5C1H40Time[allSoln], marker='o', markersize=6, linestyle="dotted", color='#F5793A', label='HADDOCK Old Split, C-I')
#plt.plot([1,2,4,8,16,32,64], M1R10I5C2H40Time[allSoln], marker='^', markersize=6, linestyle="dotted", color='#A95AA1', label='HADDOCK Old Split, C-II')
#plt.plot([1,2,4,8,16,32,64], M1R10I5C3H40Time[allSoln], marker='s', markersize=6, linestyle="dotted", color='#85C0F9', label='HADDOCK Old Split, C-III')
plt.plot([1,2,4,8,16,32,64], M1R0I5C1H40Split1FixedTime[allSoln], marker='o', markersize=6, color='#F5793A', label='HADDOCK, C-I')
plt.plot([1,2,4,8,16,32,64], M1R0I5C2H40Split1FixedTime[allSoln], marker='^', markersize=6, color='#A95AA1', label='HADDOCK, C-II')
plt.plot([1,2,4,8,16,32,64], M1R0I5C3H40Split1FixedTime[allSoln], marker='s', markersize=6, color='#85C0F9', label='HADDOCK, C-III')

plt.xlabel("Width")
plt.ylabel("CPU Time (seconds)")
plt.yscale("log")
plt.title("Classic vs Dedicated vs HADDOCK")
#plt.legend(loc=1)
plt.savefig('amongNurseTimeGraphM1R0I5' + ('AllSoln' if allSoln else 'OneSoln') + 'H40.pdf')



#plt.figure(figsize=(8,5))
#plt.plot([0,64],[classicC1H40Fail[allSoln]+1,classicC1H40Fail[allSoln]+1], color='#F5793A', linestyle=(0, (1, 10)), label='Classic, C-I')
#plt.plot([0,64],[classicC2H40Fail[allSoln]+1,classicC2H40Fail[allSoln]+1], color='#A95AA1', linestyle=(0, (3, 10, 1, 10)), label='Classic, C-II')
#plt.plot([0,64],[classicC3H40Fail[allSoln]+1,classicC3H40Fail[allSoln]+1], color='#85C0F9', linestyle=(0, (3, 5, 1, 5)), label='Classic, C-III')
#plt.plot([1,2,4,8,16,32,64], dC1H40[allSoln]+1, marker='o', markersize=6, linestyle='dashed', color='#F5793A', label='Dedicated, C-I')
#plt.plot([1,2,4,8,16,32,64], dC2H40[allSoln]+1, marker='^', markersize=6, linestyle='dashed', color='#A95AA1', label='Dedicated, C-II')
#plt.plot([1,2,4,8,16,32,64], dC3H40[allSoln]+1, marker='s', markersize=6, linestyle='dashed', color='#85C0F9', label='Dedicated, C-III')
#plt.plot([1,2,4,8,16,32,64], M1RMAXI5C1H40Fail[allSoln]+1, marker='o', markersize=6, linestyle="dotted", color='#F5793A', label='HADDOCK Old Split, C-I')
#plt.plot([1,2,4,8,16,32,64], M1RMAXI5C2H40Fail[allSoln]+1, marker='^', markersize=6, linestyle="dotted", color='#A95AA1', label='HADDOCK Old Split, C-II')
#plt.plot([1,2,4,8,16,32,64], M1RMAXI5C3H40Fail[allSoln]+1, marker='s', markersize=6, linestyle="dotted", color='#85C0F9', label='HADDOCK Old Split, C-III')
#plt.plot([1,2,4,8,16,32,64], M1RMAXI5C1H40Split1Fail[allSoln]+1, marker='o', markersize=6, color='#F5793A', label='HADDOCK New Split, C-I')
#plt.plot([1,2,4,8,16,32,64], M1RMAXI5C2H40Split1Fail[allSoln]+1, marker='^', markersize=6, color='#A95AA1', label='HADDOCK New Split, C-II')
#plt.plot([1,2,4,8,16,32,64], M1RMAXI5C3H40Split1Fail[allSoln]+1, marker='s', markersize=6, color='#85C0F9', label='HADDOCK New Split, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("Backtracks")
#plt.yscale("log")
#plt.title("Classic vs Dedicated vs HADDOCK")
#plt.legend(loc=1)
#plt.savefig('amongNurseBTGraphM1RMAXI5Comparison' + ('AllSoln' if allSoln else 'OneSoln') + 'H40.png')
#
#
#plt.figure(figsize=(8,5))
#plt.plot([0,64],[classicC1H40Time[allSoln],classicC1H40Time[allSoln]], color='#F5793A', linestyle=(0, (1, 10)), label='Classic, C-I')
#plt.plot([0,64],[classicC2H40Time[allSoln],classicC2H40Time[allSoln]], color='#A95AA1', linestyle=(0, (3, 10, 1, 10)), label='Classic, C-II')
#plt.plot([0,64],[classicC3H40Time[allSoln],classicC3H40Time[allSoln]], color='#85C0F9', linestyle=(0, (3, 5, 1, 5)), label='Classic, C-III')
#plt.plot([1,2,4,8,16,32,64], dC1H40Time[allSoln], marker='o', markersize=6, linestyle='dashed', color='#F5793A', label='Dedicated, C-I')
#plt.plot([1,2,4,8,16,32,64], dC2H40Time[allSoln], marker='^', markersize=6, linestyle='dashed', color='#A95AA1', label='Dedicated, C-II')
#plt.plot([1,2,4,8,16,32,64], dC3H40Time[allSoln], marker='s', markersize=6, linestyle='dashed', color='#85C0F9', label='Dedicated, C-III')
#plt.plot([1,2,4,8,16,32,64], M1RMAXI5C1H40Time[allSoln], marker='o', markersize=6, linestyle="dotted", color='#F5793A', label='HADDOCK Old Split, C-I')
#plt.plot([1,2,4,8,16,32,64], M1RMAXI5C2H40Time[allSoln], marker='^', markersize=6, linestyle="dotted", color='#A95AA1', label='HADDOCK Old Split, C-II')
#plt.plot([1,2,4,8,16,32,64], M1RMAXI5C3H40Time[allSoln], marker='s', markersize=6, linestyle="dotted", color='#85C0F9', label='HADDOCK Old Split, C-III')
#plt.plot([1,2,4,8,16,32,64], M1RMAXI5C1H40Split1Time[allSoln], marker='o', markersize=6, color='#F5793A', label='HADDOCK, C-I')
#plt.plot([1,2,4,8,16,32,64], M1RMAXI5C2H40Split1Time[allSoln], marker='^', markersize=6, color='#A95AA1', label='HADDOCK, C-II')
#plt.plot([1,2,4,8,16,32,64], M1RMAXI5C3H40Split1Time[allSoln], marker='s', markersize=6, color='#85C0F9', label='HADDOCK, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("CPU Time (seconds)")
#plt.yscale("log")
#plt.title("Classic vs Dedicated vs HADDOCK")
##plt.legend(loc=1)
#plt.savefig('amongNurseTimeGraphM1RMAXI5Comparison' + ('AllSoln' if allSoln else 'OneSoln') + 'H40.png')
#
#
#
#
#plt.figure(figsize=(8,5))
#plt.plot([0,64],[classicC1H40Fail[allSoln]+1,classicC1H40Fail[allSoln]+1], color='#F5793A', linestyle=(0, (1, 10)), label='Classic, C-I')
#plt.plot([0,64],[classicC2H40Fail[allSoln]+1,classicC2H40Fail[allSoln]+1], color='#A95AA1', linestyle=(0, (3, 10, 1, 10)), label='Classic, C-II')
#plt.plot([0,64],[classicC3H40Fail[allSoln]+1,classicC3H40Fail[allSoln]+1], color='#85C0F9', linestyle=(0, (3, 5, 1, 5)), label='Classic, C-III')
#plt.plot([1,2,4,8,16,32,64], dC1H40[allSoln]+1, marker='o', markersize=6, linestyle='dashed', color='#F5793A', label='Dedicated, C-I')
#plt.plot([1,2,4,8,16,32,64], dC2H40[allSoln]+1, marker='^', markersize=6, linestyle='dashed', color='#A95AA1', label='Dedicated, C-II')
#plt.plot([1,2,4,8,16,32,64], dC3H40[allSoln]+1, marker='s', markersize=6, linestyle='dashed', color='#85C0F9', label='Dedicated, C-III')
#plt.plot([1,2,4,8,16,32,64], M1R0I5C1H40Split1Fail[allSoln]+1, marker='o', markersize=6, linestyle="dotted", color='#F5793A', label='HADDOCK Reboot=0, C-I')
#plt.plot([1,2,4,8,16,32,64], M1R0I5C2H40Split1Fail[allSoln]+1, marker='^', markersize=6, linestyle="dotted", color='#A95AA1', label='HADDOCK Reboot=0, C-II')
#plt.plot([1,2,4,8,16,32,64], M1R0I5C3H40Split1Fail[allSoln]+1, marker='s', markersize=6, linestyle="dotted", color='#85C0F9', label='HADDOCK Reboot=0, C-III')
#plt.plot([1,2,4,8,16,32,64], M1RMAXI5C1H40Split1Fail[allSoln]+1, marker='o', markersize=6, color='#F5793A', label='HADDOCK Reboot=MAX, C-I')
#plt.plot([1,2,4,8,16,32,64], M1RMAXI5C2H40Split1Fail[allSoln]+1, marker='^', markersize=6, color='#A95AA1', label='HADDOCK Reboot=MAX, C-II')
#plt.plot([1,2,4,8,16,32,64], M1RMAXI5C3H40Split1Fail[allSoln]+1, marker='s', markersize=6, color='#85C0F9', label='HADDOCK Reboot=MAX, C-III')
#
#plt.xlabel("Width")
#plt.ylabel("Backtracks")
#plt.yscale("log")
#plt.title("Classic vs Dedicated vs HADDOCK")
#plt.legend(loc=1)
#plt.savefig('amongNurseBTGraphM1R0vsRMAXComparison' + ('AllSoln' if allSoln else 'OneSoln') + 'H40.png')





plt.figure(figsize=(8,5))
plt.plot([0,64],[classicC1H40Fail[allSoln]+1,classicC1H40Fail[allSoln]+1], color='#F5793A', linestyle=(0, (1, 10)), label='Classic, C-I')
plt.plot([0,64],[classicC2H40Fail[allSoln]+1,classicC2H40Fail[allSoln]+1], color='#A95AA1', linestyle=(0, (3, 10, 1, 10)), label='Classic, C-II')
plt.plot([0,64],[classicC3H40Fail[allSoln]+1,classicC3H40Fail[allSoln]+1], color='#85C0F9', linestyle=(0, (3, 5, 1, 5)), label='Classic, C-III')
plt.plot([1,2,4,8,16,32,64], dC1H40[allSoln]+1, marker='o', markersize=6, linestyle='dashed', color='#F5793A', label='Dedicated, C-I')
plt.plot([1,2,4,8,16,32,64], dC2H40[allSoln]+1, marker='^', markersize=6, linestyle='dashed', color='#A95AA1', label='Dedicated, C-II')
plt.plot([1,2,4,8,16,32,64], dC3H40[allSoln]+1, marker='s', markersize=6, linestyle='dashed', color='#85C0F9', label='Dedicated, C-III')
plt.plot([1,2,4,8,16,32,64], M1R10I5C1H40Split1Fail[allSoln]+1, marker='o', markersize=6, linestyle="dotted", color='#F5793A', label='HADDOCK Pre-fix, C-I')
plt.plot([1,2,4,8,16,32,64], M1R10I5C2H40Split1Fail[allSoln]+1, marker='^', markersize=6, linestyle="dotted", color='#A95AA1', label='HADDOCK Pre-fix, C-II')
plt.plot([1,2,4,8,16,32,64], M1R10I5C3H40Split1Fail[allSoln]+1, marker='s', markersize=6, linestyle="dotted", color='#85C0F9', label='HADDOCK Pre-fix, C-III')
plt.plot([1,2,4,8,16,32,64], M1R10I5C1H40Split1FixedFail[allSoln]+1, marker='o', markersize=6, color='#F5793A', label='HADDOCK Fixed split, C-I')
plt.plot([1,2,4,8,16,32,64], M1R10I5C2H40Split1FixedFail[allSoln]+1, marker='^', markersize=6, color='#A95AA1', label='HADDOCK Fixed split, C-II')
plt.plot([1,2,4,8,16,32,64], M1R10I5C3H40Split1FixedFail[allSoln]+1, marker='s', markersize=6, color='#85C0F9', label='HADDOCK Fixed split, C-III')

plt.xlabel("Width")
plt.ylabel("Backtracks")
plt.yscale("log")
plt.title("Classic vs Dedicated vs HADDOCK")
plt.legend(loc=1)
plt.savefig('amongNurseBTGraphM1R10I5FixComparison' + ('AllSoln' if allSoln else 'OneSoln') + 'H40.png')


plt.figure(figsize=(8,5))
plt.plot([0,64],[classicC1H40Time[allSoln],classicC1H40Time[allSoln]], color='#F5793A', linestyle=(0, (1, 10)), label='Classic, C-I')
plt.plot([0,64],[classicC2H40Time[allSoln],classicC2H40Time[allSoln]], color='#A95AA1', linestyle=(0, (3, 10, 1, 10)), label='Classic, C-II')
plt.plot([0,64],[classicC3H40Time[allSoln],classicC3H40Time[allSoln]], color='#85C0F9', linestyle=(0, (3, 5, 1, 5)), label='Classic, C-III')
plt.plot([1,2,4,8,16,32,64], dC1H40Time[allSoln], marker='o', markersize=6, linestyle='dashed', color='#F5793A', label='Dedicated, C-I')
plt.plot([1,2,4,8,16,32,64], dC2H40Time[allSoln], marker='^', markersize=6, linestyle='dashed', color='#A95AA1', label='Dedicated, C-II')
plt.plot([1,2,4,8,16,32,64], dC3H40Time[allSoln], marker='s', markersize=6, linestyle='dashed', color='#85C0F9', label='Dedicated, C-III')
plt.plot([1,2,4,8,16,32,64], M1R10I5C1H40Split1Time[allSoln], marker='o', markersize=6, linestyle="dotted", color='#F5793A', label='HADDOCK Pre-fix, C-I')
plt.plot([1,2,4,8,16,32,64], M1R10I5C2H40Split1Time[allSoln], marker='^', markersize=6, linestyle="dotted", color='#A95AA1', label='HADDOCK Pre-fix, C-II')
plt.plot([1,2,4,8,16,32,64], M1R10I5C3H40Split1Time[allSoln], marker='s', markersize=6, linestyle="dotted", color='#85C0F9', label='HADDOCK Pre-fix, C-III')
plt.plot([1,2,4,8,16,32,64], M1R10I5C1H40Split1FixedTime[allSoln], marker='o', markersize=6, color='#F5793A', label='HADDOCK Fixed split, C-I')
plt.plot([1,2,4,8,16,32,64], M1R10I5C2H40Split1FixedTime[allSoln], marker='^', markersize=6, color='#A95AA1', label='HADDOCK Fixed split, C-II')
plt.plot([1,2,4,8,16,32,64], M1R10I5C3H40Split1FixedTime[allSoln], marker='s', markersize=6, color='#85C0F9', label='HADDOCK Fixed split, C-III')

plt.xlabel("Width")
plt.ylabel("CPU Time (seconds)")
plt.yscale("log")
plt.title("Classic vs Dedicated vs HADDOCK")
#plt.legend(loc=1)
plt.savefig('amongNurseTimeGraphM1R10I5FixComparison' + ('AllSoln' if allSoln else 'OneSoln') + 'H40.png')







plt.figure(figsize=(8,5))
plt.plot([0,64],[classicC1H40Fail[allSoln]+1,classicC1H40Fail[allSoln]+1], color='#F5793A', linestyle=(0, (1, 10)), label='Classic, C-I')
plt.plot([0,64],[classicC2H40Fail[allSoln]+1,classicC2H40Fail[allSoln]+1], color='#A95AA1', linestyle=(0, (3, 10, 1, 10)), label='Classic, C-II')
plt.plot([0,64],[classicC3H40Fail[allSoln]+1,classicC3H40Fail[allSoln]+1], color='#85C0F9', linestyle=(0, (3, 5, 1, 5)), label='Classic, C-III')
plt.plot([1,2,4,8,16,32,64], dC1H40[allSoln]+1, marker='o', markersize=6, linestyle='dashed', color='#F5793A', label='Dedicated, C-I')
plt.plot([1,2,4,8,16,32,64], dC2H40[allSoln]+1, marker='^', markersize=6, linestyle='dashed', color='#A95AA1', label='Dedicated, C-II')
plt.plot([1,2,4,8,16,32,64], dC3H40[allSoln]+1, marker='s', markersize=6, linestyle='dashed', color='#85C0F9', label='Dedicated, C-III')
#plt.plot([1,2,4,8,16,32,64], M3R10I5C1H40Fail[allSoln]+1, marker='o', markersize=6, linestyle="dotted", color='#F5793A', label='HADDOCK Old Split, C-I')
#plt.plot([1,2,4,8,16,32,64], M3R10I5C2H40Fail[allSoln]+1, marker='^', markersize=6, linestyle="dotted", color='#A95AA1', label='HADDOCK Old Split, C-II')
#plt.plot([1,2,4,8,16,32,64], M3R10I5C3H40Fail[allSoln]+1, marker='s', markersize=6, linestyle="dotted", color='#85C0F9', label='HADDOCK Old Split, C-III')
plt.plot([1,2,4,8,16,32,64], M3R10I5C1H40Split1FixedFail[allSoln]+1, marker='o', markersize=6, color='#F5793A', label='HADDOCK, C-I')
plt.plot([1,2,4,8,16,32,64], M3R10I5C2H40Split1FixedFail[allSoln]+1, marker='^', markersize=6, color='#A95AA1', label='HADDOCK, C-II')
plt.plot([1,2,4,8,16,32,64], M3R10I5C3H40Split1FixedFail[allSoln]+1, marker='s', markersize=6, color='#85C0F9', label='HADDOCK, C-III')

plt.xlabel("Width")
plt.ylabel("Backtracks")
plt.yscale("log")
plt.title("Classic vs Dedicated vs HADDOCK")
plt.legend(loc=1)
plt.savefig('amongNurseBTGraphM3R10I5' + ('AllSoln' if allSoln else 'OneSoln') + 'H40.png')


plt.figure(figsize=(8,5))
plt.plot([0,64],[classicC1H40Time[allSoln],classicC1H40Time[allSoln]], color='#F5793A', linestyle=(0, (1, 10)), label='Classic, C-I')
plt.plot([0,64],[classicC2H40Time[allSoln],classicC2H40Time[allSoln]], color='#A95AA1', linestyle=(0, (3, 10, 1, 10)), label='Classic, C-II')
plt.plot([0,64],[classicC3H40Time[allSoln],classicC3H40Time[allSoln]], color='#85C0F9', linestyle=(0, (3, 5, 1, 5)), label='Classic, C-III')
plt.plot([1,2,4,8,16,32,64], dC1H40Time[allSoln], marker='o', markersize=6, linestyle='dashed', color='#F5793A', label='Dedicated, C-I')
plt.plot([1,2,4,8,16,32,64], dC2H40Time[allSoln], marker='^', markersize=6, linestyle='dashed', color='#A95AA1', label='Dedicated, C-II')
plt.plot([1,2,4,8,16,32,64], dC3H40Time[allSoln], marker='s', markersize=6, linestyle='dashed', color='#85C0F9', label='Dedicated, C-III')
#plt.plot([1,2,4,8,16,32,64], M3R10I5C1H40Time[allSoln], marker='o', markersize=6, linestyle="dotted", color='#F5793A', label='HADDOCK Old Split, C-I')
#plt.plot([1,2,4,8,16,32,64], M3R10I5C2H40Time[allSoln], marker='^', markersize=6, linestyle="dotted", color='#A95AA1', label='HADDOCK Old Split, C-II')
#plt.plot([1,2,4,8,16,32,64], M3R10I5C3H40Time[allSoln], marker='s', markersize=6, linestyle="dotted", color='#85C0F9', label='HADDOCK Old Split, C-III')
plt.plot([1,2,4,8,16,32,64], M3R10I5C1H40Split1FixedTime[allSoln], marker='o', markersize=6, color='#F5793A', label='HADDOCK, C-I')
plt.plot([1,2,4,8,16,32,64], M3R10I5C2H40Split1FixedTime[allSoln], marker='^', markersize=6, color='#A95AA1', label='HADDOCK, C-II')
plt.plot([1,2,4,8,16,32,64], M3R10I5C3H40Split1FixedTime[allSoln], marker='s', markersize=6, color='#85C0F9', label='HADDOCK, C-III')

plt.xlabel("Width")
plt.ylabel("CPU Time (seconds)")
plt.yscale("log")
plt.title("Classic vs Dedicated vs HADDOCK")
#plt.legend(loc=1)
plt.savefig('amongNurseTimeGraphM3R10I5' + ('AllSoln' if allSoln else 'OneSoln') + 'H40.png')






plt.figure(figsize=(8,5))
plt.plot([0,64],[classicC1H40Fail[allSoln]+1,classicC1H40Fail[allSoln]+1], color='#F5793A', linestyle=(0, (1, 10)), label='Classic, C-I')
plt.plot([0,64],[classicC2H40Fail[allSoln]+1,classicC2H40Fail[allSoln]+1], color='#A95AA1', linestyle=(0, (3, 10, 1, 10)), label='Classic, C-II')
plt.plot([0,64],[classicC3H40Fail[allSoln]+1,classicC3H40Fail[allSoln]+1], color='#85C0F9', linestyle=(0, (3, 5, 1, 5)), label='Classic, C-III')
plt.plot([1,2,4,8,16,32,64], dC1H40[allSoln]+1, marker='o', markersize=6, linestyle='dashed', color='#F5793A', label='Dedicated, C-I')
plt.plot([1,2,4,8,16,32,64], dC2H40[allSoln]+1, marker='^', markersize=6, linestyle='dashed', color='#A95AA1', label='Dedicated, C-II')
plt.plot([1,2,4,8,16,32,64], dC3H40[allSoln]+1, marker='s', markersize=6, linestyle='dashed', color='#85C0F9', label='Dedicated, C-III')
plt.plot([1,2,4,8,16,32,64], M1R0I5C1H40Split1FixedFail[allSoln]+1, marker='o', markersize=6, linestyle="dotted", color='#F5793A', label='HADDOCK Reboot=0, C-I')
plt.plot([1,2,4,8,16,32,64], M1R0I5C2H40Split1FixedFail[allSoln]+1, marker='^', markersize=6, linestyle="dotted", color='#A95AA1', label='HADDOCK Reboot=0, C-II')
plt.plot([1,2,4,8,16,32,64], M1R0I5C3H40Split1FixedFail[allSoln]+1, marker='s', markersize=6, linestyle="dotted", color='#85C0F9', label='HADDOCK Reboot=0, C-III')
plt.plot([1,2,4,8,16,32,64], M1R10I5C1H40Split1FixedFail[allSoln]+1, marker='o', markersize=6, color='#F5793A', label='HADDOCK Reboot=10, C-I')
plt.plot([1,2,4,8,16,32,64], M1R10I5C2H40Split1FixedFail[allSoln]+1, marker='^', markersize=6, color='#A95AA1', label='HADDOCK Reboot=10, C-II')
plt.plot([1,2,4,8,16,32,64], M1R10I5C3H40Split1FixedFail[allSoln]+1, marker='s', markersize=6, color='#85C0F9', label='HADDOCK Reboot=10, C-III')

plt.xlabel("Width")
plt.ylabel("Backtracks")
plt.yscale("log")
plt.title("Classic vs Dedicated vs HADDOCK")
plt.legend(loc=1)
plt.savefig('amongNurseBTGraphM1R0vsR10I5' + ('AllSoln' if allSoln else 'OneSoln') + 'H40.png')


plt.figure(figsize=(8,5))
plt.plot([0,64],[classicC1H40Time[allSoln],classicC1H40Time[allSoln]], color='#F5793A', linestyle=(0, (1, 10)), label='Classic, C-I')
plt.plot([0,64],[classicC2H40Time[allSoln],classicC2H40Time[allSoln]], color='#A95AA1', linestyle=(0, (3, 10, 1, 10)), label='Classic, C-II')
plt.plot([0,64],[classicC3H40Time[allSoln],classicC3H40Time[allSoln]], color='#85C0F9', linestyle=(0, (3, 5, 1, 5)), label='Classic, C-III')
plt.plot([1,2,4,8,16,32,64], dC1H40Time[allSoln], marker='o', markersize=6, linestyle='dashed', color='#F5793A', label='Dedicated, C-I')
plt.plot([1,2,4,8,16,32,64], dC2H40Time[allSoln], marker='^', markersize=6, linestyle='dashed', color='#A95AA1', label='Dedicated, C-II')
plt.plot([1,2,4,8,16,32,64], dC3H40Time[allSoln], marker='s', markersize=6, linestyle='dashed', color='#85C0F9', label='Dedicated, C-III')
plt.plot([1,2,4,8,16,32,64], M1R0I5C1H40Split1FixedTime[allSoln], marker='o', markersize=6, linestyle="dotted", color='#F5793A', label='HADDOCK Reboot=0, C-I')
plt.plot([1,2,4,8,16,32,64], M1R0I5C2H40Split1FixedTime[allSoln], marker='^', markersize=6, linestyle="dotted", color='#A95AA1', label='HADDOCK Reboot=0, C-II')
plt.plot([1,2,4,8,16,32,64], M1R0I5C3H40Split1FixedTime[allSoln], marker='s', markersize=6, linestyle="dotted", color='#85C0F9', label='HADDOCK Reboot=0, C-III')
plt.plot([1,2,4,8,16,32,64], M1R10I5C1H40Split1FixedTime[allSoln], marker='o', markersize=6, color='#F5793A', label='HADDOCK Reboot=10, C-I')
plt.plot([1,2,4,8,16,32,64], M1R10I5C2H40Split1FixedTime[allSoln], marker='^', markersize=6, color='#A95AA1', label='HADDOCK Reboot=10, C-II')
plt.plot([1,2,4,8,16,32,64], M1R10I5C3H40Split1FixedTime[allSoln], marker='s', markersize=6, color='#85C0F9', label='HADDOCK Reboot=10, C-III')

plt.xlabel("Width")
plt.ylabel("CPU Time (seconds)")
plt.yscale("log")
plt.title("Classic vs Dedicated vs HADDOCK")
#plt.legend(loc=1)
plt.savefig('amongNurseTimeGraphM1R0vsR10I5' + ('AllSoln' if allSoln else 'OneSoln') + 'H40.png')
