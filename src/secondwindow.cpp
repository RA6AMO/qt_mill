// Подключаем заголовочный файл нашего класса
#include "secondwindow.h"
// Подключаем автоматически сгенерированный UI-класс
// Файл ui_secondwindow.h создаётся из secondwindow.ui при компиляции
#include "ui_secondwindow.h"

// Конструктор SecondWindow
// Список инициализации (:) выполняется до тела конструктора
SecondWindow::SecondWindow(QWidget* parent)
    : QMainWindow(parent),  // Вызываем конструктор базового класса
      ui(new Ui::SecondWindow)  // Создаём объект UI в динамической памяти
{
    guest = true;
    // setupUi() - метод из автогенерированного класса
    // Он создаёт все виджеты, описанные в .ui файле,
    // и размещает их в окне согласно дизайну
    ui->setupUi(this);

    ui->backButton->setDisabled(true);

    fillTreeWidget();

    //ui->treeWidget->setColumnCount(2);
    // Подключаем сигнал clicked() от кнопки backButton к нашему слоту
    // connect() - функция Qt для связывания сигналов и слотов
    // Параметры:
    // 1. ui->backButton - объект, испускающий сигнал (кнопка)
    // 2. &QPushButton::clicked - указатель на сигнал (нажатие кнопки)
    // 3. this - объект, принимающий сигнал (наше окно)
    // 4. &SecondWindow::onBackButtonClicked - указатель на слот (наш метод)
    //
    // Новый синтаксис Qt5/Qt6 с указателями на функции-члены
    // Преимущества: проверка типов на этапе компиляции, автодополнение в IDE
    connect(ui->backButton, &QPushButton::clicked,
            this, &SecondWindow::onBackButtonClicked);
}

// Деструктор SecondWindow
SecondWindow::~SecondWindow() {
    // Удаляем UI-объект из памяти
    // Важно: виджеты, созданные через setupUi(), удаляются автоматически
    // как дочерние объекты окна, но сам ui-объект нужно удалить явно
    delete ui;
}

void SecondWindow::guestSeterT() {
    guest = true;
    ui->backButton->setDisabled(false);
}

// Реализация слота для обработки нажатия кнопки "Назад"
void SecondWindow::onBackButtonClicked() {
    // emit - ключевое слово Qt для испускания сигнала
    // Оно не обязательно (можно просто вызвать backToFirstWindow()),
    // но делает код более читаемым - сразу видно, что это сигнал
    emit backToFirstWindow();

    // После испускания сигнала MainWindow получит уведомление
    // и выполнит соответствующие действия (покажет себя и скроет это окно)
}

bool SecondWindow::fillTreeWidget() {
    QTreeWidgetItem *Root1 = new QTreeWidgetItem(ui->treeWidget);
    Root1->setText(0, "Фрезы");

    QTreeWidgetItem *Root1_1 = new QTreeWidgetItem(Root1);
    Root1_1->setText(0, "Фрезы концевые");
    QTreeWidgetItem *Root1_2 = new QTreeWidgetItem(Root1);
    Root1_2->setText(0, "Фрезы торцевые");

    QTreeWidgetItem *Root2 = new QTreeWidgetItem(ui->treeWidget);
    Root2->setText(0, "Сверла");

    QTreeWidgetItem *Root2_1 = new QTreeWidgetItem(Root2);
    Root2_1->setText(0, "Сверла проходные");
    QTreeWidgetItem *Root2_2 = new QTreeWidgetItem(Root2);
    Root2_2->setText(0, "Сверла ружейные");

    return true;
}

bool SecondWindow::addItemToTreeWidget(QTreeWidgetItem *parent, const QString &text) {
    QTreeWidgetItem *item = new QTreeWidgetItem(parent);
    item->setText(0, text);
    return true;
}

bool SecondWindow::removeItemFromTreeWidget(QTreeWidgetItem *item) {
    delete item;
    return true;
}
