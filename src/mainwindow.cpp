// Подключаем заголовочный файл нашего класса
#include "mainwindow.h"
// Подключаем автогенерированный UI-класс
#include "ui_mainwindow.h"
// Подключаем класс второго окна (полное определение)
#include "secondwindow.h"

// Конструктор главного окна
MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),           // Инициализация базового класса
      ui(new Ui::MainWindow),        // Создание UI-объекта
      secondWindow(nullptr)           // Инициализация указателя на второе окно как nullptr
{
    // Создаём и настраиваем интерфейс из .ui файла
    ui->setupUi(this);

    // ВАЖНО: Создаём второе окно сразу при инициализации главного окна
    // this передаём как parent - это обеспечивает автоматическое удаление
    // второго окна при удалении главного (управление памятью Qt)
    secondWindow = new SecondWindow(this);

    // Устанавливаем соединения сигналов и слотов

    // 1. Соединение: кнопка в первом окне -> слот перехода ко второму окну
    // При нажатии на pushButton вызывается наш слот onSwitchToSecondWindow
    connect(ui->pushButton, &QPushButton::clicked,
            this, &MainWindow::onSwitchToSecondWindow);

    // 2. Соединение: сигнал из второго окна -> слот показа первого окна
    // Когда второе окно испускает backToFirstWindow, мы показываем первое окно
    connect(secondWindow, &SecondWindow::backToFirstWindow,
            this, &MainWindow::onShowFirstWindow);

    // Примечание: используем новый синтаксис connect (Qt5+)
    // Преимущества:
    // - Проверка типов на этапе компиляции
    // - Автодополнение в IDE
    // - Можно подключать к лямбда-функциям
    //
    // Старый синтаксис выглядел бы так:
    // connect(ui->pushButton, SIGNAL(clicked()), this, SLOT(onSwitchToSecondWindow()));
}

// Деструктор главного окна
MainWindow::~MainWindow() {
    // Удаляем UI-объект
    delete ui;

    // secondWindow удалится автоматически, так как мы передали this как parent
    // Это одно из преимуществ системы parent-child в Qt:
    // родитель автоматически удаляет всех своих детей
}

// Слот для перехода ко второму окну
void MainWindow::onSwitchToSecondWindow() {
    // Проверяем, что второе окно существует
    if (secondWindow) {
        // hide() - скрывает текущее окно (не удаляет из памяти!)
        // Окно остаётся в памяти со всеми данными
        this->hide();

        // show() - показывает второе окно
        // Если окно было скрыто, оно появится в том же состоянии
        secondWindow->show();

        // Альтернативные методы:
        // - close(): закрывает окно (может удалить из памяти)
        // - setVisible(false/true): то же, что hide()/show()
        // - showMinimized(): показать свёрнутым
        // - showMaximized(): показать развёрнутым на весь экран
        // - showFullScreen(): полноэкранный режим
    }
}

// Слот для возврата к первому окну
void MainWindow::onShowFirstWindow() {
    // Скрываем второе окно
    if (secondWindow) {
        secondWindow->hide();
    }

    // Показываем главное окно
    this->show();

    // raise() - поднимает окно поверх других окон
    // activateWindow() - делает окно активным (передаёт фокус)
    this->raise();
    this->activateWindow();
}
