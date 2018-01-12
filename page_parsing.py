from bs4 import BeautifulSoup
import requests
import time 
import pymongo

client=pymongo.MongoClient('localhost',27017)
test = client['test']
url_list =test['url_list']
item_info=test['item_info']
#spider1
def get_links_from(channel,page,who_sells=0):
    list_view='{}{}/pn{}/'.format(channel,str(who_sells),str(page))
    web_data =requests.get(list_view,'lxml')
    time.sleep(1)
    soup=BeautifulSoup(web_data.text,'lxml')
    if soup.find('td','t'):
        for link in soup.select('td.t a.t'):
            item_link=link.get('href').split('?')[0]
            url_list.insert_one({'url':item_link})
            print(item_link)
    else:
        pass
        #nothing!
def get_item_info(url):
    web_data =requests.get(url)
    soup =BeautifulSoup(web_data.text,'lxml')
    no_longer_exist='404' in soup.find('script',type='text/javascript').get('src').split('/')

    if no_longer_exist:
        pass
    else:
        title= soup.title.text
        price= soup.select('span.price.c_f50')[0].text
        date = soup.select('li.time')[0].text
        area = list(soup.select('.c_25')[0].stripped_strings) if soup.find_all('span','c_25d') else None
        item_info.insert_one({'title':title,'price':price,'date':date,'area':area})
        #print({'title':title,'price':price,'date':date,'area':area})

#get_item_info('http://bj.58.com/pingbandiannao/30469042246069x.shtml')
#get_links_from('http://bj.58.com//shouji/',3)