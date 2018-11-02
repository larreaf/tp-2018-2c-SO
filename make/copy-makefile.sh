#!/bin/bash

declare -a arr=("mdj" "fm9" "cpu" "elDiego" "sAFA")

for i in "${arr[@]}"
do
   mkdir "$i"
   cd ../"$i"
   cp -r ./Debug ../make/"$i"
   cd ../make/"$i"/Debug
   rm "$i"
   rm *.o
   rm *.d
   rm *.log
   cd ../..
done
