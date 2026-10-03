#!/bin/bash

make test_loader > info.txt 2>/dev/null; make test_loader 2> errors.txt; make test_loader > all.txt 2>&1; make test_loader 2>&1 | grep "JVMR" > filtered.txt

echo "-----INFO-----"; cat info.txt; echo "-----ERROR-----"; cat errors.txt; echo "-----ALL-----"; cat all.txt; echo "-----FILTERED-----"; cat filtered.txt; rm -rfv info.txt errors.txt all.txt filtered.txt
