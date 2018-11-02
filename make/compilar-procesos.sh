#!/bin/bash

declare -a arr=("ensalada" "mdj" "fm9" "cpu" "elDiego" "sAFA")

for i in "${arr[@]}"
do
    cp -r ./"$i"/Debug ../"$i"
    cd ../"$i"/Debug
    make
    cd ../../make 
done
"export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:~/workspace/tp-2018-2c-Ensalada-C-sar/ensalada/Debug" >> ~/.bashrc
source ~/.bashrc