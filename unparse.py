from bs4 import BeautifulSoup
import sys,re

doc = BeautifulSoup(sys.stdin.read())

space = re.compile('[ \t\n\r]+')

itagstart = re.compile('<(i|b|u|a)([^>]*)> +')
itagend = re.compile(' +</(i|b|u|a)')
ipunct = re.compile('</(i|b|u|a)> +([^a-zA-Z0-9])')
ipuncthead = re.compile('([^a-zA-Z0-9]) +<(i|b|u|a)([^>]*)>')

for p in doc.findAll('p'):
    s = p.decode_contents().strip()
    s = space.sub(' ',s)
    s = itagstart.sub('<\\1\\2>',s)
    s = itagend.sub('</\\1',s)
    s = ipunct.sub('</\\1>\\2',s)
    s = ipuncthead.sub('\\1<\\2\\3>',s)
    s = s.strip()
    if s:
        print(s,'\n')
