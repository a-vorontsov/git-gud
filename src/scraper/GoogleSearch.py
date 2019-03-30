from googlesearch import search
import re
import requests

f = open('helloworld.html','w')

def find_questions(git_error):
    results = []

    for url in search(git_error, stop=20):
        if "stackoverflow" in url:
            m = re.search('questions/(.+?)/', url)
            print(url)
            if m:
                number = m.group(1)
            results.append(number)
    return results

def get_answer(number):
    r = requests.get("https://api.stackexchange.com/2.2/questions/%s/answers?key=U4DMV*8nvpm3EOpvf69Rxw((&site=stackoverflow&page=1&pagesize=1&order=desc&sort=votes&filter=!-*jbN.L_SaIL" % number)
    return r.json()

f.write(get_answer(find_questions("git fetch")[0]).get('items')[0].get('body'))
f.close()



#'https://api.stackexchange.com/2.2/answers?key=U4DMV*8nvpm3EOpvf69Rxw((&site=stackoverflow&page=11644858&pagesize=1&order=desc&sort=votes&filter=!-*jbN.L_SaIL'