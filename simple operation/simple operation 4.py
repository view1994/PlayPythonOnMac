# -*- coding: utf-8 -*-
from __future__ import print_function

'''
题目：输入某年某月某日，判断这一天是这一年的第几天？

程序分析：以3月5日为例，应该先把前两个月的加起来，然后再加上5天即本年的第几天，特殊情况，闰年且输入月份大于2时需考虑多加一天：
'''
def is_leap_yeer(y):
    return True if(y%400==0)|((y%100!=0)&(y%4==0)) else False
def days_of_month(y,m):
    if m in [1,3,5,7,8,10,12]:
        return 31
    elif m==2:
        return 29 if is_leap_yeer(y) else 28
    else:
        return 30
date=[int(i) for i in raw_input('please input a date:').split(' ')]
days=0
for i in range(1,date[1]):
    days+=days_of_month(date[0],i)
days+=date[2]
print ('it is the {}th day.'.format(days))