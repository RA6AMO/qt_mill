// Защита от повторного включения заголовочного файла
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

// Подключаем базовый класс для главного окна
#include <QMainWindow>

// Предварительное объявление класса SecondWindow
// Это позволяет избежать циклических зависимостей заголовочных файлов
// Полное определение класса подключим в .cpp файле
class SecondWindow;

// Макросы Qt для работы с UI-классами
QT_BEGIN_NAMESPACE
namespace Ui {
    // Класс MainWindow из UI-файла (автогенерируется)
    class MainWindow;
}
QT_END_NAMESPACE

// Основной класс первого окна приложения
class MainWindow : public QMainWindow {
    // Макрос для поддержки механизма сигналов и слотов Qt
    Q_OBJECT

public:
    // Конструктор с опциональным родительским виджетом
    // explicit предотвращает неявные преобразования типов
    explicit MainWindow(QWidget* parent = nullptr);

    // Виртуальный деструктор для корректного удаления при наследовании
    ~MainWindow() override;

private slots:
    // Слоты - специальные методы для обработки сигналов

    // Вызывается при нажатии кнопки перехода ко второму окну
    void onSwitchToSecondWindow();

    // Вызывается когда второе окно сигнализирует о необходимости вернуться
    void onShowFirstWindow();

private:
    // Указатель на UI-объект (содержит все элементы интерфейса)
    Ui::MainWindow* ui;

    // Указатель на второе окно
    // Храним его здесь, чтобы управлять видимостью и жизненным циклом
    SecondWindow* secondWindow;
};

#endif // MAINWINDOW_H
