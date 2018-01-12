# -*- coding: utf-8 -*-
from __future__ import print_function
import time

'''
题目：暂停一秒输出，并格式化当前时间。 
'''

print (time.strftime('%Y-%m-%d %H:%M:%S',time.localtime(time.time())))
# 暂停一秒
time.sleep(1)
print (time.strftime('%Y-%m-%d %H:%M:%S',time.localtime(time.time())))
print ('wait for 1 second from now:{}'.format(time.localtime(time.time())))
print (time.asctime(time.time()))
time.sleep(1)
print ('wait end :{}'.format(time.localtime()))
