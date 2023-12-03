#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

/*
 * wallpaper.c
 *
 * Usage gcc wallaper.c -o wallpaper 
 * run ./wallpaper 
 * Version: 1.0.3 - 
 *
 * Copyright (c) 2023 Jeremy Stevens
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
 
#define WALLPAPER_VERSION "1.0.3"
#define AUTHOR_NAME "Jeremy Stevens"

// Color codes for the text
#define HEADER "\033[95m"
#define OKBLUE "\033[94m"
#define OKGREEN "\033[92m"
#define WARNING "\033[93m"
#define FAIL "\033[91m"
#define ENDC "\033[0m"
#define BOLD "\033[1m"
#define UNDERLINE "\033[4m"

#define MAX_INPUT_LENGTH 100

#ifdef _WIN32
#include <windows.h>
#include <Lmcons.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

bool directoryExists(const char *path) {
    struct stat statbuf;
    return (stat(path, &statbuf) == 0 && S_ISDIR(statbuf.st_mode));
}

void createDirectory(const char *path) {
#ifdef _WIN32
    CreateDirectory(path, NULL);
#else
    mkdir(path, 0700); // Read, write, execute permissions for the user
#endif
}

void getUserInput(char *input, int length) {
    printf("Please enter your input: ");
    if (fgets(input, length, stdin) != NULL) {
        size_t len = strlen(input);
        if (len > 0 && input[len - 1] == '\n') {
            input[len - 1] = '\0';
        }
    } else {
        fprintf(stderr, "Error reading input.\n");
    }
}

static char picturesPath[256]; // Static variable to store the pictures path

int initializePicturesDirectory() {
#ifdef _WIN32
    char username[UNLEN + 1];
    DWORD username_len = UNLEN + 1;
    GetUserName(username, &username_len);
    snprintf(picturesPath, sizeof(picturesPath), "C:\\Users\\%s\\Pictures", username);
#else
    const char *homeDir = getenv("HOME");
    if (homeDir == NULL) {
        fprintf(stderr, "Unable to find home directory.\n");
        return 1;
    }
    snprintf(picturesPath, sizeof(picturesPath), "%s/Pictures", homeDir);
#endif

    if (!directoryExists(picturesPath)) {
        printf("The Pictures directory does not exist. Creating one...\n");
        createDirectory(picturesPath);
        if (!directoryExists(picturesPath)) {
            fprintf(stderr, "Failed to create Pictures directory.\n");
            return 1;
        }
        printf("Pictures directory created successfully.\n");
    } else {
        printf(OKBLUE "Found Pictures directory: %s\n" ENDC, picturesPath);
    }

    char downloadHistoryPath[256];
    snprintf(downloadHistoryPath, sizeof(downloadHistoryPath), "%s/download_history.txt", picturesPath);
    FILE *downloadHistoryFile = fopen(downloadHistoryPath, "w");
    if (downloadHistoryFile != NULL) {
        printf("Download history file created successfully.\n");
        fclose(downloadHistoryFile);
    } else {
        fprintf(stderr, "Failed to create download history file.\n");
        return 1;
    }

    return 0; // Success
}

void printAppInfo() {
    printf(OKGREEN "Wallpaper v%s\n" ENDC, WALLPAPER_VERSION);
    printf(HEADER "Created by %s\n" ENDC, AUTHOR_NAME);
}

void clearConsole() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

int initializeApp() {
    return initializePicturesDirectory();
}

void getUserPreferences(char *resolution, char *type, char *keyword, int *number, int *sleepDuration) {
    printf("Enter desired wallpaper resolution in 1920x1080 format: ");
    getUserInput(resolution, MAX_INPUT_LENGTH);

    printf("Enter your desired wallpaper type (keyword/random): ");
    getUserInput(type, MAX_INPUT_LENGTH);

    if (strcmp(type, "keyword") == 0) {
        printf("Please enter your desired wallpaper keyword: ");
        getUserInput(keyword, MAX_INPUT_LENGTH);
    } else {
        strcpy(keyword, "");
    }

    char numberInput[MAX_INPUT_LENGTH];
    printf("Enter the number of wallpapers you want to download: ");
    getUserInput(numberInput, MAX_INPUT_LENGTH);
    *number = atoi(numberInput);

    char sleepInput[MAX_INPUT_LENGTH];
    printf("enter time to sleep between downloads in seconds: ");
    getUserInput(sleepInput, MAX_INPUT_LENGTH);
    *sleepDuration = atoi(sleepInput);
}

void downloadWallpaper(int *number, int sleepDuration, const char *type, const char *keyword, const char *path, const char *resolution) {
    char downloadHistoryPath[256];
    snprintf(downloadHistoryPath, sizeof(downloadHistoryPath), "%s/download_history.txt", path);
    FILE *downloadHistoryFile = fopen(downloadHistoryPath, "a+");

    if (downloadHistoryFile == NULL) {
        fprintf(stderr, "Failed to open download history file.\n");
        return;
    }

    char line[1024];
    int downloadedCount = 0;
    while (downloadedCount < *number) {
        char url[256];
        char imageName[256];
        char command[512];
        char hashCommand[512];
        char hashValue[128];

        // Generate URL based on type
        if (strcmp(type, "keyword") == 0) {
            snprintf(url, sizeof(url), "https://source.unsplash.com/featured/%s/?%s", resolution, keyword);
        } else {
            snprintf(url, sizeof(url), "https://source.unsplash.com/random/%s", resolution);
        }

        // Generate a unique name for the image
        snprintf(imageName, sizeof(imageName), "%s/wallpaper_%d.jpg", path, downloadedCount + 1);

        // Download the image using curl with -L to follow redirects (must have the -L flag)
        snprintf(command, sizeof(command), "curl -L -o \"%s\" \"%s\"", imageName, url);
        int result = system(command);

        if (result != 0) {
            fprintf(stderr, "Error occurred during downloading. Exit code: %d\n", result);
            continue; // Skip further processing for this download
        }

        // Generate a unique identifier for the image (hash)
        snprintf(hashCommand, sizeof(hashCommand), "md5sum \"%s\" | cut -d ' ' -f 1", imageName);
        FILE *hashPipe = popen(hashCommand, "r");
        if (hashPipe == NULL) {
            fprintf(stderr, "Failed to compute hash for the image.\n");
            remove(imageName); // Remove the downloaded image
            break;
        }
        fgets(hashValue, sizeof(hashValue), hashPipe);
        pclose(hashPipe);

        // Check if the hash is already in download history
        bool alreadyDownloaded = false;
        rewind(downloadHistoryFile);
        while (fgets(line, sizeof(line), downloadHistoryFile)) {
            if (strstr(line, hashValue) != NULL) {
                alreadyDownloaded = true;
                break;
            }
        }

        if (!alreadyDownloaded) {
            // Add the hash to download history
            fprintf(downloadHistoryFile, "%s\n", hashValue);
            fflush(downloadHistoryFile);

            printf("Downloaded wallpaper %d/%d\n", downloadedCount + 1, *number);
            downloadedCount++;

            // Sleep between downloads
            sleep(sleepDuration);
        } else {
            printf("Skipping already downloaded image.\n");
            remove(imageName); // Remove the duplicate image
        }
    }

    fclose(downloadHistoryFile);
}


void performWallpaperDownload(const char *resolution, const char *type, const char *keyword, int number, int sleepDuration) {
    time_t start = time(NULL);
    downloadWallpaper(&number, sleepDuration, type, keyword, picturesPath, resolution);
    time_t end = time(NULL);
    printf("Time taken: %.2fs\n", difftime(end, start));
}

int main() {
    printAppInfo();
    sleep(2);
    clearConsole();

    if (initializeApp() != 0) {
        fprintf(stderr, "Failed to initialize application.\n");
        return 1;
    }

    char resolution[MAX_INPUT_LENGTH];
    char type[MAX_INPUT_LENGTH];
    char keyword[MAX_INPUT_LENGTH];
    int number, sleepDuration;

    getUserPreferences(resolution, type, keyword, &number, &sleepDuration);
    performWallpaperDownload(resolution, type, keyword, number, sleepDuration);

    return 0;
}
