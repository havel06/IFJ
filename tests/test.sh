#!/bin/bash

testNum=1

# 3 arguments:
# 1. name of test
# 2. tested function params
# 3. correct output
execTest () {
	echo -e "\e[33m--------------------------------\e[0m"
	bash -c "../bin/compiler < $2 > tmp_output.txt 2>&1"
	returnCode=$?
	if [ $returnCode -ne $4 ]; then
		printf "\e[1m\e[31mFailed\e[0m Test %02d: $1:\n" $testNum
		printf "\tWrong return code, expected $4, got $returnCode"
	elif cmp tmp_output.txt $3 >/dev/null 2>&1; then
		printf "\e[1m\e[32mPassed\e[0m Test %02d: $1\n" $testNum
	else
		printf "\e[1m\e[31mFailed\e[0m Test %02d: $1\n" $testNum
		diff tmp_output.txt $3 | colordiff
	fi
	testNum=$((testNum+1))
	rm -f tmp_output.txt
}

execTest "Base test" "input/00_empty.swift" output/00_empty.txt 0
