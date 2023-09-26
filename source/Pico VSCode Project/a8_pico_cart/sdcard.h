/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __UNO_SDCARD_H
#define __UNO_SDCARD_H

typedef struct {
	char isDir;
	char filename[13];
	char long_filename[32];
	char full_path[210];
} DIR_ENTRY;	// 256 bytes = 256 entries in 64k

extern char errorBuf[];
extern int num_dir_entries;

extern void init_filesystem(void);

extern char *get_filename_ext(char *filename);
extern int is_valid_file(char *filename);

extern int load_file(char *filename);
extern int read_directory(char *path);
extern int search_directory(char *path, char *search);

#endif // __UNO_SDCARD_H
