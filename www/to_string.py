#!/usr/bin/python2
# -*- encoding=UTF-8 -*-

import sys

def main():
	if len(sys.argv)<2:
		name="index.html"
	else:   name=sys.argv[1]
	f1=open(name,"r")
	f2=open(name+".c","w")
	f2.write('const uint8 html[] =" ');
	for line in f1.readlines():
		s=line.replace("\t","    ").replace("\"","\\\"").replace("\n","\\n").replace("\r","")+"\\\n"
		f2.write(s)
	f2.write('\\000\\000\\000";\n')
	f1.close()
	f2.close()


main()
