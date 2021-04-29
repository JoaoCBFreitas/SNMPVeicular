import random


def tohex(val, nbits):
    return '{:X}'.format(val & (2**nbits-1))


ids = []
for x in range(10000):
    ids.append(random.randint(0, 10000))


try:
    with open("can.log", "a") as f:
        for x in range(10000):
            hexID = "%0.4X" % ids[x]
            datap = "%0."+str(random.randint(0, 8))+"X"
            payload = tohex(random.randint(-1000, 1000), 32)
            dlc = len(payload)
            line = hexID+"   "+str(dlc)+" "+payload+"\n"
            f.write(line)
    f.close()
except FileNotFoundError:
    print("File not found")
