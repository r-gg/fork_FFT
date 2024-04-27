/**
 * @author Rastko Gajanin, 11930500
 * @brief c File for the Assignment 2 forkFFT
 * @date 19.11.2020
 */

#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <regex.h>
#include "forkFFT.h"
#include <math.h>

#define READ 0
#define WRITE 1
#define PI 3.141592654

#define COMPLEX_NUMBER_REG_EXPRESSION "\\d*(.\\d*)?((\\ )*\\d*(.\\d+)?\\*i)?(\\n)?"
#define MAXLENGTH 100
/**
 * Indices in the Pipes array
 */
#define WRITE_LEFT 0
#define WRITE_RIGHT 1
#define READ_LEFT 2
#define READ_RIGHT 3


/**
 * @brief function for printing the usage message, upon incorrect use of the program
 * @param string with the name of the file
 * @return prints the usage message and exits the program with EXIT_FAILURE
*/
static void myUsage(){ 
	fprintf(stderr, "%s\t[USAGE]\t %s < file.txt\nfile.txt must not be empty and the number of lines must be a power of two.\n", fileName,fileName); 
	exit(EXIT_FAILURE); 
} 

static complexNumber multiply(complexNumber z1, complexNumber z2){
    complexNumber res = {0, 0};
    res.a = (z1.a * z2.a) - (z1.b * z2.b);
    res.b = (z1.a * z2.b) + (z1.b * z2.a);
    return res;
}
static complexNumber add(complexNumber z1, complexNumber z2){
    complexNumber res = {0, 0};
    res.a = (z1.a + z2.a);
    res.b = (z1.b + z2.b);
    return res;
}
static complexNumber sub(complexNumber z1, complexNumber z2){
    complexNumber res = {0, 0};
    res.a = (z1.a - z2.a);
    res.b = (z1.b - z2.b);
    return res;
}


static int calculateN(FILE *in){
    int count = 0;
    char currChar = 0;
    char prevChar = 0;
    while((currChar = getc(in)) != EOF){
        if(currChar == '\n'){
            count++;
            if(prevChar == '\n' || prevChar == 0){
                count--;
            }
        }
        
        prevChar = currChar;
    }
    return count;
}

regex_t preg; 

static void regexInit(){
    regcomp(&preg, COMPLEX_NUMBER_REG_EXPRESSION, REG_EXTENDED);
}
static void freeRegex(){
    regfree(&preg);
}

// -1 if there was a mismatch, 0 if it is a match
static int checkLine(char* line){
    if(regexec(&preg, line, 0, NULL, 0) == REG_NOMATCH){
        fprintf(stderr, "Error in pattern Matching.");
        return -1;
    }
    else
    {
        return 0;
    }
    
}

static void fprintFile(FILE* out, FILE *in){
    char curr = 0;
    while((curr = getc(in)) != EOF){
        fprintf(out, "%c",curr);
    }
    
}

static void fprintArray(FILE* out, complexNumber *arr, int numOfElements){
    for(int i = 0;i < numOfElements;i++){
        fprintf(out, "%f %f*i\n", arr[i].a, arr[i].b);
    }
}

//number of lines in the file
// -1 if error
static int readInput(FILE *in, complexNumber *values){
    //char currentLine[MAX_CHARS_PER_FLOAT_NUMBER];
    /*
    while (fgets(currentLine,MAX_CHARS_PER_FLOAT_NUMBER,in) != NULL)
    { 
    }*/
    //fprintf(stdout, "current Line will be printed %d times\n", sizeof(values)/sizeof(values[0]));
    char *endpointer;
    int counter = 0;
    char* currentLine = NULL;	//read string from stdin
    ssize_t errGetLine;	//errorvalue for getline
    size_t len = 0;	//length is irrelivant because line=null
    //regexInit();
    while((errGetLine = getline(&currentLine,&len, in)) != -1){
        
        if(counter >= MAXLENGTH  - 1){
            fprintf(stderr, "------------Number of Lines to big (over %d)-------------\n" , MAXLENGTH);
            myUsage();
            return -1;
        }
        //fprintf(stderr, "Current Line :: %s", currentLine);
        /*
        if(fgets(currentLine,MAX_CHARS_PER_FLOAT_NUMBER,in) == NULL){
            fprintf(stderr, "Error in reading file");
            return -1;
        }
        if(checkLine(currentLine) == -1){
            fprintf(stderr,"Line Mismatch\n");
            return -1;
        }*/
        if(currentLine[0] == '\n' || strcmp(currentLine,"\n") == 0){ // if there was an empty row
            
            continue;
        }
        //printf("current Line: %s",currentLine);
        
        errno = 0;
        values[counter].a = strtof(currentLine,&endpointer);
        if(errno != 0 || (*endpointer != '\0' && *endpointer != 10 && *endpointer != ' ')){
            //fprintf(stderr, "*endpointer = %d\n endpointer = %s\nerrno = %d\n", *endpointer,endpointer, errno);
            fprintf(stderr, " --- Non numerical values in file ---\n");
            // close IN ????????????????????????????????????????????????????????????????????????????????????????            
            myUsage();
            return -1;
        }
        values[counter].b = strtof(endpointer,&endpointer);
        if(errno != 0 || (*endpointer != '\0' && *endpointer != 10 && *endpointer != '*' && *endpointer != ' ')){
            //fprintf(stderr, "*endpointer = %d\n endpointer = %s\nerrno = %d\n", *endpointer,endpointer, errno);
            fprintf(stderr, " --- Non numerical values in file ---\n");
            // close IN ????????????????????????????????????????????????????????????????????????????????????????            
            myUsage();
            return -1;
        }
        //fprintf(stdout, "%f %f*i\n", values[counter].a, values[counter].b);
        counter++;
    }
    //freeRegex();
    return counter;
}


/**
 * TG
 * @brief writes every even element of the values[] to the writePipeFD1 and every odd element in the writePipeFD2
 *          - closes the given FDs at the end
 * @param array1 and array2 MUST BE EXACTLY half the size of original array = n/2
 * @param n is the number of elements in original array
 * @param all arguments are NOT NULL
 */ 
static void splitIntoPipes(complexNumber values[], int writePipeFD1, int writePipeFD2, int n){
    char stringOfComplexNr[MAXLENGTH];
    for (int i = 0; i < n; i++)
    {   
        sprintf(stringOfComplexNr, "%f %f*i\n", values[i].a, values[i].b); //string formated like a complex number
        //fprintf(stdout, "index : %d, writing in one of the pipes (the string - %s", i , stringOfComplexNr);
        errno = 0;
        if(i % 2 == 0){
            write(writePipeFD1, stringOfComplexNr, (strlen(stringOfComplexNr)));
        }
        else
        {
            write(writePipeFD2, stringOfComplexNr, (strlen(stringOfComplexNr)));
        }
        
    }
    close(writePipeFD1);
    close(writePipeFD2);
}

/**
 * @brief redirects the stdin and stdout to pipes for each child and !!!! closes all other pipes and pipe ends (For One Child). !!!
 * @details The read end of the needed write pipe is redirected to stdin and 
 *              \ the write end of the needed read pipe is redirected to stdout 
 *             \ The PARENT reads the child through the neededReadPipe -> the child writes on the neededReadPipe(through stdout)
 *             \ The PARENT writes to the child through the neededWritePipe -> the child reads the neededWritePipe(as stdin)
 */
static void redirectNeededPipes(int numOfPipes, int pipes[numOfPipes][2], int neededReadPipe, int neededWritePipe){
    for (int i = 0; i < numOfPipes; i++){
        if(i == neededWritePipe){
            if(dup2(pipes[neededWritePipe][READ],STDIN_FILENO) == -1){
                fprintf(stderr,"Error in first dup");
                exit(EXIT_FAILURE);
            }
        }
        if(i == neededReadPipe){
            if(dup2(pipes[neededReadPipe][WRITE],STDOUT_FILENO) == -1){
                fprintf(stderr,"Error in second dup");
                exit(EXIT_FAILURE);
            }
        }
        // closing not needed pipes for one child
        close(pipes[i][READ]); 
        close(pipes[i][WRITE]);
    }
    
}

// TODO: Closing the stdin and pipes before every exit(EXIT_FAILURE);??????????????????????????
// https://www.geeksforgeeks.org/strtof-function-c/

/**
 * 
 * 
 */
int main (int argc, char *argv[]){
    //regexInit();
    fileName = argv[0];
    if(argc != 1){
        myUsage();
    }
    FILE *in = stdin;
    if(isatty(STDIN_FILENO)){ // the argument of isatty must be fd which is a number not a FILE object
        fprintf(stderr, "No inputs from terminal are allowed. Use a file instead, please.\n");
        myUsage();
    }
    
    complexNumber values[MAXLENGTH];

    
    int n = readInput(in,values);
    
    if(n == -1){
        fprintf(stderr, "Error in read");
        myUsage();
        exit(EXIT_FAILURE);
    }
    if(n == 0){
        fprintf(stderr, "The input file seems to be empty. Rerun the program with a non-empty file please.\n");
        myUsage();
    }

    if( (n != 1) && (n % 2 != 0) ){ // n may be 1 and MUST be a power of 2
        myUsage();
    }

    
    
    rewind(in); // must be rewinded after counting because we already went through the whole file
    //printFile(in);
    //fflush(stdout);
    //fprintf(stdout, "n = %d\n", n);
    

    //fprintf(stdout, "numOfElemsInValues after read = %d\n", sizeof(values)/sizeof(values[0]));
    //fprintArray(stdout,values,n);

    if(n == 1){
        fprintf(stdout,"%f %f*i\n",values[0].a,values[0].b);
        exit(EXIT_SUCCESS);
    }
    //fprintf(stdout, "--------------------\n");

    

    fflush(stdin);
    int pipes[4][2]; // Two pipes per child. One for writing to the stdin of the child and one for reading the stdout of the child.
    
    if(pipe(pipes[WRITE_LEFT]) == -1 || pipe(pipes[WRITE_RIGHT]) == -1 || pipe(pipes[READ_LEFT]) == -1 || pipe(pipes[READ_RIGHT]) == -1 ){
        fprintf(stderr,"Error in opening pipes.");
        exit(EXIT_FAILURE);
    }

    splitIntoPipes(values , pipes[WRITE_LEFT][WRITE] , pipes[WRITE_RIGHT][WRITE], n);

    pid_t pid1;
    if((pid1 = fork()) == -1){fprintf(stderr, "Error in Fork"); exit(EXIT_FAILURE);}
    if(pid1 == 0){ // child1
        
        redirectNeededPipes(4, pipes, READ_LEFT, WRITE_LEFT);
        //fprintf(stdout, "READING FILE IN CHILD 1\n");
        //printFile(stdin);
        //complexNumber vals[MAXLENGTH];
        //int n1 = readInput(stdin,vals);
        //fprintf(stderr,"n1 = %d",n1);
        
        //fprintf(stdout,"END OF READING FILE IN CHILD 1\n");
        //fprintf(stdout, "4.44 1.11*i\n");
        //fflush(stdout);
        if(execlp("./forkFFT", argv[0], NULL)==-1){
            fprintf(stderr, "Error in EXEC");
            exit(EXIT_FAILURE);
        }
        fprintf(stderr, "SHOULD NOT BE REACHED");
        exit(EXIT_SUCCESS);
        exit(EXIT_SUCCESS);
    }
    pid_t pid2;
    if((pid2 = fork()) == -1){fprintf(stderr, "Error in Fork"); exit(EXIT_FAILURE);}
    if(pid2 == 0){ // child2
        
        redirectNeededPipes(4, pipes, READ_RIGHT, WRITE_RIGHT);
        //fprintf(stdout, "READING FILE IN CHILD 2\n");
        //fprintFile(stderr,stdin);
        //fprintf(stdout,"END OF READING FILE IN CHILD 2\n");
        //fprintf(stdout, "1.44 12.23*i\n");
        //fflush(stdout);
        if(execlp("./forkFFT", argv[0], NULL)==-1){
            fprintf(stderr, "Error in EXEC");
            exit(EXIT_FAILURE);
        }
        fprintf(stderr, "SHOULD NOT BE REACHED");
        exit(EXIT_SUCCESS);
    }
    
    

    errno = 0;
    int state;					//return state of the child process (never checked)
    waitpid(pid1,&state,0); // Sollte uebreprueft werden ??????????????????????????????????????????????????????????
	if(WEXITSTATUS(state) != 0){ // if not 0, the child exited with exit failure
		exit(EXIT_FAILURE);
	}
    //fprintf(stdout,"exitstatus1 : %d \n", WEXITSTATUS(state));
    waitpid(pid2, &state, 0); // Sollte uebreprueft werden ??????????????????????????????????????????????????????????
	if(WEXITSTATUS(state) != 0){
		exit(EXIT_FAILURE);
	}
    
    FILE * fileE = fdopen(pipes[READ_LEFT][READ], "r");
    FILE * fileO = fdopen(pipes[READ_RIGHT][READ], "r");

    close(pipes[WRITE_LEFT][READ]);
    close(pipes[WRITE_RIGHT][READ]);
    
    close(pipes[READ_LEFT][WRITE]);
    close(pipes[READ_RIGHT][WRITE]);

    complexNumber r_E[n/2];
    complexNumber r_O[n/2];

    int n_E = readInput(fileE,r_E);
    int n_O = readInput(fileO,r_O);
    if(n_E != n_O || n_E != n/2){
        fprintf(stderr, "SubArrays of children are either unequal or not n/2\n");
        myUsage();
        exit(EXIT_FAILURE);
    }
    complexNumber resultArray[n];
    for(int k = 0; k < n/2; k++){
        complexNumber multBy;
        double val = (-2)*PI*k/n;
        multBy.a = cos(val);
        multBy.b = sin(val);
        resultArray[k] = add(r_E[k],multiply(multBy,r_O[k]));
        resultArray[k + (n/2)] = sub(r_E[k], multiply(multBy,r_O[k]));
    }
    fprintArray(stdout,resultArray,n);


    
    /*
    char  c1[700];
    if(read(pipes[READ_LEFT][READ],c1,700) == -1){
        fprintf(stderr,"Errod in read");
    }
    fprintf(stdout,"Result of read was : %s\n",c1);
    char  c2[700];
    if(read(pipes[READ_RIGHT][READ],c2,700) == -1){
        fprintf(stderr,"Errod in read");
    }
    fprintf(stdout,"Result of read was : %s\n",c2);

    close(pipes[READ_LEFT][READ]);
    close(pipes[READ_RIGHT][READ]);*/
    // TODO: Calculate and write to stdout.

    // close IN ????????????????????????????????????????????????????????????????????????????????????????
    
    //fprintf(stdout,"We made it!");
    exit(EXIT_SUCCESS);
}