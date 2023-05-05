#include <random>
#include "Log.h"

void vectorRandomFiller(std::vector<int>& vec) {
    random_device rnd_device;
    mt19937_64 mersenne_engine{ rnd_device() };
    uniform_int_distribution<int> dist{ INT_MIN, INT_MAX };
    auto gen = [&dist, &mersenne_engine](){
        return dist(mersenne_engine);
    };
    generate(begin(vec), end(vec), gen);
}

int sumArray(vector<int> arr, int n) {
    int sum = 0;
    for (int i = 0; i < n; i++)
    {
        sum += arr[i];
    }
    return sum;
}

class QPCResolutionMeter : public Meter {
    double sum = 0.0;
    vector<int> arr;
public:
    QPCResolutionMeter(int size, vector<int> arr) : Meter(size) { this->arr = arr; }
    double meter()
    {
        int load = 0, lastLoaded = 0;
        LARGE_INTEGER frequency, t_start, t_finish;
        bool first = false, second = false;

        QueryPerformanceFrequency(&frequency);

        sumArray(arr, load);

        do
        {
            QueryPerformanceCounter(&t_start);
                sum += sumArray(arr, load);
            QueryPerformanceCounter(&t_finish);

            bool isLoaded = (t_finish.QuadPart - t_start.QuadPart) > 0;

            if (isLoaded && first && (lastLoaded + 1 == load))
                second = true;
            if (isLoaded)
            {
                first = true;
                lastLoaded = load;
            }
            if (!isLoaded)
                first = false;

            load++;
        }
        while (!(first && second));

        return double(t_finish.QuadPart - t_start.QuadPart) / double(frequency.QuadPart);
    }
};

class ClockResolutionMeter : public Meter {
    double sum = 0.0;
    vector<int> arr;
public:
    ClockResolutionMeter(int size, vector<int> arr) : Meter(size) { this->arr = arr; }

    double meter()
    {
        int load = 0, lastLoaded = 0;
        double t_start, t_finish;
        bool first = false, second = false;

        sumArray(arr, load);

        do
        {
            t_start = (double) clock();
                sum += sumArray(arr, load);
            t_finish = (double) clock();

            bool isLoaded = (t_finish - t_start) > 0;

            if (isLoaded && first && (lastLoaded + 1 == load))
                second = true;
            if (isLoaded)
            {
                first = true;
                lastLoaded = load;
            }
            if (!isLoaded)
                first = false;

            load++;
        }
        while (!(first && second));

        return (t_finish - t_start) / CLOCKS_PER_SEC;
    }
};

class RDTSCWithClockResolutionMeter : public Meter {
    double sum = 0.0;
public:
    RDTSCWithClockResolutionMeter(int size) : Meter(size) {};
    double meter()
    {
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

        __cpuid(cpuid, 0);
        t_start = __rdtsc();
        __cpuid(cpuid, 0);
        t_finish = __rdtsc();

        return (t_finish - t_start) / frequency;
    }
};

void performResolutionMeasurements() {
    vector<int> arr = vector<int>(1000000);
    vectorRandomFiller(arr);

    Log log;
    int size = 100;
    int nPasses = 3;

    cout << "\nОценка разрешающей способности QPC\n";
    for (int n = 1; n <= nPasses; n++) {
        cout << "\nСерия " << n << ". ";
        log.series(true, size, new QPCResolutionMeter(size, arr))
                .calc().stat(MCS_IN_SEC, "микросекунды")
                .print(MCS_IN_SEC, 10);
    }

    cout << "\nОценка разрешающей способности CLOCK\n";
    for (int n = 1; n <= nPasses; n++) {
        cout << "\nСерия " << n << ". ";
        log.series(true, size, new ClockResolutionMeter(size, arr))
                .calc().stat(MCS_IN_SEC, "микросекунды")
                .print(MCS_IN_SEC, 10);
    }

    cout << "\nОценка разрешающей способности TSC\n";
    for (int n = 1; n <= nPasses; n++) {
        cout << "\nСерия " << n << ". ";
        log.series(true, size, new RDTSCWithClockResolutionMeter(size))
                .calc().stat(NS_IN_SEC, "наносекунды")
                .print(NS_IN_SEC, 10);
    }
}
