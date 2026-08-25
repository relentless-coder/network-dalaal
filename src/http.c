#include "http.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>

static int has_end_of_line(buf_t *buf, int start) {
  int i = start;
  for (; i < (int)buf->len - 1; i++) {
    if (buf->data[i] == '\r') {
      if (buf->data[i + 1] == '\n') {
        return i;
      }
    }
  }
  return -1;
}

static void set_http_method(char *data, http_request_t *http_req, size_t len) {
  if (len == 3) {
    if (memcmp(data, "GET", 3) == 0) {
      http_req->method = METHOD_GET;
    } else if (memcmp(data, "PUT", 3) == 0) {
      http_req->method = METHOD_PUT;
    }
  } else if (len == 4 && memcmp(data, "POST", 4) == 0) {
    http_req->method = METHOD_POST;
  } else if (len == 5 && memcmp(data, "PATCH", 5) == 0) {
    http_req->method = METHOD_PATCH;
  } else if (len == 6 && memcmp(data, "DELETE", 6) == 0) {
    http_req->method = METHOD_DELETE;
  } else if (len == 7 && memcmp(data, "OPTIONS", 7) == 0) {
    http_req->method = METHOD_OPTIONS;
  } else {
    http_req->method = METHOD_UNKNOWN;
  }
}

static void set_http_version(char *data, http_version_t *version, size_t len) {
  if (len == 8) {
    if (memcmp(data, "HTTP/1.1", 8) == 0) {
      *version = HTTP_1_1;
    } else if (memcmp(data, "HTTP/1.0", 8) == 0) {
      *version = HTTP_1_0;
    }
  } else {
    *version = HTTP_VERSION_UNKNOWN;
  }
}

static void set_http_path(char *data, http_request_t *http_req, size_t len) {
  http_req->path = data;
  http_req->path_len = len;
}

static int parse_content_length_value(char *data, size_t value_start,
                                      size_t line_end) {
  size_t n = 0;
  for (size_t i = value_start; i < line_end; i++) {
    char c = data[i];
    if (c < '0' || c > '9') {
      return -1;
    }
    n = n * 10 + (c - '0');
  }
  return (int)n;
}

static size_t remove_space(char *data, size_t start) {
  while (data[start] == ' ') {
    start++;
  }
  return start;
}

static int set_http_status(buf_t *buf, http_response_t *http_res, size_t len,
                           size_t start) {
  int value = str_extract_int(buf->data, start, start + len);
  if (value == -1)
    return PARSE_ERROR;
  http_res->status_code = value;
  return PARSE_OK;
}


parse_status_t http_parse_request(buf_t *buf, http_request_t *http_req,
                                  size_t *bytes_consumed) {
  size_t end_of_line = has_end_of_line(buf, 0);
  if (end_of_line == (size_t)-1) {
    return PARSE_NEED_DATA;
  }
  memset(http_req, 0, sizeof(*http_req));
  size_t method_len = 0;
  size_t path_len = 0;
  size_t version_len = 0;
  size_t path_start = 0;
  size_t version_start = 0;
  size_t i = 0;
  for (; i < buf->len; i++) {
    if (buf->data[i] == ' ') {
      i++;
      path_start = i;
      break;
    } else {
      method_len++;
    }
  }
  for (; i < buf->len; i++) {
    if (buf->data[i] == ' ') {
      i++;
      version_start = i;
      break;
    } else {
      path_len++;
    }
  }
  if (version_start > end_of_line) {
    return PARSE_ERROR;
  }
  if (buf->data[end_of_line] == '\r' && buf->data[end_of_line + 1] == '\n') {
    version_len = end_of_line - version_start;
  }
  if (method_len == 0 || path_len == 0 || version_len != 8) {
    return PARSE_ERROR;
  }
  set_http_method(buf->data, http_req, method_len);
  set_http_path(buf->data + path_start, http_req, path_len);
  set_http_version(buf->data + version_start, &http_req->version, version_len);
  if (http_req->version == HTTP_1_1) {
    http_req->keep_alive = 1;
  }
  size_t start = end_of_line + 2;
  fprintf(stdout, "version_start %zu, method_len %zu, path_len %zu\n",
          version_start, method_len, path_len);
  while (1) {
    size_t line_end = has_end_of_line(buf, start);
    fprintf(stdout, "has line end %zu\n", line_end);
    if (line_end == (size_t)-1) {
      return PARSE_NEED_DATA;
    }
    if (line_end == start) {
      start += 2;
      break;
    }
    int header_name_len = 0;
    for (size_t i = start; i < line_end; i++) {
      if (buf->data[i] == ':') {
        if (str_nocase_cmp(buf->data + start, "Content-Length",
                           header_name_len) == 0) {
          i = remove_space(buf->data, i + 1);
          int value = str_extract_int(buf->data, i + 1, line_end);
          if (value == -1) {
            return PARSE_ERROR;
          }
          http_req->content_length = value;

        } else if (str_nocase_cmp(buf->data + start, "Connection",
                                  header_name_len) == 0) {
          i = remove_space(buf->data, i + 1);
          if (buf->data[i] == 'c' || buf->data[i] == 'C') {
            http_req->keep_alive = 0;
          }
        }
      } else {
        header_name_len++;
      }
    }
    start = line_end + 2;
  }
  *bytes_consumed = start;
  printf("bytes consumed %zu\n", *bytes_consumed);
  return PARSE_OK;
}

parse_status_t http_parse_response(buf_t *buf, http_response_t *http_res,
                                   size_t *bytes_consumed) {
  size_t end_of_line = has_end_of_line(buf, 0);
  if (end_of_line == (size_t)-1) {
    return PARSE_NEED_DATA;
  }
  memset(http_res, 0, sizeof(*http_res));
  size_t version_len = 0;
  size_t status_len = 0;
  size_t status_start = 0;
  size_t i = 0;
  for (; i < buf->len; i++) {
    if (buf->data[i] == ' ') {
      i++;
      status_start = i;
      break;
    } else {
      version_len++;
    }
  }

  for (; i < buf->len; i++) {
    if (buf->data[i] == ' ') {
      i++;
      break;
    } else {
      status_len++;
    }
  }

  if (status_start > end_of_line) {
    return PARSE_ERROR;
  }
  if (status_len != 3) {
    return PARSE_ERROR;
  }
  set_http_version(buf->data, &http_res->version, version_len);
  int status_parse_status =
      set_http_status(buf, http_res, status_len, status_start);
  if (status_parse_status == -1)
    return PARSE_ERROR;
  if (http_res->version == HTTP_1_1) {
    http_res->keep_alive = 1;
  }
  size_t start = i + 2;
}
