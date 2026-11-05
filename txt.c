#include <stdio.h>
#include <stdlib.h>

#define ERROR_NONE 0
#define ERROR_FILE -1
#define ERROR_LINEBUFF -2
#define ERROR_NULLCTX -3
#define ERROR_NULLPATH -4
#define ERROR_LINEFETCH -5

#define PRINTABLE(c) ((c >= ' ' && c <= '~') || (c == '\t'))
#define WASD(c) ((c == 'W') || (c == 'A') || (c == 'S') || (c == 'D'))

char upper(char c){
	if(c >= 'a' && c <= 'z'){
		c = c - 32;
	}
	return c;
}

char* strnAppend(char* str, int* size, char c){
	char* tmp = malloc(sizeof(char) * (*size + 2));
	for(int i = 0; i < *size; i++){
		tmp[i] = str[i];
	}

	tmp[(*size)++] = c;
	tmp[(*size)] = 0;
	if(str){
		free(str);
	}
	return tmp;
}

char* strnInsert(char* str, int* size, char c, int n){
	char* tmp = NULL;
	int tmp_size = 0;
	for(int i = 0; (i < n) && (i < *size); i++){
		tmp = strnAppend(tmp,&tmp_size,str[i]);
	}
	tmp = strnAppend(tmp,&tmp_size,c);
	for(int i = n; i < *size; i++){
		tmp = strnAppend(tmp,&tmp_size,str[i]);
	}

	if(str){
		free(str);
	}
	
	*size = tmp_size;
	return tmp;
}

char* strnDesert(char* str, int* size, int n){
	if(*size == 0){
		if(str){
			free(str);
		}
		return NULL;
	}

	if(*size == 1){
		if(str){
			free(str);
		}
		*size = 0;
		return NULL;
	}

	char* tmp = malloc(sizeof(char) * (*size));
	int _i = 0;
	for(int i = 0; i < *size; i++){
		tmp[_i] = str[i];
		if(i != n){
			_i++;
		}
	}

	tmp[(*size)] = 0;
	(*size)--;

	if(str){
		free(str);
	}
	return tmp;
}

typedef struct line_s {
	int size;
	char* buffer;
} line_t;

void freelinebuf(line_t** lb, int size){
	if(!lb){
		return;
	}

	if(*lb != NULL){
		for(int i = 0; i < size; i++){
			char* b = (*lb)[i].buffer;
			if(b != NULL){
				free(b);
			}
		}
		free(*lb);
		*lb = NULL;
	}
}

line_t* lineAppend(line_t* lb, int* size, char* b, int s){
	line_t* tmp = malloc(sizeof(line_t) * (*size + 1));
	for(int i = 0; i < *size; i++){
		tmp[i].buffer = lb[i].buffer;
		tmp[i].size = lb[i].size;
	}

	tmp[(*size)].buffer = b;
	tmp[(*size)++].size = s;

	if(lb){
		free(lb);
	}
	return tmp;
}

// file context

typedef struct Fctx_s {
	FILE* file;
	char* filepath;
	line_t* linebuf;
	int linebuf_size;
	int linenum;
	int position;
	int lineheight;
	int linewidth;
} Fctx_t;

Fctx_t Main = {
	NULL,
	NULL,
	NULL,
	0,
	0,
	0,
	11,
	66
};

// FILE* EditFile = NULL;
// line_t* LineBuffers = NULL;
// int LineBuffers_Size = 0;
// int LineNum = 0;
// int Position = 0;

// int LineHeight = 11;
// int LineWitdh = 66;

int mode = 0;
int editmode = 0;

#define MODE_FILE 0
#define MODE_EDIT 1
#define MODE_MOVE 2

void clr(void){
	printf("\e[1;1H\e[2J");
}

void movecaret(Fctx_t* ctx){
	printf("\n[move (%d:%d)] > ",ctx->linenum + 1, ctx->position + 1);
}

void editcaret(Fctx_t* ctx){
	printf("\n[edit (%d:%d)] > ",ctx->linenum + 1, ctx->position + 1);
}

void filecaret(Fctx_t* ctx){
	if(ctx->filepath){
		printf("\n[file (%s)] > Open File: ",ctx->filepath);
	}else{
		printf("\n[file] > Open File: ");
	}
}

void appendcaret(Fctx_t* ctx){
	printf("\n[append (%d:%d)] > ",ctx->linenum + 1, ctx->position + 1);
}

int fileToLines(FILE* file, line_t** linebuf, int* linebuf_size){
	if(!file){
		return ERROR_FILE;	// File Error
	}

	if(!linebuf){
		return ERROR_LINEBUFF; // Line Buffer Error
	}

	if(*linebuf != NULL){
		freelinebuf(linebuf,*linebuf_size);
	}

	*linebuf = malloc(sizeof(line_t));
	(*linebuf)->buffer = NULL;
	(*linebuf)->size = 0;
	*linebuf_size = 1;

	fseek(file,0,SEEK_SET);
	char c = fgetc(file);
	int index = 0;
	while(c != EOF){
		if(c == '\n'){
			// next line
			*linebuf = lineAppend(*linebuf,linebuf_size,NULL,0);
			index++;
			c = fgetc(file);
			continue;
		}

		if((c >= ' ' && c <= '~') || (c == '\t')){
			// append char to line
			(*linebuf)[index].buffer = strnAppend((*linebuf)[index].buffer,&(*linebuf)[index].size,c);
		}
		c = fgetc(file);
	}

	return 0; // no errors
}

int OpenContext(Fctx_t* ctx, char* path, char* mode){
	if(!ctx){
		return ERROR_NULLCTX; // Null Context
	}

	if(ctx->file){
		fclose(ctx->file);
		ctx->file = NULL;
	}

	ctx->file = fopen(path,mode);
	if(!ctx->file){
		return ERROR_FILE; // File Error
	}

	int l = 0;
	for(int i = 0; path[i] != 0; i++){
		l = i;
	}
	l++;

	if(ctx->filepath){
		free(ctx->filepath);
		ctx->filepath = NULL;
	}

	ctx->filepath = malloc(sizeof(char) * (l + 1));
	for(int i = 0; i < l; i++){
		ctx->filepath[i] = path[i];
	}
	ctx->filepath[l] = 0;

	return 0; // no errors
}

int ReopenContext(Fctx_t* ctx, char* mode){
	if(!ctx){
		return ERROR_NULLCTX; // Null Context
	}

	if(ctx->file){
		fclose(ctx->file);
		ctx->file = NULL;
	}

	if(!ctx->filepath){
		return ERROR_NULLPATH; // Null File Path
	}

	ctx->file = fopen(ctx->filepath,mode);
	if(!ctx->file){
		return ERROR_FILE; // File Error
	}

	return 0; // no errors
}

int CloseContext(Fctx_t* ctx){
	if(!ctx){
		return ERROR_NULLCTX; // Null Context
	}

	if(ctx->file){
		fclose(ctx->file);
		ctx->file = NULL;
	}

	if(ctx->linebuf){
		freelinebuf(&ctx->linebuf,ctx->linebuf_size);
	}

	if(ctx->filepath){
		free(ctx->filepath);
		ctx->filepath = NULL;
	}

	return 0; // no errors
}

int FetchLines(Fctx_t* ctx){
	if(!ctx){
		return ERROR_NULLCTX; // Null Context
	}

	return fileToLines(ctx->file,&ctx->linebuf,&ctx->linebuf_size);
}

int DumpLines(Fctx_t* ctx){
	if(!ctx->file){
		return ERROR_FILE;	// File Error
	}

	if(!ctx->linebuf){
		return ERROR_LINEBUFF; // Line Buffer Error
	}

	if(ReopenContext(ctx,"w")){
		printf("Error: Unable to open Context\n");
		return ERROR_FILE;	// File Error
	}

	for(int i = 0; i < ctx->linebuf_size; i++){
		for(int j = 0; j < ctx->linebuf[i].size; j++){
			fputc(ctx->linebuf[i].buffer[j],ctx->file);
		}
		if(i + 1 < ctx->linebuf_size){
			fputc('\n',ctx->file);
		}
	}

	if(ReopenContext(ctx,"r+")){
		printf("Error: Unable to open Context\n");
		return ERROR_FILE;	// File Error
	}

	if(FetchLines(ctx)){
		printf("Error: Unable to fetch Context\n");
		return ERROR_LINEFETCH;	// Line Fetch Error
	}

	return 0; // no errors
}

int ShowContext(Fctx_t* ctx){
	if(!ctx){
		return ERROR_NULLCTX; // Null Context
	}

	if(!ctx->file){
		return ERROR_FILE;	// File Error
	}

	if(!ctx->linebuf){
		return ERROR_LINEBUFF; // Line Buffer Error
	}

	for(int i = ctx->linenum; (i < ctx->linebuf_size) && (i < ctx->linenum + ctx->lineheight); i++){
		if(ctx->linebuf[i].buffer){
			printf("%d: ",i+1);
			int j = 0;
			if(i == ctx->linenum){
				int N = (ctx->position / ctx->linewidth) * ctx->linewidth;
				for(j = N; (j < ctx->linebuf[i].size) && ((j - N) < ctx->linewidth); j++){
					printf("%c",ctx->linebuf[i].buffer[j]);
				}
				if((j < ctx->linebuf[i].size) && ((j - N) >= ctx->linewidth)){
					printf("...");
				}
			}else{
				for(j = 0; (j < ctx->linebuf[i].size) && (j < ctx->linewidth); j++){
					printf("%c",ctx->linebuf[i].buffer[j]);
				}
				if((j < ctx->linebuf[i].size) && (j >= ctx->linewidth)){
					printf("...");
				}
			}
			printf("\n");
		}else{
			printf("%d: \n",i+1);
		}
	}

	return 0; // no errors
}

int Display(Fctx_t* ctx){
	char r;
	if(ctx->linebuf[ctx->linenum].size > 0){
		if(ctx->position >= ctx->linebuf[ctx->linenum].size){
			ctx->position = ctx->linebuf[ctx->linenum].size - 1;
		}

		r = ctx->linebuf[ctx->linenum].buffer[ctx->position];
		if(r != '\t'){
			ctx->linebuf[ctx->linenum].buffer[ctx->position] = '_';
		}
	}
	
	clr();
	if(ShowContext(ctx)){
		printf("Error: Unable to show Context\n");
		CloseContext(ctx);
		return -1;
	}

	if(ctx->linebuf[ctx->linenum].size > 0){
		ctx->linebuf[ctx->linenum].buffer[ctx->position] = r;
	}
	return 0;
}

int main(int argc, char** argv){
	// read args

	// Text Edit Loop

	int loop = 1;
	mode = MODE_FILE;
	editmode = 0; // 0 - insert, 1 - append;
	filecaret(&Main);
	
	char* filestr = NULL;
	int filestrlen = 0;

	while(loop){
		char c = fgetc(stdin);
		switch(mode){
			case MODE_FILE:
				if(PRINTABLE(c)){
					filestr = strnAppend(filestr,&filestrlen,c);
				}
				
				if(c == '\n'){
					if(filestr){
						if(CloseContext(&Main)){
							printf("Error Closing Context\n");
							free(filestr);
							return -1;
						}

						if(OpenContext(&Main,filestr,"r+")){
							if(OpenContext(&Main,filestr,"w+")){
								printf("Error Opening Context\n");
								free(filestr);
								return -1;
							}
						}
						
						if(FetchLines(&Main)){
							printf("Error Fetching Lines\n");
							free(filestr);
							if(CloseContext(&Main)){
								printf("Error: Unable to close Context\n");
							}
							return -1;
						}

						free(filestr);
						filestr = NULL;
						filestrlen = 0;
						mode = MODE_MOVE;
						Display(&Main);
						movecaret(&Main);
					}
				}
			break;

			case MODE_EDIT:
				if(c == '\n'){
					break;
				}

				if(PRINTABLE(c)){
					switch(editmode){
						default:
						case 0:
							Main.linebuf[Main.linenum].buffer = strnInsert(
								Main.linebuf[Main.linenum].buffer,
								&Main.linebuf[Main.linenum].size,
								c,
								Main.position
							);

							Main.position++;
							Display(&Main);
							editcaret(&Main);
						break;

						case 1:
							Main.linebuf[Main.linenum].buffer = strnAppend(
								Main.linebuf[Main.linenum].buffer,
								&Main.linebuf[Main.linenum].size,
								c
							);
							Display(&Main);
							appendcaret(&Main);
						break;
					}
					break;
				}

				printf("[%c %d]\n",c,c);
				if(c == 5){ // Ctrl+E
					mode = MODE_MOVE;
					Display(&Main);
					movecaret(&Main);
					break;
				}

				if(c == 25){ // Ctrl+Y
					if(editmode){
						editmode = 0;
						Display(&Main);
						editcaret(&Main);
					}else{
						editmode = 1;
						Display(&Main);
						appendcaret(&Main);
					}
					break;
				}

				if(c == 6){ // Ctrl+F
					mode = MODE_FILE;
					filecaret(&Main);
					break;
				}

				if(c == 16){ // Ctrl+P
					if(Main.linebuf[Main.linenum].size != 0){
						Main.linebuf[Main.linenum].buffer = strnDesert(
							Main.linebuf[Main.linenum].buffer,
							&Main.linebuf[Main.linenum].size,
							Main.position
						);
					}
					Display(&Main);
					editcaret(&Main);
					break;
				}

				if(c == 20){ // Ctrl+T
					DumpLines(&Main);
					Display(&Main);
					printf("Saved\n");
					editcaret(&Main);
					break;
				}
			break;

			case MODE_MOVE:
				c = upper(c);

				if(c == '\n'){
					break;
				}

				if(WASD(c) || (c == 'E') || (c == 'R') || (c == 'T') || (c == 'I') || (c == '$') || (c == '0')){
					if(c == 'W'){
						Main.linenum--;
						if(Main.linenum < 0){
							Main.linenum = 0;
						}
					}

					if(c == 'S'){
						Main.linenum++;
						if(Main.linenum >= Main.linebuf_size){
							Main.linenum = Main.linebuf_size - 1;
						}
					}

					if(c == 'A'){
						Main.position--;
						if(Main.position < 0){
							Main.position = 0;
						}
					}

					if(c == 'D'){
						Main.position++;
						if(Main.position >= Main.linebuf[Main.linenum].size){
							Main.position = Main.linebuf[Main.linenum].size - 1;
							if(Main.position < 0){
								Main.position = 0;
							}
						}
					}

					if(c == '$'){
						Main.position = Main.linebuf[Main.linenum].size - 1;;
					}

					if(c == '0'){
						Main.position = 0;
					}

					if(c == 'E'){
						mode = MODE_EDIT;
						editmode = 0;
						Display(&Main);
						editcaret(&Main);
						break;
					}

					if(c == 'I'){
						mode = MODE_EDIT;
						editmode = 1;
						Display(&Main);
						appendcaret(&Main);
						break;
					}

					if(c == 'R'){
						mode = MODE_FILE;
						filecaret(&Main);
						break;
					}

					if(c == 'T'){
						loop = 0;
						break;
					}
				}
				
				if(Display(&Main)){
					printf("Error: Unable to Display Context\n");
					return -1;
				}

				movecaret(&Main);
			break;
			
			default:
				mode = MODE_MOVE;
			break;
		}	
	}

	if(filestr){
		free(filestr);
	}
	
	// close
	if(CloseContext(&Main)){
		printf("Error: Unable to close Context\n");
		return -1;
	}
	return 0;
}