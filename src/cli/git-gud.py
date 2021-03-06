import sys, subprocess
import re
from googlesearch import search
import requests
import html2text

h = html2text.HTML2Text()
def find_questions(git_error):
    results = []

    for url in search(git_error, stop=20):
        if "stackoverflow" in url:
            m = re.search('questions/(.+?)/', url)
            if m:
                number = m.group(1)
            results.append(number)
    return results

def get_answer(number):
    r = requests.get("https://api.stackexchange.com/2.2/questions/%s/answers?key=U4DMV*8nvpm3EOpvf69Rxw((&site=stackoverflow&page=1&pagesize=1&order=desc&sort=votes&filter=!-*jbN.L_SaIL" % number)
    return r.json()


def get_command():

    args = sys.argv

    if len(args) == 1:
        print("Error. Provide git argument.")
        return;

    strArgs = ''
    args[0] = "git"
    for arg in args:
        if " " in arg:
            arg = '\"%s\"' % arg
        strArgs += '%s ' % arg

    try:
        out = subprocess.run(strArgs, check=True, stderr=subprocess.PIPE, shell=True)
    except subprocess.CalledProcessError as e:
        err = e.stderr.decode("ascii")
        print(h.handle(get_answer(find_questions(err)[0]).get('items')[0].get('body')))

    return;

get_command()