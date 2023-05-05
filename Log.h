#pragma once

#include <iostream>
#include <vector>
#include <sstream>
#include <numeric>
#include <cmath>
#include <intrin.h>
#include <algorithm>
#include <thread>
#include <omp.h>
#include <iomanip>
#include <map>
#include <string>
#include <sysinfoapi.h>
#include <winnt.h>

using namespace std;

#define MCS_IN_SEC 1.E6 // число микросекунд в секунде
#define NS_IN_SEC 1.E9 // число наносекунд в секунде
#define PS_IN_SEC 1.E12 // число пикосекунд в секунде

extern double sum;

// Базовый класс для порождения объектов, с которыми проводятся измерения времени
class Meter {
public:
    int size; // размер рабочей нагрузки
    // переменная для обмана оптимизатора через иллюзию, что результат
    // функции, для которой замеряется время, где-то используется
    double sum = 0.0;

    Meter(int size) : size(size) {}

    // функция для поддержки исследования с изменяемой рабочей нагрузкой
    void setSize(int s) {
        size = s;
    }

    int getSize() {
        return size;
    }

    // Функция измерения, возвращающая double,
    // которое чаще всего время в секундах, хотя может быть и числом тактов
    virtual double meter() { return 0.0; }
    virtual double meter(int step) { return 0.0; }
};

// Код упорядочевания: естественный, вначале минимальные, вначале максимальные
enum {
    O_NATURE, O_MIN, O_MAX
};
/* Параметры конфигурирования формата распечатки и фильтрации
   PREC_VAL - число знаков после запятой в распечатки сохраняемых в протоколе значений
   PREC_AVG - число знаков после запятой в распечатке
   FILTR_MIN, FILTR_MAX - число отбрасываемых наименьших и наибольших перед подсчетом среднего и СКО
   SQDEV_BAD, MAX_BAD, MIN_BAD - граничные значения отклонений СКО%, dMax% и dMin%,
      превышение которых активизирует вывод через log.print(scale, len)
      ряда результатов измерений
*/
enum Config {
    CONFIG_FIRST = 0, PREC_VAL = 0, PREC_AVG, FILTR_MIN, FILTR_MAX,
    SQDEV_BAD, MAX_BAD, MIN_BAD, CONFIG_SIZE
};

// Получение информации о процессоре через команду функцию __cpuid(unsigned *) info, funcCode),
// которая выполняет машинную  команду CPUID, предварительно загружая в EAX код функции funcCode.
// CPUID возвращает 4 четырехбайтных кода: 0:EAX, 1:EBX, 2:ECX, 3:EDX,
// а функция __cpuid перемещает эти слова по адресу info
void cpuInfo() {
    // int regs[4]; // 0:EAX, 1:EBX, 2:ECX, 3:EDX
    char nameCPU[80]; // имя процессора
    // Получение имени процессора
    // Коды 0x80000002..0x80000004 позволяют получить полное имя CPU по 16 байтов
    __cpuid((int *) nameCPU, 0x80000002);
    __cpuid((int *) nameCPU + 4, 0x80000003);
    __cpuid((int *) nameCPU + 8, 0x80000004);
    int cores = thread::hardware_concurrency();

    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);

    int numCPU = sysinfo.dwNumberOfProcessors;
    cout << endl << nameCPU << "\tПотоков: " << cores
         << "  Ядер: " << omp_get_num_procs() << "  numCPU: " << numCPU << endl << endl;
}

// Функция поддержки упорядочения по убыванию
bool greater_then(double a, double b) {
    return a > b;
}

class Log {
public:
    double k = 0.;
    double dmin = 1.0E10, dmax = 0.; // минимальный и максимальный элемент
    double avg = 0., sqdev = 0., ksqdev = 0., sqdevPerc, ksqdevPerc; // среднее, СКО, СКО%
    double dMinPerc, dMaxPerc; // минимум и максимум отклонения от среднего в процентах
    vector<double> arr; // результаты измерений
    int conf[CONFIG_SIZE] = {0, 0, 0, 0, 3, 3, 5};
    ostringstream msgfiltr; // для динамического формирования кусков текста распечаток
    Log() {}

    void setK(double k) {
        this->k = k;
    }

    // Обработка результатов измерения
    Log &calc(double kToSet = 0, int size = 0) { //
        if (kToSet != 0)
            setK(kToSet);
        vector<double> vect = arr;
        msgfiltr.str("");
        if (conf[FILTR_MIN] > 0) { // удаление наименьших
            sort(vect.begin(), vect.end());
            vect.erase(vect.begin(), vect.begin() + conf[FILTR_MIN]);
            msgfiltr << "\nУдалено " << conf[FILTR_MIN] << " наименьших";
        }
        if (conf[FILTR_MAX] > 0) { // удаление наибольших
            sort(vect.begin(), vect.end(), greater_then);
            vect.erase(vect.begin(), vect.begin() + conf[FILTR_MAX]);
            msgfiltr << "  Удалено " << conf[FILTR_MAX] << " наибольших";
        }
        // Подсчет среднего, СКО, СКО%, min и max, а также их относительных отклонений от среднего
        avg = accumulate(vect.begin(), vect.end(), 0.0, [&](double x, double y) { return x + y / vect.size(); });
        sqdev = sqrt(accumulate(vect.begin(), vect.end(), 0.,
                                [&](double sq, double v) {
                                    double q = avg - v;
                                    return sq + q * q;
                                })
                     / vect.size());

        ksqdev = sqrt(accumulate(vect.begin(), vect.end(), 0.,
                                [&](double sq, double v) {
                                    double q = (k * vect.size()) - v;
                                    return sq + q * q;
                                })
                     / vect.size());

        double absAvg = abs(avg);
        sqdevPerc = 100 * sqdev / absAvg;

        //не вект сайз а текущий сайз
        ksqdevPerc = (100 * ksqdev) / (k * size);
        dmin = *min_element(vect.begin(), vect.end());
        dmax = *max_element(vect.begin(), vect.end());
        dMinPerc = 100 * (avg - dmin) / absAvg;
        dMaxPerc = 100 * (dmax - avg) / absAvg;
        return *this;
    }

    // Перенос группы результатов source в общий массив результатов arr
    Log &set(double val) {
        arr.push_back(val);
        return *this;
    }

    // Перенос группы результатов source в общий массив результатов arr
    Log &setGroup(vector<double> source) {
        for (double v: source)
            arr.push_back(v);
        return *this;
    }

    // Конфигурирование организуем через передачу пар
    // { индекс_параметра, значение_параметра },
    // чтобы не конфигурировать параметры независимо друг от друга
    Log &config(map<int, int> param) {
        for (auto it = param.begin(); it != param.end(); it++) {
            int n = (*it).first;
            // ЕСЛИ индекс параметра валиден
            if (n >= CONFIG_FIRST && n < CONFIG_SIZE)
                conf[n] = (*it).second; // ТО фиксация параметра
        }
        return *this; // Поддержка цепочки вызова log.config().еще_метод
    }

    // Очистка массива результатов измерения
    Log &clear() {
        arr.clear();
        return *this;
    }

    // Организация серии из count измерений средствами объекта m, в котором
    // функция meter делает однократное измерение.
    // При init == true массив очищается, иначе результаты серии дописываются
    // к ранее полученным результатам
    Log &series(bool init, int count, Meter *m) {
        if (init)
            arr.clear();
        for (int n = 0; n < count; n++) {
            arr.push_back(m->meter());
        }
        return *this;
    }

    Log &series(bool init, int count, int step, Meter *m) {
        if (init)
            arr.clear();
        for (int n = 0; n < count; n++) {
            arr.push_back(m->meter(step));
        }
        return *this;
    }

    // Распечатка описательной статистики результатов измерений
    Log &stat(double scale, string unit) {
        cout.setf(ios::fixed);
        cout << arr.size() << " измерений: " << unit << msgfiltr.str() << endl
             << setprecision(conf[PREC_VAL]) << "Min: " << scale * dmin << " Max: " << scale * dmax
             << setprecision(conf[PREC_AVG]) << " Avg: " << scale * avg
             << " СКО: " << scale * sqdev << " СКО%: " << sqdevPerc
             << " dMax%: " << dMaxPerc << " dMin%: " << dMinPerc << endl;
        return *this;
    }

    // Модификация для 5 пункта
    Log &stat(double scale, string unit, boolean doK) {
        if (doK) {
            cout.setf(ios::fixed);
            cout << arr.size() << " измерений: " << unit << msgfiltr.str() << endl
                 << setprecision(conf[PREC_VAL]) << "Min: " << scale * dmin << " Max: " << scale * dmax
                 << setprecision(conf[PREC_AVG]) << " Avg: " << scale * avg
                 << " СКО: " << scale * ksqdev << " СКО%: " << ksqdevPerc << endl;
            return *this;
        } else {
            cout << "failed!" << endl;
        }
    }

    // Целочисленный вывод с масштабом scale первых len в одном из трех порядков:
    // O_NORM - естественный, O_MIN - минимальные, O_MAX - максимальные
    Log &print(int ord, double scale, unsigned len) {
        vector<double> vect = arr;
        string head;
        int nsetw = int(ceil(log10(dmax * scale))) + conf[PREC_VAL] + 2;
        switch (ord) {
            case O_NATURE:
                head = "Первые:     ";
                break;
            case O_MIN:
                sort(vect.begin(), vect.end());
                head = "Наименьшие: ";
                break;
            case O_MAX:
                sort(vect.begin(), vect.end(), greater_then);
                head = "Наибольшие: ";
                break;
        }
        cout.setf(ios::fixed);
        cout << head << setprecision(conf[PREC_VAL]);
        for (unsigned n = 0; n < min(len, unsigned(vect.size())); n++)
            cout << setw(nsetw) << scale * vect[n];
        cout << endl;
        return *this;
    }

    // Вывод, состав которого формируется путем сопоставления СКО%, dMin% и dMax%
    // с "плохими" значениями из conf
    // O_NORM - естественный, O_MIN - минимальные, O_MAX - максимальные
    Log &print(double scale, unsigned len) {
        if (sqdevPerc > conf[SQDEV_BAD]) {
            print(O_NATURE, scale, len);
            print(O_MIN, scale, len);
            print(O_MAX, scale, len);
            return *this;
        }
        if (dMinPerc > conf[MIN_BAD])
            print(O_MIN, scale, len);
        if (dMaxPerc > conf[MAX_BAD])
            print(O_MAX, scale, len);

        vector<double> vect = arr;
        int nsetw = int(ceil(log10(dmax * scale))) + conf[PREC_VAL] + 2;
    }
};