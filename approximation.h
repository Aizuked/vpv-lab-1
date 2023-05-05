
class MyFuncQPCMeterApprox : public Meter {
    double sum = 0.0;
    vector<double> Ai;
    vector<double> Bi;
    vector<double> Xi;
    vector<double> Yi;
public:
    MyFuncQPCMeterApprox(int size, vector<double> Ai, vector<double> Bi, vector<double> Xi) : Meter(size)
    {
        this->Ai = Ai;
        this->Bi = Bi;
        this->Xi = Xi;
        this->Yi = vector<double>(size);
    }

    double meter(int step) {
        if(Ai.size()>=10){
            Ai.resize(Ai.size()-step);
        }
        if(Bi.size()>=10){
            Bi.resize(Bi.size()-step);
        }
        if(Ai.size()>=10){
            Xi.resize(Xi.size()-step);
        }
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

class MyFuncClockMeterApprox : public Meter {
    double sum = 0.0;
    vector<double> Ai;
    vector<double> Bi;
    vector<double> Xi;
    vector<double> Yi;
public:
    MyFuncClockMeterApprox(int size, vector<double> Ai, vector<double> Bi, vector<double> Xi) : Meter(size)
    {
        this->Ai = Ai;
        this->Bi = Bi;
        this->Xi = Xi;
        this->Yi = vector<double>(size);
    }

    double meter(int step) {
        if(Ai.size()>=10){
            Ai.resize(Ai.size()-step);
        }
        if(Bi.size()>=10){
            Bi.resize(Bi.size()-step);
        }
        if(Ai.size()>=10){
            Xi.resize(Xi.size()-step);
        }

        double t_start, t_finish;

        sum = calculateY(Ai, Bi, Xi, Yi);

        t_start = clock();
            sum = calculateY(Ai, Bi, Xi, Yi);
        t_finish = clock();

        Yi.clear();

        return (t_finish - t_start) / CLOCKS_PER_SEC;
    }
};

class MyFuncRDTSCWithClockMeterApprox : public Meter {
    double sum = 0.0;
    vector<double> Ai;
    vector<double> Bi;
    vector<double> Xi;
    vector<double> Yi;
public:
    MyFuncRDTSCWithClockMeterApprox(int size, vector<double> Ai, vector<double> Bi, vector<double> Xi) : Meter(size)
    {
        this->Ai = Ai;
        this->Bi = Bi;
        this->Xi = Xi;
        this->Yi = vector<double>(size);
    }

    double meter(int step) {
        if(Ai.size()>=10){
            Ai.resize(Ai.size()-step);
        }
        if(Bi.size()>=10){
            Bi.resize(Bi.size()-step);
        }
        if(Ai.size()>=10){
            Xi.resize(Xi.size()-step);
        }
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

void performApproximationMeasurements() {
    Log log;
    int size = 2500;
    int nPasses = 3;
    double kQPC, kClock, kRDTSCWithClock;

    vector<double> Ai = vector<double>(size);
    vector<double> Bi = vector<double>(size);
    vector<double> Xi = vector<double>(size);

    vectorRandomFiller(Ai);
    vectorRandomFiller(Bi);
    vectorRandomFiller(Xi);

    kQPC = MyFuncQPCMeterApprox(size, Ai, Bi, Xi).meter(0) / size;
    kClock = MyFuncClockMeterApprox(size, Ai, Bi, Xi).meter(0) / size;
    kRDTSCWithClock = MyFuncRDTSCWithClockMeterApprox(size, Ai, Bi, Xi).meter(0) / size;

    cout << "\nОтклонения от линейности QPC\n";
    for (int n = 1; n <= nPasses; n++) {
        cout << "\nСерия " << n << ". ";
        log.series(true, 100, size / 250, new MyFuncQPCMeterApprox(size, Ai, Bi, Xi))
                .calc(kQPC, size).stat(MCS_IN_SEC, "микросекунды", true)
                .print(MCS_IN_SEC, 10);
    }

    cout << "\nОтклонения от линейности CLOCK\n";
    for (int n = 1; n <= nPasses; n++) {
        cout << "\nСерия " << n << ". ";
        log.series(true, 100, size / 250, new MyFuncClockMeterApprox(size, Ai, Bi, Xi))
                .calc(kClock, size).stat(MCS_IN_SEC, "микросекунды", true)
                .print(MCS_IN_SEC, 10);
    }

    cout << "\nОтклонения от линейности RDTSC+CLOCK\n";
    for (int n = 1; n <= nPasses; n++) {
        cout << "\nСерия " << n << ". ";
        log.series(true, 100, size / 250, new MyFuncRDTSCWithClockMeterApprox(size, Ai, Bi, Xi))
                .calc(kRDTSCWithClock, size).stat(MCS_IN_SEC, "микросекунды", true)
                .print(MCS_IN_SEC, 10);
    }
}
