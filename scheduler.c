#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define TIME_QUANTUM 5 //RR에 대한 time quantum
#define ALGORITHM_NUM 8 //알고리즘 개수
#define ALGORITHM_NAME_LEN 100
int processNum = 0; //process 개수

typedef struct {
    int pid; //1~N 랜덤, unique
    int arrivalTime; //0~10 랜덤
    int CPUburst; //10~30 랜덤
    int IOburst; //0~5 랜덤, 0이면 I/O 작업 없음
    int priority; //1~N 랜덤, 값이 낮을수록 높은 우선순위
    int IOstart; //1~(CPUburst-1) 랜덤, 이만큼 CPU burst 후 I/O burst 시작
    int CPUremaining;
    int IOremaining;
    int waitingTime;
    int turnaroundTime;

} process;
process** processes = NULL; 

typedef struct {
	double avgWaiting;
	double avgTurnaround;
    char algorithm[ALGORITHM_NAME_LEN];
} evaluation;
evaluation* evals[ALGORITHM_NUM] = {NULL};
int algCount = 0; //몇번째 알고리즘인지 카운트

void sort_arrival(process** processes, int num){

    //sort processes (arrival time 작은 순서로, arrival time 같을 시 pid 작은 순서로)
    for (int i = 0; i < num - 1; i++) {
        for (int j = 0; j < num - i - 1; j++) {
            if (processes[j]->arrivalTime > processes[j + 1]->arrivalTime ||
                (processes[j]->arrivalTime == processes[j + 1]->arrivalTime && 
                processes[j]->pid > processes[j + 1]->pid)) {
                
                process* temp = processes[j];
                processes[j] = processes[j + 1];
                processes[j + 1] = temp;
            }
        }
    }
}

void sort_burst(process** processes, int num){

    //sort processes (CPU remaining burst time 작은 순서로, CPU remaining burst time 같을 시 arrival time이 작은 순서로, arrival time도 같으면 pid가 작은 순서로)
    for (int i = 0; i < num - 1; i++) {
        for (int j = 0; j < num - i - 1; j++) {
            if (processes[j]->CPUremaining > processes[j + 1]->CPUremaining ||
                (processes[j]->CPUremaining == processes[j + 1]->CPUremaining &&
                 (processes[j]->arrivalTime > processes[j + 1]->arrivalTime ||
                  (processes[j]->arrivalTime == processes[j + 1]->arrivalTime &&
                   processes[j]->pid > processes[j + 1]->pid)))) {
                
                process* temp = processes[j];
                processes[j] = processes[j + 1];
                processes[j + 1] = temp;
            }
        }
    }
}

void sort_priority(process** processes, int num){

    //sort processes (priority 작은 순서로, priority 같을 시 arrival time이 작은 순서로, arrival time도 같으면 pid가 작은 순서로)
    for (int i = 0; i < num - 1; i++) {
        for (int j = 0; j < num - i - 1; j++) {
            if (processes[j]->priority > processes[j + 1]->priority || 
                (processes[j]->priority == processes[j + 1]->priority && 
                processes[j]->arrivalTime > processes[j + 1]->arrivalTime) || 
                (processes[j]->priority == processes[j + 1]->priority && 
                processes[j]->arrivalTime == processes[j + 1]->arrivalTime && 
                processes[j]->pid > processes[j + 1]->pid)) {
                
                process* temp = processes[j];
                processes[j] = processes[j + 1];
                processes[j + 1] = temp;
            }
        }
    }
}

void sort_IO(process** processes, int num){

    //sort processes (I/O remaining burst time 큰 순서로, I/O remaining burst time 같을 시 CPU remaining burst time 작은 순서로, CPU remaining burst time 같을 시 arrival time이 작은 순서로, arrival time도 같으면 pid가 작은 순서로)
    for (int i = 0; i < num - 1; i++) {
        for (int j = 0; j < num - i - 1; j++) {
            if (processes[j]->IOremaining < processes[j + 1]->IOremaining || 
                (processes[j]->IOremaining == processes[j + 1]->IOremaining &&
                 (processes[j]->CPUremaining > processes[j + 1]->CPUremaining ||
                  (processes[j]->CPUremaining == processes[j + 1]->CPUremaining &&
                   (processes[j]->arrivalTime > processes[j + 1]->arrivalTime ||
                    (processes[j]->arrivalTime == processes[j + 1]->arrivalTime &&
                     processes[j]->pid > processes[j + 1]->pid)))))) {

                process* temp = processes[j];
                processes[j] = processes[j + 1];
                processes[j + 1] = temp;
            }
        }
    }
}

void CreateProcess(){
    processes = malloc(processNum * sizeof(process*));
    if (processes == NULL) {
        printf("malloc error");
        return;}

    //process 생성
    srand(time(NULL));
    for (int i = 0; i < processNum; i++) {
        processes[i] = malloc(sizeof(process));
        if (processes[i] == NULL) {
        printf("malloc error");
        return;}

        processes[i]->pid = i+1; //1~N 랜덤, unique
        processes[i]->arrivalTime = rand() % 11; //0~10 랜덤
        processes[i]->CPUburst = (rand() % 21) + 10; //10~30 랜덤
        processes[i]->IOburst = rand() % 6; //0~5 랜덤
        if(processes[i]->IOburst==0){
            processes[i]->IOstart = -1; //I/O 작업 안 함
        }else{
            processes[i]->IOstart = (rand() % (processes[i]->CPUburst - 1)) + 1; //1~(CPUburst-1) 랜덤, 이만큼 CPU burst 후 I/O burst 시작
        }
        processes[i]->priority = (rand() % processNum) + 1; //1~N 랜덤, 값이 낮을수록 높은 우선순위
        processes[i]->CPUremaining = processes[i]->CPUburst;
        processes[i]->IOremaining = processes[i]->IOburst;
        processes[i]->waitingTime = 0;
        processes[i]->turnaroundTime = 0;
    }
    sort_arrival(processes, processNum); //arrival time으로 정렬

    //각 프로세스 정보 출력
    printf("Created processes information:\n");
    for (int i = 0; i < processNum; i++) {
        printf("[Pid: %d, Arrival Time: %d, CPU Burst: %d, I/O Burst: %d, Priority: %d, I/O start: %d]\n",
               processes[i]->pid, processes[i]->arrivalTime, processes[i]->CPUburst, processes[i]->IOburst, processes[i]->priority, processes[i]->IOstart);
    }
    printf("\n");
    
    for (int i = 0; i < ALGORITHM_NUM; i++) {//evals 배열 초기화
        evals[i] = malloc(sizeof(evaluation));
        if (evals[i] == NULL) {
        printf("malloc error");
        return;}
        
    }
}

//한 알고리즘에 대한 각 프로세스 정보 출력 & evals 배열 update
void EvaluateOne(process** p, char* algorithm_name){
    printf("\n");

    //각 프로세스 정보 출력
    for (int i = 0; i < processNum; i++) {
        printf("[Pid: %d, Waiting Time: %d, Turnaround Time: %d]\n",
               p[i]->pid, p[i]->waitingTime, p[i]->turnaroundTime);
    }
    printf("\n");

    //evals update
    double avgWaiting = 0;
	double avgTurnaround = 0;

    for(int i=0; i<processNum; i++){
        avgWaiting += (double)p[i]->waitingTime;
        avgTurnaround += (double)p[i]->turnaroundTime;
    }

    avgWaiting /= (double)processNum;
    avgTurnaround /= (double)processNum;

    evals[algCount]->avgWaiting = avgWaiting; //이후 EvaluateAll을 위해 저장해둠
    evals[algCount]->avgTurnaround = avgTurnaround;
    strncpy(evals[algCount]->algorithm, algorithm_name, ALGORITHM_NAME_LEN-1);
    evals[algCount]->algorithm[ALGORITHM_NAME_LEN - 1] = '\0'; 

    algCount += 1;

    printf("Average waiting time: %.2f\n", avgWaiting);
	printf("Average turnaround time: %.2f\n", avgTurnaround);
    printf("===============================================================\n");
}

//마지막으로 evals 배열의 각각 algorithm 별 결과 비교
void EvaluateAll(){ 

    //Average waiting time
    printf("<Average waiting time min to max>\n");

    for (int i = 0; i < ALGORITHM_NUM - 1; i++) {
        for (int j = 0; j < ALGORITHM_NUM - i - 1; j++) {
            if (evals[j]->avgWaiting > evals[j + 1]->avgWaiting) {
                
                evaluation* temp = evals[j];
                evals[j] = evals[j + 1];
                evals[j + 1] = temp;
            }
        }
    }
    for(int i=0; i<ALGORITHM_NUM; i++){
        printf("%d. %s: %.2f\n", i+1, evals[i]->algorithm, evals[i]->avgWaiting);
    }

    //Average turnaround time
    printf("\nAverage turnaround time min to max:\n");
    for (int i = 0; i < ALGORITHM_NUM - 1; i++) {
        for (int j = 0; j < ALGORITHM_NUM - i - 1; j++) {
            if (evals[j]->avgTurnaround > evals[j + 1]->avgTurnaround) {
                
                evaluation* temp = evals[j];
                evals[j] = evals[j + 1];
                evals[j + 1] = temp;
            }
        }
    }
    for(int i=0; i<ALGORITHM_NUM; i++){
        printf("%d. %s: %.2f\n", i+1, evals[i]->algorithm, evals[i]->avgTurnaround);
    }
}

process** copy_processes(){ //deep copy
    process** p = malloc(sizeof(process*) * processNum);
    if (p == NULL) {
        printf("malloc error");
        return NULL;}
    
    for (int i = 0; i < processNum; i++) {
        p[i] = malloc(sizeof(process));
        if (p[i] == NULL) {
        printf("malloc error");
        return NULL;}

        p[i]->pid = processes[i]->pid;
        p[i]->arrivalTime = processes[i]->arrivalTime;
        p[i]->CPUburst = processes[i]->CPUburst;
        p[i]->IOburst = processes[i]->IOburst;
        p[i]->priority = processes[i]->priority;
        p[i]->CPUremaining = processes[i]->CPUremaining;
        p[i]->IOremaining = processes[i]->IOremaining;
        p[i]->waitingTime = processes[i]->waitingTime;
        p[i]->turnaroundTime = processes[i]->turnaroundTime;
        p[i]->IOstart = processes[i]->IOstart;
    }

    return p;
}

void FCFS(){
    char algorithm_name[ALGORITHM_NAME_LEN] = "First Come First Served Algorithm";
    printf("<%s>\n", algorithm_name);
    //Config: readyQueue, waitingQueue, p 설정
    process** readyQueue = malloc(sizeof(process*) * processNum);
    if (readyQueue == NULL) {
        printf("malloc error");
        return;}
    process** waitingQueue = malloc(sizeof(process*) * processNum);
    if (waitingQueue == NULL) {
        printf("malloc error");
        return;}
    process** p = NULL; 
    p = copy_processes(); //p에 processes를 복사, arrival time으로 정렬되어 있음

    //FCFS 알고리즘: arrival time 같을 시 pid 작은 순서로
    int readyCount = 0;
    int waitingCount = 0;
    int currentTime = 0;
    int finished = 0;
    int checked = 0;
    process* runningProcess = NULL;

    while (finished < processNum) {

        printf("Time %d: ", currentTime);
        
        while (checked < processNum && p[checked]->arrivalTime <= currentTime) { //도착한 프로세스들을 레디큐에 업데이트
            readyQueue[readyCount++] = p[checked++];
        }
        sort_arrival(readyQueue, readyCount); //arrival time으로 정렬
        
        if (runningProcess != NULL) { //작업 중인 프로세스 존재함

            runningProcess->CPUremaining--;
            runningProcess->turnaroundTime++;
            printf("pid %d [running], ", runningProcess->pid);

        } else if (readyCount > 0) { //레디큐에서 새로운 프로세스 가져오기

            runningProcess = readyQueue[0]; 
            for (int j = 1; j < readyCount; j++) { //레디큐에서 제거 후 CPU burst
                readyQueue[j - 1] = readyQueue[j];}
            readyCount--;

            runningProcess->CPUremaining--;
            runningProcess->turnaroundTime++;
            printf("pid %d [running], ", runningProcess->pid);

        } else { //idle
            printf("[CPU idle], ");
        }
        if(runningProcess != NULL){
            if (runningProcess->CPUremaining <= 0) { //terminated
                printf("pid %d [terminated], ", runningProcess->pid);
                runningProcess = NULL;
                finished++;
            }
        }
        //레디큐 프로세스들 기다림
        for(int i = 0; i < readyCount; i++) { 
            readyQueue[i]->waitingTime++;
            readyQueue[i]->turnaroundTime++;
        }
        //웨이팅큐 업데이트
        if (waitingCount > 0) {
            for (int i = 0; i < waitingCount; i++) {
                waitingQueue[i]->turnaroundTime++;
                waitingQueue[i]->IOremaining--;
                if (waitingQueue[i]->IOremaining <= 0) { //IO 작업 완료
                    printf("pid %d [I/O complete], ", waitingQueue[i]->pid);

                    //레디큐에 삽입, 웨이팅큐에서 삭제
                    readyQueue[readyCount++] = waitingQueue[i];
                    for (int j = i + 1; j < waitingCount; j++) { 
                        waitingQueue[j - 1] = waitingQueue[j];}
                    waitingCount--;
                    i--; 
                }
            }
        }
        
        //(CPUburst-CPUremaining(CPU burst 진행한 시간))==IOstart일 때 I/O작업 시작
        if(runningProcess != NULL){
            if(runningProcess->IOremaining > 0){
                if (((runningProcess->CPUburst)-(runningProcess->CPUremaining))==runningProcess->IOstart){ 
                    printf("pid %d [I/O request], ", runningProcess->pid);
                    waitingQueue[waitingCount++] = runningProcess;
                    runningProcess = NULL;
                }
            } 
        }
        
        currentTime++;
        printf("\n");
    }
    
    EvaluateOne(p, algorithm_name); //한 알고리즘에 대한 각 프로세스 정보 출력 & evals 배열 update
    
    //메모리 해제
    free(waitingQueue);
    waitingQueue=NULL;
    for (int i = 0; i < processNum; i++) {
        free(p[i]);
        p[i]=NULL;
    }
    free(p);
    p=NULL;
    free(readyQueue);
    readyQueue=NULL;
}

void NonPreemptive_SJF(){
    char algorithm_name[ALGORITHM_NAME_LEN] = "Non-preemptive Shortest Job First Algorithm";
    printf("<%s>\n", algorithm_name);

    //Config: readyQueue, waitingQueue, p 설정
    process** readyQueue = malloc(sizeof(process*) * processNum);
    if (readyQueue == NULL) {
        printf("malloc error");
        return;}
    process** waitingQueue = malloc(sizeof(process*) * processNum);
    if (waitingQueue == NULL) {
        printf("malloc error");
        return;}
    process** p = NULL; 
    p = copy_processes(); //p에 processes를 복사

    //NP_SJF 알고리즘: CPUburst 같으면 FCFS
    int readyCount = 0;
    int waitingCount = 0;
    int currentTime = 0;
    int finished = 0;
    int checked = 0;
    process* runningProcess = NULL;

    while (finished < processNum) {

        printf("Time %d: ", currentTime);
        
        while (checked < processNum && p[checked]->arrivalTime <= currentTime) { //도착한 프로세스들을 레디큐에 업데이트
            readyQueue[readyCount++] = p[checked++];
        }
        sort_burst(readyQueue, readyCount); //CPU burst time으로 정렬
        
        if (runningProcess != NULL) { //작업 중인 프로세스 존재함

            runningProcess->CPUremaining--;
            runningProcess->turnaroundTime++;
            printf("pid %d [running], ", runningProcess->pid);

        } else if (readyCount > 0) { //레디큐에서 새로운 프로세스 가져오기

            runningProcess = readyQueue[0]; 
            for (int j = 1; j < readyCount; j++) { //레디큐에서 제거 후 CPU burst
                readyQueue[j - 1] = readyQueue[j];}
            readyCount--;
            runningProcess->CPUremaining--;
            runningProcess->turnaroundTime++;
            printf("pid %d [running], ", runningProcess->pid);

        } else { //idle
            printf("[CPU idle], ");
        }
        if(runningProcess != NULL){
            if (runningProcess->CPUremaining <= 0) { //terminated
                printf("pid %d [terminated], ", runningProcess->pid);
                runningProcess = NULL;
                finished++;
            }
        }
        //레디큐 프로세스들 기다림
        for(int i = 0; i < readyCount; i++) { 
            readyQueue[i]->waitingTime++;
            readyQueue[i]->turnaroundTime++;
        }
        //웨이팅큐 업데이트
        if (waitingCount > 0) {
            for (int i = 0; i < waitingCount; i++) {
                waitingQueue[i]->turnaroundTime++;
                waitingQueue[i]->IOremaining--;
                if (waitingQueue[i]->IOremaining <= 0) { //IO 작업 완료
                    printf("pid %d [I/O complete], ", waitingQueue[i]->pid);

                    //레디큐에 삽입, 웨이팅큐에서 삭제
                    readyQueue[readyCount++] = waitingQueue[i];
                    for (int j = i + 1; j < waitingCount; j++) { 
                        waitingQueue[j - 1] = waitingQueue[j];}
                    waitingCount--;
                    i--; 
                }
            }
        }

        //(CPUburst-CPUremaining(CPU burst 진행한 시간))==IOstart일 때 I/O작업 시작
        if(runningProcess != NULL){
            if(runningProcess->IOremaining > 0){
                if ((runningProcess->CPUburst-runningProcess->CPUremaining)==runningProcess->IOstart){ 
                    printf("pid %d [I/O request], ", runningProcess->pid);
                    waitingQueue[waitingCount++] = runningProcess;
                    runningProcess = NULL;
                }
            } 
        }
        
        currentTime++;
        printf("\n");
    }

    EvaluateOne(p, algorithm_name);

    //메모리 해제
    free(waitingQueue);
    waitingQueue=NULL;
    for (int i = 0; i < processNum; i++) {
        free(p[i]);
        p[i]=NULL;
    }
    free(p);
    p=NULL;
    free(readyQueue);
    readyQueue=NULL;
}

void Preemptive_SJF(){
    char algorithm_name[ALGORITHM_NAME_LEN] = "Preemptive Shortest Job First Algorithm";
    printf("<%s>\n", algorithm_name);
    
    //Config: readyQueue, waitingQueue, p 설정
    process** readyQueue = malloc(sizeof(process*) * processNum);
    if (readyQueue == NULL) {
        printf("malloc error");
        return;}
    process** waitingQueue = malloc(sizeof(process*) * processNum);
    if (waitingQueue == NULL) {
        printf("malloc error");
        return;}
    process** p = NULL; 
    p = copy_processes(); //p에 processes를 복사

    //P_SJF 알고리즘: CPUburst 같으면 FCFS
    int readyCount = 0;
    int waitingCount = 0;
    int currentTime = 0;
    int finished = 0;
    int checked = 0;
    process* runningProcess = NULL;

    while (finished < processNum) {

        printf("Time %d: ", currentTime);
        
        while (checked < processNum && p[checked]->arrivalTime <= currentTime) { //도착한 프로세스들을 레디큐에 업데이트
            readyQueue[readyCount++] = p[checked++];
        }
        sort_burst(readyQueue, readyCount); //CPU burst time으로 정렬
        
        if (runningProcess != NULL) { //작업 중인 프로세스 존재함
            if(readyCount > 0){ //레디큐 다시 확인해서 필요시 preemption
                if(runningProcess->CPUremaining > readyQueue[0]->CPUremaining){ //이미 수행 중일 경우 burst 같을 시 기존 프로세스 유지 (pid 비교 X)
                    readyQueue[readyCount++] = runningProcess; //기존 프로세스를 레디큐에 다시 넣기
                    runningProcess = readyQueue[0];
                    for (int j = 1; j < readyCount; j++) { //새 프로세스를 레디큐에서 제거
                        readyQueue[j - 1] = readyQueue[j];}
                    readyCount--;
                }
            }
            runningProcess->CPUremaining--; //CPU burst
            runningProcess->turnaroundTime++;
            printf("pid %d [running], ", runningProcess->pid);

        } else if (readyCount > 0) { //레디큐에서 새로운 프로세스 가져오기

            runningProcess = readyQueue[0]; 
            for (int j = 1; j < readyCount; j++) { //레디큐에서 제거 후 CPU burst
                readyQueue[j - 1] = readyQueue[j];}
            readyCount--;

            runningProcess->CPUremaining--;
            runningProcess->turnaroundTime++;
            printf("pid %d [running], ", runningProcess->pid);

        } else { //idle
            printf("[CPU idle], ");
        }
        if(runningProcess != NULL){
            if (runningProcess->CPUremaining <= 0) { //terminated
                printf("pid %d [terminated], ", runningProcess->pid);
                runningProcess = NULL;
                finished++;
            }
        }
        //레디큐 프로세스들 기다림
        for(int i = 0; i < readyCount; i++) { 
            readyQueue[i]->waitingTime++;
            readyQueue[i]->turnaroundTime++;
        }
        //웨이팅큐 업데이트
        if (waitingCount > 0) {
            for (int i = 0; i < waitingCount; i++) {
                waitingQueue[i]->turnaroundTime++;
                waitingQueue[i]->IOremaining--;
                if (waitingQueue[i]->IOremaining <= 0) { //IO 작업 완료
                    printf("pid %d [I/O complete], ", waitingQueue[i]->pid);

                    //레디큐에 삽입, 웨이팅큐에서 삭제
                    readyQueue[readyCount++] = waitingQueue[i];
                    for (int j = i + 1; j < waitingCount; j++) { 
                        waitingQueue[j - 1] = waitingQueue[j];}
                    waitingCount--;
                    i--; 
                }
            }
        }
        //(CPUburst-CPUremaining(CPU burst 진행한 시간))==IOstart일 때 I/O작업 시작
        if(runningProcess != NULL){
            if(runningProcess->IOremaining > 0){
                if ((runningProcess->CPUburst-runningProcess->CPUremaining)==runningProcess->IOstart){ 
                    printf("pid %d [I/O request], ", runningProcess->pid);
                    waitingQueue[waitingCount++] = runningProcess;
                    runningProcess = NULL;
                }
            } 
        }
        
        currentTime++;
        printf("\n");
    }

    EvaluateOne(p, algorithm_name);

    //메모리 해제
    free(waitingQueue);
    waitingQueue=NULL;
    for (int i = 0; i < processNum; i++) {
        free(p[i]);
        p[i]=NULL;
    }
    free(p);
    p=NULL;
    free(readyQueue);
    readyQueue=NULL;
}

void NonPreemptive_Priority(){
    char algorithm_name[ALGORITHM_NAME_LEN] = "Non-preemptive Priority Algorithm";
    printf("<%s>\n", algorithm_name);
  
    //Config: readyQueue, waitingQueue, p 설정
    process** readyQueue = malloc(sizeof(process*) * processNum);
    if (readyQueue == NULL) {
        printf("malloc error");
        return;}
    process** waitingQueue = malloc(sizeof(process*) * processNum);
    if (waitingQueue == NULL) {
        printf("malloc error");
        return;}
    process** p = NULL; 
    p = copy_processes(); //p에 processes를 복사

    //N_P 알고리즘: priority 같으면 FCFS
    int readyCount = 0;
    int waitingCount = 0;
    int currentTime = 0;
    int finished = 0;
    int checked = 0;
    process* runningProcess = NULL;

    while (finished < processNum) {

        printf("Time %d: ", currentTime);
        
        while (checked < processNum && p[checked]->arrivalTime <= currentTime) { //도착한 프로세스들을 레디큐에 업데이트
            readyQueue[readyCount++] = p[checked++];
        }
        sort_priority(readyQueue, readyCount); //priority로 정렬
        
        if (runningProcess != NULL) { //작업 중인 프로세스 존재함

            runningProcess->CPUremaining--;
            runningProcess->turnaroundTime++;
            printf("pid %d [running], ", runningProcess->pid);

        } else if (readyCount > 0) { //레디큐에서 새로운 프로세스 가져오기

            runningProcess = readyQueue[0]; 
            for (int j = 1; j < readyCount; j++) { //레디큐에서 제거 후 CPU burst
                readyQueue[j - 1] = readyQueue[j];}
            readyCount--;
            runningProcess->CPUremaining--;
            runningProcess->turnaroundTime++;
            printf("pid %d [running], ", runningProcess->pid);

        } else { //idle
            printf("[CPU idle], ");
        }
        if(runningProcess != NULL){
            if (runningProcess->CPUremaining <= 0) { //terminated
                printf("pid %d [terminated], ", runningProcess->pid);
                runningProcess = NULL;
                finished++;
            }
        }
        //레디큐 프로세스들 기다림
        for(int i = 0; i < readyCount; i++) { 
            readyQueue[i]->waitingTime++;
            readyQueue[i]->turnaroundTime++;
        }
        //웨이팅큐 업데이트
        if (waitingCount > 0) {
            for (int i = 0; i < waitingCount; i++) {
                waitingQueue[i]->turnaroundTime++;
                waitingQueue[i]->IOremaining--;
                if (waitingQueue[i]->IOremaining <= 0) { //IO 작업 완료
                    printf("pid %d [I/O complete], ", waitingQueue[i]->pid);

                    //레디큐에 삽입, 웨이팅큐에서 삭제
                    readyQueue[readyCount++] = waitingQueue[i];
                    for (int j = i + 1; j < waitingCount; j++) { 
                        waitingQueue[j - 1] = waitingQueue[j];}
                    waitingCount--;
                    i--; 
                }
            }
        }
        //(CPUburst-CPUremaining(CPU burst 진행한 시간))==IOstart일 때 I/O작업 시작
        if(runningProcess != NULL){
            if(runningProcess->IOremaining > 0){
                if ((runningProcess->CPUburst-runningProcess->CPUremaining)==runningProcess->IOstart){ 
                    printf("pid %d [I/O request], ", runningProcess->pid);
                    waitingQueue[waitingCount++] = runningProcess;
                    runningProcess = NULL;
                }
            } 
        }
        
        currentTime++;
        printf("\n");
    }


    EvaluateOne(p, algorithm_name);

    //메모리 해제
    free(waitingQueue);
    waitingQueue=NULL;
    for (int i = 0; i < processNum; i++) {
        free(p[i]);
        p[i]=NULL;
    }
    free(p);
    p=NULL;
    free(readyQueue);
    readyQueue=NULL;
}

void Preemptive_Priority(){
    char algorithm_name[ALGORITHM_NAME_LEN] = "Preemptive Priority Algorithm";
    printf("<%s>\n", algorithm_name);
    
    //Config: readyQueue, waitingQueue, p 설정
    process** readyQueue = malloc(sizeof(process*) * processNum);
    if (readyQueue == NULL) {
        printf("malloc error");
        return;}
    process** waitingQueue = malloc(sizeof(process*) * processNum);
    if (waitingQueue == NULL) {
        printf("malloc error");
        return;}
    process** p = NULL; 
    p = copy_processes(); //p에 processes를 복사

    //P_P 알고리즘: priority 같으면 FCFS
    int readyCount = 0;
    int waitingCount = 0;
    int currentTime = 0;
    int finished = 0;
    int checked = 0;
    process* runningProcess = NULL;
    while (finished < processNum) {

        printf("Time %d: ", currentTime);
        
        while (checked < processNum && p[checked]->arrivalTime <= currentTime) { //도착한 프로세스들을 레디큐에 업데이트
            readyQueue[readyCount++] = p[checked++];
        }
        sort_priority(readyQueue, readyCount); //priority로 정렬
        
        if (runningProcess != NULL) { //작업 중인 프로세스 존재함
            if(readyCount > 0){ //레디큐 다시 확인해서 필요시 preemption
                if(runningProcess->priority > readyQueue[0]->priority){ //이미 수행 중일 경우 priority 같을 시 기존 프로세스 유지 (pid 비교 X)
                    readyQueue[readyCount++] = runningProcess; //기존 프로세스를 레디큐에 다시 넣기
                    runningProcess = readyQueue[0];
                    for (int j = 1; j < readyCount; j++) { //새 프로세스를 레디큐에서 제거
                        readyQueue[j - 1] = readyQueue[j];}
                    readyCount--;
                }
            }
            runningProcess->CPUremaining--; //CPU burst
            runningProcess->turnaroundTime++;
            printf("pid %d [running], ", runningProcess->pid);

        } else if (readyCount > 0) { //레디큐에서 새로운 프로세스 가져오기

            runningProcess = readyQueue[0]; 
            for (int j = 1; j < readyCount; j++) { //레디큐에서 제거 후 CPU burst
                readyQueue[j - 1] = readyQueue[j];}
            readyCount--;
            runningProcess->CPUremaining--;
            runningProcess->turnaroundTime++;
            printf("pid %d [running], ", runningProcess->pid);

        } else { //idle
            printf("[CPU idle], ");
        }
        if(runningProcess != NULL){
            if (runningProcess->CPUremaining <= 0) { //terminated
                printf("pid %d [terminated], ", runningProcess->pid);
                runningProcess = NULL;
                finished++;
            }
        }
        //레디큐 프로세스들 기다림
        for(int i = 0; i < readyCount; i++) { 
            readyQueue[i]->waitingTime++;
            readyQueue[i]->turnaroundTime++;
        }
        //웨이팅큐 업데이트
        if (waitingCount > 0) {
            for (int i = 0; i < waitingCount; i++) {
                waitingQueue[i]->turnaroundTime++;
                waitingQueue[i]->IOremaining--;
                if (waitingQueue[i]->IOremaining <= 0) { //IO 작업 완료
                    printf("pid %d [I/O complete], ", waitingQueue[i]->pid);

                    //레디큐에 삽입, 웨이팅큐에서 삭제
                    readyQueue[readyCount++] = waitingQueue[i];
                    for (int j = i + 1; j < waitingCount; j++) { 
                        waitingQueue[j - 1] = waitingQueue[j];}
                    waitingCount--;
                    i--; 
                }
            }
        }

        //(CPUburst-CPUremaining(CPU burst 진행한 시간))==IOstart일 때 I/O작업 시작
        if(runningProcess != NULL){
            if(runningProcess->IOremaining > 0){
                if ((runningProcess->CPUburst-runningProcess->CPUremaining)==runningProcess->IOstart){ 
                    printf("pid %d [I/O request], ", runningProcess->pid);
                    waitingQueue[waitingCount++] = runningProcess;
                    runningProcess = NULL;
                }
            } 
        }
        
        currentTime++;
        printf("\n");
    }


    EvaluateOne(p, algorithm_name);

    //메모리 해제
    free(waitingQueue);
    waitingQueue=NULL;
    for (int i = 0; i < processNum; i++) {
        free(p[i]);
        p[i]=NULL;
    }
    free(p);
    p=NULL;
    free(readyQueue);
    readyQueue=NULL;
}

void RR(){
    char algorithm_name[ALGORITHM_NAME_LEN] = "Round Robin Algorithm";
    printf("<%s (time quantum: %d)>\n", algorithm_name, TIME_QUANTUM);
    
    //Config: readyQueue, waitingQueue, p 설정
    process** readyQueue = malloc(sizeof(process*) * processNum);
    if (readyQueue == NULL) {
        printf("malloc error");
        return;}
    process** waitingQueue = malloc(sizeof(process*) * processNum);
    if (waitingQueue == NULL) {
        printf("malloc error");
        return;}
    process** p = NULL; 
    p = copy_processes(); //p에 processes를 복사

    //RR 알고리즘 (FCFS with time quantum): preemption 시 현재 레디큐의 첫 프로세스 선택
    int readyCount = 0;
    int waitingCount = 0;
    int currentTime = 0;
    int finished = 0;
    int checked = 0;
    int quantum = 0;
    process* runningProcess = NULL;

    while (finished < processNum) {
        
        printf("Time %d: ", currentTime);
        
        while (checked < processNum && p[checked]->arrivalTime <= currentTime) { //도착한 프로세스들을 레디큐에 업데이트
            readyQueue[readyCount++] = p[checked++];
        }
        
        if (runningProcess != NULL) { //작업 중인 프로세스 존재함
            
            if(readyCount > 0){ 
                if(quantum >= TIME_QUANTUM){ //레디큐 다시 확인해서 필요시 preemption
                    readyQueue[readyCount++] = runningProcess; //기존 프로세스를 레디큐에 다시 넣기
                    runningProcess = readyQueue[0];
                    for (int j = 1; j < readyCount; j++) { //새 프로세스를 레디큐에서 제거
                        readyQueue[j - 1] = readyQueue[j];}
                    readyCount--;
                    quantum = 0;
                }
            }

            runningProcess->CPUremaining--;
            runningProcess->turnaroundTime++;
            quantum++;
            printf("pid %d [running], ", runningProcess->pid);

        } else if (readyCount > 0) { //레디큐에서 새로운 프로세스 가져오기

            runningProcess = readyQueue[0]; 
            for (int j = 1; j < readyCount; j++) { //레디큐에서 제거 후 CPU burst
                readyQueue[j - 1] = readyQueue[j];}
            readyCount--;
            quantum = 0;

            runningProcess->CPUremaining--;
            runningProcess->turnaroundTime++;
            quantum++;
            printf("pid %d [running], ", runningProcess->pid);

        } else { //idle
            printf("[CPU idle], ");
        }
        if(runningProcess != NULL){
            if (runningProcess->CPUremaining <= 0) { //terminated
                printf("pid %d [terminated], ", runningProcess->pid);
                runningProcess = NULL;
                finished++;
            }
        }
        //레디큐 프로세스들 기다림
        for(int i = 0; i < readyCount; i++) { 
            readyQueue[i]->waitingTime++;
            readyQueue[i]->turnaroundTime++;
        }
        //웨이팅큐 업데이트
        if (waitingCount > 0) {
            for (int i = 0; i < waitingCount; i++) {
                waitingQueue[i]->turnaroundTime++;
                waitingQueue[i]->IOremaining--;
                if (waitingQueue[i]->IOremaining <= 0) { //IO 작업 완료
                    printf("pid %d [I/O complete], ", waitingQueue[i]->pid);

                    //레디큐에 삽입, 웨이팅큐에서 삭제
                    readyQueue[readyCount++] = waitingQueue[i];
                    for (int j = i + 1; j < waitingCount; j++) { 
                        waitingQueue[j - 1] = waitingQueue[j];}
                    waitingCount--;
                    i--; 
                }
            }
        }

        //(CPUburst-CPUremaining(CPU burst 진행한 시간))==IOstart일 때 I/O작업 시작
        if(runningProcess != NULL){
            if(runningProcess->IOremaining > 0){
                if ((runningProcess->CPUburst-runningProcess->CPUremaining)==runningProcess->IOstart){ 
                    printf("pid %d [I/O request], ", runningProcess->pid);
                    waitingQueue[waitingCount++] = runningProcess;
                    runningProcess = NULL;
                }
            } 
        }
        
        currentTime++;
        printf("\n");
    }

    EvaluateOne(p, algorithm_name);

    //메모리 해제
    free(waitingQueue);
    waitingQueue=NULL;
    for (int i = 0; i < processNum; i++) {
        free(p[i]);
        p[i]=NULL;
    }
    free(p);
    p=NULL;
    free(readyQueue);
    readyQueue=NULL;
}

void NonPreemptive_LIOSJF(){
    char algorithm_name[ALGORITHM_NAME_LEN] = "Non-preemptive Longest I/O Shortest Job First Algorithm";
    printf("<%s>\n", algorithm_name);
  
    //Config: readyQueue, waitingQueue, p 설정
    process** readyQueue = malloc(sizeof(process*) * processNum);
    if (readyQueue == NULL) {
        printf("malloc error");
        return;}
    process** waitingQueue = malloc(sizeof(process*) * processNum);
    if (waitingQueue == NULL) {
        printf("malloc error");
        return;}
    process** p = NULL; 
    p = copy_processes(); //p에 processes를 복사

    //NP_LIOSJF 알고리즘: Longest I/O, SJF, 모두 같으면 FCFS
    int readyCount = 0;
    int waitingCount = 0;
    int currentTime = 0;
    int finished = 0;
    int checked = 0;
    process* runningProcess = NULL;

    while (finished < processNum) {

        printf("Time %d: ", currentTime);
        
        while (checked < processNum && p[checked]->arrivalTime <= currentTime) { //도착한 프로세스들을 레디큐에 업데이트
            readyQueue[readyCount++] = p[checked++];
        }
        sort_IO(readyQueue, readyCount); //remaining I/O & CPU burst time으로 정렬
        
        if (runningProcess != NULL) { //작업 중인 프로세스 존재함

            runningProcess->CPUremaining--;
            runningProcess->turnaroundTime++;
            printf("pid %d [running], ", runningProcess->pid);

        } else if (readyCount > 0) { //레디큐에서 새로운 프로세스 가져오기

            runningProcess = readyQueue[0]; 
            for (int j = 1; j < readyCount; j++) { //레디큐에서 제거 후 CPU burst
                readyQueue[j - 1] = readyQueue[j];}
            readyCount--;
            runningProcess->CPUremaining--;
            runningProcess->turnaroundTime++;
            printf("pid %d [running], ", runningProcess->pid);

        } else { //idle
            printf("[CPU idle], ");
        }
        if(runningProcess != NULL){
            if (runningProcess->CPUremaining <= 0) { //terminated
                printf("pid %d [terminated], ", runningProcess->pid);
                runningProcess = NULL;
                finished++;
            }
        }
        //레디큐 프로세스들 기다림
        for(int i = 0; i < readyCount; i++) { 
            readyQueue[i]->waitingTime++;
            readyQueue[i]->turnaroundTime++;
        }
        //웨이팅큐 업데이트
        if (waitingCount > 0) {
            for (int i = 0; i < waitingCount; i++) {
                waitingQueue[i]->turnaroundTime++;
                waitingQueue[i]->IOremaining--;
                if (waitingQueue[i]->IOremaining <= 0) { //IO 작업 완료
                    printf("pid %d [I/O complete], ", waitingQueue[i]->pid);

                    //레디큐에 삽입, 웨이팅큐에서 삭제
                    readyQueue[readyCount++] = waitingQueue[i];
                    for (int j = i + 1; j < waitingCount; j++) { 
                        waitingQueue[j - 1] = waitingQueue[j];}
                    waitingCount--;
                    i--; 
                }
            }
        }

        //(CPUburst-CPUremaining(CPU burst 진행한 시간))==IOstart일 때 I/O작업 시작
        if(runningProcess != NULL){
            if(runningProcess->IOremaining > 0){
                if ((runningProcess->CPUburst-runningProcess->CPUremaining)==runningProcess->IOstart){ 
                    printf("pid %d [I/O request], ", runningProcess->pid);
                    waitingQueue[waitingCount++] = runningProcess;
                    runningProcess = NULL;
                }
            } 
        }
        
        currentTime++;
        printf("\n");
    }

    EvaluateOne(p, algorithm_name);

    //메모리 해제
    free(waitingQueue);
    waitingQueue=NULL;
    for (int i = 0; i < processNum; i++) {
        free(p[i]);
        p[i]=NULL;
    }
    free(p);
    p=NULL;
    free(readyQueue);
    readyQueue=NULL;
}

void Preemptive_LIOSJF(){
    char algorithm_name[ALGORITHM_NAME_LEN] = "Preemptive Longest I/O Shortest Job First Algorithm";
    printf("<%s>\n", algorithm_name);

    //Config: readyQueue, waitingQueue, p 설정
    process** readyQueue = malloc(sizeof(process*) * processNum);
    if (readyQueue == NULL) {
        printf("malloc error");
        return;}
    process** waitingQueue = malloc(sizeof(process*) * processNum);
    if (waitingQueue == NULL) {
        printf("malloc error");
        return;}
    process** p = NULL; 
    p = copy_processes(); //p에 processes를 복사

    //P_LIOSJF 알고리즘: Longest I/O, SJF, 모두 같으면 FCFS
    int readyCount = 0;
    int waitingCount = 0;
    int currentTime = 0;
    int finished = 0;
    int checked = 0;
    process* runningProcess = NULL;

    while (finished < processNum) {

        printf("Time %d: ", currentTime);
        
        while (checked < processNum && p[checked]->arrivalTime <= currentTime) { //도착한 프로세스들을 레디큐에 업데이트
            readyQueue[readyCount++] = p[checked++];
        }
        sort_IO(readyQueue, readyCount); //remaining I/O & CPU burst time으로 정렬
        
        if (runningProcess != NULL) { //작업 중인 프로세스 존재함
            if(readyCount > 0){ //레디큐 다시 확인해서 필요시 preemption
                if((runningProcess->IOremaining < readyQueue[0]->IOremaining) || (runningProcess->IOremaining == readyQueue[0]->IOremaining && runningProcess->CPUremaining > readyQueue[0]->CPUremaining)){ //remaining I/O burst가 더 크거나, remaining I/O burst는 같은데 reamining CPU burst가 더 작은 경우에만 preempt
                    readyQueue[readyCount++] = runningProcess; //기존 프로세스를 레디큐에 다시 넣기
                    runningProcess = readyQueue[0];
                    for (int j = 1; j < readyCount; j++) { //새 프로세스를 레디큐에서 제거
                        readyQueue[j - 1] = readyQueue[j];}
                    readyCount--;
                }
            }
            runningProcess->CPUremaining--; //CPU burst
            runningProcess->turnaroundTime++;
            printf("pid %d [running], ", runningProcess->pid);

        } else if (readyCount > 0) { //레디큐에서 새로운 프로세스 가져오기

            runningProcess = readyQueue[0]; 
            for (int j = 1; j < readyCount; j++) { //레디큐에서 제거 후 CPU burst
                readyQueue[j - 1] = readyQueue[j];}
            readyCount--;

            runningProcess->CPUremaining--;
            runningProcess->turnaroundTime++;
            printf("pid %d [running], ", runningProcess->pid);

        } else { //idle
            printf("[CPU idle], ");
        }
        if(runningProcess != NULL){
            if (runningProcess->CPUremaining <= 0) { //terminated
                printf("pid %d [terminated], ", runningProcess->pid);
                runningProcess = NULL;
                finished++;
            }
        }
        //레디큐 프로세스들 기다림
        for(int i = 0; i < readyCount; i++) { 
            readyQueue[i]->waitingTime++;
            readyQueue[i]->turnaroundTime++;
        }
        //웨이팅큐 업데이트
        if (waitingCount > 0) {
            for (int i = 0; i < waitingCount; i++) {
                waitingQueue[i]->turnaroundTime++;
                waitingQueue[i]->IOremaining--;
                if (waitingQueue[i]->IOremaining <= 0) { //IO 작업 완료
                    printf("pid %d [I/O complete], ", waitingQueue[i]->pid);

                    //레디큐에 삽입, 웨이팅큐에서 삭제
                    readyQueue[readyCount++] = waitingQueue[i];
                    for (int j = i + 1; j < waitingCount; j++) { 
                        waitingQueue[j - 1] = waitingQueue[j];}
                    waitingCount--;
                    i--; 
                }
            }
        }

        //(CPUburst-CPUremaining(CPU burst 진행한 시간))==IOstart일 때 I/O작업 시작
        if(runningProcess != NULL){
            if(runningProcess->IOremaining > 0){
                if ((runningProcess->CPUburst-runningProcess->CPUremaining)==runningProcess->IOstart){ 
                    printf("pid %d [I/O request], ", runningProcess->pid);
                    waitingQueue[waitingCount++] = runningProcess;
                    runningProcess = NULL;
                }
            } 
        }
        
        currentTime++;
        printf("\n");
    }

    EvaluateOne(p, algorithm_name);

    //메모리 해제
    free(waitingQueue);
    waitingQueue=NULL;
    for (int i = 0; i < processNum; i++) {
        free(p[i]);
        p[i]=NULL;
    }
    free(p);
    p=NULL;
    free(readyQueue);
    readyQueue=NULL;
}

int main(int argc, char **argv){
    processNum = atoi(argv[1]); //프로세스 개수 인자로 전달
    CreateProcess();

    FCFS();
    NonPreemptive_SJF();
    Preemptive_SJF();
    NonPreemptive_Priority();
    Preemptive_Priority();
    RR();

    //추가적인 알고리즘
    NonPreemptive_LIOSJF(); 
    Preemptive_LIOSJF();

    //마지막으로 evals 배열의 전체 algorithm 결과 비교
    EvaluateAll(); 

    //메모리 해제
    for (int i = 0; i < processNum; i++) {
        free(processes[i]);
        processes[i]=NULL;
    }
    free(processes);
    processes=NULL;
    for (int i = 0; i < ALGORITHM_NUM; i++) {
        free(evals[i]);
        evals[i]=NULL;
    }

    return 0;
}