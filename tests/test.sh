#!/bin/bash

testNum=1

# arguments:
# 1. name of test
# 2. input file
# 3. expected output file
# 4. expected return code
execTest () {
	echo -e "\e[33m--------------------------------\e[0m"
	bash -c "../bin/compiler < $2 > tmp_output.txt 2>&1"
	ic23int tmp_output.txt > tmp_output2.txt
	printf "\n" >> tmp_output2.txt
	returnCode=$?
	if [ $returnCode -ne $4 ]; then
		printf "\e[1m\e[31mFailed\e[0m Test %02d: $1:\n" $testNum
		printf "\tWrong return code, expected $4, got $returnCode"
	elif [ -z "$(diff --ignore-trailing-space --ignore-blank-lines tmp_output2.txt $3)" ]; then
		printf "\e[1m\e[32mPassed\e[0m Test %02d: $1\n" $testNum
	else
		printf "\e[1m\e[31mFailed\e[0m Test %02d: $1\n" $testNum
		diff tmp_output2.txt $3 | colordiff
	fi
	testNum=$((testNum+1))
	rm -f tmp_output.txt tmp_output2.txt
}

execTest "Empty program" "input/00_empty.swift" output/00_empty.txt 0
