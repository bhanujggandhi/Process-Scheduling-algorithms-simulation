#include <algorithm>
#include <iostream>
#include <queue>
#include <unordered_map>
#include <vector>

using namespace std;

/* Type Definition */
typedef unsigned long long ull;
typedef long long ll;
typedef long double lld;

/* UUID Queue */

queue<unsigned int> UUID;

unsigned int getUUID()
{
    if (!UUID.empty())
    {
        unsigned int uid = UUID.front();
        UUID.pop();
        return uid;
    }

    return rand();
}

/* DataTypes */

enum States
{
    NEW,
    READY,
    RUNNING,
    WAITING,
    TERMINATED
};

struct PCB
{
    unsigned int pid;
    string pname;
    bool ptype; /* 0->IO, 1->CPU */
    int priority;
    lld arrival_time;
    lld burst_time1;
    lld io_time;
    lld burst_time2;
    lld completion_time;
    lld turnaround_time;
    lld waiting_time;
    lld response_time;
    States state;

    PCB(string _pname, lld _arrival_time, lld _burst_time1, lld _io_time, lld _burst_time2, lld _priority)
    {
        pid = getUUID();
        pname = _pname;
        ptype = _pname[0] == 'I' ? 0 : 1;
        priority = _priority;
        arrival_time = _arrival_time;
        burst_time1 = _burst_time1;
        io_time = _io_time;
        burst_time2 = _burst_time2;
        completion_time = INT64_MIN;
        turnaround_time = INT64_MIN;
        waiting_time = INT64_MIN;
        response_time = INT64_MIN;
        state = NEW;
    }

    ~PCB()
    {
        UUID.push(pid);
    }
};

struct CompareProcessFcfs
{
    bool operator()(PCB *const &p1, PCB *const &p2)
    {
        if (p1->priority == p2->priority)
        {
            if (p1->arrival_time == p2->arrival_time)
            {
                if (p1->ptype == 0 and p2->ptype == 0)
                {
                    return p1->pid > p2->pid;
                }
                else if (p1->ptype == 0)
                {
                    return false;
                }
                else
                {
                    return true;
                }
            }
            return p1->arrival_time > p2->arrival_time;
        }
        return p1->priority < p2->priority;
    }
};

/* Global Queues */
 vector<PCB *> processes;
unordered_map<int, PCB *> process_map;
unordered_map<int, PCB *> FcfsRunning_map;
priority_queue<PCB *, vector<PCB *>, CompareProcessFcfs> Fcfs_Ready_queue;
priority_queue<PCB *, vector<PCB *>, CompareProcessFcfs> Fcfs_Device_queue;

/* Utility Functions */
vector<PCB *> Fcfs(){
    vector<PCB *>  FcfsVect;

    return FcfsVect;
}
void context_switch();
void scheduleFcfs();
int main()
{
    for (int i = 0; i < 500; i++)
    {
        UUID.push(i);
    }
    PCB *IdleProcess1 = new PCB("IIdle1", 0,0,0, INT64_MAX, -1);

    int n;
    cout<<"Enter the Number of process: ";
    cin >> n;
    string ProcessName;
    lld ArrivalTime, BurstTime1, IOTime, BurstTime2, Priority;
    cout<<"\nInput format should be :  ProcessName  ArrivalTime  BurstTime1  IOTime  BurstTime2  Priority\n  ";
   
    for (int i = 0; i < n; i++)
    {
        cin>>ProcessName;
        cin>>ArrivalTime>>BurstTime1>>IOTime>>BurstTime2>>Priority;
        PCB *temp = new PCB( ProcessName, ArrivalTime, BurstTime1, IOTime, BurstTime2, Priority);
        processes.push_back(temp);
    }
   
    return 0;
}

void context_switch(PCB *currp, States st)
{
    currp->state = st;
}

void scheduleFcfs()
{
    // 0 pid is for idle process
    if (FcfsRunning_map.size() == 1 and FcfsRunning_map.find(0) == FcfsRunning_map.end())
    {
        return;
    }
    // If only idle process is present and some other process comes
    if (Fcfs_Ready_queue.size() >= 1 and FcfsRunning_map.size() == 1 and FcfsRunning_map.find(0) != FcfsRunning_map.end())
    {
        auto idlep = FcfsRunning_map[0];
        FcfsRunning_map.erase(0);
        Fcfs_Ready_queue.push(idlep);
        context_switch(idlep, READY);

        auto currprocess = Fcfs_Ready_queue.top();
        Fcfs_Ready_queue.pop();
        FcfsRunning_map.insert({currprocess->pid, currprocess});
        context_switch(currprocess, RUNNING);
    }
    // If running queue is empty, schedule any process in pq
    else if (FcfsRunning_map.size() == 0)
    {
        auto currprocess = Fcfs_Ready_queue.top();
        Fcfs_Ready_queue.pop();
        FcfsRunning_map.insert({currprocess->pid, currprocess});
        context_switch(currprocess, RUNNING);
    }
}