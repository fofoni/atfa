/*
 * Universidade Federal do Rio de Janeiro
 * Escola Politécnica
 * Projeto Final de Graduação
 * Ambiente de Teste para Filtros Adaptativos
 * Pedro Angelo Medeiros Fonini
 * Orientador: Markus Lima
 */

#ifndef FILESELECTWIDGET_H
#define FILESELECTWIDGET_H

#include <QtWidgets>
#include <QtGui>

class FileSelectWidget : public QWidget
{
    Q_OBJECT

public:
    explicit FileSelectWidget(
        const QString& cpt = QString(),
        const QString& d = QString(),
        const QString& flt = QString(),
        QWidget *parent = 0,
        QWidget *parentWindow = 0
    );

    QString text();

signals:
    void returnPressed();
    void textChanged(const QString&);

private:
    QString caption;
    QString dir;
    QString filter;

    QWidget *pWin;

    QLineEdit *file_path_edit;
    QPushButton *choose_button;

private slots:
    void showDialog();

public slots:
    void setPath(const QString& str);

};

#endif // FILESELECTWIDGET_H
