#include <algorithm>
#include <iostream>
#include <queue>
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
    lld burst_time;
    lld completion_time;
    lld turnaround_time;
    lld waiting_time;
    lld response_time;
    States state;

    PCB(string _pname, lld _arrival_time, lld _burst_time, lld _priority) {
        pid = getUUID();
        pname = _pname;
        ptype = _pname[0] == 'I' ? 0 : 1;
        priority = _priority;
        arrival_time = _arrival_time;
        burst_time = _burst_time;
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

struct CompareProcess {
    bool operator()(PCB* const& p1, PCB* const& p2) {
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

queue<PCB*> job_queue;
priority_queue<PCB*, vector<PCB*>, CompareProcess> device_queue;

/* Utility Functions */

void context_switch();
void schedule();

int main() {
    priority_queue<PCB*, vector<PCB*>, CompareProcess> ready_queue;
    for (int i = 0; i < 100; i++) {
        UUID.push(i);
    }
    PCB* IdleProcess1 = new PCB("IIdle1", 0, INT64_MAX, 0);

    return 0;
}