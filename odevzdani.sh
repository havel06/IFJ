#!/bin/sh

make docs
mkdir -p /tmp/ifj_odevzdani
cp -r src/. /tmp/ifj_odevzdani/
cp makefile_odevzdani /tmp/ifj_odevzdani/Makefile
cp rozdeleni /tmp/ifj_odevzdani/
cp docs/out/dokumentace.pdf /tmp/ifj_odevzdani/
cd /tmp/ifj_odevzdani && zip xhavli65.zip *
cd -
mv /tmp/ifj_odevzdani/xhavli65.zip xhavli65.zip
rm -r /tmp/ifj_odevzdani
