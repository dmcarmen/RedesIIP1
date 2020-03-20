import sys
temps = list()
try:
    for line in sys.stdin:
        temps += line.split(" ")
        temps = list(map(float,temps))
except:
    ignore = 1
aux = list(map(lambda x: (x*9/5)+32, temps))
for i in range(len(temps)):
    print(str(temps[i]) + 'C = ' + str(aux[i]) + 'F')
