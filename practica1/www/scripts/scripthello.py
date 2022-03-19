import sys
try:
    for i in sys.stdin:
        print("Hello " + i)
except:
    ignore = 1