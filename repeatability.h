#include "Log.h"

void vectorRandomFiller(std::vector<double>& vec) {
    random_device rnd_device;
    mt19937_64 mersenne_engine{ rnd_device() };
    uniform_real_distribution<double> dist{ DBL_MIN, DBL_MAX };
    auto gen = [&dist, &mersenne_engine](){
        return dist(mersenne_engine);
    };
    generate(begin(vec), end(vec), gen);
}

double calculateY(vector<double>& Ai, vector<double>& Bi,
               vector<double>& Xi, vector<double>& Yi)
{
    for (int i = 0; i < Ai.size(); i++)
    {
        Yi.push_back(Ai.at(i) * sin(Xi.at(i)) - Bi.at(i) * cos(Xi.at(i)));
    }
    return Yi.size() > 0 ? Yi.back() : 0.0;
}

class MyFuncQPCMeter : public Meter {
    double sum = 0.0;
    vector<double> Ai;
    vector<double> Bi;
    vector<double> Xi;
    vector<double> Yi;
public:
    MyFuncQPCMeter(int size, vector<double> Ai, vector<double> Bi, vector<double> Xi) : Meter(size)
    {
        this->Ai = Ai;
        this->Bi = Bi;
        this->Xi = Xi;
        this->Yi = vector<double>(size);
    }

    double meter() {
        LARGE_INTEGER t_start, t_finish, frequency;

        QueryPerformanceFrequency(&frequency);

        sum = calculateY(Ai, Bi, Xi, Yi);

        QueryPerformanceCounter(&t_start);
            sum = calculateY(Ai, Bi, Xi, Yi);
        QueryPerformanceCounter(&t_finish);

        Yi.clear();

        return double(t_finish.QuadPart - t_start.QuadPart) / double(frequency.QuadPart);
    }
};

class MyFuncClockMeter : public Meter {
    double sum = 0.0;
    vector<double> Ai;
    vector<double> Bi;
    vector<double> Xi;
    vector<double> Yi;
public:
    MyFuncClockMeter(int size, vector<double> Ai, vector<double> Bi, vector<double> Xi) : Meter(size)
    {
        this->Ai = Ai;
        this->Bi = Bi;
        this->Xi = Xi;
        this->Yi = vector<double>(size);
    }

    double meter() {
        double t_start, t_finish;

        sum = calculateY(Ai, Bi, Xi, Yi);

        t_start = clock();
            sum = calculateY(Ai, Bi, Xi, Yi);
        t_finish = clock();

        Yi.clear();

        return (t_finish - t_start) / CLOCKS_PER_SEC;
    }
};

class MyFuncRDTSCWithClockMeter : public Meter {
    double sum = 0.0;
    vector<double> Ai;
    vector<double> Bi;
    vector<double> Xi;
    vector<double> Yi;
public:
    MyFuncRDTSCWithClockMeter(int size, vector<double> Ai, vector<double> Bi, vector<double> Xi) : Meter(size)
    {
        this->Ai = Ai;
        this->Bi = Bi;
        this->Xi = Xi;
        this->Yi = vector<double>(size);
    }

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

        sum = calculateY(Ai, Bi, Xi, Yi);

        __cpuid(cpuid, 0);
        t_start = __rdtsc();
            sum = calculateY(Ai, Bi, Xi, Yi);
        __cpuid(cpuid, 0);
        t_finish = __rdtsc();

        Yi.clear();

        return (t_finish - t_start) / frequency;
    }
};

void performRepeatabilityMeasurements() {
    Log log;
    int size = 100000;
    int nsize = 25;
    int nPasses = 3;

    vector<double> Ai = vector<double>(size);
    vector<double> Bi = vector<double>(size);
    vector<double> Xi = vector<double>(size);

    vectorRandomFiller(Ai);
    vectorRandomFiller(Bi);
    vectorRandomFiller(Xi);

    cout << "\nОценка повторяемости QPC\n";
    for (int n = 1; n <= nPasses; n++) {
        cout << "\nСерия " << n << ". ";
        log.series(true, nsize, new MyFuncQPCMeter(size, Ai, Bi, Xi))
                .calc().stat(MCS_IN_SEC, "микросекунды")
                .print(MCS_IN_SEC, 10);
    }

    cout << "\nОценка повторяемости CLOCK\n";
    for (int n = 1; n <= nPasses; n++) {
        cout << "\nСерия " << n << ". ";
        log.series(true, nsize, new MyFuncClockMeter(size, Ai, Bi, Xi))
                .calc().stat(MCS_IN_SEC, "микросекунды")
                .print(MCS_IN_SEC, 10);
    }

    cout << "\nОценка повторяемости RDTSC+CLOCK\n";
    for (int n = 1; n <= nPasses; n++) {
        cout << "\nСерия " << n << ". ";
        log.series(true, nsize, new MyFuncRDTSCWithClockMeter(size, Ai, Bi, Xi))
                .calc().stat(MCS_IN_SEC, "микросекунды")
                .print(MCS_IN_SEC, 10);
    }
}
