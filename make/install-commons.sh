#!/bin/bash
cd ../../..
git clone https://github.com/sisoputnfrba/so-commons-library
make
sudo make install
ls -l /usr/include/commons