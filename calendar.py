#coding:utf-8
from __future__ import print_function
def is_leap_year(year):
    return True if(((year%100!=0)&(year%4==0))|(year%400==0)) else False
def days_of_year(year):
    return 366 if is_leap_year(year) else 365
def days_of_the_month(year,month):
    if month in [1,3,5,7,8,10,12]:
        return 31
    elif month!=2:
        return 30
    elif is_leap_year(year):#闰年二月
        return 29
    else:
        return 28
def which_day_in_week(year,month):
    day=0#2017年1月1日是周日
    if year<2017:
        for i in range(2016,year-1,-1):
            day = day - days_of_year(i) % 7
    else:
        for i in range(2018,year+1):
            day=day + days_of_year(i-1)%7
    for i in range(1,month):
        day = day + days_of_the_month(year,i)%7
    return day%7
def print_calendar(year=2017,month=1):
    print ('\n{:=^56}\n{: ^56}\n{:-^56}\n'.format('',str(year)+'年'+str(month)+'月',''),
           '{: ^8}{: ^8}{: ^8}{: ^8}{: ^8}{: ^8}{: ^8}'
           .format('SUN','MON','TUE','WEN','THU','FRI','SAT'),
           end='\n')
    l=[None]*which_day_in_week(year,month)+list(range(1,days_of_the_month(year,month)+1))
    for i,value in enumerate(l):
        print ('{: ^8}'.format(value if value else ''),end=''if (i+1)%7 else '\n')
    print ('\n{:=^56}'.format(''))
def main():
    year=int (raw_input('{: ^56}'.format('WELCOME TO CALENDAR')+'\n'+' '*12+'please input a year number:'))
    month=int (raw_input(' '*12+'please input a month number:'))
    print_calendar(year,month)
    # print_calendar(2017,1)
if __name__ == "__main__":
    main()


