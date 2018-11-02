#!/bin/bash

declare -a arr=("ensalada" "mdj" "fm9" "cpu" "elDiego" "sAFA")

for i in "${arr[@]}"
do
    cd ../"$1"
    mkdir Debug
    cd ../make
    cp -r ./"$i"/Debug ../"$i"
    cd ../"$i"/Debug
    make
    cd ../../make 
done

