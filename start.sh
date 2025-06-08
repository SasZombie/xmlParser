#bin/bash

g++ -g main.cpp xmlparser.cpp -o build/main -std=c++23 -Wall -Wextra -Wformat-nonliteral -Wcast-align -Wpointer-arith -Wmissing-declarations -Winline -Wundef -Wcast-qual -Wshadow -Wwrite-strings -Wno-unused-parameter -Wfloat-equal -pedantic -fsanitize=address -fsanitize=leak

./build/main