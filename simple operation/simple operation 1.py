# -*- coding: utf-8 -*-
from __future__ import print_function
'''
题目：有四个数字：1、2、3、4，能组成多少个互不相同且无重复数字的三位数？各是多少？
注意：个十百位数字不得重复，111不对！
程序分析：可填在百位、十位、个位的数字都是1、2、3、4。组成所有的排列后再去 掉不满足条件的排列。
'''
base_num=[1,2,3,4]
result_list=[(i*100+j*10+k) for i in base_num for j in base_num for k in base_num if not((i==j)|(j==k)|(i==k))]
print (len(result_list))
print (result_list)