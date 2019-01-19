#!/bin/bash

function testLibcurl(){
  ldconfig -p | grep libcurl
}

function testFuse3(){
	ldconfig -p | grep fuse3
}

curlVal = "$(testLibcurl)"
fuseVal = "$(testFuse3)"

curlExists=false
fuseExists=false

if [ ! -z "$curlVal" ]
then
  curlExists=true
fi

if [ ! -z "$fuseVal" ]
then
  fuseExists=true
fi



if($curlExists)
then 
  echo "library curl detected in your computer"
else
  echo "curl not detected in your computer, install it with 'sudo apt-get install libcurl4-openssl-dev'(if your ubuntu, if search for an alternative)"
fi

if($fuseExists)
then 
  echo "library fuse3 detected in your computer"
  echo "if there's an error when trying to compile please check that the library is in /lib/x86_64-linux-gnu/ and not somewhere else"
else
  echo "fuse3 not detected in your computer"
  echo "unfortunatly we have no idea how to fix this, and we can't run the program without it"
  echo "sorry about that"
fi


if(($fuseExists) && ($curlExists))
then
	gcc -Wall $1.c `pkg-config fuse3 --cflags --libs` -o $1
else
  echo "one of the libraries was not detected, compiling won't progress"
