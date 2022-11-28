#include <algorithm>
#include <iostream>
#include <queue>
#include <unordered_map>
#include <vector>
#define TIME_SLICE1 4
#define TIME_SLICE2 8
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
    lld RR;
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

    PCB(string _pname, lld _arrival_time, lld _burst_time1, lld _io_time, lld _burst_time2, lld _RR) {
        pid = getUUID();
        pname = _pname;
        ptype = _pname[0] == 'I' ? 0 : 1;
        RR = _RR;
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

struct CompareProcess_RR {
    bool operator()(PCB *const &p1, PCB *const &p2) {
        if (p1->RR == p2->RR) {
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
        return p1->RR > p2->RR;
    }
};
/* Global Queues */
vector<PCB *> processes, org_processes;
unordered_map<int, PCB *> process_map;
unordered_map<int, PCB *> RRRunning_map;

priority_queue<PCB *, vector<PCB *>, CompareProcess_RR> Start_queue, Ready_Queue1, Ready_Queue2, RR_StartQueue, Fcfs_StartQueue;
queue<PCB *> Ready_Queue3;
lld Avg_TAT = 0;
lld Avg_WT = 0;
lld Avg_RT = 0;
/* Utility Functions */
void context_switch();
void terminate(PCB *, lld);
void getAvg();
lld RR_Sched1(lld);
lld RR_Sched2(lld, lld);
lld Fcfs_Sched(lld, lld);
vector<PCB *> Mlfq_Sched();
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
    cout << "Input format should be :  ProcessName  ArrivalTime  BurstTime1  IOTime  BurstTime2 \n  ";

    for (int i = 0; i < n; i++) {
        cin >> ProcessName;
        cin >> ArrivalTime >> BurstTime1 >> IOTime >> BurstTime2;
        PCB *temp = new PCB(ProcessName, ArrivalTime, BurstTime1, IOTime, BurstTime2, ArrivalTime);
        PCB *temp1 = new PCB(ProcessName, ArrivalTime, BurstTime1, IOTime, BurstTime2, ArrivalTime);
        org_processes.push_back(temp1);
        processes.push_back(temp);
    }
    Mlfq_Sched();
    for (int i = 0; i < processes.size(); i++) {
        org_processes[i]->completion_time = processes[i]->completion_time;
        org_processes[i]->response_time = processes[i]->response_time;
        org_processes[i]->turnaround_time = org_processes[i]->completion_time - org_processes[i]->arrival_time;
        org_processes[i]->waiting_time = org_processes[i]->turnaround_time - (org_processes[i]->burst_time1 + org_processes[i]->burst_time2 + org_processes[i]->io_time);
    }
    cout << "pname   arrival_time  burst_time1  io_time  burst_time2  completion_time  response_time  turnaround_time  waiting_time     state  " << endl;
    for (int i = 0; i < processes.size(); i++) {
        cout << org_processes[i]->pname << "\t\t"
             << org_processes[i]->arrival_time << "\t\t" << org_processes[i]->burst_time1 << "\t"
             << org_processes[i]->io_time << "\t" << org_processes[i]->burst_time2 << "\t\t"
             << org_processes[i]->completion_time << "\t\t" << org_processes[i]->response_time
             << "\t\t" << org_processes[i]->turnaround_time << "\t\t"
             << org_processes[i]->waiting_time << "\t\t" << org_processes[i]->state << "\n";
    }
    getAvg();
    cout << endl
         << " Avg Turn Around Time : " << Avg_TAT << endl
         << " Avg Waiting Time : " << Avg_WT << endl
         << " Avg Response Time : " << Avg_RT << endl;
    return 0;
}

void context_switch(PCB *currp, States st) {
    currp->state = st;
}

void terminate(PCB *currp, lld Curr_Time) {
    currp->completion_time = Curr_Time;
    context_switch(currp, TERMINATED);
}

void getAvg() {
    for (int i = 0; i < processes.size(); i++) {
        Avg_TAT += org_processes[i]->turnaround_time;
        Avg_WT += org_processes[i]->waiting_time;
        Avg_RT += org_processes[i]->response_time;
    }
    lld n = processes.size();
    Avg_TAT /= n;
    Avg_WT /= n;
    Avg_RT /= n;
    return;
}

vector<PCB *> Mlfq_Sched() {
    vector<PCB *> RRVect;
    sort(processes.begin(), processes.end(), [](auto &a, auto &b) { return a->arrival_time < b->arrival_time; });
    sort(org_processes.begin(), org_processes.end(), [](auto &a, auto &b) { return a->arrival_time < b->arrival_time; });
    lld Curr_Time = 0;

    int no_of_Process = processes.size();
    for (int i = 0; i < no_of_Process; i++) {
        Start_queue.push(processes[i]);
    }

    Curr_Time = Start_queue.top()->arrival_time;

    while (!Start_queue.empty() or !Ready_Queue1.empty() or !Ready_Queue2.empty() or !Ready_Queue3.empty() or !RR_StartQueue.empty()) {
        lld time_slice = INT32_MAX;
        while (!RR_StartQueue.empty()) {
            PCB *pcb = RR_StartQueue.top();
            if (pcb->RR > Curr_Time) break;
            Ready_Queue2.push(pcb);
            RR_StartQueue.pop();
        }

        while (!Start_queue.empty()) {
            PCB *pcb = Start_queue.top();
            if (pcb->RR > Curr_Time) break;
            Ready_Queue1.push(pcb);
            Start_queue.pop();
        }

        if (!Ready_Queue1.empty()) {
            Curr_Time = RR_Sched1(Curr_Time);
        } else if (!Ready_Queue2.empty()) {
            time_slice = INT32_MAX;
            if (!Start_queue.empty()) {
                lld startqueue_proc = Start_queue.top()->RR;
                lld time_diff = startqueue_proc - Curr_Time;
                Curr_Time = RR_Sched2(Curr_Time, time_diff);
            } else {
                Curr_Time = RR_Sched2(Curr_Time, time_slice);
            }
        } else if (!Ready_Queue3.empty()) {
            lld at1 = -1;
            lld at2 = -1;
            if (!Ready_Queue1.empty() or !Start_queue.empty()) {
                if (!Ready_Queue1.empty()) {
                    at1 = Ready_Queue1.top()->RR;
                } else {
                    at1 = Start_queue.top()->RR;
                }
            }
            if (!Ready_Queue2.empty() || !RR_StartQueue.empty()) {
                if (!Ready_Queue2.empty()) {
                    at2 = Ready_Queue2.top()->RR;
                } else {
                    at2 = RR_StartQueue.top()->RR;
                }
            }
            if (at1 < 0 and at2 < 0)
                time_slice = INT32_MAX;
            else if (at1 >= 0 and at2 >= 0)
                time_slice = min(at1, at2) - Curr_Time;
            else if (at1 < 0)
                time_slice = at2 - Curr_Time;
            else
                time_slice = at1 - Curr_Time;
            Curr_Time = Fcfs_Sched(Curr_Time, time_slice);
        } else if (!RR_StartQueue.empty()) {
            lld first_arrival = RR_StartQueue.top()->RR;
            if (!Start_queue.empty()) {
                lld startqueue_proc = Start_queue.top()->RR;
                if (startqueue_proc > first_arrival) {
                    lld time_diff = startqueue_proc - first_arrival;
                    Curr_Time = first_arrival;
                    while (!RR_StartQueue.empty()) {
                        PCB *pcb = RR_StartQueue.top();
                        if (pcb->RR > Curr_Time) break;
                        Ready_Queue2.push(pcb);
                        RR_StartQueue.pop();
                    }
                    Curr_Time = RR_Sched2(Curr_Time, time_diff);
                } else {
                    Curr_Time = startqueue_proc;
                    while (!Start_queue.empty()) {
                        PCB *pcb = Start_queue.top();
                        if (pcb->RR > Curr_Time) break;
                        Ready_Queue1.push(pcb);
                        Start_queue.pop();
                    }
                    Curr_Time = RR_Sched1(Curr_Time);
                }
            }

        } else if (!Start_queue.empty()) {
            lld startqueue_proc = Start_queue.top()->RR;
            Curr_Time = startqueue_proc;
            while (!Start_queue.empty()) {
                PCB *pcb = Start_queue.top();
                if (pcb->RR > Curr_Time) break;
                Ready_Queue1.push(pcb);
                Start_queue.pop();
            }
            RR_Sched1(Curr_Time);
        }
    }

    return RRVect;
}

lld RR_Sched1(lld Curr_Time) {
    while (!Ready_Queue1.empty()) {
        PCB *pcb = Ready_Queue1.top();
        Ready_Queue1.pop();
        if (pcb->response_time == INT64_MIN) {
            pcb->response_time = Curr_Time;
        }
        if (pcb->burst_time1 > 0) {
            if (pcb->burst_time1 > TIME_SLICE1) {
                pcb->burst_time1 -= TIME_SLICE1;
                Curr_Time += TIME_SLICE1;
                Ready_Queue2.push(pcb);
            } else {
                Curr_Time += pcb->burst_time1;
                pcb->burst_time1 = 0;
                if (pcb->io_time > 0) {
                    pcb->RR = Curr_Time + pcb->io_time;
                    Start_queue.push(pcb);
                } else {
                    pcb->RR = -1;
                    Ready_Queue1.push(pcb);
                }
            }
        } else {
            if (pcb->burst_time2 > TIME_SLICE1) {
                pcb->burst_time2 -= TIME_SLICE1;
                Curr_Time += TIME_SLICE1;
                Ready_Queue2.push(pcb);
            } else {
                Curr_Time += pcb->burst_time2;
                pcb->burst_time2 = 0;
                terminate(pcb, Curr_Time);
            }
        }

        while (!Start_queue.empty()) {
            PCB *pcb = Start_queue.top();
            if (pcb->RR > Curr_Time) break;
            Ready_Queue1.push(pcb);
            Start_queue.pop();
        }
    }
    return Curr_Time;
}
lld RR_Sched2(lld Curr_Time, lld time_slice) {
    while (!Ready_Queue2.empty() && time_slice > 0) {
        PCB *pcb = Ready_Queue2.top();
        Ready_Queue2.pop();
        if (time_slice < TIME_SLICE2) {
            if (pcb->burst_time1 > 0) {
                if (pcb->burst_time1 > time_slice) {
                    pcb->burst_time1 -= time_slice;
                    Curr_Time += time_slice;
                    time_slice -= time_slice;
                    Ready_Queue2.push(pcb);
                } else {
                    Curr_Time += pcb->burst_time1;
                    time_slice -= pcb->burst_time1;
                    pcb->burst_time1 = 0;
                    if (pcb->io_time > 0) {
                        pcb->RR = Curr_Time + pcb->io_time;
                        RR_StartQueue.push(pcb);
                    } else {
                        pcb->RR = -1;
                        Ready_Queue2.push(pcb);
                    }
                }
            } else {
                if (pcb->burst_time2 > time_slice) {
                    pcb->burst_time2 -= time_slice;
                    Curr_Time += time_slice;
                    time_slice -= time_slice;
                    Ready_Queue2.push(pcb);
                } else {
                    Curr_Time += pcb->burst_time2;
                    time_slice -= pcb->burst_time2;
                    pcb->burst_time2 = 0;
                    terminate(pcb, Curr_Time);
                }
            }

        } else {
            if (pcb->burst_time1 > 0) {
                if (pcb->burst_time1 > TIME_SLICE2) {
                    pcb->burst_time1 -= TIME_SLICE2;
                    Curr_Time += TIME_SLICE2;
                    time_slice -= TIME_SLICE2;
                    Ready_Queue2.push(pcb);
                } else {
                    Curr_Time += pcb->burst_time1;
                    time_slice -= pcb->burst_time1;
                    pcb->burst_time1 = 0;
                    if (pcb->io_time > 0) {
                        pcb->RR = Curr_Time + pcb->io_time;
                        RR_StartQueue.push(pcb);
                    } else {
                        pcb->RR = -1;
                        Ready_Queue2.push(pcb);
                    }
                }
            } else {
                if (pcb->burst_time2 > TIME_SLICE2) {
                    pcb->burst_time2 -= TIME_SLICE2;
                    Curr_Time += TIME_SLICE2;
                    time_slice -= TIME_SLICE2;
                    Ready_Queue2.push(pcb);
                } else {
                    Curr_Time += pcb->burst_time2;
                    time_slice -= pcb->burst_time2;
                    pcb->burst_time2 = 0;
                    terminate(pcb, Curr_Time);
                }
            }
        }
        while (!RR_StartQueue.empty()) {
            PCB *pcb = RR_StartQueue.top();
            if (pcb->RR > Curr_Time) break;
            Ready_Queue2.push(pcb);
            RR_StartQueue.pop();
        }
    }

    return Curr_Time;
}
lld Fcfs_Sched(lld Curr_Time, lld time_slice) {
    while (!Ready_Queue3.empty() && time_slice > 0) {
        PCB *pcb = Ready_Queue3.front();
        Ready_Queue3.pop();
        if (pcb->RR > Curr_Time)
            Curr_Time = pcb->RR;
        if (pcb->burst_time1 > 0) {
            if (pcb->burst_time1 <= time_slice) {
                Curr_Time += pcb->burst_time1;
                time_slice -= pcb->burst_time1;
                pcb->burst_time1 = 0;
                if (pcb->io_time > 0) {
                    pcb->RR = Curr_Time + pcb->io_time;
                    Ready_Queue3.push(pcb);
                } else {
                    Ready_Queue3.push(pcb);
                }
            } else {
                Curr_Time += time_slice;
                pcb->burst_time1 -= time_slice;
                time_slice = 0;
                Ready_Queue3.push(pcb);
            }
        } else {
            if (pcb->burst_time2 <= time_slice) {
                Curr_Time += pcb->burst_time2;
                time_slice -= pcb->burst_time2;
                pcb->burst_time2 = 0;
                terminate(pcb, Curr_Time);
            } else {
                Curr_Time += time_slice;
                pcb->burst_time2 -= time_slice;
                time_slice = 0;
                Ready_Queue3.push(pcb);
            }
        }
    }
    return Curr_Time;
}