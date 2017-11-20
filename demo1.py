#coding:utf-8
from __future__ import print_function

month=[1,2,3,4,5,6,7,8,9,10,11,12,1]
test=True
print (month[12])
def credit_card(card_info,consum=[1,1]):
    pay_day=[]
    if consum[1]<=card_info['Statement Date']:
        pay_day.append(consum[0]+(1 if card_info['Statement Date']>card_info['Repayment Date'] else 0))
    else:
        pay_day.append(consum[0]+1+(1 if card_info['Statement Date']>card_info['Repayment Date'] else 0))
    pay_day.append(card_info['Repayment Date'])
    delay_days = pay_day[1] + 30 * (pay_day[0] - consum[0]) - consum[1]
    pay_day[0]=month[pay_day[0]-1]
    if not test:
        print(card_info['abbreviation'],'\tpay day : %d-%d'%tuple(pay_day),'\tdelay days: %d'%delay_days)
    pay=[card_info['abbreviation'],pay_day,delay_days]
    return pay


def main():
    CMB_info={'name':'China Merchants Bank','abbreviation':'CMB',
              'Statement Date':9,'Repayment Date':27}
    BCM_info={'name':'Bank of Communications','abbreviation':'BCM',
              'Statement Date':18,'Repayment Date':12}
    card_info=[CMB_info,BCM_info]
    print ('\t\t\tWelcome to Repayment Date calculate system!'
           '\n\t\tyou have {} credit card, informations are as follow: \n'
           '\n\tabbreviation & bank name\t||\tStatement Date\t|\tRepayment Date\t'
           '\n==================================================================='
           '===='.format(len(card_info)))
    for item in card_info:
        print ('\t{}({})\t||\t\t{}\t\t\t|\t\t{}'
               .format(item['abbreviation'],item['name'],
                       item['Statement Date'],item['Repayment Date']))
    print ('\n')
    if not test:
        consum = raw_input('\n\nplease input a consum date: ').split('-')
        for i in range(len(consum)):
            consum[i]=int(consum[i])
        credit_card(CMB_info,consum)
        credit_card(BCM_info,consum)
    else:
        print ('consum||{: ^18}||{: ^18}||{: ^7}'.format('CMB','BCM','card'),
               '\n{: ^6}||pay day|dalay days||pay day|dalay days||suggest'.format('day'),
               '\n{:-^56}'.format('-'))
        for i in range(1,31):
            CMB_pay=credit_card(CMB_info,[1,i])
            BCM_pay=credit_card(BCM_info,[1,i])
            print ('{0: >2}.{1: <3}||{2[1][0]: >3}-{2[1][1]: <3}|{2[2]: ^10}'
                   '||{3[1][0]: >3}-{3[1][1]: <3}|{3[2]: ^10}||{4: ^7} '
                   .format(1,i,CMB_pay,BCM_pay,'CBM'if CMB_pay[2]>BCM_pay[2] else 'BCM'))


if __name__ == "__main__":
    main()



    # for i in consum:
    #     consum[consum.index(i)]=int(i)

    # print ('{: >2}.{: <3}||{: >3}-{: <3}|{: ^10}||{: >3}-{: <3}|{: ^10}||{: ^7}'
    #        .format(1,i,CMB_pay[1][0],CMB_pay[1][1],CMB_pay[2],
    #                BCM_pay[1][0],BCM_pay[1][1],BCM_pay[2],
    #                'CBM'if CMB_pay[2]>BCM_pay[2] else 'BCM'))
