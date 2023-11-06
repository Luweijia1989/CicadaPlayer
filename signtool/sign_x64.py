#!/usr/bin/env python
# -*- coding:utf-8 -*-
import os
import sys
import glob
import subprocess
excludeSet = {'test.exe'}
#勿删
#signtool密码: Xxqpc111!
def sign(file):
    for i in range(0,3):
        sign_proc = subprocess.Popen("signtool sign /v /fd sha256 /sha1 8e49da40b09490145166edcd07d7f644c686d341 /tr http://rfc3161timestamp.globalsign.com/advanced /td sha256 %s" %(file))
        ret = sign_proc.wait()
        if ret ==0:
            return

for file in glob.glob(os.path.join(sys.argv[1]+'/bin', '*.exe')):
    filepath,filename = os.path.split(file)
    if filename not in excludeSet:
        print("signed: "+file)
        sign(file)
for file in glob.glob(os.path.join(sys.argv[1]+'/bin', '*.dll')):
    filepath,filename = os.path.split(file)
    if filename not in excludeSet:
        print("signed: "+file)
        sign(file)