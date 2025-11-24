#include <iostream>
#include <pthread.h>
#include <unistd.h>   // usleep
#include <cstdlib>    // atoi, rand
#include <ctime>      // time
#include "semaphore_class.h"   // from the starter code

using namespace std;

// ---------- Common ----------
const int NUM_READERS = 5;
const int NUM_WRITERS = 5;
const int NUM_PHILOSOPHERS = 5;
const int REPEAT_COUNT = 3;   // how many times each thread acts

// Small helper to sleep a bit
void random_sleep() {
    usleep(100000 + rand() % 200000); // 0.1–0.3s
}

/******************************************************************
 * Problem 1: No-starve Readers–Writers (fair solution)
 * Based on Downey's fair readers-writers (turnstile).
 ******************************************************************/

Semaphore rw_mutex_p1(1);     // controls access to shared resource
Semaphore mutex_p1(1);        // protects read_count_p1
Semaphore turnstile_p1(1);    // fairness (line up readers & writers)
int read_count_p1 = 0;

void* reader_p1(void* arg) {
    long id = (long) arg;

    for (int i = 0; i < REPEAT_COUNT; ++i) {
        // Entry section
        turnstile_p1.wait();
        turnstile_p1.signal();

        mutex_p1.wait();
        read_count_p1++;
        if (read_count_p1 == 1) {
            rw_mutex_p1.wait();
        }
        mutex_p1.signal();

        // Critical section
        cout << "P1 - Reader " << id << ": reading" << endl;
        random_sleep();

        // Exit section
        mutex_p1.wait();
        read_count_p1--;
        if (read_count_p1 == 0) {
            rw_mutex_p1.signal();
        }
        mutex_p1.signal();

        cout << "P1 - Reader " << id << ": done reading" << endl;
        random_sleep();
    }
    return nullptr;
}

void* writer_p1(void* arg) {
    long id = (long) arg;

    for (int i = 0; i < REPEAT_COUNT; ++i) {
        // Entry section
        turnstile_p1.wait();
        rw_mutex_p1.wait();

        // Critical section
        cout << "P1 - Writer " << id << ": writing" << endl;
        random_sleep();
        cout << "P1 - Writer " << id << ": done writing" << endl;

        // Exit section
        rw_mutex_p1.signal();
        turnstile_p1.signal();

        random_sleep();
    }
    return nullptr;
}

void run_problem1() {
    cout << "Running Problem 1: No-starve Readers-Writers" << endl;
    pthread_t readers[NUM_READERS], writers[NUM_WRITERS];

    for (long i = 0; i < NUM_READERS; ++i)
        pthread_create(&readers[i], nullptr, reader_p1, (void*) i);
    for (long i = 0; i < NUM_WRITERS; ++i)
        pthread_create(&writers[i], nullptr, writer_p1, (void*) i);

    for (int i = 0; i < NUM_READERS; ++i)
        pthread_join(readers[i], nullptr);
    for (int i = 0; i < NUM_WRITERS; ++i)
        pthread_join(writers[i], nullptr);
}

/******************************************************************
 * Problem 2: Writer-priority Readers–Writers
 ******************************************************************/

Semaphore resource_p2(1);      // controls access to resource
Semaphore rmutex_p2(1);        // protects readcount_p2
Semaphore wmutex_p2(1);        // protects writecount_p2
Semaphore readTry_p2(1);       // lets writers block new readers
int readcount_p2 = 0;
int writecount_p2 = 0;

void* reader_p2(void* arg) {
    long id = (long) arg;

    for (int i = 0; i < REPEAT_COUNT; ++i) {
        // Entry
        readTry_p2.wait();
        rmutex_p2.wait();
        readcount_p2++;
        if (readcount_p2 == 1) {
            resource_p2.wait();  // first reader locks resource
        }
        rmutex_p2.signal();
        readTry_p2.signal();

        // Critical section
        cout << "P2 - Reader " << id << ": reading" << endl;
        random_sleep();

        // Exit
        rmutex_p2.wait();
        readcount_p2--;
        if (readcount_p2 == 0) {
            resource_p2.signal();   // last reader releases resource
        }
        rmutex_p2.signal();

        cout << "P2 - Reader " << id << ": done reading" << endl;
        random_sleep();
    }
    return nullptr;
}

void* writer_p2(void* arg) {
    long id = (long) arg;

    for (int i = 0; i < REPEAT_COUNT; ++i) {
        // Entry
        wmutex_p2.wait();
        writecount_p2++;
        if (writecount_p2 == 1) {
            readTry_p2.wait();    // first writer blocks new readers
        }
        wmutex_p2.signal();

        resource_p2.wait();

        // Critical section
        cout << "P2 - Writer " << id << ": writing" << endl;
        random_sleep();
        cout << "P2 - Writer " << id << ": done writing" << endl;

        resource_p2.signal();

        // Exit
        wmutex_p2.wait();
        writecount_p2--;
        if (writecount_p2 == 0) {
            readTry_p2.signal();  // last writer allows readers again
        }
        wmutex_p2.signal();

        random_sleep();
    }
    return nullptr;
}

void run_problem2() {
    cout << "Running Problem 2: Writer-priority Readers-Writers" << endl;
    pthread_t readers[NUM_READERS], writers[NUM_WRITERS];

    for (long i = 0; i < NUM_READERS; ++i)
        pthread_create(&readers[i], nullptr, reader_p2, (void*) i);
    for (long i = 0; i < NUM_WRITERS; ++i)
        pthread_create(&writers[i], nullptr, writer_p2, (void*) i);

    for (int i = 0; i < NUM_READERS; ++i)
        pthread_join(readers[i], nullptr);
    for (int i = 0; i < NUM_WRITERS; ++i)
        pthread_join(writers[i], nullptr);
}

/******************************************************************
 * Problem 3: Dining Philosophers Solution #1 (asymmetric)
 ******************************************************************/

Semaphore chopstick_p3[NUM_PHILOSOPHERS] = {
    Semaphore(1), Semaphore(1), Semaphore(1), Semaphore(1), Semaphore(1)
};

void* philosopher_p3(void* arg) {
    long id = (long) arg;
    int left = id;
    int right = (id + 1) % NUM_PHILOSOPHERS;

    for (int i = 0; i < REPEAT_COUNT; ++i) {
        cout << "P3 - Philosopher " << id << ": thinking" << endl;
        random_sleep();

        if (id % 2 == 0) {
            // even: left then right
            chopstick_p3[left].wait();
            chopstick_p3[right].wait();
        } else {
            // odd: right then left
            chopstick_p3[right].wait();
            chopstick_p3[left].wait();
        }

        cout << "P3 - Philosopher " << id << ": eating" << endl;
        random_sleep();
        cout << "P3 - Philosopher " << id << ": done eating" << endl;

        chopstick_p3[left].signal();
        chopstick_p3[right].signal();
    }
    return nullptr;
}

void run_problem3() {
    cout << "Running Problem 3: Dining Philosophers Solution #1 (Asymmetric)" << endl;
    pthread_t phils[NUM_PHILOSOPHERS];

    for (long i = 0; i < NUM_PHILOSOPHERS; ++i)
        pthread_create(&phils[i], nullptr, philosopher_p3, (void*) i);

    for (int i = 0; i < NUM_PHILOSOPHERS; ++i)
        pthread_join(phils[i], nullptr);
}

/******************************************************************
 * Problem 4: Dining Philosophers Solution #2 (room = N-1)
 ******************************************************************/

Semaphore chopstick_p4[NUM_PHILOSOPHERS] = {
    Semaphore(1), Semaphore(1), Semaphore(1), Semaphore(1), Semaphore(1)
};
Semaphore room_p4(NUM_PHILOSOPHERS - 1);   // allow at most 4 at table

void* philosopher_p4(void* arg) {
    long id = (long) arg;
    int left = id;
    int right = (id + 1) % NUM_PHILOSOPHERS;

    for (int i = 0; i < REPEAT_COUNT; ++i) {
        cout << "P4 - Philosopher " << id << ": thinking" << endl;
        random_sleep();

        room_p4.wait();             // enter the room (max N-1)
        chopstick_p4[left].wait();
        chopstick_p4[right].wait();

        cout << "P4 - Philosopher " << id << ": eating" << endl;
        random_sleep();
        cout << "P4 - Philosopher " << id << ": done eating" << endl;

        chopstick_p4[left].signal();
        chopstick_p4[right].signal();
        room_p4.signal();           // leave the room
    }
    return nullptr;
}

void run_problem4() {
    cout << "Running Problem 4: Dining Philosophers Solution #2 (Room limit)" << endl;
    pthread_t phils[NUM_PHILOSOPHERS];

    for (long i = 0; i < NUM_PHILOSOPHERS; ++i)
        pthread_create(&phils[i], nullptr, philosopher_p4, (void*) i);

    for (int i = 0; i < NUM_PHILOSOPHERS; ++i)
        pthread_join(phils[i], nullptr);
}

/******************************************************************
 * main: choose which problem to run
 ******************************************************************/

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <problem# 1-4>" << endl;
        return 1;
    }

    int problem = atoi(argv[1]);
    srand(time(nullptr));

    switch (problem) {
        case 1: run_problem1(); break;
        case 2: run_problem2(); break;
        case 3: run_problem3(); break;
        case 4: run_problem4(); break;
        default:
            cerr << "Invalid problem number. Use 1, 2, 3, or 4." << endl;
            return 1;
    }

    return 0;
}
