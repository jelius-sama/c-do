build: 
	mkdir -p bin db
	clang -DDEBUG -o bin/c-do src/main.c src/cjson/cJSON.c -l sqlite3 -Wall -Wextra

release:
	mkdir -p bin db
	clang -o bin/c-do src/main.c src/cjson/cJSON.c -l sqlite3 -Wall -Wextra
