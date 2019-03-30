from googlesearch import search
import re
import bs4
import requests

def find_questions(git_error):
    results = []

    for url in search(git_error, stop=20):
        if "stackoverflow" in url:
            print(url)
            m = re.search('questions/(.+?)/', url)
            if m:
                number = m.group(1)
            results.append(number)

    return results

def get_answer(number):
    r = requests.get('https://api.stackexchange.com/2.2/answers?key=U4DMV*8nvpm3EOpvf69Rxw(('
                     '&site=stackoverflow&page=%s&pagesize=1&order=desc&sort=votes&filter=!-*jbN.L_SaIL' % number)
    return r.json()



print(get_answer(find_questions("git fetch")[0]))


#'https://api.stackexchange.com/2.2/answers?key=U4DMV*8nvpm3EOpvf69Rxw((&site=stackoverflow&page=1&pagesize=1&order=desc&sort=votes&filter=!-*jbN.L_SaIL'