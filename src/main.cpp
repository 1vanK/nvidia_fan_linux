#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <nvml.h>
#include <thread>

using namespace std;


// Флаг, что требуется остановить программу
static std::atomic<bool> stop = false;
static_assert(std::atomic<bool>::is_always_lock_free);

// Обработчик сигнала остановки программы
static void signal_handler(int)
{
    stop = true;
}

void iteration()
{
    // Число видеокарт NVIDIA
    unsigned device_count = 0;
    if (nvmlDeviceGetCount_v2(&device_count) != NVML_SUCCESS)
        throw runtime_error("nvmlDeviceGetCount_v2(...) != NVML_SUCCESS");

    // Цикл по всем видеокартам NVIDIA
    for (unsigned dev_indx = 0; dev_indx < device_count; ++dev_indx)
    {
        nvmlDevice_t device = nullptr;
        if (nvmlDeviceGetHandleByIndex(dev_indx, &device) != NVML_SUCCESS)
            continue;

        // Температура устройства
        unsigned temperature = 0;
        if (nvmlDeviceGetTemperature(device, NVML_TEMPERATURE_GPU, &temperature) != NVML_SUCCESS)
            continue;

        // Скорость вращения вентиляторов в процентах
        unsigned speed = 0;
        if (nvmlDeviceGetFanSpeed(device, &speed) != NVML_SUCCESS)
            continue;

        unsigned target_temperature = 50;
        unsigned target_speed = 30;

        if (speed == 0)
        {
            target_speed = 30; // Начальное значение
        }
        else
        {
            // Может быть отрицательной
            int temperature_delta = static_cast<int>(temperature) - static_cast<int>(target_temperature);
            cout << "temperature_delta: " << temperature_delta << endl;

            // Чем больше temperature_delta, тем больше должен быть прирост скорости
            int speed_delta = temperature_delta;
            cout << "speed_delta: " << speed_delta << endl;

            int target_speed_int = static_cast<int>(speed) + speed_delta;
            target_speed_int = std::clamp(target_speed_int, 10, 80);
            target_speed = static_cast<unsigned>(target_speed_int);
            cout << "target_speed: " << target_speed << endl;
        }

        // Число раздельно управляемых вентиляторов.
        // Может возвращать 1, если все вентиляторы управляются через один контроллер
        unsigned num_fans = 0;
        if (nvmlDeviceGetNumFans(device, &num_fans) != NVML_SUCCESS)
            continue;

        for (unsigned fan_indx = 0; fan_indx < num_fans; ++fan_indx)
        {
            // Требует повышенных привелегий
            if (nvmlDeviceSetFanSpeed_v2(device, fan_indx, target_speed) == NVML_ERROR_NO_PERMISSION)
                throw runtime_error("[ОШИБКА] Забыли sudo");
        }
    }

    // Пауза - 5 секунд
    this_thread::sleep_for(chrono::seconds(5));
}

// Восстанавливает автоматический режим управления вентиляторами, чтобы не спалить видюху
void set_default_fan_speed()
{
    // Число видеокарт NVIDIA
    unsigned device_count = 0;
    if (nvmlDeviceGetCount_v2(&device_count) != NVML_SUCCESS)
        return;

    // Цикл по всем видеокартам NVIDIA
    for (unsigned dev_indx = 0; dev_indx < device_count; ++dev_indx)
    {
        nvmlDevice_t device = nullptr;
        if (nvmlDeviceGetHandleByIndex(dev_indx, &device) != NVML_SUCCESS)
            continue;

        // Число раздельно управляемых вентиляторов.
        // Может возвращать 1, если все вентиляторы управляются через один контроллер
        unsigned num_fans = 0;
        if (nvmlDeviceGetNumFans(device, &num_fans) != NVML_SUCCESS)
            continue;

        for (unsigned fan_indx = 0; fan_indx < num_fans; ++fan_indx)
        {
            // Требует повышенных привелегий. На ошибку не реагируем
            nvmlDeviceSetDefaultFanSpeed_v2(device, fan_indx);
        }
    }
}

int main()
{
    // Устанавливаем обработчик для сигналов остановки программы
    signal(SIGTERM, signal_handler); // systemd stop
    signal(SIGINT,  signal_handler); // Ctrl-C

    int ret = EXIT_SUCCESS;

    cout << "Запущено" << endl;
    bool lib_inited = false;

    try
    {
        // Инициализируем библиотеку
        if (nvmlInit_v2() != NVML_SUCCESS)
            throw runtime_error("nvmlInit_v2() != NVML_SUCCESS");

        lib_inited = true;
        cout << "Библиотека NVML инициализирована" << endl;

        while (!stop)
            iteration();
    }
    catch (const std::exception& ex)
    {
        cerr << ex.what() << endl;
        ret = EXIT_FAILURE;
    }

    if (lib_inited)
    {
        set_default_fan_speed(); // Пытаемся, игнорируя ошибки
        nvmlShutdown();
    }
    
    cout << "Остановлено" << endl;
    
    return ret;
}
