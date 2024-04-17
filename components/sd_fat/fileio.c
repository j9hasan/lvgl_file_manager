
#include "esp_system.h"
#include <esp_log.h>
#include "esp_event.h"
#include "driver/gpio.h"
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "lvgl.h"
#include <dirent.h>
#include <string.h>
#include "ui.h"

// Use POSIX and C standard library functions to work with files.First create a file.
const char *file_hello = MOUNT_POINT "/hello.txt";

printf("Opening file %s", file_hello);
FILE *f = fopen(file_hello, "w");
if (f == NULL)
{
    ESP_LOGE(TAG, "Failed to open file for writing");
    return;
}
fprintf(f, "Hello %s!\n", card->cid.name);
fclose(f);
printf("File written");

const char *file_foo = MOUNT_POINT "/foo.txt";

// Check if destination file exists before renaming
struct stat st;
if (stat(file_foo, &st) == 0)
{
    // Delete it if it exists
    unlink(file_foo);
}

// Rename original file
printf("Renaming file %s to %s", file_hello, file_foo);
if (rename(file_hello, file_foo) != 0)
{
    ESP_LOGE(TAG, "Rename failed");
    return;
}

// Open renamed file for reading
printf("Reading file %s", file_foo);
f = fopen(file_foo, "r");
if (f == NULL)
{
    ESP_LOGE(TAG, "Failed to open file for reading");
    return;
}

// Read a line from file
char line[64];
fgets(line, sizeof(line), f);
fclose(f);

// Strip newline
char *pos = strchr(line, '\n');
if (pos)
{
    *pos = '\0';
}
printf("Read from file: '%s'", line);

All done, unmount partition and disable SPI peripheral
              esp_vfs_fat_sdcard_unmount(mount_point, card);
printf("Card unmounted");
