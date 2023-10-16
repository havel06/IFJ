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
	returnCode=$?
	touch tmp_output2.txt
	if [ "$returnCode" = "0" ]; then
		ic23int tmp_output.txt > tmp_output2.txt
	fi
	printf "\n" >> tmp_output2.txt
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

execTest "Empty program" "input/empty.swift" "output/empty.txt" 0
execTest "Legal variable names" "input/variable_name.swift" "output/empty.txt" 0
execTest "Variable names starting with numbers" "input/variable_name_number.swift" "output/empty.txt" 2
execTest "Variable name as single underscore" "input/variable_name_underscore.swift" "output/empty.txt" 2
execTest "Variable name as keyword" "input/variable_name_keyword.swift" "output/empty.txt" 2
execTest "Legal nil initialization" "input/nil_init.swift" "output/empty.txt" 0
execTest "Illegal nil initialization" "input/nil_init_illegal.swift" "output/empty.txt" 7
execTest "Type deduction" "input/type_deduction.swift" "output/empty.txt" 0
execTest "Nil type deduction" "input/nil_type_deduction.swift" "output/empty.txt" 7
execTest "Comments" "input/comments.swift" "output/empty.txt" 0
execTest "Variable redefinition" "input/variable_redefinition.swift" "output/empty.txt" 3
execTest "Variable shadowing" "input/variable_shadowing.swift" "output/variable_shadowing.txt" 0
execTest "Variable out of scope" "input/variable_out_of_scope.swift" "output/empty.txt" 5
execTest "Undefined variable" "input/undefined_variable.swift" "output/empty.txt" 5
execTest "Default nil initialisation" "input/default_nil_init.swift" "output/default_nil_init.txt" 0
execTest "Uninitialised variable" "input/uninitialised_variable.swift" "output/empty.txt" 5
execTest "Uninitialised variable (init in deeper scope)" "input/uninitialised_variable_in_scope.swift" "output/empty.txt" 5
execTest "Variable initialised in scope" "input/initialised_variable_in_scope.swift" "output/empty.txt" 0
execTest "Function call" "input/function_call.swift" "output/function_call.txt" 0
execTest "Function call before definition" "input/function_call_before_def.swift" "output/function_call.txt" 0
execTest "Undefined function" "input/undefined_function.swift" "output/empty.txt" 3
execTest "Function redefinition" "input/function_redefinition.swift" "output/empty.txt" 3
execTest "Function call with arguments" "input/function_call_args.swift" "output/function_call_args.txt" 0
execTest "Function with wrong parameter id" "input/funct_param_wrong_id.swift" "output/empty.txt" 5
execTest "Function call with wrong parameter name" "input/func_param_wrong_name.swift" "output/empty.txt" 4
execTest "Function call with omitted name" "input/func_param_omit_name.swift" "output/function_call_args.txt" 0
execTest "Function call with illegaly omitted name" "input/func_param_omit_name_illegal.swift" "output/empty.txt" 4
execTest "Function call with wrong parameter type" "input/func_call_wrong_type.swift" "output/empty.txt" 4
execTest "Function call with wrong number of params" "input/func_call_wrong_param_count.swift" "output/empty.txt" 4
