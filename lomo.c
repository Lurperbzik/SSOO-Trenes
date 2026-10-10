#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <signal.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <string.h>
#include <sys/shm.h>
#include "lomo.h"
#include <sys/wait.h>

struct memoria{
    int x;
    int y;
};

union semun{
    int val;
    struct semid_ds * buf;
    unsigned short *array;
}semun;

struct IPCss{
    int semaforo, buzon, memoria;
    struct memoria *CC;
} IPCss;

void handler(int signum);
void Signal(int semNum);
void Wait(int semNum);
int cordenadaAvance(int * y, struct mensaje *msg);
int avanceTren(int y, struct mensaje *msg);
int seccion(int v, int Seccion);

void checkSection(int pos, int Seccion, int* v, int* y, struct mensaje* msg, int *flg2);
int interbloqueo(int pos, int * vect, int casoBase);
int mensajes(int * vect, int pos);
void avanceBloq(int pos, int *flg, int * y, struct mensaje *msg, int *v);
void sincBloq(int cont, int * vect, int pos, int *flg, int * y, struct mensaje *msg, int *v);
void desbloquearSeccion(int pos, int * vect);

int main(int argc, char * argv[]){
    FILE * file;
    struct mensaje msg, msgtren;
    int retardo, ntrenes, numSem = 10; // 1 bib + 1 ini + 1 mem + 6 sec + 1 sinc
    int x, y, i, v, pos, s;
    int w, z;
    int flg = 1, salida;

    signal(SIGINT, handler);

    if(argc == 2 && !strcmp(argv[1], "--mapa")){ 
        close(1);
        file = fopen("mapa.htm", "w+");
        if(file == NULL){
            perror("Error creando fichero : ");
        }
        LOMO_generar_mapa("i3519833", "i1970351");
        fclose(file);
    
    }
    else if(argc == 3){
        retardo = atoi(argv[1]);
        ntrenes = atoi(argv[2]);
    
        if(retardo < 0 || ntrenes < 0 || ntrenes > 100){
            write(1, "Forma de uso: lomo retardo (>=0) ntrenes (>0 y <=100)\n", 55);
            return 0;
        }


        //----------------------Crear Semaforo (GETVAL = 0)--------------------------
        // El semaforo 0 será reservado para el funcionamiento de la biblioteca
        if((IPCss.semaforo = semget(IPC_PRIVATE, numSem, IPC_CREAT | 0600)) < 0){
            perror("Error creando Semaforo : ");
            return 0;
        }
        
        semun.val = 1;
        for(i = 0 ; i < 3; i++){
            if(semctl(IPCss.semaforo, i, SETVAL, semun) == -1){
                perror("Error en semctl : ");
                handler(15);
            }
        }

        semun.val = 0;
        for(i = 3 ; i < numSem; i++){
            if(semctl(IPCss.semaforo, i, SETVAL, semun) == -1){
                perror("Error en semctl : ");
                handler(15);
            }
        }
       
        //----------------------------------Crear Buzon------------------------------------------
        if((IPCss.buzon = msgget(IPC_PRIVATE, IPC_CREAT | 0600)) == -1){
            perror("Error creando Buzon : ");
            return 0;
        }

        IPCss.memoria = shmget(IPC_PRIVATE, (6+1) * sizeof(struct memoria), IPC_CREAT | 0600); //  1 orden trenes + 6 secciones
        if(IPCss.memoria == -1){
            write(1, "Error creacion de momoria", 25);
            exit(0);
        }
    
        // Obtenemos puntero a Zona compartida y ya esta listo para su uso
        IPCss.CC = (struct memoria*)shmat(IPCss.memoria, 0, 0);
        for(i = 0; i < 7; i++){
            IPCss.CC[i].x = 0;
            IPCss.CC[i].y = 0;
        }

        if(LOMO_inicio(retardo, IPCss.semaforo, IPCss.buzon, "i3519833", "i1970351") == -1){
            write(1, "Error en LOMO_inicio", 20);
        }

        i = 1;
        
        while(i < ntrenes){
            switch(fork()){
            case -1 : 
                perror("Error en el fork : ");
                handler(15);
                break;
            case 0 :
                i = ntrenes;
                break;
            default :
                break;
            }
            i++;
        }

        //-------------------------------- creación de tren nuevo -------------------------

        msg.tipo = TIPO_TRENNUEVO;
        
        if(msgsnd(IPCss.buzon,&msg,sizeof(msg)-sizeof(long),0) == -1){
            perror("Error enviando mensaje :  ");
        }

        if(msgrcv(IPCss.buzon,&msg,sizeof(msg)-sizeof(long),TIPO_RESPTRENNUEVO, MSG_NOERROR) == -1){
                perror("Error recibir mensaje: ");
        } 

        y = msg.y;
        Wait(1);
        IPCss.CC[0].x++;
        pos = IPCss.CC[0].x;
        IPCss.CC[pos].x = 100+pos;
        v = cordenadaAvance(&y, &msg);

        while(1){
            w = seccion(v, IPCss.CC[pos].x);            
            while(IPCss.CC[pos].x == w){                       
                w = seccion(v, IPCss.CC[pos].x);
                if(IPCss.CC[pos].x == w){
                    salida = avanceTren(y, &msg);
                    v = cordenadaAvance(&y, &msg);
                    //-----------------------
                    if(salida == 1600 && flg){
                        flg = 0;
                        Signal(1);
                    }
                    //-----------------------
                }else{
                    if(salida == 1600 && flg){
                        flg = 0;
                        Signal(1);
                    }
                    break;
                }
            }
            checkSection(pos, w, &v, &y, &msg, &flg);
           
        }
    }

    else{
        write(1, "\nForma de uso: lomo --mapa\n", 28);
        write(1, "Forma de uso: lomo retardo (>=0) ntrenes (>0 y <=100)\n", 55);
    }
    return 0;

} // end of main

void Wait(int semNum){  
    struct sembuf semOpt;
    semOpt.sem_num = semNum;
    semOpt.sem_op = -1; 
    semOpt.sem_flg = 0; 
    
    if(semop(IPCss.semaforo, &semOpt, 1) == -1){
        handler(15);
    }
    
}

void Signal(int semNum){
    struct sembuf semOpt;
    semOpt.sem_num = semNum;
    semOpt.sem_op = 1; 
    semOpt.sem_flg = 0;
    
    if(semop(IPCss.semaforo, &semOpt, 1) == -1){
        handler(15);
    }
}

void handler(int signum){
    if(getpid() == getpgrp()){
     // Eliminamos semaforo
     if(semctl(IPCss.semaforo, 0, IPC_RMID) == -1){
         perror("Error eliminando semaforo : ");
     }
     // Eliminamos IPCss.buzon
     if((msgctl(IPCss.buzon, IPC_RMID, NULL)) == -1){
         perror("Error eliminando buzon : ");
     }

     //Eliminamos memoria compartida
     //shmdt(IPCss.CC); 
     shmctl(IPCss.memoria, IPC_RMID, NULL); 
     LOMO_fin();

     kill(0 , SIGTERM);
    }
    exit(0);
}





int cordenadaAvance(int * y, struct mensaje * msg){
    //------------------------------ Solicitud de avance del tren ---------------------------

    msg->tipo = TIPO_PETAVANCE;
    if(msgsnd(IPCss.buzon,msg,sizeof(*msg)-sizeof(long), 0)){
        perror("Error enviando mensaje :  ");
    }

    // COORDENADA DONDE VAMOS A IR

    if(msgrcv(IPCss.buzon,msg,sizeof(*msg)-sizeof(long),TIPO_RESPPETAVANCETREN0+msg->tren, MSG_NOERROR) == -1){
            perror("Error recibir mensaje: ");           
    }
    *y = msg->y;
    return 100 * msg->x + msg->y;
}


int avanceTren(int y, struct mensaje * msg){
    //----------------------------------VERIFICAR CORDENADA--------------------------------------------------------

    msg->tipo = TIPO_AVANCE;
    if(msgsnd(IPCss.buzon,msg,sizeof(*msg)-sizeof(long), 0)){
        perror("Error enviando mensaje :  ");
    }


    if(msgrcv(IPCss.buzon,msg,sizeof(*msg)-sizeof(long),TIPO_RESPAVANCETREN0+msg->tren, MSG_NOERROR) == -1){
            perror("Error : 4 ");
    }


    //---------------------------------------------------------------------------------------------
    LOMO_espera(y, msg->y);
    //---------------------------------------------------------------------------------------------
    return 100 * msg->x + msg->y;
}


int seccion(int v, int Seccion){
    int o[13] = { 100, 3611, 1707, 1604, 9, 3512, 1716, 5316, 3707, 3712, 5407, 6700, 1600};
    
    if(v == o[1] || v == o[2])              { return 1; }
    if(v == o[3] || v == o[0] || v == o[12]){ return 2; }
    if(v == o[4] || v == o[5])              { return 3; } //
    if(v == o[6] || v == o[7])              { return 4; }
    if(v == o[9] || v == o[10])             { return 5; }
    if(v == o[11])                          { return 6; }
    return Seccion;
}

void checkSection(int pos, int Seccion, int* v, int* y, struct mensaje* msg, int *flg2){
    int i, Sec;
    int mem = 2;
    int secciones = 6;
    int vect[secciones+1];
    int cont;
    struct mensaje msgtren;

    for(i = 0; i < secciones+1; i++){
        vect[i] = 0;
    }
    //  CC[i].x --> seccion actual, CC[i].y --> seccion donde quiero ir
    Wait(mem);
    Sec = IPCss.CC[pos].x;
    IPCss.CC[pos].y = Seccion;
    
    // SI SECCION A LA QUE VA ESTÁ OCUPADA
    for(i = 1; i < secciones+1; i++){
        if(i != pos && (IPCss.CC[pos].y == IPCss.CC[i].x)){
            
            // Si interbloqueo
            if(interbloqueo(i, vect, IPCss.CC[pos].x)){
                //pon_error("interbloqueo");
                cont = mensajes(vect, pos);
                sincBloq(cont, vect, pos, flg2, y, msg, v);
                Signal(mem);
                return ;
            }
            // Si se bloquea
            msgtren.tipo = 20000 + pos;
            Signal(mem);
            if(msgrcv(IPCss.buzon,&msgtren,sizeof(msgtren)-sizeof(long),msgtren.tipo, 0) == -1){
                perror("Error : 4 ");
                handler(15);
            }
            
            // Si se desbloquea
            avanceBloq(pos, flg2, y, msg, v);
            return ;
        }
    }
    // SI SECCION A LA QUE VA ESTÁ LIBRE
    vect[pos] = pos;
    //pon_error("voy a seccion libre");
    desbloquearSeccion(pos, vect);
    cont = mensajes(vect, pos);
    sincBloq(cont, vect, pos, flg2, y, msg, v);
    Signal(mem);
    return;
}



int interbloqueo(int pos, int * vect, int casoBase){
    int i;
    int secciones = 6;

    if(IPCss.CC[pos].y == 0){
        for(i = 0; i < secciones+1; i++){
            vect[i] = 0;
        }
        return 0;
    }
    for(i = 1; i < secciones+1; i++){
        if(i != pos && IPCss.CC[pos].y == IPCss.CC[i].x){
            vect[pos] = pos;
            if(IPCss.CC[pos].y == casoBase){
                return 1;
            }
            return interbloqueo(i, vect, casoBase);
            break;
        }
    }
    
}

int mensajes(int * vect, int pos){
    int i;
    int cont = 0;
    int secciones = 6;
    struct mensaje msgtren;
    IPCss.CC[pos].x = IPCss.CC[pos].y;
    IPCss.CC[pos].y = 0;

    for(i = 1; i < secciones+1; i++){
        if(vect[i] != 0 && i != pos){
            cont++;
            IPCss.CC[i].x = IPCss.CC[i].y;
            IPCss.CC[i].y = 0;
            msgtren.tipo = 20000 + vect[i];
            msgtren.tren = 0;
            msgsnd(IPCss.buzon, &msgtren, sizeof(msgtren)-sizeof(long), 0);
        }
    }
    return cont;
}

void avanceBloq(int pos, int *flg, int * y, struct mensaje *msg, int *v){
    int i, w;
    int sem_sinc = 9;

    for(i = 0; i < 20; i++){
        Wait(pos+2);
        w = avanceTren(*y, msg);  
        *v = cordenadaAvance(y, msg);
        if(w == 1600 && *flg){
            Signal(1);
            *flg = 0;
        }
        Signal(sem_sinc);
    }
    return;
}

void sincBloq(int cont, int * vect, int pos, int *flg, int * y, struct mensaje *msg, int *v){
    int secciones = 6;
    int sem_sinc = 9;
    int i, j;

    switch(fork()){
        case 0 :
        break;
        default : 
            avanceBloq(pos, flg, y, msg, v);
            return ;
        break;
    }
    vect[pos] = pos;
    
    for(i = 0; i < 20; i++){
        for(j = 1 ; j < secciones+1 ; j++){
            if(vect[j] != 0){
                Signal(vect[j]+2);
                Wait(sem_sinc);
            }
        }    
    }
    for(i = 1; i < secciones+1; i++){
        vect[i] = 0;
    }
    exit(0);
    
}

void desbloquearSeccion(int pos, int * vect){
    int i;
    int secciones = 6;

    for(i = 1; i < secciones+1; i++){
        if(IPCss.CC[i].y == IPCss.CC[pos].x && i!=pos){
            vect[i] = i;
            desbloquearSeccion(i, vect);
            return;
        }
    }
    return;
}
