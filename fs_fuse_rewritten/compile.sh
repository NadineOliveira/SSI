#!/bin/bash

function testLibcurl(){ 
  ldconfig -p | grep libcurl 
  }

function testNCurses(){
   ldconfig -p | grep ncurses 
}

curlVal="$(testLibcurl)"
ncusesVal="$(testNCurses)"

curlExists=false
ncursesExists=false

if [ ! -z "$curlVal" ]
then
  curlExists=true
fi


if [ ! -z "$ncusesVal" ]
then
  ncursesExists=true
fi

echo "---------------------------------------------------------------------------"

if($curlExists)
then 
  echo "livraria curl detetada"
  echo "começando compilação"
  gcc -Wall teste.c `pkg-config fuse3 libcurl --cflags --libs` -o teste
  echo "terminada compilação"
else
  echo "livraria curl não foi detetada no seu computador"
  echo "para instalar(assumindo que está numa destruição ubuntu) use o comando 'sudo apt-get install libcurl4-openssl-dev'"
fi

echo "---------------------------------------------------------------------------"
