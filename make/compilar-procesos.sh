#!/bin/bash

declare -a arr=("mdj" "fm9" "cpu" "elDiego" "sAFA")

for i in "${arr[@]}"
do
     
   cp -r ./"$i" ../"$i"/Debug
   cd ../"$i"/Debug
   make
   cd ../../make   
done