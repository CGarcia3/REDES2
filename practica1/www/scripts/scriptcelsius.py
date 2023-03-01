import sys
temperatures = list()
try:
    for l in sys.stdin:
        temperatures += l.split(" ")
        temperatures = list(map(float,temperatures))
except:
    ignore = 1
aux = list(map(lambda x: (x*9.0/5.0)+32, temperatures))
for i in range(len(temperatures)):
    print(str(temperatures[i]) + 'C = ' + str(aux[i]) + 'F')