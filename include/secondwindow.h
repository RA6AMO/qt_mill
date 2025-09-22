
// secondwindow.h — окно, интегрирующее сервис/репозиторий/Db с QTreeWidget
#pragma once
// Защита от повторного включения заголовочного файла
// Если SECONDWINDOW_H уже определён, препроцессор пропустит весь код до #endif
#ifndef SECONDWINDOW_H
#define SECONDWINDOW_H

// Подключаем базовый класс для создания окон в Qt
#include <QMainWindow>
#include "INodeRepository.h"
#include <map>
class QSqlDatabase;

class QTreeWidgetItem;
class QString;
// Макросы Qt для начала пространства имён
// Это нужно для правильной работы с UI-файлами, созданными в Qt Designer
QT_BEGIN_NAMESPACE
namespace Ui {
    // Предварительное объявление класса SecondWindow из UI
    // Реальный класс генерируется из .ui файла
    class SecondWindow;
}
QT_END_NAMESPACE

// Объявляем наш класс SecondWindow, наследуемся от QMainWindow
// QMainWindow - это готовый класс Qt для создания главных окон приложения
class SecondWindow : public QMainWindow {
    // Q_OBJECT - специальный макрос Qt
    // Он необходим для:
    // 1. Работы системы сигналов и слотов
    // 2. Метаобъектной системы Qt (рефлексия, динамические свойства)
    // 3. Интернационализации (перевода интерфейса)
    Q_OBJECT
public:
    void guestSeterT();
    bool fillTreeWidget();
    bool addItemToTreeWidget(QTreeWidgetItem *parent, const QString &text);
    bool removeItemFromTreeWidget(QTreeWidgetItem *item);

    // Рекурсивная функция для заполнения map дерева
    void fillTreeMapRecursive(qint64 nodeId, std::map<qint64, RepoRow>& tree);

    // Конструктор класса
    // explicit - запрещает неявное преобразование типов
    // QWidget* parent = nullptr - родительский виджет (для управления памятью)
    // По умолчанию nullptr - окно будет независимым
    explicit SecondWindow(QWidget* parent = nullptr);

    // Деструктор - освобождает память при удалении окна
    // override - указывает, что мы переопределяем виртуальный метод базового класса
    ~SecondWindow() override;

signals:
    // Сигнал - специальный механизм Qt для оповещения о событиях
    // Этот сигнал будет испускаться, когда нужно вернуться к первому окну
    // Сигналы не имеют реализации - Qt генерирует её автоматически
    void backToFirstWindow();

private slots:
    // Слот - это метод, который может быть вызван через сигнал
    // private slots - приватные слоты, доступные только внутри класса
    // Этот слот будет вызываться при нажатии кнопки "Назад"
    void onBackButtonClicked();
    void treeButtn();


private:
    // Указатель на UI-класс, автоматически генерируемый из .ui файла
    // Через него мы получаем доступ ко всем элементам интерфейса
    Ui::SecondWindow* ui;
    bool guest;

    // Интеграция сервиса/БД
    std::unique_ptr<class WidgetsTreeFeeler> m_feeler;
    std::unique_ptr<class TreeService> m_service;
    std::unique_ptr<class INodeRepository> m_repo;
    std::unique_ptr<class INodeFactory> m_factory;
    QSqlDatabase *m_db {nullptr};

    std::map<qint64, RepoRow> m_treeMap;

public:
    //const std::map<qint64, RepoRow>& getTreeMap() const { return m_treeMap; }

//public slots:
  //  void resetTreeMap();
};

#endif // SECONDWINDOW_H
