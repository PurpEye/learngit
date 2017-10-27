import random, string

myfile = open("origin",'w')

count = 0
total = 1024*1024*4
while count<total:
    count = count + 1
    print >> myfile,(''.join(random.sample(string.ascii_letters+string.digits,32 )))*32
