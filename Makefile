all: 
	mkdir -p bin db
	clang -o bin/c-do src/main.c src/cjson/cJSON.c -l sqlite3 -Wall -Wextra
