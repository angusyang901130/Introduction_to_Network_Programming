#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    uint32_t magic;
    int32_t  off_str;
    int32_t  off_dat;
    uint32_t n_files;
} __attribute__((packed)) pako_header_t;

typedef struct {
    uint32_t off_str;
    uint32_t file_size;
    uint32_t off_cont;
    uint64_t checksum;
} __attribute__((packed)) FILE_E;

typedef struct {
    char* file_name;
    char* content;
} file_t;

char* make_path(char* path, char* file_name){
    char* path_name = (char*)calloc(strlen(file_name)+strlen(path)+2, sizeof(char));
    memcpy(path_name, path, strlen(path));
    path_name[strlen(path)] = '/';
    memcpy(&path_name[strlen(path)+1], file_name, strlen(file_name));

    return path_name;
}

uint64_t cut_stoull(char* content, int pos){
    char* str = (char*)calloc(9, sizeof(char));
    memcpy(str, &content[pos], 8);
    uint64_t* value = (uint64_t*)str;
    return *value;
}

int main(int argc, char* argv[]){

    //printf("%s", argv[0]);
    int fd = open(argv[1], O_RDONLY);
    //printf("Opening %s", *argv[1]);
    //printf("%d\n", fd);

    // Read header
    pako_header_t h;
    ssize_t h_byte = read(fd, &h, sizeof(pako_header_t));
    h.magic = __builtin_bswap32(h.magic);
    //printf("%d\n", h.n_files);

    // Read files information
    FILE_E files[h.n_files];
    for(int i = 0; i < h.n_files; i++){
        ssize_t file_byte = read(fd, &files[i], sizeof(FILE_E));
        files[i].file_size = __builtin_bswap32(files[i].file_size);
        //printf("file%d size: %d\n", i+1, files[i].file_size);
        files[i].checksum = __builtin_bswap64(files[i].checksum);
        //printf("file%d string offset: %d\n", i+1, files[i].off_str);
        //printf("file%d file size: %d\n", i+1, files[i].file_size);
        //printf("file%d content offset: %d\n", i+1, files[i].off_cont);
        //printf("\n");
    }

    file_t file_info[h.n_files];

    // Read files name
    off_t str_sec = lseek(fd, h.off_str, SEEK_SET);

    for(int i = 0; i < h.n_files; i++){
        int len = 0;
        if(i != h.n_files-1){
            len = files[i+1].off_str - files[i].off_str;
        }else len = h.off_dat - files[i].off_str;

        file_info[i].file_name = (char*)calloc(len, sizeof(char));

        ssize_t name_byte = read(fd, file_info[i].file_name, len);
        //printf("file%d name: %s\n", i+1, file_info[i].file_name);
    }

    // Read file content
    off_t cont_sec = lseek(fd, h.off_dat, SEEK_SET);

    for(int i = 0; i < h.n_files; i++){
        off_t file_cont = lseek(fd, cont_sec + files[i].off_cont, SEEK_SET);

        file_info[i].content = (char*)calloc(files[i].file_size+1, sizeof(char));
        ssize_t cont_byte = read(fd, file_info[i].content, files[i].file_size);
        //printf("file%d content:\n%s\n", i+1, file_info[i].content);
    }

    int file_num = h.n_files;
    
    // checksum
    for(int i = 0; i < h.n_files; i++){
        uint64_t checksum = 0;
        off_t file_cont = lseek(fd, h.off_dat+files[i].off_cont, SEEK_SET);

        for(int j = 0; j < (files[i].file_size-1)/8 + 1; j++){
            uint64_t num = cut_stoull(file_info[i].content, j*8);
            
            checksum = checksum ^ num;
            //printf("%lu\n", checksum);
        }
        
        if(files[i].checksum == checksum){
            //printf("file%d is correct\n", i+1);
            char* path_name = make_path(argv[2], file_info[i].file_name);

            int w_fd = open(path_name, O_WRONLY | O_CREAT , S_IRWXU);
            //printf("%d\n", w_fd);
            ssize_t wbytes = write(w_fd, file_info[i].content, files[i].file_size);
            int ret = close(w_fd);

            printf("file name: %s\n", file_info[i].file_name);
            printf("file size: %d\n\n", files[i].file_size);
        }else{
            //printf("file%d is incorrect\n", i+1);
            file_num--;
            //printf("file%d real: %lu\n", i+1, files[i].checksum);
            //printf("calculated: %lu\n", checksum);
        }
    }

    printf("Total file num: %d\n", file_num);
    int pack_ret = close(fd);
}