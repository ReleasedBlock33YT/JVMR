#include <errno.h>
#include <dirent.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <zlib.h>
#include "classloader.h"

typedef struct { JVMR_Class klass; uint8_t *data; size_t size; } LoadedClass;
struct JVMR_ClassLoader { char **paths; size_t path_count; LoadedClass **classes; size_t class_count; };
static void errorf(char *error,size_t n,const char *message) { if(error&&n){snprintf(error,n,"%s",message);} }
static uint16_t le16(const uint8_t *p){return (uint16_t)(p[0]|(p[1]<<8));}
static uint32_t le32(const uint8_t *p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static uint8_t *read_file(const char *path,size_t *size) { FILE *f=fopen(path,"rb"); if(!f)return NULL; if(fseek(f,0,SEEK_END)){fclose(f);return NULL;} long n=ftell(f);if(n<0){fclose(f);return NULL;}rewind(f);uint8_t *data=malloc((size_t)n);if(!data||fread(data,1,(size_t)n,f)!=(size_t)n){free(data);data=NULL;}fclose(f);if(data)*size=(size_t)n;return data; }
static uint8_t *zip_entry(const char *path,const char *entry,size_t *size) {
	size_t archive_size; uint8_t *archive=read_file(path,&archive_size); if(!archive)return NULL;
	uint32_t prefix = archive_size >= 4 && archive[0] == 'J' && archive[1] == 'M' ? 4 : 0;
	size_t start=archive_size>65557?archive_size-65557:0, eocd=archive_size;
	for(size_t i=archive_size;i-- > start;)if(i+4<=archive_size&&le32(archive+i)==0x06054b50u){eocd=i;break;}
	if(eocd==archive_size){free(archive);return NULL;}
	uint16_t entries=le16(archive+eocd+10);uint32_t cd_size=le32(archive+eocd+12),cd_offset=le32(archive+eocd+16)+prefix;if((uint64_t)cd_offset+cd_size>archive_size){free(archive);return NULL;}
	size_t p=cd_offset;for(uint16_t i=0;i<entries&&p+46<=archive_size;i++){if(le32(archive+p)!=0x02014b50u)break;uint16_t name_len=le16(archive+p+28),extra_len=le16(archive+p+30),comment_len=le16(archive+p+32);if(p+46+name_len+extra_len+comment_len>archive_size)break;const char *name=(const char *)(archive+p+46);if(strlen(entry)==name_len&&!memcmp(name,entry,name_len)){uint16_t method=le16(archive+p+10);uint32_t comp=le32(archive+p+20),uncomp=le32(archive+p+24),local=le32(archive+p+42)+prefix;if(local+30>archive_size||le32(archive+local)!=0x04034b50u){free(archive);return NULL;}uint16_t ln=le16(archive+local+26),en=le16(archive+local+28);if((uint64_t)local+30+ln+en+comp>archive_size){free(archive);return NULL;}uint8_t *out=malloc(uncomp?uncomp:1);if(!out){free(archive);return NULL;}int ok=0;if(method==0&&comp==uncomp){memcpy(out,archive+local+30+ln+en,uncomp);ok=1;}else if(method==8){z_stream stream={0};stream.next_in=archive+local+30+ln+en;stream.avail_in=comp;stream.next_out=out;stream.avail_out=uncomp;if(inflateInit2(&stream,-MAX_WBITS)==Z_OK){ok=inflate(&stream,Z_FINISH)==Z_STREAM_END;inflateEnd(&stream);}}if(ok){*size=uncomp;free(archive);return out;}free(out);free(archive);return NULL;}p+=46+name_len+extra_len+comment_len;}
	free(archive);return NULL;
}
JVMR_ClassLoader *jvmr_classloader_create(void){return calloc(1,sizeof(JVMR_ClassLoader));}
void jvmr_classloader_destroy(JVMR_ClassLoader *l){if(!l)return;for(size_t i=0;i<l->class_count;i++){jvmr_class_destroy(&l->classes[i]->klass);free(l->classes[i]->data);free(l->classes[i]);}for(size_t i=0;i<l->path_count;i++)free(l->paths[i]);free(l->paths);free(l->classes);free(l);}
int jvmr_classloader_add_path(JVMR_ClassLoader *l,const char *path){if(!l||!path)return -1;char *copy=strdup(path);if(!copy)return -1;char **next=realloc(l->paths,(l->path_count+1)*sizeof(*next));if(!next){free(copy);return -1;}l->paths=next;l->paths[l->path_count++]=copy;return 0;}
int jvmr_classloader_add_java_runtime(JVMR_ClassLoader *l,const char *java_home){if(!l||!java_home)return -1;char directory[2048];snprintf(directory,sizeof(directory),"%s/jmods",java_home);DIR *dir=opendir(directory);if(!dir)return -1;struct dirent *entry;int added=0;while((entry=readdir(dir))){size_t length=strlen(entry->d_name);if(length>5&&!strcmp(entry->d_name+length-5,".jmod")){char path[4096];snprintf(path,sizeof(path),"%s/%s",directory,entry->d_name);if(jvmr_classloader_add_path(l,path)==0)added++;}}closedir(dir);return added?0:-1;}
const JVMR_Class *jvmr_classloader_load(JVMR_ClassLoader *l,const char *name,char *error,size_t n){if(!l||!name){errorf(error,n,"invalid class loader request");return NULL;}for(size_t i=0;i<l->class_count;i++)if(l->classes[i]->klass.name&&!strcmp(l->classes[i]->klass.name,name))return &l->classes[i]->klass;char entry[1024],jmod_entry[2048];if(strlen(name)+6>=sizeof(entry)){errorf(error,n,"class name too long");return NULL;}snprintf(entry,sizeof(entry),"%s.class",name);snprintf(jmod_entry,sizeof(jmod_entry),"classes/%s",entry);for(size_t i=0;i<l->path_count;i++){struct stat st;if(stat(l->paths[i],&st))continue;size_t size=0;uint8_t *data=NULL;if(S_ISDIR(st.st_mode)){char path[2048];snprintf(path,sizeof(path),"%s/%s",l->paths[i],entry);data=read_file(path,&size);}else{data=zip_entry(l->paths[i],entry,&size);if(!data)data=zip_entry(l->paths[i],jmod_entry,&size);}if(!data)continue;LoadedClass *loaded=calloc(1,sizeof(*loaded));char parse_error[256];if(!loaded){free(data);errorf(error,n,"out of memory");return NULL;}if(jvmr_class_parse(data,size,&loaded->klass,parse_error,sizeof(parse_error))){free(loaded);free(data);continue;}LoadedClass **next=realloc(l->classes,(l->class_count+1)*sizeof(*next));if(!next){jvmr_class_destroy(&loaded->klass);free(loaded);free(data);errorf(error,n,"out of memory");return NULL;}l->classes=next;loaded->data=data;l->classes[l->class_count++]=loaded;return &loaded->klass;}errorf(error,n,"class not found");return NULL;}
