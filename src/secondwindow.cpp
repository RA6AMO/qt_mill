// Подключаем заголовочный файл нашего класса
// secondwindow.cpp — инициализация Db/репозитория/сервиса и связывание с QTreeWidget
#include "secondwindow.h"
// Подключаем автоматически сгенерированный UI-класс
// Файл ui_secondwindow.h создаётся из secondwindow.ui при компиляции
#include "ui_secondwindow.h"
#include <QtSql/QSqlDatabase>

#include "Db.h"
#include "INodeFactory.h"
#include "TreeService.h"
#include "widgetsTreeFeeler.h"
#include "TreeWidgetEx.h"
#include <QThread>
//#include <cstddef>
#include <qobject.h>



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

    // Инициализация БД и сервисов
    const QString conn = Db::openAndInit("app_conn", "tree.sqlite");
    QSqlDatabase db = QSqlDatabase::database(conn);
    m_db = new QSqlDatabase(db);
    m_factory = makeNodeFactory();
    m_repo = makeSqliteNodeRepository(*m_db);


    m_service = std::make_unique<TreeService>(std::move(m_repo), std::move(m_factory));

    // Подмена QTreeWidget на расширенный класс в рантайме не требуется — он уже QTreeWidget.
    // Для простоты обернём существующий в feeler (он принимает TreeWidgetEx*, кастуем безопасно)
    auto *treeEx = qobject_cast<TreeWidgetEx*>(ui->treeWidget);
    if (!treeEx) {
        // Если ui->treeWidget — обычный QTreeWidget, заменим виджет на наш класс
        TreeWidgetEx *replacement = new TreeWidgetEx(ui->centralwidget);
        replacement->setObjectName("treeWidget");
        replacement->setGeometry(ui->treeWidget->geometry());
        replacement->setHeaderHidden(true);
        QTreeWidget *oldWidget = ui->treeWidget;
        oldWidget->hide();
        oldWidget->setParent(nullptr);
        ui->treeWidget = replacement; // безопасно, тк это поле из ui-класса
        delete oldWidget;
        treeEx = replacement;
    }

    m_feeler = std::make_unique<WidgetsTreeFeeler>(treeEx, m_service.get(), this);
    m_feeler->initialize();

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

   // Возвращает всех детей родителя, отсортированных по имени.
  // virtual std::vector<RepoRow> getChildren(qint64 parentId) = 0;

   // Быстрая проверка наличия хотя бы одного ребёнка.
    //  virtual bool hasChildren(qint64 id) = 0;
/*
    std::map<qint64, RepoRow> tree;
    fillTreeMapRecursive(1, tree);
    size_t size = tree.size();
*/
    //QThread::sleep(10);
}
/*
void SecondWindow::resetTreeMap() {
    m_treeMap.clear();
    fillTreeMapRecursive(1, m_treeMap);
}

void SecondWindow::fillTreeMapRecursive(qint64 nodeId, std::map<qint64, RepoRow>& tree) {
    if (!m_repo) return;

    // Получаем данные узла по id
    auto nodeOpt = m_repo->get(nodeId);
    if (!nodeOpt.has_value()) {
        return; // Узел не найден
    }

    RepoRow node = nodeOpt.value();

    // Добавляем узел в map
    tree[nodeId] = node;

    // Получаем всех детей этого узла
    std::vector<RepoRow> children = m_repo->getChildren(nodeId);

    // Рекурсивно обрабатываем каждого ребёнка
    for (const auto& child : children) {
        fillTreeMapRecursive(child.id, tree);
    }
}
*/

bool SecondWindow::fillTreeWidget() {
    if (m_feeler) { m_feeler->initialize(); return true; }
    return false;
}

bool SecondWindow::addItemToTreeWidget(QTreeWidgetItem *parent, const QString &text) {
    if (!ui || !ui->treeWidget || !m_service) return false;
    qint64 parentId = 1;
    if (parent) parentId = parent->data(0, Qt::UserRole).toLongLong();
    try {
        const qint64 newId = m_service->createNode(parentId, text, {});
        m_treeMap.clear();
        m_treeMap = m_service->getTree();
        QTreeWidgetItem *item = new QTreeWidgetItem();
        item->setText(0, text);
        item->setData(0, Qt::UserRole, QVariant::fromValue<qlonglong>(newId));
        if (parent) parent->addChild(item); else ui->treeWidget->addTopLevelItem(item);
        return true;
    } catch (...) {
        return false;
    }
}

bool SecondWindow::removeItemFromTreeWidget(QTreeWidgetItem *item) {
    if (!item || !m_service) return false;
    const qint64 id = item->data(0, Qt::UserRole).toLongLong();
    try {
        m_service->deleteNode(id);
        m_treeMap.clear();
        m_treeMap = m_service->getTree();
        delete item;
        return true;
    } catch (...) {
        return false;
    }
}

// Деструктор SecondWindow
SecondWindow::~SecondWindow() {
    // Удаляем UI-объект из памяти
    // Важно: виджеты, созданные через setupUi(), удаляются автоматически
    // как дочерние объекты окна, но сам ui-объект нужно удалить явно
    delete ui;
    if (m_db) {
        QString name = m_db->connectionName();
        m_db->close();
        delete m_db;
        QSqlDatabase::removeDatabase(name);
    }
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
