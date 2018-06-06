#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include "sparse_format.h"

#define BLOCK_SIZE 4096
#define MB 1024 * 1024

struct file_list {
	char f_name[300];
	unsigned long f_size;
	int f_cnt;
};
static struct file_list f_list_t[200];

char current_dir[150];
char name[10],prefix[10],line[100];
int count, total_chunk;
unsigned long int all_size;
sparse_header_t out_file_header;

void sort(int n) {
	int i,j;
	struct file_list temp;
    for(j=0;j<n-1;j++) {
        for(i=0;i<n-j-1;i++) {
            if(f_list_t[i].f_cnt > f_list_t[i+1].f_cnt) {
                temp = f_list_t[i];
                f_list_t[i] = f_list_t[i+1];
                f_list_t[i+1] = temp;
            }
        }
    }
}

int scandir(char *dir, char *fname, char *ext) {
	struct _finddata_t fa;
	long handle;
	int cnt = 0;
	char temp[15], dir_d[150];

	sprintf(dir_d, "%s", dir);
    if((handle = _findfirst(strcat(dir_d,"\\*"),&fa)) == -1L)
    {
        // printf("The Path %s is wrong!\n",dir_d);
        return 0;
    }

    do
    {
        if((fa.attrib == _A_ARCH||_A_HIDDEN||_A_RDONLY||_A_SUBDIR||_A_SYSTEM)
				&& strcmp(fa.name, ".") && strcmp(fa.name, "..")
				&& !strncmp(fa.name, fname,strlen(fname))) {

			char *fa_ext = strrchr(fa.name, '.');
			if (fa_ext != NULL && !strncmp(fa_ext, ext,strlen(ext))) {
            	//printf("%s , size is %u\n",fa.name, fa.size);
            	sprintf(f_list_t[cnt].f_name,"%s",fa.name);
            	f_list_t[cnt].f_size = fa.size;
            	sscanf(fa.name, "%[^_]_%d.img", temp, &f_list_t[cnt].f_cnt);
            	//printf("f_cnt is %d\n", f_list_t[cnt].f_cnt);
            	cnt++;
        	}
        }
    } while(_findnext(handle,&fa) == 0);

    // printf("\n");
    _findclose(handle);
    
    sort(cnt);

    return cnt;
}

void writeFile(FILE* out_fd, FILE* in_fd, FILE* ini_fd,long int size) {
    long int length, total_size = 0;
    int chunk = 0, ret;
    chunk_header_t chunk_header_f;

    // printf("this file size is %ld\n", size);

    while (total_size <= size) {
        if(fgets(line, sizeof(line), ini_fd) == NULL) {
            // printf("reach the last line, break!\n");
            break;
        }
        // printf("writeFile -- > line is %s", line);

        ret = sscanf(line, "(%hx,%hx,%x,%x)",
            &chunk_header_f.chunk_type,&chunk_header_f.reserved1,&chunk_header_f.chunk_sz,
            &chunk_header_f.total_sz);
        if (ret != 4) {
            // printf("error read ini file !!!\n");
            return;
        }

        if (chunk_header_f.chunk_type == CHUNK_TYPE_RAW) {
            fwrite(&chunk_header_f,sizeof(struct chunk_header),1,out_fd);
            length = chunk_header_f.chunk_sz * BLOCK_SIZE;
            // printf("length = %ld\n", length);
            while (length != 0) {
                if (length >= MB) {
                    chunk = MB;
                } else {
                    chunk = length;
                }
                // printf("chunk = %d\n", chunk);
                char *buff = malloc(chunk);
                fread(buff,1,chunk,in_fd);
                fwrite(buff,1,chunk,out_fd);
                length -= chunk;
                total_size += chunk;
                free(buff);
            }
            total_chunk++;
        } else if (chunk_header_f.chunk_type == CHUNK_TYPE_DONT_CARE) {
            fwrite(&chunk_header_f,sizeof(struct chunk_header),1,out_fd);
            total_chunk++;
            if (chunk_header_f.chunk_sz <= 2) {
                chunk = chunk_header_f.chunk_sz * BLOCK_SIZE;
                char *buff = malloc(chunk);
                fread(buff,1,chunk ,in_fd);
                // fwrite(buff,1,chunk_size,out_fd);
                total_size += chunk;
                // printf("chunk type don't care but chunk_size <= 2,\n");
                free(buff);
            } else {
                // printf("read the file end,break!\n");
                break;
            }
        } else {
            // printf("do nothing ...\n");
        }
        // printf("item_zie = %ld\n\n", total_size);
    };
    all_size += total_size;
    // printf("current_zie = %ld\n\n", total_size);
}

void main(int argc, const char *argv[]) {

    char full_name[160], ini_file[160];
    struct stat statbuff;
    struct dirent **namelist;
    FILE *fd, *list_fd, *ini_fd;
    int n,m,ret;
    unsigned long total_size;
    char file_name[15];
	char path_buffer[_MAX_PATH];
	char drive[_MAX_DRIVE];
    char dir[_MAX_DIR];
    char fname[_MAX_FNAME];
    char ext[_MAX_EXT];

    getcwd(current_dir,sizeof(current_dir));
    if (argc <= 1) {
        return;
    }

    if(argv[1] != NULL) {
    	sscanf(argv[1], "%s", file_name);
    	if((_access(file_name, 0)) != -1) {
    		// printf("%s exists !\n", file_name);
    		return;
		} else {
			sprintf(path_buffer,"%s/%s", current_dir,file_name);
			_splitpath(path_buffer, drive, dir, fname, ext);

			// printf("fname is %s, ext is %s\n",fname,ext);
		}
	} else {
		return;
	}
	
	printf("Generating %s ...\n", file_name);

    sprintf(ini_file, "%s/%s.ini", current_dir, fname);

    if ((ini_fd = fopen(ini_file, "r"))  == NULL) {
        printf("%s.ini is not exist !", fname);
        return;
    }
    fgets(line, sizeof(line), ini_fd);
    // printf("header line is %s\n", line);

    n = scandir(current_dir, fname, ext);
    m = n;

    sprintf(full_name,"%s\\%s.img", current_dir, fname);
    fd = fopen(full_name, "wb");
    if(fd < 0) {
    	printf("create %s failed !\n",full_name);
    	return;
	}

    ret = sscanf(line, "(%x,%hx,%hx,%hx,%hx,%x,%x,%x,%x)",
        &out_file_header.magic,&out_file_header.major_version,&out_file_header.minor_version,
        &out_file_header.file_hdr_sz,&out_file_header.chunk_hdr_sz,&out_file_header.blk_sz,
        &out_file_header.total_blks,&out_file_header.total_chunks,&out_file_header.image_checksum);
    if (ret != 9) {
        printf("format error for header! ret = %d\n",ret);
        return;
    }

    if (n < 0) {
        perror("not found\n");
    } else {
        while(ret < n) {
            total_size += f_list_t[ret].f_size;
            ret++;
        }
        ret = 0;
        // printf("total_size is 0x%lx\n", total_size);
        fwrite(&out_file_header, sizeof(struct sparse_header), 1, fd);

        while(ret < n) {
            // printf("%s\n", f_list_t[ret].f_name);
            sprintf(full_name,"%s\\%s", current_dir, f_list_t[ret].f_name);
            // printf("full_name is %s, size is %ld\n", full_name, f_list_t[ret].f_size);
            list_fd = fopen(full_name, "rb");
            writeFile(fd,list_fd,ini_fd,f_list_t[ret].f_size);
            fclose(list_fd);
			ret++;
        }
        // printf("all_zie = %ld\n\n", all_size);
    }
    fclose(fd);
    fclose(ini_fd);

    // printf("total_chunk is %d\n",total_chunk);
    printf("Done\n");
}
