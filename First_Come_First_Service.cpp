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

struct CompareProcess_fcfs {
    bool operator()(PCB *const &p1, PCB *const &p2) {
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
};
/* Global Queues */
vector<PCB *> processes;
unordered_map<int, PCB *> process_map;
unordered_map<int, PCB *> FcfsRunning_map;
unordered_map<unsigned int, lld> IO_start_time;
priority_queue<PCB *, vector<PCB *>, CompareProcess_fcfs> fcfs_Ready_queue;
queue<PCB *> Device_queue;
lld Last_io_UpdationTime = 0;
/* Utility Functions */
void terminate(PCB *, lld);
void adjust_Device_queue(lld Curr_Time);
vector<PCB *> Fcfs();
void context_switch(PCB *, States);
bool scheduleFcfs();
int main() {
    for (int i = 0; i < 500; i++) {
        UUID.push(i);
    }
    PCB *IdleProcess1 = new PCB("IIdle1", 0, 0, 0, INT64_MAX, -1);

    int n;
    cout << "Enter the Number of process: ";
    cin >> n;
    string ProcessName;
    lld ArrivalTime, BurstTime1, IOTime, BurstTime2;
    cout << "\nInput format should be :  ProcessName  ArrivalTime  BurstTime1  IOTime  BurstTime2 \n  ";

    for (int i = 0; i < n; i++) {
        cin >> ProcessName;
        cin >> ArrivalTime >> BurstTime1 >> IOTime >> BurstTime2;
        PCB *temp = new PCB(ProcessName, ArrivalTime, BurstTime1, IOTime, BurstTime2, 0);
        processes.push_back(temp);
    }
    Fcfs();
    for (int i = 0; i < processes.size(); i++) {
        cout << processes[i]->pid << "\t" << processes[i]->pname << "\t" << processes[i]->ptype << "\t" << processes[i]->priority << "\t" << processes[i]->arrival_time << "\t" << processes[i]->burst_time1 << "\t"
             << processes[i]->io_time << "\t" << processes[i]->burst_time2 << "\t" << processes[i]->completion_time << "\t" << processes[i]->turnaround_time << "\t"
             << processes[i]->waiting_time << "\t" << processes[i]->response_time << "\t" << processes[i]->state << "\n";
    }
    return 0;
}

void context_switch(PCB *currp, States st) {
    currp->state = st;
}

void terminate(PCB *currp, lld Curr_Time) {
    currp->completion_time = Curr_Time;
    context_switch(currp, TERMINATED);
}
void adjust_Device_queue(lld Curr_Time) {
    int len = Device_queue.size();
    int i = 0;
    while (i < len) {
        i++;
        PCB *pcb = Device_queue.front();
        // cout << "checking for : " << pcb->pid << endl;
        Device_queue.pop();
        if (pcb->io_time > (Curr_Time - IO_start_time[pcb->pid])) {
            Device_queue.push(pcb);
            continue;
        } else {
            lld tempIo_time = pcb->io_time;
            pcb->io_time = 0;
            if (pcb->burst_time2 <= 0) {
                terminate(pcb, tempIo_time + IO_start_time[pcb->pid]);
            } else {
                // cout << "pushing into ready : " << pcb->pid << endl;
                fcfs_Ready_queue.push(pcb);
            }
        }
    }
}
lld getCurrTime() {
    lld New_Curr_Time = 0;
    int len = Device_queue.size();
    int i = 0;
    while (i < len) {
        i++;
        PCB *pcb = Device_queue.front();
        Device_queue.pop();
        if (New_Curr_Time < (pcb->io_time + IO_start_time[pcb->pid])) {
            New_Curr_Time = (pcb->io_time + IO_start_time[pcb->pid]);
        }
        Device_queue.push(pcb);
    }
    return New_Curr_Time;
}
bool scheduleFcfs() {
    // 0 pid is for idle process
    if (FcfsRunning_map.size() == 1 and FcfsRunning_map.find(0) == FcfsRunning_map.end()) {
        return true;
    }
    // If only idle process is present and some other process comes
    if (fcfs_Ready_queue.size() >= 1 and FcfsRunning_map.size() == 1 and FcfsRunning_map.find(0) != FcfsRunning_map.end()) {
        PCB *idlep = FcfsRunning_map[0];
        FcfsRunning_map.erase(0);
        fcfs_Ready_queue.push(idlep);
        context_switch(idlep, READY);

        PCB *currprocess = fcfs_Ready_queue.top();
        fcfs_Ready_queue.pop();
        FcfsRunning_map[currprocess->pid] = currprocess;
        context_switch(currprocess, RUNNING);
        return true;
    }
    // If running queue is empty, schedule any process in pq
    else if (FcfsRunning_map.size() == 0) {
        PCB *currprocess = fcfs_Ready_queue.top();
        fcfs_Ready_queue.pop();
        FcfsRunning_map[currprocess->pid] = currprocess;
        context_switch(currprocess, RUNNING);
        return true;
    }
    return false;
}

vector<PCB *> Fcfs() {
    vector<PCB *> FcfsVect;
    sort(processes.begin(), processes.end(), [](auto &a, auto &b) { return a->arrival_time < b->arrival_time; });
    lld Curr_Time = 0;
    bool flag = true;
    for (int i = 0; i < processes.size(); i++) {
        fcfs_Ready_queue.push(processes[i]);
    }
    while (fcfs_Ready_queue.size() > 0) {
        adjust_Device_queue(Curr_Time);
        PCB *pcb = fcfs_Ready_queue.top();
        lld prev_Curr_Time = Curr_Time;
        bool check = scheduleFcfs();
        if (!check && !Device_queue.empty()) {
            Curr_Time = getCurrTime();
            adjust_Device_queue(Curr_Time);
            scheduleFcfs();
        }
        if (flag) {
            Curr_Time = pcb->arrival_time + pcb->burst_time1;
            pcb->burst_time1 = 0;
            if (pcb->io_time > 0) {
                Device_queue.push(pcb);
                IO_start_time[pcb->pid] = Curr_Time;
            } else {
                Curr_Time += pcb->burst_time2;
                pcb->burst_time2 = 0;
                terminate(pcb, Curr_Time);
            }
            flag = false;
        } else {
            if (pcb->burst_time1 > 0) {
                if (pcb->arrival_time < Curr_Time) {
                    Curr_Time += pcb->burst_time1;
                } else {
                    Curr_Time = pcb->arrival_time + pcb->burst_time1;
                }
                pcb->burst_time1 = 0;
                if (pcb->io_time > 0) {
                    Device_queue.push(pcb);
                    IO_start_time[pcb->pid] = Curr_Time;
                } else {
                    Curr_Time += pcb->burst_time2;
                    pcb->burst_time2 = 0;
                    terminate(pcb, Curr_Time);
                }
            } else {
                if (pcb->arrival_time < Curr_Time) {
                    Curr_Time += pcb->burst_time2;
                } else {
                    Curr_Time = pcb->arrival_time + pcb->burst_time2;
                }
                pcb->burst_time2 = 0;
                terminate(pcb, Curr_Time);
            }
        }
        FcfsRunning_map.erase(pcb->pid);
        FcfsRunning_map.clear();
        if (fcfs_Ready_queue.empty() && !Device_queue.empty()) {
            Curr_Time = getCurrTime();
            adjust_Device_queue(Curr_Time);
        }
    }

    adjust_Device_queue(Curr_Time + 10000000);
    return FcfsVect;
}