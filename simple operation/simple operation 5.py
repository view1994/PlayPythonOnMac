# -*- coding: utf-8 -*-
from __future__ import print_function

'''
题目：输入三个整数x,y,z，请把这三个数由小到大输出。

程序分析：我们想办法把最小的数放到x上，先将x与y进行比较，如果x>y则将x与y的值进行交换，然后再用x与z进行比较，如果x>z则将x与z的值进行交换，这样能使x最小。
'''

x=int(raw_input('please input a number:'))
temp=int(raw_input('please input a number:'))
(x,y)=(temp,x) if x>temp else (x,temp)
temp=int(raw_input('please input a number:'))
(y,z)=(temp,y) if y>temp else (y,temp)
(x,y)=(y,x) if x>y else (x,y)
print (x,y,z)

l=[]
l.append(int(raw_input('please input a number:')))
for i in range(0,2):
    temp = int(raw_input('please input a number:'))
    if temp<l[i]:
        l.insert(i,temp)
    else:
        l.append(temp)
    print (l)
print (l)
