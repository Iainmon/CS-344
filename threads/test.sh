#!/bin/bash

make
./line_processor < input1.txt > output1_test.txt
./line_processor < input2.txt > output2_test.txt
./line_processor < input3.txt > output3_test.txt

cmp --silent output1.txt output1_test.txt && echo "files are the same" || echo "files are different"
cmp --silent output2.txt output2_test.txt && echo "files are the same" || echo "files are different"
cmp --silent output3.txt output3_test.txt && echo "files are the same" || echo "files are different"

rm -rf output1_test.txt output2_test.txt output3_test.txt
rm -rf ./line_processor