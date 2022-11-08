#include <algorithm>
#include <iostream>
#include <queue>
#include <vector>

using namespace std;

/* Type Definition */

typedef unsigned long long ull;
typedef long long ll;
typedef long double lld;

/* Global Queues */

queue<PCB*> job_queue;
priority_queue<PCB*, vector<PCB*>, CompareProcess> ready_queue;
priority_queue<PCB*, vector<PCB*>, CompareProcess> device_queue;
queue<unsigned int> UUID;

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
    ll priority;
    lld arrival_time;
    lld burst_time;
    lld completion_time;
    lld turnaround_time;
    lld waiting_time;
    lld response_time;
    States state;

    PCB(string _pname, lld _arrival_time, lld _burst_time, lld _priority) {
        pid = !UUID.empty() ? UUID.front() : rand();
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
};

struct CompareProcess {
    bool operator()(PCB* const& p1, PCB* const& p2) {
        if (p1->priority == p2->priority) {
            if (p1->arrival_time == p2->arrival_time) {
                return p1->pid < p2->pid;
            }
            return p1->arrival_time < p2->arrival_time;
        }
        return p1->priority < p2->priority;
    }
};

/* Utility Functions */

void context_switch();
void schedule();

int main() {
    PCB* IdleProcess = new PCB("CIdle", 0, INT64_MAX, -1);
    for (int i = 0; i < 500; i++) {
        UUID.push(i);
    }

    return 0;
}