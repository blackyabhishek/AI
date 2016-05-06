#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <time.h>
#include <iostream>
#include <math.h>
#include <vector>
#include <sys/time.h>


using namespace std;

#define DOWN 0
#define LEFT 1
#define UP 2
#define RIGHT 3

#define MY 0
#define OPP 1

#define PROHIBITIONTIME 30
#define PROHIBITIONDEPTH 4

clock_t start,endt;

char date[20],timeString[40];
int baseHour,baseMinute;
float baseSeconds,milliseconds,elapsedTime,diffSec,sec,timeLimit,timeOneLength;
struct timeval tv;
struct tm* ptm;
float currentTime,timeOpp=0.0,timeMy=0.0;
int diffHour,diffMinute,hours,minutes;

bool prohibition=false;
bool newProhibition=false;
bool oscillate=false;

int depthLimit=5;
int movesCount=0;
float TL;
float avgTime=0;
int N,M,K,time_left;
int d=3;
int nodesExplored=0;

float wallClockTime()
{
    gettimeofday (&tv, NULL);
    ptm = localtime (&tv.tv_sec);
    strftime (timeString, sizeof (timeString), "%Y-%m-%d %H:%M:%S", ptm);
    milliseconds = tv.tv_usec / 1000000.0;
    sscanf(timeString,"%s %d:%d:%f",date,&hours,&minutes,&sec);
    sec=sec+milliseconds;
    
    if(sec<baseSeconds)
    {
        minutes--;
        diffSec=sec+60.0-baseSeconds;
    }
    else
        diffSec=sec-baseSeconds;
    if(minutes<baseMinute)
    {
        hours--;
        diffMinute=minutes+60-baseMinute;
    }
    else
        diffMinute=minutes-baseMinute;
    if(hours<baseHour)
        diffHour=hours+24-baseHour;
    else
        diffHour=hours-baseHour;
    
    elapsedTime=3600*diffHour+60*diffMinute+diffSec;
    
    return elapsedTime;
}

struct Coordinate
{
    int x;
    int y;
};

struct QueueNode
{
    Coordinate coord;
    int parentIndex;
};

struct Ply
{
    int option;
    Coordinate coord;
}ply;

struct BoardSquare
{
    bool directions[4];
};

struct PlayerInfo
{
    int ID;
    int wallsLeft;
    Coordinate currentPosition;
    bool **shortestPath;
    int pathLength;
};
struct BoardInfo
{
    BoardSquare **vertex;
    bool **wallCentre;
};

struct History
{
    int myWalls;
    int oppWalls;
    Coordinate myPos;
};

History pastMoves[5];

class Queue
{
    QueueNode *queueArray;
    int front;
    int back;
    int i;
public:
    QueueNode poppedNode;
    Queue(int length);
    void resetQueue();
    void push(int x,int y,int pI);
    bool pop();
    void assignShortestPath(bool **shortestPath);
};

class Graph
{
    int i,j,depth,popCount,pushCount,popIndex;
    bool found;
    bool **visitedNodes;
    Queue *Q;
public:
    Graph(int N,int M);
    void resetVariables();
    void addToQueue(int x,int y);
    int shortestPath(int x,int y,int p,bool **sP);
};

class Quoridor
{
    PlayerInfo player[2];
    BoardInfo board;
    Graph *Gr;
public:
    Quoridor(int N,int M,int K,int p);
    bool neighbouringPathExist(int x,int y,int d);
    void removeHorizontalWall(int x,int y);
    void removeVerticalWall(int x,int y);
    bool addHorizontalWall(int x,int y);
    bool addVerticalWall(int x,int y);
    bool checkPathChangeOnAddingHorizontalWall(int x,int y,int p,bool **sP);
    bool checkPathChangeOnAddingVerticalWall(int x,int y,int p,bool **sP);
    int movePlayer(int p,int direction);
    bool jumpPlayer(int p,int direction);
    int wallsLeft(int p);
    int pathLength(int p);
    void assignPathlength(int length,int p);
    void assignPosition(int x,int y,int p);
    void increaseWalls(int p);
    void decreaseWalls(int p);
    void currentPosition(int p,int &x,int &y);
    int playerID(int p);
    bool reachedGoal(int p);
    void turn(int o,int x,int y,int p);
    void move(int x,int y,int p);
    void updateShortestPath(int p);
    void copyShortestPath(bool **temp,int p);
    void printState();
}*Quo;

class MinMax
{
    bool ***tempShortestPath[2];
    Coordinate ***spiralPath;
public:
    int weights[2]={0,10};
    MinMax(int N,int K,int depth);
    void spiral(int x,int y);
    void printSpiral();
    int evaluationFunction();
    int alphaBetaPruning(int depth,int alpha,int beta,bool turn,int myDepthIndex,int oppDepthIndex);
};


Queue::Queue(int length)
{
    queueArray=new QueueNode[length];
    front=0;
    back=-1;
}
void Queue::resetQueue()
{
    front=0;
    back=-1;
}
void Queue::push(int x,int y,int pI)
{
    back++;
    queueArray[back].coord.x=x;
    queueArray[back].coord.y=y;
    queueArray[back].parentIndex=pI;
}
bool Queue::pop()
{
    if(front>back)
        return false;
    poppedNode.coord.x=queueArray[front].coord.x;
    poppedNode.coord.y=queueArray[front].coord.y;
    poppedNode.parentIndex=queueArray[front].parentIndex;
    front++;
    return true;
}
void Queue::assignShortestPath(bool **shortestPath)
{
    for(i=1;i<=N;i++)
        memset(shortestPath[i],0,(M+1));
    for(int tempInd=front-1;tempInd!=-1;tempInd=queueArray[tempInd].parentIndex)
        shortestPath[queueArray[tempInd].coord.x][queueArray[tempInd].coord.y]=true;
}

Graph::Graph(int N,int M)
{
    Q=new Queue(N*M);
    visitedNodes=new bool*[N+1];
    for(int i=0;i<=N;i++)
        visitedNodes[i]=new bool[M+1];
}
void Graph::resetVariables()
{
    for(i=1;i<=N;i++)
        memset(visitedNodes[i],0,(M+1));
    depth=0;
    popCount=0;
    pushCount=1;
    popIndex=2;
}
void Graph::addToQueue(int x,int y)
{
    if(!visitedNodes[x][y])
    {
        Q->push(x,y,popCount-1);
        pushCount++;
        visitedNodes[x][y]=true;
    }
}
int Graph::shortestPath(int x,int y,int p,bool **sP)
{
    /*int tempx,tempy;
     if(p==Quo->playerID(MY))
     Quo->currentPosition(OPP,tempx,tempy);
     else
     Quo->currentPosition(MY,tempx,tempy);*/
    
    resetVariables();
    
    Q->resetQueue();
    
    Q->push(x,y,-1);
    
    visitedNodes[x][y]=true;
    
    while(Q->pop())
    {
        popCount++;
        if(popCount==popIndex)
        {
            //if(tempx!=Q->poppedNode.coord.x && tempy!=Q->poppedNode.coord.y)
            depth++;
            popIndex=pushCount+1;
        }
        if((p==1 && Q->poppedNode.coord.x==N) || (p==2 && Q->poppedNode.coord.x==1))
        {
            Q->assignShortestPath(sP);
            return depth;
        }
        if(Quo->neighbouringPathExist(Q->poppedNode.coord.x,Q->poppedNode.coord.y,DOWN))
        {
            x=Q->poppedNode.coord.x+1;
            y=Q->poppedNode.coord.y;
            addToQueue(x,y);
        }
        if(Quo->neighbouringPathExist(Q->poppedNode.coord.x,Q->poppedNode.coord.y,LEFT))
        {
            x=Q->poppedNode.coord.x;
            y=Q->poppedNode.coord.y-1;
            addToQueue(x,y);
        }
        if(Quo->neighbouringPathExist(Q->poppedNode.coord.x,Q->poppedNode.coord.y,UP))
        {
            x=Q->poppedNode.coord.x-1;
            y=Q->poppedNode.coord.y;
            addToQueue(x,y);
        }
        if(Quo->neighbouringPathExist(Q->poppedNode.coord.x,Q->poppedNode.coord.y,RIGHT))
        {
            x=Q->poppedNode.coord.x;
            y=Q->poppedNode.coord.y+1;
            addToQueue(x,y);
        }
    }
    return -1;
}

Quoridor::Quoridor(int N,int M,int K,int p)
{
    player[MY].ID=p;
    player[MY].wallsLeft=K;
    player[MY].pathLength=N;
    player[MY].shortestPath=new bool*[N+1];
    for(int i=0;i<=N;i++)
        player[MY].shortestPath[i]=new bool[M+1];
    
    player[OPP].ID=3-p;
    player[OPP].wallsLeft=K;
    player[OPP].pathLength=K;
    player[OPP].shortestPath=new bool*[N+1];
    for(int i=0;i<=N;i++)
        player[OPP].shortestPath[i]=new bool[M+1];
    
    board.vertex=new BoardSquare*[N+1];
    for(int i=0;i<=N;i++)
        board.vertex[i]=new BoardSquare[M+1];
    
    board.wallCentre=new bool*[N+1];
    for(int i=0;i<=N;i++)
        board.wallCentre[i]=new bool[M+1];
    
    for(int i=0;i<=N;i++)
    {
        memset(player[MY].shortestPath[i],0,(M+1));
        memset(player[OPP].shortestPath[i],0,(M+1));
        memset(board.wallCentre[i],0,(M+1));
    }
    
    for(int i=1;i<=N;i++)
    {
        for(int j=1;j<=M;j++)
        {
            board.vertex[i][j].directions[0]=(i==N)?false:true;
            board.vertex[i][j].directions[1]=(j==1)?false:true;
            board.vertex[i][j].directions[2]=(i==1)?false:true;
            board.vertex[i][j].directions[3]=(j==M)?false:true;
        }
    }
    
    if(p==1)
    {
        player[MY].currentPosition.y=(M+1)/2;
        player[MY].currentPosition.x=1;
        player[OPP].currentPosition.y=(M+1)/2;
        player[OPP].currentPosition.x=N;
    }
    else
    {
        player[OPP].currentPosition.y=(M+1)/2;
        player[OPP].currentPosition.x=1;
        player[MY].currentPosition.y=(M+1)/2;
        player[MY].currentPosition.x=N;
    }
    
    Gr=new Graph(N,M);
}
bool Quoridor::neighbouringPathExist(int x,int y,int d)
{
    return board.vertex[x][y].directions[d];
}
void Quoridor::removeHorizontalWall(int x,int y)
{
    board.wallCentre[x][y]=false;
    board.vertex[x-1][y-1].directions[DOWN]=true;
    board.vertex[x-1][y].directions[DOWN]=true;
    board.vertex[x][y-1].directions[UP]=true;
    board.vertex[x][y].directions[UP]=true;
}
void Quoridor::removeVerticalWall(int x,int y)
{
    board.wallCentre[x][y]=false;
    board.vertex[x-1][y-1].directions[RIGHT]=true;
    board.vertex[x][y-1].directions[RIGHT]=true;
    board.vertex[x-1][y].directions[LEFT]=true;
    board.vertex[x][y].directions[LEFT]=true;
}
bool Quoridor::addHorizontalWall(int x,int y)
{
    if(!board.wallCentre[x][y] && board.vertex[x-1][y-1].directions[DOWN] && board.vertex[x-1][y].directions[DOWN])
    {
        board.vertex[x-1][y-1].directions[DOWN]=false;
        board.vertex[x-1][y].directions[DOWN]=false;
        board.vertex[x][y-1].directions[UP]=false;
        board.vertex[x][y].directions[UP]=false;
        board.wallCentre[x][y]=true;
        return true;
    }
    return false;
}
bool Quoridor::addVerticalWall(int x,int y)
{
    if(!board.wallCentre[x][y] && board.vertex[x-1][y-1].directions[RIGHT] && board.vertex[x][y-1].directions[RIGHT])
    {
        board.vertex[x-1][y-1].directions[RIGHT]=false;
        board.vertex[x][y-1].directions[RIGHT]=false;
        board.vertex[x-1][y].directions[LEFT]=false;
        board.vertex[x][y].directions[LEFT]=false;
        board.wallCentre[x][y]=true;
        return true;
    }
    return false;
}
void Quoridor::currentPosition(int p,int &x,int &y)
{
    x=player[p].currentPosition.x;
    y=player[p].currentPosition.y;
}
bool Quoridor::checkPathChangeOnAddingHorizontalWall(int x,int y,int p,bool **sP)
{
    if((sP[x-1][y-1]==true && sP[x][y-1]==true && board.vertex[x-1][y-1].directions[0]==false) || (sP[x-1][y]==true && sP[x][y]==true && board.vertex[x-1][y].directions[0]==false))
        return true;
    return false;
}
bool Quoridor::checkPathChangeOnAddingVerticalWall(int x,int y,int p,bool **sP)
{
    if((sP[x-1][y-1]==true && sP[x-1][y]==true && board.vertex[x-1][y-1].directions[3]==false) || (sP[x][y-1]==true && sP[x][y]==true && board.vertex[x][y-1].directions[3]==false))
        return true;
    return false;
}
int Quoridor::movePlayer(int p,int direction)
{
    if(direction==DOWN && neighbouringPathExist(player[p].currentPosition.x,player[p].currentPosition.y,DOWN))
    {
        if(!(player[1-p].currentPosition.x==player[p].currentPosition.x+1 && player[p].currentPosition.y==player[1-p].currentPosition.y))
        {
            player[p].currentPosition.x++;
            return 1;
        }
        else if(board.vertex[player[1-p].currentPosition.x][player[1-p].currentPosition.y].directions[0])
        {
            player[p].currentPosition.x+=2;
            return 2;
        }
        else if(board.vertex[player[1-p].currentPosition.x][player[1-p].currentPosition.y].directions[1])
        {
            player[p].currentPosition.x++;
            player[p].currentPosition.y--;
            return 3;
        }
        else if(board.vertex[player[1-p].currentPosition.x][player[1-p].currentPosition.y].directions[3])
        {
            player[p].currentPosition.x++;
            player[p].currentPosition.y++;
            return 4;
        }
    }
    else if(direction==LEFT && board.vertex[player[p].currentPosition.x][player[p].currentPosition.y].directions[LEFT])
    {
        if(!(player[1-p].currentPosition.y+1==player[p].currentPosition.y && player[p].currentPosition.x==player[1-p].currentPosition.x))
        {
            player[p].currentPosition.y--;
            return 1;
        }
        else if(board.vertex[player[1-p].currentPosition.x][player[1-p].currentPosition.y].directions[1])
        {
            player[p].currentPosition.y-=2;
            return 2;
        }
        else if(board.vertex[player[1-p].currentPosition.x][player[1-p].currentPosition.y].directions[2])
        {
            player[p].currentPosition.x--;
            player[p].currentPosition.y--;
            return 3;
        }
        else if(board.vertex[player[1-p].currentPosition.x][player[1-p].currentPosition.y].directions[0])
        {
            player[p].currentPosition.x++;
            player[p].currentPosition.y--;
            return 4;
        }
    }
    else if(direction==UP && board.vertex[player[p].currentPosition.x][player[p].currentPosition.y].directions[UP])
    {
        if(!(player[1-p].currentPosition.x+1==player[p].currentPosition.x && player[p].currentPosition.y==player[1-p].currentPosition.y))
        {
            player[p].currentPosition.x--;
            return 1;
        }
        else if(board.vertex[player[1-p].currentPosition.x][player[1-p].currentPosition.y].directions[2])
        {
            player[p].currentPosition.x-=2;
            return 2;
        }
        else if(board.vertex[player[1-p].currentPosition.x][player[1-p].currentPosition.y].directions[3])
        {
            player[p].currentPosition.x--;
            player[p].currentPosition.y++;
            return 3;
        }
        else if(board.vertex[player[1-p].currentPosition.x][player[1-p].currentPosition.y].directions[1])
        {
            player[p].currentPosition.x--;
            player[p].currentPosition.y--;
            return 4;
        }
    }
    else if(direction==RIGHT && board.vertex[player[p].currentPosition.x][player[p].currentPosition.y].directions[RIGHT])
    {
        if(!(player[1-p].currentPosition.y==player[p].currentPosition.y+1 && player[p].currentPosition.x==player[1-p].currentPosition.x))
        {
            player[p].currentPosition.y++;
            return 1;
        }
        else if(board.vertex[player[1-p].currentPosition.x][player[1-p].currentPosition.y].directions[3])
        {
            player[p].currentPosition.y+=2;
            return 2;
        }
        else if(board.vertex[player[1-p].currentPosition.x][player[1-p].currentPosition.y].directions[0])
        {
            player[p].currentPosition.x++;
            player[p].currentPosition.y++;
            return 3;
        }
        else if(board.vertex[player[1-p].currentPosition.x][player[1-p].currentPosition.y].directions[2])
        {
            player[p].currentPosition.x--;
            player[p].currentPosition.y++;
            return 4;
        }
    }
    return 0;
}
bool Quoridor::jumpPlayer(int p,int direction)
{
    if(direction==DOWN && board.vertex[player[1-p].currentPosition.x][player[1-p].currentPosition.y].directions[RIGHT])
    {
        player[p].currentPosition.x++;
        player[p].currentPosition.y++;
        return true;
    }
    else if(direction==LEFT && board.vertex[player[1-p].currentPosition.x][player[1-p].currentPosition.y].directions[DOWN])
    {
        player[p].currentPosition.x++;
        player[p].currentPosition.y--;
        return true;
    }
    else if(direction==UP && board.vertex[player[1-p].currentPosition.x][player[1-p].currentPosition.y].directions[LEFT])
    {
        player[p].currentPosition.x--;
        player[p].currentPosition.y--;
        return true;
    }
    else if(direction==RIGHT && board.vertex[player[1-p].currentPosition.x][player[1-p].currentPosition.y].directions[UP])
    {
        player[p].currentPosition.x--;
        player[p].currentPosition.y++;
        return true;
    }
    return false;
}
int Quoridor::wallsLeft(int p)
{
    return player[p].wallsLeft;
}
int Quoridor::pathLength(int p)
{
    return player[p].pathLength;
}
int Quoridor::playerID(int p)
{
    return player[p].ID;
}
void Quoridor::assignPathlength(int length,int p)
{
    player[p].pathLength=length;
}
void Quoridor::assignPosition(int x, int y, int p)
{
    player[p].currentPosition.x=x;
    player[p].currentPosition.y=y;
}
void Quoridor::increaseWalls(int p)
{
    player[p].wallsLeft++;
}
void Quoridor::decreaseWalls(int p)
{
    player[p].wallsLeft--;
}
bool Quoridor::reachedGoal(int p)
{
    if((player[p].ID==1 && player[p].currentPosition.x==N) || (player[p].ID==2 && player[p].currentPosition.x==1))
        return true;
    return false;
}
void Quoridor::updateShortestPath(int p)
{
    player[p].pathLength=Gr->shortestPath(player[p].currentPosition.x,player[p].currentPosition.y,player[p].ID,player[p].shortestPath);
}
void Quoridor::move(int x,int y,int p)
{
    if(x!=0)
    {
        player[p].currentPosition.x=x;
        player[p].currentPosition.y=y;
    }
}
void Quoridor::turn(int o, int x,int y,int p)
{
    if(o==0)
        move(x,y,p);
    else if(o==1)
    {
        player[p].wallsLeft--;
        addHorizontalWall(x,y);
    }
    else
    {
        player[p].wallsLeft--;
        addVerticalWall(x,y);
    }
    
}
void Quoridor::copyShortestPath(bool **temp,int p)
{
    for(int i=1;i<=N;i++)
        for(int j=1;j<=M;j++)
            temp[i][j]=player[p].shortestPath[i][j];
}

void Quoridor::printState()
{
    for(int i=1;i<=N;i++)
    {
        for(int j=1;j<=M;j++)
            cout<<" "<<board.vertex[i][j].directions[2]<<"  ";
        cout<<"\n";
        for(int j=1;j<=M;j++)
            cout<<board.vertex[i][j].directions[1]<<"*"<<board.vertex[i][j].directions[3]<<" ";
        cout<<"\n";
        for(int j=1;j<=M;j++)
            cout<<" "<<board.vertex[i][j].directions[0]<<"  ";
        cout<<"\n";
    }
    
    for(int i=1;i<=N;i++)
    {
        for(int j=1;j<=M;j++)
            cout<<board.wallCentre[i][j]<<" ";
        cout<<"\n";
    }
    cout<<"My ID : "<<player[MY].ID<<"\n";
    cout<<"My position : ("<<player[MY].currentPosition.x<<","<<player[MY].currentPosition.y<<")\n";
    cout<<"My walls left : "<<player[MY].wallsLeft<<"\n";
    Quo->updateShortestPath(MY);
    cout<<"My shortest path : "<<player[MY].pathLength<<"\n";
    
    cout<<"Opponent's ID : "<<player[OPP].ID<<"\n";
    cout<<"Opponent's position : ("<<player[OPP].currentPosition.x<<","<<player[OPP].currentPosition.y<<")\n";
    cout<<"Opponent's walls left : "<<player[OPP].wallsLeft<<"\n";
    Quo->updateShortestPath(OPP);
    cout<<"Opp shortest path : "<<player[OPP].pathLength<<"\n";
    
    cout<<"Nodes Explored : "<<nodesExplored<<"\n";
    cout<<"Average time : "<<avgTime<<"\n";
    
    for(int i=1;i<=N;i++)
    {
        for(int j=1;j<=M;j++)
            cout<<player[MY].shortestPath[i][j]<<" ";
        cout<<"\n";
    }
    cout<<"\n";
    for(int i=1;i<=N;i++)
    {
        for(int j=1;j<=M;j++)
            cout<<player[OPP].shortestPath[i][j]<<" ";
        cout<<"\n";
    }
    
    cout<<"\n";
}

MinMax::MinMax(int N,int M,int depth)
{
    tempShortestPath[MY]=new bool**[depth+1];
    tempShortestPath[OPP]=new bool**[depth+1];
    for(int i=0;i<=depth;i++)
    {
        tempShortestPath[MY][i]=new bool*[N+1];
        tempShortestPath[OPP][i]=new bool*[N+1];
        for(int j=0;j<=N;j++)
        {
            tempShortestPath[MY][i][j]=new bool[M+1];
            tempShortestPath[OPP][i][j]=new bool[M+1];
        }
    }
    spiralPath=new Coordinate**[N+1];
    for(int i=0;i<=N;i++)
    {
        spiralPath[i]=new Coordinate*[N+1];
        for(int j=0;j<=M;j++)
        {
            spiralPath[i][j]=new Coordinate[N*M];
        }
    }
    for(int i=1;i<=N;i++)
        for(int j=1;j<=M;j++)
            spiral(i,j);
}

int MinMax::evaluationFunction()
{
    nodesExplored++;
    //cout<<"E : "<<Quo->pathLength(OPP)<<" "<<Quo->pathLength(MY)<<"\n";
    return weights[0]*Quo->wallsLeft(MY) + weights[1]*(Quo->pathLength(OPP)-Quo->pathLength(MY));
}
void MinMax::spiral(int x,int y)
{
    int i,j,k,ind,num,c;
    for(i=x,j=y,ind=0,num=0,c=0;c<(N-1)*(M-1);)
    {
        if(ind%2==0)
            num++;
        for(k=0;k<num;k++)
        {
            if(i>=2 && i<=N && j>=2 && j<=M)
            {
                spiralPath[x][y][c].x=i;
                spiralPath[x][y][c].y=j;
                c++;
            }
            
            if(ind%4==0)
                j++;
            else if(ind%4==1)
                i++;
            else if(ind%4==2)
                j--;
            else if(ind%4==3)
                i--;
        }
        ind++;
    }
}
void MinMax::printSpiral()
{
    for(int i=2;i<=N;i++)
    {
        for(int j=2;j<=M;j++)
        {
            cout<<"***********"<<i<<" "<<j<<"**********\n";
            for(int k=0;k<(N-1)*(M-1);k++)
                cout<<spiralPath[i][j][k].x<<" "<<spiralPath[i][j][k].y<<"\n";
        }
    }
}
int MinMax::alphaBetaPruning(int depth,int alpha,int beta,bool myTurn,int myDepthIndex,int oppDepthIndex)
{
    if(depth==0)
        return evaluationFunction();
    int tempx,tempy,moveTemp,tempMyMin,tempOppMin,v,temp;
    bool prune = false,myPathChange,oppPathChange;
    tempMyMin=Quo->pathLength(MY);
    tempOppMin=Quo->pathLength(OPP);
    if(myTurn)
    {
        Quo->currentPosition(MY,tempx,tempy);
        v=-100000000;
        if(myDepthIndex==-1)
        {
            myDepthIndex=depth-1;
            Quo->copyShortestPath(tempShortestPath[MY][depth-1],MY);
        }
        if(oppDepthIndex==-1)
        {
            oppDepthIndex=depth-1;
            Quo->copyShortestPath(tempShortestPath[OPP][depth-1],OPP);
        }
        if(!oscillate && !((Quo->playerID(MY)==1 && d==31) || (Quo->playerID(MY)==2 && d==32)))
        {
            for(int i=0;i<4 && !prune;i++)
            {
                if(!prohibition && depth==depthLimit)
                {
                    timeMy=wallClockTime()-timeOpp;
                    //cout<<"Time Left : "<<time_left-timeMy<<"\n";
                    if(time_left-timeMy<PROHIBITIONTIME)
                    {
                        prohibition=true;
                        prune=true;
                        newProhibition=true;
                    }
                }
                moveTemp=Quo->movePlayer(MY,i);
                if(moveTemp!=0)
                {
                    //cout<<"**** MY :"<<depth<<" 0 ("<<i<<")\n";
                    Quo->updateShortestPath(MY);
                    if(Quo->reachedGoal(MY))
                    {
                        v=1000000+evaluationFunction();
                        alpha=v;
                        if(depth==depthLimit)
                        {
                            ply.option=0;
                            Quo->currentPosition(MY,ply.coord.x,ply.coord.y);
                            cout<<"EVAL1 : "<<v<<"\n";
                        }
                        Quo->assignPosition(tempx,tempy,MY);
                        Quo->assignPathlength(tempMyMin,MY);
                        return v;
                    }
                    temp=alphaBetaPruning(depth-1,alpha,beta,!myTurn,-1,oppDepthIndex);
                    //cout<<"#### MY :"<<depth<<" 0 ("<<i<<")"<<temp<<" "<<v<<"\n";
                    if(temp>v)
                    {
                        v=temp;
                        if(depth==depthLimit)
                        {
                            ply.option=0;
                            Quo->currentPosition(MY,ply.coord.x,ply.coord.y);
                        }
                    }
                    Quo->assignPosition(tempx,tempy,MY);
                    Quo->assignPathlength(tempMyMin,MY);
                    alpha=(alpha>v)?alpha:v;
                    if(beta<=alpha)
                    {
                        prune=true;
                        break;
                    }
                    if(moveTemp==3 && Quo->jumpPlayer(MY,i))
                    {
                        //cout<<"**** MY :"<<depth<<" 0 ("<<i<<")\n";
                        Quo->updateShortestPath(MY);
                        if(Quo->reachedGoal(MY))
                        {
                            v=1000000+evaluationFunction();
                            alpha=v;
                            if(depth==depthLimit)
                            {
                                ply.option=0;
                                Quo->currentPosition(MY,ply.coord.x,ply.coord.y);
                                //cout<<"EVAL2 : "<<v<<"\n";
                            }
                            Quo->assignPosition(tempx,tempy,MY);
                            Quo->assignPathlength(tempMyMin,MY);
                            return v;
                        }
                        temp=alphaBetaPruning(depth-1,alpha,beta,!myTurn,-1,oppDepthIndex);
                        //cout<<"#### MY :"<<depth<<" 0 ("<<i<<")"<<temp<<" "<<v<<"\n";
                        if(temp>v)
                        {
                            v=temp;
                            if(depth==depthLimit)
                            {
                                ply.option=0;
                                Quo->currentPosition(MY,ply.coord.x,ply.coord.y);
                            }
                        }
                        Quo->assignPosition(tempx,tempy,MY);
                        Quo->assignPathlength(tempMyMin,MY);
                        alpha=(alpha>v)?alpha:v;
                        if(beta<=alpha)
                        {
                            prune=true;
                            break;
                        }
                    }
                }
            }
        }
        if(Quo->wallsLeft(MY)>0)
        {
            int temp2x,temp2y,lim=(N-1)*(M-1);
            int i,j;
            Coordinate *c;
            Quo->currentPosition(OPP,temp2x,temp2y);
            if(temp2x==0)
                c=spiralPath[2][2];
            else
                c=spiralPath[temp2x][temp2y];
            for(int it=0;it<lim && !prune;it++)
            {
                if(!prohibition && depth==depthLimit)
                {
                    timeMy=wallClockTime()-timeOpp;
                    //cout<<"Time Left : "<<time_left-timeMy<<"\n";
                    if(time_left-timeMy<PROHIBITIONTIME)
                    {
                        prohibition=true;
                        prune=true;
                        newProhibition=true;
                    }
                }
                i=c[it].x;
                j=c[it].y;
                if(Quo->addHorizontalWall(i,j))
                {
                    myPathChange=oppPathChange=false;
                    Quo->decreaseWalls(MY);
                    if(Quo->checkPathChangeOnAddingHorizontalWall(i,j,MY,tempShortestPath[MY][myDepthIndex]))
                    {
                        Quo->updateShortestPath(MY);
                        myPathChange=true;
                    }
                    if(Quo->checkPathChangeOnAddingHorizontalWall(i,j,OPP,tempShortestPath[OPP][oppDepthIndex]))
                    {
                        Quo->updateShortestPath(OPP);
                        oppPathChange=true;
                    }
                    if(!(Quo->pathLength(MY)==-1 || Quo->pathLength(OPP)==-1))
                    {
                        //cout<<"**** MY :"<<depth<<" 1 ("<<i<<","<<j<<")\n";
                        if(myPathChange && oppPathChange)
                            temp=alphaBetaPruning(depth-1,alpha,beta,!myTurn,-1,-1);
                        else if(myPathChange && !oppPathChange)
                            temp=alphaBetaPruning(depth-1,alpha,beta,!myTurn,-1,oppDepthIndex);
                        else if(!myPathChange && oppPathChange)
                            temp=alphaBetaPruning(depth-1,alpha,beta,!myTurn,myDepthIndex,-1);
                        else
                            temp=alphaBetaPruning(depth-1,alpha,beta,!myTurn,myDepthIndex,oppDepthIndex);
                        //cout<<"#### MY :"<<depth<<" 1 ("<<i<<","<<j<<")"<<temp<<" "<<v<<"\n";
                        if(temp>v)
                        {
                            v=temp;
                            if(depth==depthLimit)
                            {
                                ply.option=1;
                                ply.coord.x=i;
                                ply.coord.y=j;
                            }
                        }
                        Quo->removeHorizontalWall(i,j);
                        Quo->increaseWalls(MY);
                        Quo->assignPathlength(tempMyMin,MY);
                        Quo->assignPathlength(tempOppMin,OPP);
                        alpha=(alpha>v)?alpha:v;
                        if(beta<=alpha)
                        {
                            prune=true;
                            break;
                        }
                    }
                    else
                    {
                        Quo->removeHorizontalWall(i,j);
                        Quo->increaseWalls(MY);
                        Quo->assignPathlength(tempMyMin,MY);
                        Quo->assignPathlength(tempOppMin,OPP);
                    }
                }
                if(Quo->addVerticalWall(i,j))
                {
                    myPathChange=oppPathChange=false;
                    Quo->decreaseWalls(MY);
                    if(Quo->checkPathChangeOnAddingVerticalWall(i,j,MY,tempShortestPath[MY][myDepthIndex]))
                    {
                        Quo->updateShortestPath(MY);
                        myPathChange=true;
                    }
                    if(Quo->checkPathChangeOnAddingVerticalWall(i,j,OPP,tempShortestPath[OPP][oppDepthIndex]))
                    {
                        Quo->updateShortestPath(OPP);
                        oppPathChange=true;
                    }
                    if(!(Quo->pathLength(MY)==-1 || Quo->pathLength(OPP)==-1))
                    {
                        //cout<<"**** MY :"<<depth<<" 2 ("<<i<<","<<j<<")\n";
                        if(myPathChange && oppPathChange)
                            temp=alphaBetaPruning(depth-1,alpha,beta,!myTurn,-1,-1);
                        else if(myPathChange && !oppPathChange)
                            temp=alphaBetaPruning(depth-1,alpha,beta,!myTurn,-1,oppDepthIndex);
                        else if(!myPathChange && oppPathChange)
                            temp=alphaBetaPruning(depth-1,alpha,beta,!myTurn,myDepthIndex,-1);
                        else
                            temp=alphaBetaPruning(depth-1,alpha,beta,!myTurn,myDepthIndex,oppDepthIndex);
                        //cout<<"#### MY :"<<depth<<" 2 ("<<i<<","<<j<<")"<<temp<<" "<<v<<"\n";
                        if(temp>v)
                        {
                            v=temp;
                            if(depth==depthLimit)
                            {
                                ply.option=2;
                                ply.coord.x=i;
                                ply.coord.y=j;
                            }
                        }
                        Quo->removeVerticalWall(i,j);
                        Quo->increaseWalls(MY);
                        Quo->assignPathlength(tempMyMin,MY);
                        Quo->assignPathlength(tempOppMin,OPP);
                        alpha=(alpha>v)?alpha:v;
                        if(beta<=alpha)
                        {
                            prune=true;
                            break;
                        }
                    }
                    else
                    {
                        Quo->removeVerticalWall(i,j);
                        Quo->increaseWalls(MY);
                        Quo->assignPathlength(tempMyMin,MY);
                        Quo->assignPathlength(tempOppMin,OPP);
                    }
                }
            }
        }
        if((Quo->playerID(MY) && d==31) || (Quo->playerID(MY)==2 && d==32))
        {
            temp=alphaBetaPruning(depth-1,alpha,beta,!myTurn,myDepthIndex,oppDepthIndex);
            if(temp>v)
            {
                v=temp;
                if(depth==depthLimit)
                {
                    ply.option=0;
                    ply.coord.x=0;
                    ply.coord.y=0;
                }
            }
            alpha=(alpha>v)?alpha:v;
        }
        if(depthLimit==depth)
            cout<<"EVAL3 : "<<v<<"\n";
        return v;
    }
    else
    {
        Quo->currentPosition(OPP,tempx,tempy);
        v=100000000;
        if(myDepthIndex==-1)
        {
            myDepthIndex=depth-1;
            Quo->copyShortestPath(tempShortestPath[MY][depth-1],MY);
        }
        if(oppDepthIndex==-1)
        {
            oppDepthIndex=depth-1;
            Quo->copyShortestPath(tempShortestPath[OPP][depth-1],OPP);
        }
        if(!((Quo->playerID(OPP)==1 && d==31) || (Quo->playerID(OPP)==2 && d==32)))
        {
            for(int i=0;i<4 && !prune;i++)
            {
                moveTemp=Quo->movePlayer(OPP,i);
                if(moveTemp!=0)
                {
                    //cout<<"**** OPP :"<<depth<<" 0 ("<<i<<")\n";
                    Quo->updateShortestPath(OPP);
                    if(Quo->reachedGoal(OPP))
                    {
                        v=-1000000+evaluationFunction();
                        beta=v;
                        Quo->assignPosition(tempx,tempy,OPP);
                        Quo->assignPathlength(tempOppMin,OPP);
                        return v;
                    }
                    temp=alphaBetaPruning(depth-1,alpha,beta,!myTurn,myDepthIndex,-1);
                    //cout<<"#### OPP :"<<depth<<" 0 ("<<i<<")"<<temp<<" "<<v<<"\n";
                    v=(temp<v)?temp:v;
                    Quo->assignPosition(tempx,tempy,OPP);
                    Quo->assignPathlength(tempOppMin,OPP);
                    beta=(beta<v)?beta:v;
                    if(beta<=alpha)
                    {
                        prune=true;
                        break;
                    }
                    if(moveTemp==3 && Quo->jumpPlayer(OPP,i))
                    {
                        //cout<<"**** OPP :"<<depth<<" 0 ("<<i<<")\n";
                        Quo->updateShortestPath(OPP);
                        if(Quo->reachedGoal(OPP))
                        {
                            v=-1000000+evaluationFunction();
                            beta=v;
                            Quo->assignPosition(tempx,tempy,OPP);
                            Quo->assignPathlength(tempOppMin,OPP);
                            return v;
                        }
                        temp=alphaBetaPruning(depth-1,alpha,beta,!myTurn,myDepthIndex,-1);
                        //cout<<"#### OPP :"<<depth<<" 0 ("<<i<<")"<<temp<<" "<<v<<"\n";
                        v=(temp<v)?temp:v;
                        Quo->assignPosition(tempx,tempy,OPP);
                        Quo->assignPathlength(tempOppMin,OPP);
                        beta=(beta<v)?beta:v;
                        if(beta<=alpha)
                        {
                            prune=true;
                            break;
                        }
                    }
                }
            }
        }
        if(Quo->wallsLeft(OPP)>0)
        {
            int temp2x,temp2y,lim=(N-1)*(M-1);
            int i,j;
            Quo->currentPosition(MY,temp2x,temp2y);
            Coordinate *c=spiralPath[temp2x][temp2y];
            for(int it=0;it<lim && !prune;it++)
            {
                i=c[it].x;
                j=c[it].y;
                if(Quo->addHorizontalWall(i,j))
                {
                    myPathChange=oppPathChange=false;
                    Quo->decreaseWalls(OPP);
                    if(Quo->checkPathChangeOnAddingHorizontalWall(i,j,MY,tempShortestPath[MY][myDepthIndex]))
                    {
                        Quo->updateShortestPath(MY);
                        myPathChange=true;
                    }
                    if(Quo->checkPathChangeOnAddingHorizontalWall(i,j,OPP,tempShortestPath[OPP][oppDepthIndex]))
                    {
                        Quo->updateShortestPath(OPP);
                        oppPathChange=true;
                    }
                    if(!(Quo->pathLength(MY)==-1 || Quo->pathLength(OPP)==-1))
                    {
                        //cout<<"**** OPP :"<<depth<<" 1 ("<<i<<","<<j<<")\n";
                        if(myPathChange && oppPathChange)
                            temp=alphaBetaPruning(depth-1,alpha,beta,!myTurn,-1,-1);
                        else if(myPathChange && !oppPathChange)
                            temp=alphaBetaPruning(depth-1,alpha,beta,!myTurn,-1,oppDepthIndex);
                        else if(!myPathChange && oppPathChange)
                            temp=alphaBetaPruning(depth-1,alpha,beta,!myTurn,myDepthIndex,-1);
                        else
                            temp=alphaBetaPruning(depth-1,alpha,beta,!myTurn,myDepthIndex,oppDepthIndex);
                        //cout<<"#### OPP :"<<depth<<" 1 ("<<i<<","<<j<<")"<<temp<<" "<<v<<"\n";
                        v=(temp<v)?temp:v;
                        
                        Quo->removeHorizontalWall(i,j);
                        Quo->increaseWalls(OPP);
                        Quo->assignPathlength(tempMyMin,MY);
                        Quo->assignPathlength(tempOppMin,OPP);
                        
                        beta=(beta<v)?beta:v;
                        
                        if(beta<=alpha)
                        {
                            prune=true;
                            break;
                        }
                    }
                    else
                    {
                        Quo->removeHorizontalWall(i,j);
                        Quo->increaseWalls(OPP);
                        Quo->assignPathlength(tempOppMin,OPP);
                        Quo->assignPathlength(tempMyMin,MY);
                    }
                }
                if(Quo->addVerticalWall(i,j))
                {
                    
                    myPathChange=oppPathChange=false;
                    Quo->decreaseWalls(OPP);
                    if(Quo->checkPathChangeOnAddingVerticalWall(i,j,MY,tempShortestPath[MY][myDepthIndex]))
                    {
                        Quo->updateShortestPath(MY);
                        myPathChange=true;
                    }
                    if(Quo->checkPathChangeOnAddingVerticalWall(i,j,OPP,tempShortestPath[OPP][oppDepthIndex]))
                    {
                        Quo->updateShortestPath(OPP);
                        oppPathChange=true;
                    }
                    if(!(Quo->pathLength(MY)==-1 || Quo->pathLength(OPP)==-1))
                    {
                        //cout<<"**** OPP :"<<depth<<" 2 ("<<i<<","<<j<<")\n";
                        if(myPathChange && oppPathChange)
                            temp=alphaBetaPruning(depth-1,alpha,beta,!myTurn,-1,-1);
                        else if(myPathChange && !oppPathChange)
                            temp=alphaBetaPruning(depth-1,alpha,beta,!myTurn,-1,oppDepthIndex);
                        else if(!myPathChange && oppPathChange)
                            temp=alphaBetaPruning(depth-1,alpha,beta,!myTurn,myDepthIndex,-1);
                        else
                            temp=alphaBetaPruning(depth-1,alpha,beta,!myTurn,myDepthIndex,oppDepthIndex);
                        //cout<<"#### OPP :"<<depth<<" 2 ("<<i<<","<<j<<")"<<temp<<" "<<v<<"\n";
                        v=(temp<v)?temp:v;
                        
                        Quo->removeVerticalWall(i,j);
                        Quo->increaseWalls(OPP);
                        Quo->assignPathlength(tempMyMin,MY);
                        Quo->assignPathlength(tempOppMin,OPP);
                        
                        beta=(beta<v)?beta:v;
                        
                        if(beta<=alpha)
                        {
                            prune=true;
                            break;
                        }
                    }
                    else
                    {
                        Quo->removeVerticalWall(i,j);
                        Quo->increaseWalls(OPP);
                        Quo->assignPathlength(tempOppMin,OPP);
                        Quo->assignPathlength(tempMyMin,MY);
                    }
                }
            }
        }
        if((Quo->playerID(OPP)==1 && d==31) || (Quo->playerID(OPP)==2 && d==32))
        {
            temp=alphaBetaPruning(depth-1,alpha,beta,!myTurn,myDepthIndex,oppDepthIndex);
            if(temp<v)
            {
                v=temp;
                if(depth==depthLimit)
                {
                    ply.option=0;
                    ply.coord.x=0;
                    ply.coord.y=0;
                }
            }
            beta=(beta<v)?beta:v;
        }
        return v;
    }
}

void shiftHistory()
{
    for(int i=0;i<4;i++)
    {
        pastMoves[i].myPos.x=pastMoves[i+1].myPos.x;
        pastMoves[i].myPos.y=pastMoves[i+1].myPos.y;
        pastMoves[i].myWalls=pastMoves[i+1].myWalls;
        pastMoves[i].oppWalls=pastMoves[i+1].oppWalls;
    }
    if(ply.option==0)
    {
        pastMoves[4].myPos.x=ply.coord.x;
        pastMoves[4].myPos.y=ply.coord.y;
        pastMoves[4].myWalls=Quo->wallsLeft(MY);
        pastMoves[4].oppWalls=Quo->wallsLeft(OPP);
    }
    else
    {
        int tempx,tempy;
        Quo->currentPosition(MY,tempx,tempy);
        pastMoves[4].myPos.x=tempx;
        pastMoves[4].myPos.y=tempy;
        pastMoves[4].myWalls=Quo->wallsLeft(MY);
        pastMoves[4].oppWalls=Quo->wallsLeft(OPP);
    }
}
bool checkOscillate()
{
    if(ply.option!=0)
        return false;
    if(Quo->wallsLeft(MY)==pastMoves[4].myWalls && pastMoves[4].myWalls==pastMoves[3].myWalls && pastMoves[3].myWalls==pastMoves[2].myWalls && pastMoves[2].myWalls==pastMoves[1].myWalls && pastMoves[1].myWalls==pastMoves[0].myWalls && Quo->wallsLeft(OPP)==pastMoves[4].oppWalls && pastMoves[4].oppWalls==pastMoves[3].oppWalls && pastMoves[3].oppWalls==pastMoves[2].oppWalls && pastMoves[2].oppWalls==pastMoves[1].oppWalls && pastMoves[1].oppWalls==pastMoves[0].oppWalls)
    {
        //cout<<"11111111111111111\n";
        if(ply.coord.x==pastMoves[3].myPos.x && ply.coord.y==pastMoves[3].myPos.y && pastMoves[3].myPos.x==pastMoves[1].myPos.x && pastMoves[3].myPos.y==pastMoves[1].myPos.y)
        {
            //cout<<"222222222222222222\n";
            if(pastMoves[4].myPos.x==pastMoves[2].myPos.x && pastMoves[4].myPos.y==pastMoves[2].myPos.y && pastMoves[2].myPos.x==pastMoves[0].myPos.x && pastMoves[2].myPos.y==pastMoves[0].myPos.y)
            {
                //cout<<"33333333333333333\n";
                oscillate=true;
                return true;
            }
        }
    }
    return false;
}
void printHistory()
{
    for(int i=0;i<5;i++)
    {
        cout<<i<<"\n";
        cout<<pastMoves[i].myPos.x<<" "<<pastMoves[i].myPos.y<<"\n";
        cout<<pastMoves[i].myWalls<<" "<<pastMoves[i].
        oppWalls<<"\n";
    }
}
int main(int argc, char *argv[])
{
    gettimeofday (&tv, NULL);
    ptm = localtime (&tv.tv_sec);
    strftime (timeString, sizeof (timeString), "%Y-%m-%d %H:%M:%S", ptm);
    milliseconds = tv.tv_usec / 1000000.0;
    sscanf(timeString,"%s %d:%d:%f",date,&baseHour,&baseMinute,&baseSeconds);
    baseSeconds=baseSeconds+milliseconds;
    
    srand (time(NULL));
    int sockfd = 0, n = 0;
    //argv[1]=(char *)"127.0.0.1\0";
    //argv[2]=(char *)"12345\0";
    argc=3;
    char recvBuff[1024];
    char sendBuff[1025];
    struct sockaddr_in serv_addr;
    int player;
    int tempTL=0;
    if(argc != 3)
    {
        printf("\n Usage: %s <ip of server> <port no> \n",argv[0]);
        return 1;
    }
    
    
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Error : Could not create socket \n");
        return 1;
    }
    
    memset(&serv_addr, '0', sizeof(serv_addr));
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[2]));
    
    if(inet_pton(AF_INET, argv[1], &serv_addr.sin_addr)<=0)
    {
        printf("\n inet_pton error occured\n");
        return 1;
    }
    
    if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("\n Error : Connect Failed \n");
        return 1;
    }
    
    cout<<"Quoridor will start..."<<endl;
    
    memset(recvBuff, '0',sizeof(recvBuff));
    n = read(sockfd, recvBuff, sizeof(recvBuff)-1);
    recvBuff[n] = 0;
    sscanf(recvBuff, "%d %d %d %d %d", &player, &N, &M, &K, &time_left);
    
    TL=time_left;
    cout<<"Player "<<player<<endl;
    cout<<"Time "<<time_left<<endl;
    cout<<"Board size "<<N<<"x"<<M<<" :"<<K<<endl;
    int om,oro,oc;
    //int m,r,c;
    //char s[100];
    int x=1;
    
    Quo=new Quoridor(N,M,K,player);
    MinMax *ABP=new MinMax(N,M,depthLimit);
    
    //ABP->printSpiral();
    
    Quo->printState();
    
    if(N==13 || N==11)
        depthLimit=4;
    
    
    if(player == 1)
    {
        memset(sendBuff, '0', sizeof(sendBuff));
        string temp;
        
        int tD=depthLimit;
        depthLimit=1;
        ABP->alphaBetaPruning(depthLimit,-100000000,100000000,true,-1,-1);
        depthLimit=tD;
        
        Quo->turn(ply.option,ply.coord.x,ply.coord.y,MY);
        
        snprintf(sendBuff, sizeof(sendBuff), "%d %d %d", ply.option, ply.coord.x , ply.coord.y);
        write(sockfd, sendBuff, strlen(sendBuff));
        
        memset(recvBuff, '0',sizeof(recvBuff));
        n = read(sockfd, recvBuff, sizeof(recvBuff)-1);
        recvBuff[n] = 0;
        sscanf(recvBuff, "%f %d", &TL, &d);
        
        timeMy=time_left-TL;
        avgTime=(time_left-TL)/movesCount;
        cout<<"PLY : "<<ply.option<<" "<<ply.coord.x<<" "<<ply.coord.y<<"\n";
        cout<<TL<<" "<<d<<endl;
        Quo->printState();
        movesCount++;
        nodesExplored=0;
        
        if(d==1)
        {
            cout<<"You win!! Yayee!! :D ";
            x=0;
        }
        else if(d==2)
        {
            cout<<"Loser :P ";
            x=0;
        }
    }
    
    while(x)
    {
        memset(recvBuff, '0',sizeof(recvBuff));
        n = read(sockfd, recvBuff, sizeof(recvBuff)-1);
        recvBuff[n] = 0;
        sscanf(recvBuff, "%d %d %d %d", &om,&oro,&oc,&d);
        
        timeOpp=wallClockTime()-timeMy;
        //cout<<"Time Left OPP: "<<time_left-timeOpp<<"\n";
        
        Quo->turn(om,oro,oc,OPP);
        
        if(Quo->reachedGoal(OPP))
            Quo->assignPosition(0,0,OPP);
        
        cout << "Opp : "<<om<<" "<<oro<<" "<<oc << " "<<d<<endl;
        Quo->printState();
        
        if(d==1)
        {
            cout<<"You win!! Yayee!! :D ";
            break;
        }
        else if(d==2)
        {
            cout<<"Loser :P ";
            break;
        }
        memset(sendBuff, '0', sizeof(sendBuff));
        string temp;
        
        if(prohibition)
            depthLimit=PROHIBITIONDEPTH;
        
        if(Quo->wallsLeft(MY)==0)
            depthLimit=1;
        
        ABP->alphaBetaPruning(depthLimit,-100000000,100000000,true,-1,-1);
        
        
        if(newProhibition)
        {
            newProhibition=false;
            ABP->alphaBetaPruning(PROHIBITIONDEPTH,-100000000,100000000,true,-1,-1);
        }
        
        
        if(movesCount>6 && checkOscillate())
        {
            //cout<<"OSCILLATE\n";
            ABP->weights[1]=-1*ABP->weights[1];
            ABP->alphaBetaPruning(depthLimit,-100000000,100000000,true,-1,-1);
            ABP->weights[1]=-1*ABP->weights[1];
            oscillate=false;
        }
        shiftHistory();
        printHistory();
        
        Quo->turn(ply.option,ply.coord.x,ply.coord.y,MY);
        
        snprintf(sendBuff, sizeof(sendBuff), "%d %d %d", ply.option, ply.coord.x , ply.coord.y);
        
        write(sockfd, sendBuff, strlen(sendBuff));
        
        memset(recvBuff, '0',sizeof(recvBuff));
        n = read(sockfd, recvBuff, sizeof(recvBuff)-1);
        
        recvBuff[n] = 0;
        sscanf(recvBuff, "%f %d", &TL, &d);//d=3 indicates game continues.. d=2 indicates lost game, d=1 means game won.
        
        
        cout<<"PLY : "<<ply.option<<" "<<ply.coord.x<<" "<<ply.coord.y<<"\n";
        cout<<TL<<" "<<d<<endl;
        timeMy=time_left-TL;
        avgTime=(time_left-TL)/movesCount;
        if(!((Quo->playerID(MY)==1 && d==31) || (Quo->playerID(MY)==2 &&d==32)))
            movesCount++;
        Quo->printState();
        cout<<"Moves : "<<movesCount<<"\n";
        nodesExplored=0;
        
        if(d==1)
        {
            cout<<"You win!! Yayee!! :D ";
            break;
        }
        else if(d==2)
        {
            cout<<"Loser :P ";
            break;
        }
        if(N==13 || N==11)
        {
            if(movesCount<5)
                ABP->weights[0]=0;
            else if(movesCount<15)
                ABP->weights[0]=10;
            else
                ABP->weights[0]=20;
        }
    }
    cout<<"Moves : "<<movesCount<<"\n";
    cout<<endl<<"The End"<<endl;
    return 0;
}

