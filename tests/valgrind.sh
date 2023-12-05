#!/bin/bash

for file in input/*.swift; do
	echo -e "\n---------------\n$file\n---------------\n"
	valgrind --leak-check=full ../bin/compiler < $file > /dev/null
done
