cmake -B build && cmake --build build
cd tests/2-ir-gen/autogen
sh eval_lab2.sh
cat ./eval_result
cd ../../..