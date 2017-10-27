import random, string

myfile = open("another",'w')

count = 0
total = 1024*200
while count<total:
    count = count + 1
    print >> myfile,(''.join(random.sample(string.ascii_letters+string.digits,32 )))*32
