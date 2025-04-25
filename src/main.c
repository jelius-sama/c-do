#include "log.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h> 
#include <netinet/in.h> 
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <limits.h>
#include <sqlite3.h>
#include <time.h>
#include "cjson/cJSON.h"

#define PORT 6969
#define MAX_CONN 5
#define SA struct sockaddr 
#define MAX_REQUEST_SIZE 4096

typedef struct {
     char *data;
     long data_len;
     char *header;
} response;

void hprintf(char **w, char *status, char *content_type) {
	asprintf(w, "HTTP/1.1 %s\r\n"
		     "Content-Type: %s\r\n"
		     "Connection: close\r\n"
		     "\r\n", 
	  status, content_type);
}

response *api_handler(char path[1024], char method[8], char *root_path, char *body) {
	sqlite3 *db;
	char *zErrMsg = 0;
	char *sql_path; asprintf(&sql_path, "%s%s", root_path, "/db/todo.db");
	int rc;
	char *sql, *data = "{\"error\": \"404 - API Not Found!\"}";
	char *header; hprintf(&header, "404 - API Not Found!", "application/json");
	response *resp = (response*)malloc(sizeof(response));

	rc = sqlite3_open(sql_path, &db);
	if(rc) {
		errorf("Could not open database: %s", sqlite3_errmsg(db));
		return NULL;
	}
	
	sql =   "CREATE TABLE IF NOT EXISTS todos ("
		"id INTEGER PRIMARY KEY AUTOINCREMENT,"
		"title TEXT NOT NULL,"
		"description TEXT,"
		"completed BOOLEAN NOT NULL DEFAULT 0,"
		"created_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
		"deleted_at DATETIME"
		");";
	rc = sqlite3_exec(db, sql, NULL, 0, &zErrMsg);
	
	if(rc != SQLITE_OK) {
		errorf("SQL error: %s", zErrMsg);
		sqlite3_free(zErrMsg);
		return NULL;
	}

	if (strcmp(path, "/api/add") == 0) {
		if (strcmp(method, "POST") != 0) {
			hprintf(&header, "405 Method Not Allowed", "application/json");
			data = "{\"error\": \"405 - Method Not Allowed!\"}";	
		}

		cJSON *json = cJSON_Parse(body);
		const char *title = cJSON_GetObjectItem(json, "title")->valuestring;
		const char *desc = cJSON_GetObjectItem(json, "description")->valuestring;

		sqlite3_stmt *stmt;
		sql = "INSERT INTO todos (title, description) VALUES (?, ?);";
		if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
			sqlite3_bind_text(stmt, 1, title, -1, SQLITE_TRANSIENT);
			sqlite3_bind_text(stmt, 2, desc, -1, SQLITE_TRANSIENT);

			if (sqlite3_step(stmt) != SQLITE_DONE) errorf("Failed to insert TODO.");
			sqlite3_finalize(stmt);
		}
		cJSON_Delete(json);

		hprintf(&header, "200 OK", "application/json");
		asprintf(&data, "{\"message\": \"200 - Added New TODO Successfully!\"}");
	}
	
	if (strcmp(path, "/api/remove") == 0) {
		if (strcmp(method, "DELETE") != 0) {
			hprintf(&header, "405 Method Not Allowed", "application/json");
			data = "{\"error\": \"405 - Method Not Allowed!\"}";
		}

		cJSON *json = cJSON_Parse(body);
		int id = cJSON_GetObjectItem(json, "id")->valueint;
		sqlite3_stmt *stmt;

		sql = "UPDATE todos SET deleted_at = datetime('now') WHERE id = ?;";
		if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
			sqlite3_bind_int(stmt, 1, id);
			if (sqlite3_step(stmt) != SQLITE_DONE) errorf("Failed to soft delete TODO.");
			sqlite3_finalize(stmt);
		}
		cJSON_Delete(json);

		hprintf(&header, "200 OK", "application/json");
		asprintf(&data, "{\"message\": \"200 - Deleted TODO Successfully!\"}");
	}

	if (strcmp(path, "/api/mark") == 0) {
		if (strcmp(method, "POST") != 0) {
			hprintf(&header, "405 Method Not Allowed", "application/json");
			data = "{\"error\": \"405 - Method Not Allowed!\"}";	
		}

		cJSON *json = cJSON_Parse(body);
		int id = cJSON_GetObjectItem(json, "id")->valueint;
		int mark = cJSON_GetObjectItem(json, "completed")->valueint;
		sqlite3_stmt *stmt;

		sql = "UPDATE todos SET completed = ? WHERE id = ?;";
		if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
			sqlite3_bind_int(stmt, 1, mark);
			sqlite3_bind_int(stmt, 2, id);
			if (sqlite3_step(stmt) != SQLITE_DONE) errorf("Failed to update completion status.");
			sqlite3_finalize(stmt);
		}
		cJSON_Delete(json);

		hprintf(&header, "200 OK", "application/json");
		asprintf(&data, "{\"message\": \"200 - Marked %s Successfull!\"}", mark ? "completed" : "not completed");	
	}

	if (strcmp(path, "/api/get_all") == 0) {
		if (strcmp(method, "GET") != 0) {
			hprintf(&header, "405 Method Not Allowed", "application/json");
			data = "{\"error\": \"405 - Method Not Allowed!\"}";	
		}

		sqlite3_stmt *stmt;
		sql = "SELECT id, title, description, completed, created_at FROM todos WHERE deleted_at IS NULL;";
		cJSON *todos = cJSON_CreateArray();

		if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
			while (sqlite3_step(stmt) == SQLITE_ROW) {
				cJSON *todo = cJSON_CreateObject();
				cJSON_AddNumberToObject(todo, "id", sqlite3_column_int(stmt, 0));
				cJSON_AddStringToObject(todo, "title", (const char*)sqlite3_column_text(stmt, 1));
				cJSON_AddStringToObject(todo, "description", (const char*)sqlite3_column_text(stmt, 2));
				cJSON_AddBoolToObject(todo, "completed", sqlite3_column_int(stmt, 3));
				cJSON_AddStringToObject(todo, "created_at", (const char*)sqlite3_column_text(stmt, 4));
				cJSON_AddItemToArray(todos, todo);
			}
			sqlite3_finalize(stmt);
		}

		hprintf(&header, "200 OK", "application/json");
		data = cJSON_PrintUnformatted(todos);
		cJSON_Delete(todos);
	}

	sqlite3_close(db);
	resp->header = strdup(header);
	resp->data = data;
	resp->data_len = strlen(data);
	return resp;
}

response *route_handler(char path[1024], char method[8], char *root_path, char *body) {
	if (strstr(path, "/api/") != NULL) {
		response *api_response = api_handler(path, method, root_path, body);
		return api_response;
	}

	long data_len;
	char *data;
	char *header; hprintf(&header, "404 Not Found", "text/html; charset=utf-8");
	char *file_path; asprintf(&file_path, "%s%s", root_path, "/assets/404.html");

	response *resp = (response*)malloc(sizeof(response));

	if (strcmp(method, "GET") != 0) {
		asprintf(&file_path, "%s%s", root_path, "/assets/405.html");
		hprintf(&header, "405 Method Not Allowed", "text/html; charset=utf-8");
	}

	if (strcmp(path, "/") == 0) {
		asprintf(&file_path, "%s%s", root_path, "/assets/index.html");
		hprintf(&header, "200 OK", "text/html; charset=utf-8");
	}

	if (strcmp(path, "/favicon.ico") == 0) {
		asprintf(&file_path, "%s%s", root_path, "/assets/favicon.ico");
		hprintf(&header, "200 OK", "image/x-icon");
	}

	if (strcmp(path, "/logo.png") == 0) {
		asprintf(&file_path, "%s%s", root_path, "/assets/logo.png");
		hprintf(&header, "200 OK", "image/png");
	}

	FILE *file = fopen(file_path, "rb");
	if (file == NULL) return NULL;

	fseek(file, 0L, SEEK_END);
	data_len = ftell(file);
	fseek(file, 0L, SEEK_SET);

	// Allocate buffer to hold raw bytes
	data = (char*)malloc(data_len);
	if (data == NULL) return NULL;
	fread(data, sizeof(char), data_len, file);
	fclose(file);

	// Assign values to response struct
	resp->header = strdup(header);
	resp->data = data;
	resp->data_len = data_len;

	return resp;
}

int main() {
	char root_path[PATH_MAX];
	
	todof("Make it work on a niche OS for gamers named Doors 😂 or was it Windows 🤣");
	ssize_t len = readlink("/proc/self/exe", root_path, sizeof(root_path) - 1);
	if (len != -1) {
		root_path[len] = '\0';
		for (size_t i = 1; i <= 2; ++i) {
			char *last_slash = strrchr(root_path, '/');
			if (last_slash) *last_slash = '\0';
		}

		infof("Project Root Directory: %s", root_path);
	} else {
		panicf("Could not read the project's root directory, terminating...");
	}

	int sockfd, connfd;
	socklen_t client_len;
	struct sockaddr_in servaddr, client;
	char *err_file_path; asprintf(&err_file_path, "%s%s", root_path, "/assets/500.html");
	FILE *err_file = fopen(err_file_path, "rb");
	char *err_template;
	long err_file_len;
	char *err_header; hprintf(&err_header, "500 Internal Server Error", "text/html; charset=utf-8");

	if (err_file == NULL) panicf("Necessary files not found, terminating...");
	fseek(err_file, 0L, SEEK_END);
	err_file_len = ftell(err_file);
	fseek(err_file, 0L, SEEK_SET);
	err_template = (char*)malloc(err_file_len);
	if (err_template == NULL) panicf("Not enough memory, terminating...");
	fread(err_template, sizeof(char), err_file_len, err_file);
	fclose(err_file);

	// Creating socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		free(err_template);
		panicf("socket creation failed...");
	}

	// Assigning IP & PORT
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
	servaddr.sin_port = htons(PORT);

	// Binding the socket
	if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) { 
		free(err_template);
		close(sockfd);
		panicf("socket bind failed...");
	}

	// Listening for connections
	if ((listen(sockfd, MAX_CONN)) != 0) {
		free(err_template);
		close(sockfd);
		panicf("could not listen for incoming connections...");
	}

	infof("Server listening on port %d", PORT);
	todof("Make the client connection either multithreaded or asynchronous so that it can handle multiple connections at a time.");

	// Accepting the connections
	for (;;) {
		client_len = sizeof(client);
		connfd = accept(sockfd, (SA*)&client, &client_len); 
		if (connfd < 0) {
			errorf("could not accept client...\n");
			continue;
		}

		char buffer[MAX_REQUEST_SIZE];
		memset(buffer, 0, MAX_REQUEST_SIZE);
		ssize_t bytes_read = read(connfd, buffer, sizeof(buffer) - 1);

		if (bytes_read > 0) {
			buffer[bytes_read] = '\0';

			char method[8], path[1024];
			sscanf(buffer, "%7s %1023s", method, path);
			char *body = NULL;

			char *body_start = strstr(buffer, "\r\n\r\n");
			if (body_start != NULL) {
				body_start += 4;
				body = body_start;
			}

			response *raw_resp = route_handler(path, method, root_path, body);

			if (raw_resp == NULL) {
				write(connfd, err_header, strlen(err_header));
				write(connfd, err_template, err_file_len);
				close(connfd);
        		        errorf("Encountered error when processing request.");
				continue;
			} else {
				write(connfd, raw_resp->header, strlen(raw_resp->header));
				write(connfd, raw_resp->data, raw_resp->data_len);
				free(raw_resp->data);
				free(raw_resp->header);
				free(raw_resp);
				close(connfd);
			}
		}
	}

	free(err_template);
	close(sockfd);
	return 0;
}
