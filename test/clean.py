import sys
import os

file = open(sys.argv[1], 'r')
filename = os.path.splitext(sys.argv[1])[0] + ".csv"
newFile = open(filename, 'w')

for line in file:
    #print(line)
    if not line.rstrip():
        #print("Throw empty line")
        continue
    write = True
    for char in line:
        if char.isalpha() or char == '+':
            #print(char)
            #print("Throw this line!")
            write = False
            break
    if write == True:
        #print("Got this line!")
        newFile.write(line)
