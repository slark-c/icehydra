#!/bin/sh

 
./client_test -P test2-Publisher -S test2-Subscriber-1 -i 101,103,104 -s To101103104 -D

./client_test -P test2-Publisher -S test2-Subscriber-2 -i 100 -s To100  -D

./client_test -P test2-Publisher -S test2-Subscriber-3 -D

./client_test -P test2-Publisher -S test2-Subscriber-4 -D

./client_test -P test2-Publisher -S test2-Subscriber-5 -D
