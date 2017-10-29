#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include <signal.h>

#ifdef _WIN32
#include <windows.h>
#define chdir _chdir

#else
#include <unistd.h>
#endif

#define MAX_LENGTH 1024
#define DELIMS " \t\r\n"
#define MAX_PROC 10

// struct de comandos
typedef struct cmdlines
{
	char *argv[MAX_LENGTH];										//nome do prog + parametros
	int bg;														//1 se tiver &, 0 cc
	char *arq_entr; 											//arq depois do <
	char *arq_saida; 										    //arq depois do >
	struct cmdline *proximo;									//(ptr para prox comando, depois do pipe (|)
}cmdline;

//struct de Processos (para o jobs)
typedef struct Processos
{
	char nome[255];
	pid_t pid;
	char current; //+, - ou vazio
	char status; //s for Stopped, r for Running, d for Done
} Processo;

// var globais
char line[MAX_LENGTH];   //linha de comando
int argc = 0;  
cmdline comando;
Processo *listaP[MAX_PROC]; //Array de processos para controle de jobs
int totalProc = 0; //Numero total de processos

// headers
void avalia(char *line);
void parsing(char *line);  
int cmd(char **argv);
void programa(cmdline comando);
void argvNULL();
void addProcLista(char *file, pid_t pidCHL);
void removeProcLista(pid_t pidCHL);
int searchPID(pid_t auxPID);
void jobs();
void listaPNULL();

int main(int argc, char *argv[]) {

	listaPNULL();

	while (1) {
	    printf("%s at %s \n", getenv("USER"),getcwd(NULL,1024));  //printa usuario e diretorio corrente
	    printf("$ ");       
	    if (!fgets(line, MAX_LENGTH, stdin)) break;               //le a linha
	    avalia(line);											  //verifica se eh comando ou programa a ser executado									    
	}
	return 0;
}

void addProcLista(char *file, pid_t pidCHL)
{
	Processo pFilho;	

	strcpy(pFilho.nome, file);
	pFilho.current = 'r';
	pFilho.pid = pidCHL;

	if(listaP[0] == NULL)	//Se a lista estiver vazia
	pFilho.status = '+';
		
	else if(listaP[1] == NULL) //Se a lista tiver apenas 1 elemento
	pFilho.status = '-';

	else
	pFilho.status = ' ';


	listaP[totalProc-1] = &pFilho;


	//printf("TOTAL PROC: %d", totalProc);

	printf("\n\nNome: %s", pFilho.nome);
	printf("\nPID: %d", (int)pFilho.pid);
	printf("\nStatus: %c", pFilho.status);
	printf("\nCurrent: %c", pFilho.current);
	printf("\n");
}

void removeProcLista(pid_t pidCHL)
{
	printf("\nENTROU NO REMOVE");

	int j;
	
	//printf("\nTOTAL PROC: %d", totalProc);	
	//printf("\nPID NA POSICAO: %d", searchPID(pidCHL));
	//printf("\nPID NA POSICAO: %d", searchPID(pidCHL));

	
	if(searchPID(pidCHL) >= 0)	
	{

		for(j=searchPID(pidCHL); j < totalProc; j++)
			listaP[j] = listaP[j+1];

		listaP[totalProc-1] = NULL;

		printf("\nREMOVIDO");
		
	}	

	else
	{
		printf("\nErro ao localizar PID do processo filho na lista de processos");
	}

	//totalProc--;
}

int searchPID(pid_t auxPID)
{
	int i=1;

	printf("ENTROU");
	//Processo *ptr;
	//ptr = listaP[totalProc-i];


	while((listaP[totalProc-i]->pid != auxPID) && (totalProc-i >= 0))
	{

		
		i++;
		//ptr = listaP[totalProc-i];
	}


	if(totalProc-i < 0)
		return -1; //PID não encontrado

	else
		return (totalProc-i);
}

void jobs()
{
	//printf("ENTROU NO JOBS COM TOTALPROC = %d", totalProc);
	int i=0;
	for(i=0;i<totalProc;i++)
	{
		printf("\nlistaP[%d]", i);
		printf("\nPID: %d", (int)listaP[i]->pid); 
	}
}

void listaPNULL()
{
	int i;
	for (i = 0; i < MAX_PROC; i++)
	{
		listaP[i] = NULL;
	}
}

void avalia(char *line){

	//char *argv[MAX_LENGTH]; 
	//char prog[20];
         						
	parsing(line);												//parsing: cria struct comando 

	if(!cmd(comando.argv)){									//trata comandos
		printf("Vai executar o programa %s \n", comando.argv[0]);
		totalProc++;
		programa(comando);
	}

	/*printf("arquivo de entrada: %s\n",comando.arq_entr);
	printf("arquivo de saida: %s\n",comando.arq_saida);
	printf("bg: %d\n",comando.bg);
	printf("proximo: %s\n",comando.proximo);*/
}
// preencher atributos do comando
void parsing(char *line){
	char *ptchar;
	char *args;
	char *linevar1;
	char *linevar2;
	argvNULL();  											//limpa argv e zera argc
	linevar1 = line;
	linevar2 = line;

	ptchar = strchr(linevar2, '<');							//procura < na linha
	if (ptchar!= NULL){   									//achou
		// preenche o argv
		args = strtok(linevar1, DELIMS);
		while(args < ptchar){
			comando.argv[argc] = args;
			args = strtok(NULL, DELIMS);
			printf("Argv[%d]: %s\n", argc, comando.argv[argc]);
			argc++;
		}
		//preenche demais atributos
		ptchar++;
		comando.arq_entr = ptchar;						//proximo elemento eh arquivo de entrada	
		comando.arq_saida = NULL;
		comando.bg = 0;
	}

	ptchar = strchr(linevar2, '>');						//procura > na linha
	if (ptchar!= NULL){   								//achou
		// preenche o argv
		args = strtok(linevar1, DELIMS);
		while(args < ptchar){
			comando.argv[argc] = args;
			args = strtok(NULL, DELIMS);
			printf("Argv[%d]: %s\n", argc, comando.argv[argc]);
			argc++;
		}
		//preenche demais atributos
		ptchar++;
		comando.arq_entr = NULL;						//proximo elemento eh arquivo de saida	
		comando.arq_saida = ptchar;
		comando.bg = 0;
	}

	//Caso 3: prog &
	ptchar = strchr(linevar2, '&');						//procura & na linha
	if (ptchar!= NULL){   								//achou
		// preenche o argv
		args = strtok(linevar1, DELIMS);
		while(args < ptchar){
			comando.argv[argc] = args;
			args = strtok(NULL, DELIMS);
			printf("Argv[%d]: %s\n", argc, comando.argv[argc]);
			argc++;
		}
		//preenche demais atributos
		comando.arq_entr = NULL;							
		comando.arq_saida = NULL;
		comando.bg = 1;
	}	

	/*//Caso 3: prog | prog
	ptchar = strchr(line, '|');
	if(ptchar!= NULL){   								//procura | na linha
		ptchar++;
		comando.proximo = ptchar;						//proximo elemento eh programa2 
	}*/		

	// Caso base: prog param1 param2 

	else{
		comando.arq_saida = NULL;
		comando.arq_entr = NULL;
		comando.bg = 0;

		args = strtok(linevar1, DELIMS);
		while(args != NULL){
			comando.argv[argc] = args;
			args = strtok(NULL, DELIMS);
			printf("Argv[%d]: %s\n", argc, comando.argv[argc]);
			argc++;
		}
	}	

}
//limpar argv
void argvNULL()
{
	int i;
	for (i = 0; i < argc; i++)
	{
		comando.argv[i] = NULL;
	}
	argc = 0; 
}

//tratar comandos cd, pwd, exit, jobs
int cmd(char **argv){
	// Caso 1: comando cd
	if (strcmp(argv[0], "cd") == 0){
        char *arg = argv[1];
       // printf("%s\n",arg);                                
        if (!arg) 
          fprintf(stderr, "cd missing argument.\n");
        else chdir(arg);  									  //syscall change directory                        
        return 1;

    }      
    // Caso 2: comando pwd
    else if (strcmp(argv[0],"pwd") == 0){
        char cwd[1024];                                       //buffer to current directory
        if (getcwd(cwd, sizeof(cwd)) != NULL)                 //syscall getcwd
          fprintf(stdout, "Current working dir: %s\n\n", cwd);  
        else perror("getcwd() error");
        return 1;
    }
    // Caso 3: comando exit
    else if (strcmp(argv[0], "exit") == 0){
        exit(0);
        return 1;
    } 

    return 0;
}

// tratar programas 
void programa(cmdline comando){
	pid_t pid;
	pid_t auxPID;
	int status;

	//processo em bg
	if (comando.bg){	

        if((pid=fork()) < 0){
           	printf("Erro na execucao do fork\n");
           	exit(0);	
        }

        if(pid==0){                                        //processo filho            
	        //printf("\n Filho: %d\n",(int)getpid());
		auxPID = getpid();	        

		addProcLista(comando.argv[0], auxPID);

		execvp(comando.argv[0], comando.argv);							//sobrepoe a imagem com programa a ser executado
		//totalProc--;
		removeProcLista(auxPID);
	        printf("Filho: erro na execucao do execvp\n");
    	}
        // processo pai
	    else{
		printf("\n Pai: %d\n",(int)getpid());
		//totalProc--;
		}   	
    }

    // processo em fg
    else{

		// criacao do processo filho
	    if((pid=fork()) < 0){
	        printf("Erro na execucao do fork\n");
	        exit(0);
	    }	

	  	if(pid==0){                                        //processo filho            
	        // printf("\n Filho: %d\n",(int)getpid());
	        execvp(comando.argv[0], comando.argv);							//sobrepoe a imagem com programa a ser executado
	        printf("Filho: erro na execucao do execvp\n");
	    }
	          
	    else{                                              //processo pai
	        //printf("\n Pai: %d\n",(int)getpid());
	        wait(&status);									//espera filho terminar
	    }   	
	}

	if (errno) perror("Erro:");		
}
