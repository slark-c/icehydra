#!/bin/sh

 
./client_test -P test1-Publisher -S test1-Subscriber-1 -D

./client_test -P test1-Publisher -S test1-Subscriber-2 -i 10 -s helloworld 
