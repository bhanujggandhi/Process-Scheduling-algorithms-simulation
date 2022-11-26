#include <algorithm>
#include <iostream>
#include <queue>
#include <unordered_map>
#include <vector>
using namespace std;

#define TIME_SLICE_FORE 4
#define TIME_SLICE_BACK 2

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
    lld RR;
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
        RR = 0;
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
struct CompareProcessMLQ {
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
vector<PCB *> processes, org_processes;
unordered_map<int, PCB *> process_map;
unordered_map<unsigned int, vector<bool>> Process_Status;  // vector(0)->bt1,vector(0)->IO,vector(0)->bt2
unordered_map<int, PCB *> MLQRunning_map;
unordered_map<unsigned int, lld> IO_start_time;

priority_queue<PCB *, vector<PCB *>, CompareProcess_RR> MLQ_foreground_Ready_queue;
priority_queue<PCB *, vector<PCB *>, CompareProcessMLQ> MLQ_Background_Ready_queue;
queue<PCB *> Background_Device_queue;
queue<PCB *> foreground_Device_queue;
queue<PCB *> Start_queue;
lld Last_io_UpdationTime = 0;
bool flag = false;
lld time_slice_foreground = TIME_SLICE_FORE;
lld time_slice_background = TIME_SLICE_BACK;
lld time_slice = 2;
lld Avg_TAT = 0;
lld Avg_WT = 0;
lld Avg_RT = 0;
/* Utility Functions */
void terminate(PCB *, lld);
void adjust_Background_Device_queue(lld Curr_Time);
vector<PCB *> MLQ();
void context_switch(PCB *, States);
bool scheduleMLQ();
lld do_foreground_sched(lld);
lld do_background_sched(lld);
void getAvg();
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
    cout << "\nInput format should be :  ProcessName  ArrivalTime  BurstTime1  IOTime  BurstTime2  QueueNumber(Foreground-1,Background-2)\n  ";

    for (int i = 0; i < n; i++) {
        cin >> ProcessName;
        cin >> ArrivalTime >> BurstTime1 >> IOTime >> BurstTime2 >> Priority;
        PCB *temp = new PCB(ProcessName, ArrivalTime, BurstTime1, IOTime, BurstTime2, Priority);
        PCB *temp1 = new PCB(ProcessName, ArrivalTime, BurstTime1, IOTime, BurstTime2, Priority);
        org_processes.push_back(temp1);
        processes.push_back(temp);
    }
    MLQ();
    for (int i = 0; i < processes.size(); i++) {
        org_processes[i]->completion_time = processes[i]->completion_time;
        org_processes[i]->response_time = processes[i]->response_time;
        org_processes[i]->turnaround_time = org_processes[i]->completion_time - org_processes[i]->arrival_time;
        org_processes[i]->waiting_time = org_processes[i]->turnaround_time - (org_processes[i]->burst_time1 + org_processes[i]->burst_time2 + org_processes[i]->io_time);
    }
    cout << "pname  priority   arrival_time  burst_time1  io_time  burst_time2  completion_time  response_time  turnaround_time  waiting_time     state  " << endl;
    for (int i = 0; i < processes.size(); i++) {
        cout << org_processes[i]->pname << "\t" << org_processes[i]->priority << "\t\t"
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
void adjust_foreground_Device_queue(lld Curr_Time) {
    int len = foreground_Device_queue.size();
    int i = 0;
    while (i < len) {
        i++;
        PCB *pcb = foreground_Device_queue.front();
        // cout << "checking for : " << pcb->pid << endl;
        foreground_Device_queue.pop();
        if (pcb->io_time > (Curr_Time - IO_start_time[pcb->pid])) {
            foreground_Device_queue.push(pcb);
            continue;
        } else {
            lld tempIo_time = pcb->io_time;
            pcb->io_time = 0;
            if (pcb->burst_time2 <= 0) {
                terminate(pcb, tempIo_time + IO_start_time[pcb->pid]);
            } else {
                // cout << "pushing into ready : " << pcb->pid << endl;
                pcb->RR = tempIo_time + IO_start_time[pcb->pid];
                MLQ_foreground_Ready_queue.push(pcb);
            }
        }
    }
}
void adjust_Background_Device_queue(lld Curr_Time) {
    int len = Background_Device_queue.size();
    int i = 0;
    while (i < len) {
        i++;
        PCB *pcb = Background_Device_queue.front();
        // cout << "checking for : " << pcb->pid << endl;
        Background_Device_queue.pop();
        if (pcb->io_time > (Curr_Time - IO_start_time[pcb->pid])) {
            Background_Device_queue.push(pcb);
            continue;
        } else {
            lld tempIo_time = pcb->io_time;
            pcb->io_time = 0;
            if (pcb->burst_time2 <= 0) {
                terminate(pcb, tempIo_time + IO_start_time[pcb->pid]);
            } else {
                // cout << "pushing into ready : " << pcb->pid << endl;
                MLQ_Background_Ready_queue.push(pcb);
            }
        }
    }
}

lld getCurrTime_background() {
    lld New_Curr_Time = 0;
    int len = Background_Device_queue.size();
    int i = 0;
    PCB *pcb = Background_Device_queue.front();
    New_Curr_Time = (pcb->io_time + IO_start_time[pcb->pid]);
    while (i < len) {
        i++;
        PCB *pcb = Background_Device_queue.front();
        Background_Device_queue.pop();
        if (New_Curr_Time > (pcb->io_time + IO_start_time[pcb->pid])) {
            New_Curr_Time = (pcb->io_time + IO_start_time[pcb->pid]);
        }
        Background_Device_queue.push(pcb);
    }
    return New_Curr_Time;
}

lld getCurrTime_foreground() {
    lld New_Curr_Time = 0;
    int len = foreground_Device_queue.size();
    int i = 0;
    PCB *pcb = foreground_Device_queue.front();
    New_Curr_Time = (pcb->io_time + IO_start_time[pcb->pid]);
    while (i < len) {
        i++;
        PCB *pcb = foreground_Device_queue.front();
        foreground_Device_queue.pop();
        if (New_Curr_Time > (pcb->io_time + IO_start_time[pcb->pid])) {
            New_Curr_Time = (pcb->io_time + IO_start_time[pcb->pid]);
        }
        foreground_Device_queue.push(pcb);
    }
    return New_Curr_Time;
}

bool scheduleRR() {
    // 0 pid is for idle process
    if (MLQRunning_map.size() == 1 and MLQRunning_map.find(0) == MLQRunning_map.end()) {
        return true;
    }
    // If only idle process is present and some other process comes
    if (MLQ_foreground_Ready_queue.size() >= 1 and MLQRunning_map.size() == 1 and MLQRunning_map.find(0) != MLQRunning_map.end()) {
        auto idlep = MLQRunning_map[0];
        MLQRunning_map.erase(0);
        idlep->RR = 0;
        MLQ_foreground_Ready_queue.push(idlep);
        context_switch(idlep, READY);

        auto currprocess = MLQ_foreground_Ready_queue.top();
        MLQ_foreground_Ready_queue.pop();
        MLQRunning_map.insert({currprocess->pid, currprocess});
        context_switch(currprocess, RUNNING);
        return true;
    }
    // If running queue is empty, schedule any process in pq
    else if (MLQRunning_map.size() == 0) {
        auto currprocess = MLQ_foreground_Ready_queue.top();
        MLQ_foreground_Ready_queue.pop();
        MLQRunning_map.insert({currprocess->pid, currprocess});
        context_switch(currprocess, RUNNING);
        return true;
    }
    return false;
}

bool scheduleFcfs() {
    // 0 pid is for idle process
    if (MLQRunning_map.size() == 1 and MLQRunning_map.find(0) == MLQRunning_map.end()) {
        return true;
    }
    // If only idle process is present and some other process comes
    if (MLQ_Background_Ready_queue.size() >= 1 and MLQRunning_map.size() == 1 and MLQRunning_map.find(0) != MLQRunning_map.end()) {
        PCB *idlep = MLQRunning_map[0];
        MLQRunning_map.erase(0);
        MLQ_Background_Ready_queue.push(idlep);
        context_switch(idlep, READY);

        PCB *currprocess = MLQ_Background_Ready_queue.top();
        MLQ_Background_Ready_queue.pop();
        MLQRunning_map[currprocess->pid] = currprocess;
        context_switch(currprocess, RUNNING);
        return true;
    }
    // If running queue is empty, schedule any process in pq
    else if (MLQRunning_map.size() == 0) {
        PCB *currprocess = MLQ_Background_Ready_queue.top();
        MLQ_Background_Ready_queue.pop();
        MLQRunning_map[currprocess->pid] = currprocess;
        context_switch(currprocess, RUNNING);
        return true;
    }
    return false;
}

vector<PCB *> MLQ() {
    vector<PCB *> MLQVect;
    sort(processes.begin(), processes.end(), [](auto &a, auto &b) { return a->arrival_time < b->arrival_time; });
    lld Curr_Time = 0;
    bool flag = true;
    for (int i = 0; i < processes.size(); i++) {
        if (processes[i]->priority == 2) {
            MLQ_Background_Ready_queue.push(processes[i]);
        } else {
            Start_queue.push(processes[i]);
        }
    }
    lld ATFirst = 0;
    if (!MLQ_Background_Ready_queue.empty()) {
        PCB *pcb = MLQ_Background_Ready_queue.top();
        ATFirst = pcb->arrival_time;
    }
    if (!Start_queue.empty()) {
        PCB *pcb = Start_queue.front();
        Curr_Time = pcb->arrival_time;
        while (!Start_queue.empty()) {
            PCB *pcb = Start_queue.front();
            if (pcb->arrival_time > Curr_Time) break;
            pcb->RR = Curr_Time;
            MLQ_foreground_Ready_queue.push(pcb);
            Start_queue.pop();
        }
    } else {
        Curr_Time = ATFirst;
    }
    // if (pcb->arrival_time != 0 && Start_queue.empty()) {
    //     PCB *IdleProcess = new PCB("IIdle1", 0, pcb->arrival_time, 0, 0, -1);
    //     MLQ_Background_Ready_queue.push(IdleProcess);
    // }

    while (MLQ_Background_Ready_queue.size() > 0 && (Start_queue.size() > 0 || !foreground_Device_queue.empty() || !MLQ_foreground_Ready_queue.empty())) {
        Curr_Time = do_foreground_sched(Curr_Time);
        if (Curr_Time >= ATFirst)
            Curr_Time = do_background_sched(Curr_Time);
    }

    while (Start_queue.size() > 0 || !foreground_Device_queue.empty() || !MLQ_foreground_Ready_queue.empty()) {
        Curr_Time = do_foreground_sched(Curr_Time);
    }
    while (MLQ_Background_Ready_queue.size() > 0) {
        Curr_Time = do_background_sched(Curr_Time);
    }

    return MLQVect;
}

lld do_foreground_sched(lld Curr_Time) {
    adjust_foreground_Device_queue(Curr_Time);
    while (MLQ_foreground_Ready_queue.size() > 0 && time_slice_foreground > 0) {
        adjust_foreground_Device_queue(Curr_Time);
        PCB *pcb = MLQ_foreground_Ready_queue.top();
        // cout << "Curr_Time : " << Curr_Time << " Pid :- " << pcb->pid << " Burst Time1- " << pcb->burst_time1 << " IO- " << pcb->io_time
        //      << "  Burst Time2- " << pcb->burst_time2 << endl;
        lld prev_Curr_Time = Curr_Time;
        if (pcb->response_time == INT64_MIN) {
            pcb->response_time = Curr_Time;
        }
        bool check = scheduleRR();
        if (!check && !foreground_Device_queue.empty()) {
            Curr_Time = getCurrTime_foreground();
            adjust_foreground_Device_queue(Curr_Time);
            scheduleRR();
        }
        if (pcb->burst_time1 > 0) {
            if (pcb->arrival_time <= Curr_Time) {
                if (pcb->burst_time1 <= time_slice) {
                    Curr_Time += pcb->burst_time1;
                    time_slice_foreground -= pcb->burst_time1;
                    pcb->burst_time1 = 0;
                    if (pcb->io_time > 0) {
                        foreground_Device_queue.push(pcb);
                        IO_start_time[pcb->pid] = Curr_Time;
                    } else {
                        if (pcb->burst_time2 <= 0) {
                            terminate(pcb, Curr_Time);
                        } else {
                            pcb->RR = Curr_Time;
                            MLQ_foreground_Ready_queue.push(pcb);
                        }
                    }
                } else {
                    Curr_Time += time_slice;
                    time_slice_foreground -= time_slice;
                    pcb->burst_time1 -= time_slice;
                    pcb->RR = Curr_Time;
                    MLQ_foreground_Ready_queue.push(pcb);
                }

            } else {
                if (pcb->burst_time1 <= time_slice) {
                    if (!MLQ_Background_Ready_queue.empty()) {
                        MLQ_foreground_Ready_queue.push(pcb);
                        MLQRunning_map.erase(pcb->pid);
                        MLQRunning_map.clear();
                        break;
                    }

                    Curr_Time = pcb->arrival_time + pcb->burst_time1;
                    time_slice_foreground -= pcb->burst_time1;
                    pcb->burst_time1 = 0;
                    if (pcb->io_time > 0) {
                        foreground_Device_queue.push(pcb);
                        IO_start_time[pcb->pid] = Curr_Time;
                    } else {
                        if (pcb->burst_time2 <= 0) {
                            terminate(pcb, Curr_Time);
                        } else {
                            pcb->RR = Curr_Time;
                            MLQ_foreground_Ready_queue.push(pcb);
                        }
                    }
                } else {
                    if (!MLQ_Background_Ready_queue.empty()) {
                        MLQ_foreground_Ready_queue.push(pcb);
                        MLQRunning_map.erase(pcb->pid);
                        MLQRunning_map.clear();
                        break;
                    }
                    Curr_Time = pcb->arrival_time + time_slice;
                    time_slice_foreground -= pcb->burst_time1;
                    pcb->burst_time1 -= time_slice;
                    pcb->RR = Curr_Time;
                    MLQ_foreground_Ready_queue.push(pcb);
                }
            }
        } else {
            if (pcb->arrival_time <= Curr_Time) {
                if (pcb->burst_time2 <= time_slice) {
                    Curr_Time += pcb->burst_time2;
                    time_slice_foreground -= pcb->burst_time2;
                    pcb->burst_time2 = 0;
                    terminate(pcb, Curr_Time);
                } else {
                    Curr_Time += time_slice;
                    time_slice_foreground -= time_slice;
                    pcb->burst_time2 -= time_slice;
                    pcb->RR = Curr_Time;
                    MLQ_foreground_Ready_queue.push(pcb);
                }

            } else {
                if (pcb->burst_time2 <= time_slice) {
                    if (!MLQ_Background_Ready_queue.empty()) {
                        MLQ_foreground_Ready_queue.push(pcb);
                        MLQRunning_map.erase(pcb->pid);
                        MLQRunning_map.clear();
                        break;
                    }
                    Curr_Time = pcb->arrival_time + pcb->burst_time2;
                    time_slice_foreground -= pcb->burst_time2;
                    pcb->burst_time2 = 0;
                    terminate(pcb, Curr_Time);
                } else {
                    Curr_Time = pcb->arrival_time + time_slice;
                    pcb->burst_time2 -= time_slice;
                    pcb->RR = Curr_Time;
                    MLQ_foreground_Ready_queue.push(pcb);
                }
            }
        }

        MLQRunning_map.erase(pcb->pid);
        MLQRunning_map.clear();
        while (!Start_queue.empty()) {
            PCB *pcb = Start_queue.front();
            if (pcb->arrival_time > Curr_Time) break;
            pcb->RR = Curr_Time;
            MLQ_foreground_Ready_queue.push(pcb);
            Start_queue.pop();
        }
        if (MLQ_foreground_Ready_queue.empty() && !Start_queue.empty()) {
            PCB *pcb = Start_queue.front();
            Curr_Time = pcb->arrival_time;
            pcb->RR = Curr_Time;
            MLQ_foreground_Ready_queue.push(pcb);
            Start_queue.pop();
        }
        if (MLQ_foreground_Ready_queue.empty() && !foreground_Device_queue.empty()) {
            Curr_Time = getCurrTime_foreground();
            adjust_foreground_Device_queue(Curr_Time);
        }
    }
    time_slice_foreground = TIME_SLICE_FORE;
    return Curr_Time;
}
lld do_background_sched(lld Curr_Time) {
    // time_slice_background;
    adjust_Background_Device_queue(Curr_Time);
    while (MLQ_Background_Ready_queue.size() > 0 && time_slice_background > 0) {
        adjust_Background_Device_queue(Curr_Time);
        PCB *pcb = MLQ_Background_Ready_queue.top();
        lld prev_Curr_Time = Curr_Time;
        if (pcb->response_time == INT64_MIN) {
            pcb->response_time = Curr_Time;
        }
        bool check = scheduleFcfs();
        if (!check && !Background_Device_queue.empty()) {
            Curr_Time = getCurrTime_background();
            adjust_Background_Device_queue(Curr_Time);
            scheduleFcfs();
        }
        if (flag) {
            Curr_Time = pcb->arrival_time + pcb->burst_time1;
            pcb->burst_time1 = 0;
            if (pcb->io_time > 0) {
                Background_Device_queue.push(pcb);
                IO_start_time[pcb->pid] = Curr_Time;
            } else {
                Curr_Time += pcb->burst_time2;
                pcb->burst_time2 = 0;
                terminate(pcb, Curr_Time);
            }
            flag = false;
        } else {
            if (pcb->burst_time1 > 0) {
                if (pcb->arrival_time <= Curr_Time) {
                    if (pcb->burst_time1 > time_slice_background) {
                        pcb->burst_time1 -= time_slice_background;
                        MLQ_Background_Ready_queue.push(pcb);
                        Curr_Time += time_slice_background;
                        time_slice_background = 0;
                        MLQRunning_map.erase(pcb->pid);
                        MLQRunning_map.clear();
                        break;
                    }
                    Curr_Time += pcb->burst_time1;
                    time_slice_background -= pcb->burst_time1;
                    pcb->burst_time1 = 0;
                } else {
                    if (!MLQ_foreground_Ready_queue.empty()) {
                        // time_slice_foreground = TIME_SLICE_FORE;
                        MLQ_Background_Ready_queue.push(pcb);
                        MLQRunning_map.erase(pcb->pid);
                        MLQRunning_map.clear();
                        break;
                    }
                    Curr_Time = pcb->arrival_time + pcb->burst_time1;
                    time_slice_background -= pcb->burst_time1;
                    pcb->burst_time1 = 0;
                }
                if (pcb->burst_time1 == 0) {
                    if (pcb->io_time > 0) {
                        Background_Device_queue.push(pcb);
                        IO_start_time[pcb->pid] = Curr_Time;
                    } else {
                        if (pcb->burst_time2 > time_slice_background) {
                            pcb->burst_time2 -= time_slice_background;
                            MLQ_Background_Ready_queue.push(pcb);
                            Curr_Time += time_slice_background;
                            MLQRunning_map.erase(pcb->pid);
                            MLQRunning_map.clear();
                            break;
                        }
                        Curr_Time += pcb->burst_time2;
                        time_slice_background -= pcb->burst_time2;
                        pcb->burst_time2 = 0;
                        terminate(pcb, Curr_Time);
                    }
                }
            } else {
                if (pcb->arrival_time <= Curr_Time) {
                    if (pcb->burst_time2 > time_slice_background) {
                        pcb->burst_time2 -= time_slice_background;
                        MLQ_Background_Ready_queue.push(pcb);
                        Curr_Time += time_slice_background;
                        MLQRunning_map.erase(pcb->pid);
                        MLQRunning_map.clear();
                        break;
                    }
                    Curr_Time += pcb->burst_time2;
                    time_slice_background -= pcb->burst_time2;
                    pcb->burst_time2 = 0;
                } else {
                    if (!MLQ_foreground_Ready_queue.empty()) {
                        // time_slice_foreground = TIME_SLICE_FORE;
                        MLQ_Background_Ready_queue.push(pcb);
                        MLQRunning_map.erase(pcb->pid);
                        MLQRunning_map.clear();
                        break;
                    }
                    Curr_Time = pcb->arrival_time + pcb->burst_time2;
                    time_slice_background -= pcb->burst_time2;
                    pcb->burst_time2 = 0;
                }
                if (pcb->burst_time2 == 0)
                    terminate(pcb, Curr_Time);
            }
        }
        MLQRunning_map.erase(pcb->pid);
        MLQRunning_map.clear();
        if (MLQ_Background_Ready_queue.empty() && !Background_Device_queue.empty()) {
            Curr_Time = getCurrTime_background();
            adjust_Background_Device_queue(Curr_Time);
        }
    }
    time_slice_background = TIME_SLICE_BACK;
    return Curr_Time;
}
