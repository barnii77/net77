#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#include <tchar.h>
#include <direct.h>
#include <io.h>
#define getcwd _getcwd
#else
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <libgen.h>
#endif

typedef struct {
    char suite_name[1024];
    int all_tests_passed;
} TestResult;

// Check if a file is executable
#if defined(_WIN32) || defined(_WIN64)
int is_executable(const char *path) {
    return _access(path, 0) == 0 && strstr(path, ".exe") != NULL;
}
#else
int is_executable(const char *path) {
    struct stat st;
    return (stat(path, &st) == 0) && (st.st_mode & S_IXUSR) && S_ISREG(st.st_mode);
}
#endif

// Case-insensitive search for " all "
int case_insensitive_search(const char *str, const char *substr) {
    size_t str_len = strlen(str);
    size_t substr_len = strlen(substr);

    for (size_t i = 0; i <= str_len - substr_len; i++) {
        int match = 1;
        for (size_t j = 0; j < substr_len; j++) {
            if (tolower((unsigned char)str[i + j]) != tolower((unsigned char)substr[j])) {
                match = 0;
                break;
            }
        }
        if (match) return 1;
    }
    return 0;
}

// Check if the last line contains the word " all " in a case-insensitive manner
int check_last_line(const char *line) {
    if (strncmp(line, "***", 3) != 0) return 0; // Ensure line starts with ***
    return case_insensitive_search(line, " all ");
}

// Run a single test suite and capture its result
int run_suite(const char *suite_name, TestResult *result) {
    printf("########## Running test suite: %s ##########\n", suite_name);
    fflush(stdout);

    FILE *pipe;
    char last_line[1024] = "";
    char buffer[1024];

#if defined(_WIN32) || defined(_WIN64)
    char command[1024];
    snprintf(command, sizeof(command), "\"%s\"", suite_name);
    pipe = _popen(command, "r");
#else
    pipe = popen(suite_name, "r");
#endif

    if (!pipe) {
        fprintf(stderr, "########## Failed to execute test suite ##########\n");
        return -1;
    }

    while (fgets(buffer, sizeof(buffer), pipe)) {
        printf("%s", buffer); // Print test suite output
        strncpy(last_line, buffer, sizeof(last_line) - 1);
        last_line[sizeof(last_line) - 1] = '\0';
    }

#if defined(_WIN32) || defined(_WIN64)
    int status = _pclose(pipe);
#else
    int status = pclose(pipe);
#endif

    printf("########## Test suite %s finished ##########\n\n", suite_name);  // Two newlines

    strncpy(result->suite_name, suite_name, sizeof(result->suite_name) - 1);
    result->suite_name[sizeof(result->suite_name) - 1] = '\0';

    result->all_tests_passed = check_last_line(last_line);
    return status;
}

// Main function
int main(int argc, char **argv) {
    char *dir_path = NULL;
    TestResult results[1024];
    int result_count = 0;
    int all_tests_passed_in_all_suites = 1;  // Track if all tests passed in all suites

#if defined(_WIN32) || defined(_WIN64)
    char exe_path[MAX_PATH];
    if (!GetModuleFileName(NULL, exe_path, MAX_PATH)) {
        fprintf(stderr, "########## Failed to determine executable path ##########\n");
        return EXIT_FAILURE;
    }
    char *last_slash = strrchr(exe_path, '\\');
    if (last_slash) *last_slash = '\0';
    dir_path = exe_path;
#else
    char exe_path[1024];
    ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if (len == -1) {
        perror("########## Failed to determine executable path ##########");
        return EXIT_FAILURE;
    }
    exe_path[len] = '\0';
    dir_path = dirname(strdup(exe_path));
#endif

    printf("########## Discovering test suites in directory: %s ##########\n\n", dir_path);

#if defined(_WIN32) || defined(_WIN64)
    struct _finddata_t file;
    intptr_t handle;
    char search_path[MAX_PATH];
    snprintf(search_path, sizeof(search_path), "%s\\tests_*", dir_path);

    handle = _findfirst(search_path, &file);
    if (handle == -1) {
        fprintf(stderr, "########## No test suites found ##########\n");
        return EXIT_FAILURE;
    }

    do {
        char full_path[MAX_PATH];
        snprintf(full_path, sizeof(full_path), "%s\\%s", dir_path, file.name);

#ifdef NET77_ENABLE_LOGGING
        if (strncmp(file.name, "tests_with_logging_", 19) == 0 && is_executable(full_path)) {
            run_suite(full_path, &results[result_count++]);
        }
#else
        if (strncmp(file.name, "tests_", 6) == 0 && strncmp(file.name, "tests_with_logging_", 19) != 0 && is_executable(full_path)) {
            run_suite(full_path, &results[result_count++]);
        }
#endif
    } while (_findnext(handle, &file) == 0);

    _findclose(handle);
#else
    DIR *dir = opendir(dir_path);
    if (!dir) {
        perror("########## Failed to open directory ##########");
        return EXIT_FAILURE;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);

#ifdef NET77_ENABLE_LOGGING
        if (strncmp(entry->d_name, "tests_with_logging_", 19) == 0 && is_executable(full_path)) {
            run_suite(full_path, &results[result_count++]);
        }
#else
        if (strncmp(entry->d_name, "tests_", 6) == 0 && strncmp(entry->d_name, "tests_with_logging_", 19) != 0 && is_executable(full_path)) {
            run_suite(full_path, &results[result_count++]);
        }
#endif
    }

    closedir(dir);
#endif

    printf("\n########## Test Summary ##########\n");
    for (int i = 0; i < result_count; i++) {
        char *suite_name = strrchr(results[i].suite_name, '/');
#if defined(_WIN32) || defined(_WIN64)
        suite_name = strrchr(results[i].suite_name, '\\');
#endif
        suite_name = suite_name ? suite_name + 1 : results[i].suite_name;

        if (results[i].all_tests_passed) {
            printf("%s: All tests passed!\n", suite_name);
        } else {
            printf("%s: Some tests failed!\n", suite_name);
            all_tests_passed_in_all_suites = 0;
        }
    }

    if (all_tests_passed_in_all_suites) {
        printf("+++ ALL TESTS PASSED +++\n");
    }

    printf("########## Finished running all test suites ##########\n");
    return EXIT_SUCCESS;
}