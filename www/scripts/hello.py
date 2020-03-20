import sys
try:
    for line in sys.stdin:
        print("Hello " + line)
except:
    ignore = 1
