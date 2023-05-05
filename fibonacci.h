#include <profileapi.h>
#include "Log.h"

int countFib(int a) {
    if (a < 2)
        return a;
    return countFib(a - 1) + countFib(a - 2);
}

class QPCFibonacciMeter : public Meter {
    double sum = 0.0;
public:
    QPCFibonacciMeter(int size) : Meter(size) { }
    double meter() {
        LARGE_INTEGER t_start, t_finish, frequency;

        QueryPerformanceFrequency(&frequency);

        sum = countFib(size);

        QueryPerformanceCounter(&t_start);
            sum = countFib(size);
        QueryPerformanceCounter(&t_finish);

        return double(t_finish.QuadPart - t_start.QuadPart) / double(frequency.QuadPart);
    }
};

class ClockFibonacciMeter : public Meter {
    double sum = 0.0;
public:
    ClockFibonacciMeter(int size) : Meter(size) { }
    double meter() {
        double t_start, t_finish;

        sum = countFib(size);

        t_start = clock();
            sum = countFib(size);
        t_finish = clock();

        return (t_finish - t_start) / CLOCKS_PER_SEC;
    }
};

class RDTSCWithClockFibonacciMeter : public Meter {
    double sum = 0.0;
public:
    RDTSCWithClockFibonacciMeter(int size) : Meter(size) { }
    double meter() {
        uint64_t t_start, t_finish;
        double t_clock_start, frequency;
        int cpuid[4];

        t_clock_start = clock();
        while (clock() < t_clock_start + 1);
        t_start = __rdtsc();
        while (clock() < t_clock_start + 2);
        t_finish = __rdtsc();

        t_finish -= t_start;

        frequency = t_finish * CLOCKS_PER_SEC;

        sum = countFib(size);

        __cpuid(cpuid, 0);
        t_start = __rdtsc();
            sum = countFib(size);
        __cpuid(cpuid, 0);
        t_finish = __rdtsc();

        return (t_finish - t_start) / frequency;
    }
};

void printResults(Log& log, int size, int nPasses) {
    cout << "\nОценка Фибоначчи(" << size << ") debug/release QPC\n";
    for (int n = 1; n <= nPasses; n++) {
        cout << "\nСерия " << n << ". ";
        log.series(true, size, new QPCFibonacciMeter(size))
                .calc().stat(MCS_IN_SEC, "микросекунды")
                .print(MCS_IN_SEC, 10);
    }

    cout << "\nОценка Фибоначчи(" << size << ") debug/release CLOCK\n";
    for (int n = 1; n <= nPasses; n++) {
        cout << "\nСерия " << n << ". ";
        log.series(true, size, new ClockFibonacciMeter(size))
                .calc().stat(MCS_IN_SEC, "микросекунды")
                .print(MCS_IN_SEC, 10);
    }

    cout << "\nОценка Фибоначчи(" << size << ") debug/release RDTSC+CLOCK\n";
    for (int n = 1; n <= nPasses; n++) {
        cout << "\nСерия " << n << ". ";
        log.series(true, size, new RDTSCWithClockFibonacciMeter(size))
                .calc().stat(MCS_IN_SEC, "микросекунды")
                .print(MCS_IN_SEC, 10);
    }
}

void countFibonacci() {
    Log log;
    int nPasses = 3;

    printResults(log, 20, nPasses);

    cout << endl << endl;

    printResults(log, 30, nPasses);
}

