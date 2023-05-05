// MeasuringTime.cpp :  Учебный пример организации измерения времени
// через вставку счетчиков времени в код
// nvnulstu@gmail.com: февраль 2023

#include <iostream>
#include <Windows.h>
#include "Log.h"
#include "measurementResolutions.h"
#include "repeatability.h"
#include "approximation.h"
#include "fibonacci.h"

using namespace std;

#define Repeat10(x) x x x x x x x x x x
#define Repeat100(x) Repeat10(Repeat10(x))
#define Repeat1000(x) Repeat100(Repeat10(x))
#define Repeat10000(x) Repeat1000(Repeat10(x))

typedef void(*pFu)(void);

void fexit() { exit(0); }

int main()
{
    system("chcp 65001");
    cpuInfo();
    string func;
    vector <pFu> funcs {fexit, countFibonacci, performResolutionMeasurements, performRepeatabilityMeasurements, performApproximationMeasurements };
    do {
        cout << "\n0-выход 1-фибоначчи 2-разрешающая способность 3-повторяемость 4-линейность : ";
        cin >> func;
        char cmd = func[0] - '0';

        if (cmd >= 0 && cmd <= 4)
        {
            funcs[cmd]();
        }
    } while (true);

}
