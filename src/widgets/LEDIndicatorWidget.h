/*
 * Universidade Federal do Rio de Janeiro
 * Escola Politécnica
 * Projeto Final de Graduação
 * Ambiente de Teste para Filtros Adaptativos
 * Pedro Angelo Medeiros Fonini
 * Orientador: Markus Lima
 */

#ifndef LEDINDICATORWIDGET_H
#define LEDINDICATORWIDGET_H

#include <QWidget>
#include <QPainter>

class LEDIndicatorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LEDIndicatorWidget(QWidget *parent = 0);

    QSize sizeHint() const Q_DECL_OVERRIDE;
    QSize minimumSizeHint() const Q_DECL_OVERRIDE;

    static constexpr int LEDRADIUS = 5;

signals:


protected:
    void paintEvent (QPaintEvent *event) Q_DECL_OVERRIDE;


public slots:
    void setLEDStatus(bool status);

private:
    bool ledstatus;
};

#endif // LEDINDICATORWIDGET_H
