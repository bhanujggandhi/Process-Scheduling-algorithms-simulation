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

unsigned int getUUID() {
    if (!UUID.empty()) {
        unsigned int uid = UUID.front();
        UUID.pop();
        return uid;
    }

    return rand();
}

/* DataTypes */

enum States {
    NEW,
    READY,
    RUNNING,
    WAITING,
    TERMINATED
};

struct PCB {
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

    PCB(string _pname, lld _arrival_time, lld _burst_time1, lld _io_time, lld _burst_time2, lld _priority) {
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

    ~PCB() {
        UUID.push(pid);
    }
};

struct CompareProcessPriority_P {
    bool operator()(PCB *const &p1, PCB *const &p2) {
        if (p1->priority == p2->priority) {
            if (p1->arrival_time == p2->arrival_time) {
                if (p1->ptype == 0 and p2->ptype == 0) {
                    return p1->pid > p2->pid;
                } else if (p1->ptype == 0) {
                    return false;
                } else {
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
unordered_map<int, PCB *> Priority_PRunning_map;
priority_queue<PCB *, vector<PCB *>, CompareProcessPriority_P> Priority_P_Ready_queue;
priority_queue<PCB *, vector<PCB *>, CompareProcessPriority_P> Priority_P_Device_queue;

/* Utility Functions */
vector<PCB *> Priority_P() {
    vector<PCB *> Priority_PVect;

    return Priority_PVect;
}
void context_switch();
void schedulePriority_P();
int main() {
    for (int i = 0; i < 500; i++) {
        UUID.push(i);
    }
    PCB *IdleProcess1 = new PCB("IIdle1", 0, 0, 0, INT64_MAX, -1);

    int n;
    cout << "Enter the Number of process: ";
    cin >> n;
    string ProcessName;
    lld ArrivalTime, BurstTime1, IOTime, BurstTime2, Priority;
    cout << "\nInput format should be :  ProcessName  ArrivalTime  BurstTime1  IOTime  BurstTime2  Priority\n  ";

    for (int i = 0; i < n; i++) {
        cin >> ProcessName;
        cin >> ArrivalTime >> BurstTime1 >> IOTime >> BurstTime2 >> Priority;
        PCB *temp = new PCB(ProcessName, ArrivalTime, BurstTime1, IOTime, BurstTime2, Priority);
        processes.push_back(temp);
    }

    return 0;
}

void context_switch(PCB *currp, States st) {
    currp->state = st;
}

void schedulePriority_P() {
    // 0 pid is for idle process
    if (Priority_PRunning_map.size() == 1 and Priority_PRunning_map.find(0) == Priority_PRunning_map.end()) {
        return;
    }
    // If only idle process is present and some other process comes
    if (Priority_P_Ready_queue.size() >= 1 and Priority_PRunning_map.size() == 1 and Priority_PRunning_map.find(0) != Priority_PRunning_map.end()) {
        auto idlep = Priority_PRunning_map[0];
        Priority_PRunning_map.erase(0);
        Priority_P_Ready_queue.push(idlep);
        context_switch(idlep, READY);

        auto currprocess = Priority_P_Ready_queue.top();
        Priority_P_Ready_queue.pop();
        Priority_PRunning_map.insert({currprocess->pid, currprocess});
        context_switch(currprocess, RUNNING);
    }
    // If running queue is empty, schedule any process in pq
    else if (Priority_PRunning_map.size() == 0) {
        auto currprocess = Priority_P_Ready_queue.top();
        Priority_P_Ready_queue.pop();
        Priority_PRunning_map.insert({currprocess->pid, currprocess});
        context_switch(currprocess, RUNNING);
    }
}