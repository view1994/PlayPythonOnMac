# -*- coding: utf-8 -*-
from __future__ import print_function
'''
题目：斐波那契数列。

程序分析：斐波那契数列（Fibonacci sequence），又称黄金分割数列，指的是这样一个数列：0、1、1、2、3、5、8、13、21、34、……。
'''

l=[0,1]
def febonacci(n):
    for i in range(0,n):
        l.append(l[i]+l[i+1])
    print (l)
febonacci(10)
print (l[10])

def feb(n):
    if n==0:
        return 0
    elif n==1:
        return 1
    else:
        return feb(n-2)+feb(n-1)
print (feb(10))