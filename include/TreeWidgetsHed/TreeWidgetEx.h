// TreeWidgetEx — QTreeWidget с перехватом Drop и запросом к сервису перед перемещением
#pragma once

#include <QTreeWidget>

// Наследуемся для кастомного DnD (подтверждение перемещения сервисом)
class TreeWidgetEx : public QTreeWidget {
    Q_OBJECT
public:
    explicit TreeWidgetEx(QWidget *parent = nullptr);

signals:
    void requestMove(qint64 nodeId, qint64 newParentId, bool &accepted);

protected:
    void dropEvent(QDropEvent *event) override;
};
