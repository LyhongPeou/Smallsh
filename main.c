#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>


#define MAXLENGHT 2048


pid_t processList[500]; //Array to hold the process
int processNum;     // The process Number 

int process;
int allowBG = 1;




/*********************************************************************
** Function: getInput
** Description: The function will ask for user input
**              
**               
** Parameters:  argList - the user input in 
**              cmd  - string that input
** Pre-Conditions: None
** Post-Conditions:  None
*********************************************************************/

int getInput(char* cmd, char** argList)
{
    int j, numArgs = 0;

    char* token;

    printf(": ");
	fflush(stdout);
	fgets(cmd, MAXLENGHT, stdin);
    strtok(cmd, "\n");

    for(j = 0; j < strlen(cmd) - 1; j++)
    {
        if(cmd[j] == '$' && cmd[j + 1] == '$')
        {
            cmd[j] = '\0';
            
            sprintf(cmd, "%s%d",cmd, getpid()); //change 
        }
        
    };

    token = strtok(cmd, " ");

    while(token != NULL)
    {
        argList[numArgs] = token;
        
        token = strtok(NULL, " ");	// Get next token
        numArgs++;
    }

    return numArgs;

}

/*********************************************************************
** Function: status_cmd
** Description: The function will check the exit status and signal and print it outt
**              
**               
** Parameters:  childMethodExit - the user input in 
**            
** Pre-Conditions: None
** Post-Conditions:  None
*********************************************************************/

void status_cmd(int* childMethodExit)
{

    waitpid(getpid(), &process, 0);

    if(WIFEXITED(process) !=0)
    {
       printf("The exit status:  %d \n", WEXITSTATUS(process));
    }
    else
    {
        *childMethodExit = 1;
       printf("The signal value: %d \n", WTERMSIG(process));
    }


    fflush(stdout);
}


/*********************************************************************
** Function: cd_cmd
** Description: The function will change directory
**              If no specific input - change directroy home
**                  else change directory to user specific
**               
** Parameters:  argList - the user input in 
**              numArgs  - number of argument
**             
** Pre-Conditions: None
** Post-Conditions:  None
*********************************************************************/

void cd_cmd(char** argList, int numArgs )
{
    char cwd[256];
    int error = 0;
    if (numArgs == 1)
    {
        error = chdir(getenv("HOME"));
    }
    else
    {
        error = chdir(argList[1]);
    }

    if (error  == 0)
    {
       getcwd(cwd, sizeof(cwd)); //get the current directory and print it out
       printf("Current directory is: %s\n", cwd);
      
    }
    else
    {
        printf("No Directroy Found");
    }

     fflush(stdout);

}

/*********************************************************************
** Function: checkBG
** Description: The function will check if user want the command to run in the background.
**              
**               
** Parameters:  argList - the user input in 
**              numArgs  - number of argument
**              isBG - the value to set if the background 
** Pre-Conditions: None
** Post-Conditions:  None
*********************************************************************/

void checkBG(char** argList, int numArgs, int* isBG)
{
    if(strcmp(argList[numArgs -1], "&") == 0)
    {
        if(allowBG == 1)
        {
            *isBG = 1; //set the BG if background is allow
        }

        argList[numArgs -1] = NULL; //ignore the &
    }
}

/*********************************************************************
** Function: checkFile
** Description: The function will check if user has input or output file and copy the output/input from user input
**              
**               
** Parameters:  inputFile - the variable to input file
**              outputFile  - the variable to output file
**              argList - the variable to hold input 
** Pre-Conditions: None
** Post-Conditions:  None
*********************************************************************/

void checkFile(char* inputFile, char* outputFile, char** argList)
{
    int i;

    for(i = 0; argList[i] != NULL; i++)
    {
        if(strcmp(argList[i], "<") == 0)
        {
            argList[i] = NULL;
            strcpy(inputFile, argList[i+1]);
            i++;
        }
        else if (strcmp(argList[i], ">") == 0)
        {
            argList[i] = NULL;
            strcpy(outputFile, argList[i+1]);
            i++;
            
        }
    }
}



/*********************************************************************
** Function: runChild
** Description: The child will check for file input/output and redirect the stdout/stdin to file and execute the commmand
**              
**               
** Parameters:  argList - the user input in list of command
**              isBG  - check for background
**              SIGINT - the signal  
** Pre-Conditions: None
** Post-Conditions:  None
*********************************************************************/

void runChild(char** argList, struct sigaction SIGINT_action, int isBG)
{
    char inputFile[255], outputFile[255];   //to hold the directory 
    inputFile[0] = '\0';
    outputFile[0] = '\0';
    checkFile(inputFile, outputFile, argList);
    
    int input, output, stat;

        if (strcmp(inputFile, "") != 0) 
            {
                input = open(inputFile, O_RDONLY );

                if(input == -1 )
                {
                    printf("Failed to open the input file \n");
                    fflush(stdout);
                    exit(1);
                }

                stat = dup2(input, 0);  //change the stdin to file

                if(stat == -1)
                {
                    printf("source dup2 \n");
                    fflush(stdout);
                    exit(1);
                }
            

                fcntl(input, F_SETFD, FD_CLOEXEC);  
            }

        if(strcmp(outputFile, "") != 0)
        {
            output = open(outputFile, O_WRONLY|O_CREAT | O_TRUNC , 0640 );

            if(output == -1 )
            {
                printf("Failed to open the output file \n");
                fflush(stdout);
                exit(1);
            }

    
            stat = dup2(output, 1);

            if(stat == -1)
            {
                printf("source dup2 \n");
                fflush(stdout);
                exit(1);
            }

            fcntl(output, F_SETFD, FD_CLOEXEC);
        }
        
        if(!isBG) 
		    {
                SIGINT_action.sa_handler=SIG_DFL;
	            sigaction(SIGINT, &SIGINT_action, NULL);
            }
    
        if(execvp(argList[0], argList) == -1)   //excute the cmd
        {
            perror(argList[0]);
            exit(1);
        }
        

} 

/*********************************************************************
** Function: runParent
** Description: The parent will check if it's background or not.
**              if BG = 1 -> keep the specifc child in background
**                 else block the parent and let the specifc child process run
** Parameters:  isBG - the background value
**              spawnPid - the process id
** Pre-Conditions: None
** Post-Conditions:  None
*********************************************************************/

void runParent(pid_t spawnPid, int isBG )
{
    if(isBG == 1)
    {
        waitpid(spawnPid, &process, WNOHANG);
        printf("background pid: %d \n", spawnPid);
        fflush(stdout);
    }
    else
    {
        waitpid(spawnPid, &process, 0);
    }
}

/*********************************************************************
** Function: all_cmds
** Description: The function will fork the process into child and parent. The child will perfrom the command and redicrection.
**              
** Parameters:  argList - the user input
**              cont - the number for the while loop 
**              numArgs - The number of args from the user input
**              SIGINT - the signal 
** Pre-Conditions: None
** Post-Conditions:    None
*********************************************************************/

void all_cmd(int* errorState, char** argList, int numArgs, struct sigaction SIGINT_action) 
{   
    int isBG = 0;

    pid_t spawnPid = -5;

    checkBG(argList, numArgs, &isBG);


    spawnPid = fork();



    switch(spawnPid)
    {
        case -1:

            perror("Hull Brench \n");
            exit(1);
            break;

        case 0: 
            processList[processNum] = spawnPid;
            processNum++;
            runChild(argList, SIGINT_action, isBG);
            break;

        default:
            runParent(spawnPid, isBG);
            
    }

     while((spawnPid = waitpid(-1, &process, WNOHANG)) > 0)
        {
            printf("background child %d terminated \n", spawnPid);
            fflush(stdout);
            status_cmd(errorState);   
        }


}

/*********************************************************************
** Function: checkCmds
** Description: The function check the user input to run build-in commmand and other command
** Parameters:  argList - the user input
**              cont - the number for the while loop 
**              numArgs - The number of args from the user input
**              SIGINT - the signal 
** Pre-Conditions: None
** Post-Conditions:     None
*********************************************************************/

void checkCmds(char** argList , int* cont, int numArgs, struct sigaction SIGINT_action)  
{
    int i;
    int errorState = 0;

    if(argList[0][0] == '#' || argList[0][0] =='\n')
    {
        return;
    }
    else if(strcmp(argList[0], "cd") == 0) {
		cd_cmd(argList, numArgs);
	}
    else if(strcmp(argList[0], "exit") == 0) {
        
        for(i = 0; i < processNum; i++)
        {
            kill(processList[i], SIGTERM);
        }


		*cont = 0;
        return;
	}
    else if(strcmp(argList[0], "status") == 0) {
		status_cmd(&errorState);
	}
    else
    {
        all_cmd(&errorState, argList, numArgs, SIGINT_action);

        
    }


}

/*********************************************************************
** Function: catchSTP
** Description: The function is check for responds as responding when signal ^Z
** Parameters:  signo 
** Pre-Conditions: allowBG - allow background is allow.
** Post-Conditions:     The value changed.
*********************************************************************/


void catchSTP(int signo)
{
    if( allowBG == 1)
    {
        char * output = "Entering foreground-only mode (& ignored) \n";
        write(1, output, 45 );
        fflush(stdout);
        allowBG = 0;
    }
    else
    {
        char * output = "Exiting foreground-only mode \n";
        write(1, output, 44 );
        fflush(stdout);
        allowBG = 1;
    }
}



/*********************************************************************
** Program Filename: main.c
** Author: Lyhong Peou 
** Date:    11/9/2922
** Description: 
**           The program will allow user to run different command and accept input/output to file
**           The program will tested using the script 
**           
** Input:   None
** Output:  None
*********************************************************************/


int main(int argc, char* argv[]){


    struct sigaction SIGINT_action = {0};
    struct sigaction SIGTSTP_action = {0};

    //ignore ^C 
    SIGINT_action.sa_handler = SIG_IGN;
    sigfillset(&SIGINT_action.sa_mask);
    SIGINT_action.sa_flags =  0;
    sigaction(SIGINT, &SIGINT_action, NULL);

    //catch ^Z
   	SIGTSTP_action.sa_handler = catchSTP; 	
    sigfillset(&SIGTSTP_action.sa_mask);
    SIGTSTP_action.sa_flags =  0;
    sigaction(SIGTSTP, &SIGTSTP_action, NULL);

    char cmd[2048];
    char* argList[512];
    int numArgs;
    pid_t processList[500];
    int processNum = 0;

    int cont = 1;

    while(cont)
    {
        numArgs = getInput(cmd, argList);
        argList[numArgs] = NULL;
        checkCmds(argList, &cont, numArgs, SIGINT_action);    
    }

    return 0;
}