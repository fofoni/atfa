/*
 * Universidade Federal do Rio de Janeiro
 * Escola Politécnica
 * Projeto Final de Graduação
 * Ambiente de Teste para Filtros Adaptativos
 * Pedro Angelo Medeiros Fonini
 * Orientador: Markus Lima
 */

/* Classe adaptada de:
 *  http://qt-apps.org/content/show.php/QLedIndicator?content=118610
 * (GNU LGPL)
 */

#include "LEDIndicatorWidget.h"

LEDIndicatorWidget::LEDIndicatorWidget(QWidget *parent)
    : QWidget(parent), ledstatus(false)
{
    setMinimumSize(2*LEDRADIUS+2, 2*LEDRADIUS+2);
}

QSize LEDIndicatorWidget::sizeHint() const {
    return QSize(2*LEDRADIUS+2, 2*LEDRADIUS+2);
}

QSize LEDIndicatorWidget::minimumSizeHint() const {
    return QSize(2*LEDRADIUS+2, 2*LEDRADIUS+2);
}

void LEDIndicatorWidget::paintEvent(QPaintEvent *event) {
    (void) event;
    QPainter painter(this);
    QPen pen(Qt::black);
    pen.setWidth(1.5);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.translate(width()-LEDRADIUS-1, height()/2);
    painter.setPen(pen);
    painter.drawEllipse(QPointF(0,0), LEDRADIUS, LEDRADIUS);
    if (ledstatus)
        painter.setBrush(QBrush(Qt::green));
    else
        painter.setBrush(QBrush(Qt::gray));
    painter.drawEllipse(QPointF(0,0), LEDRADIUS, LEDRADIUS);
}

void LEDIndicatorWidget::setLEDStatus(bool status) {
    ledstatus = status;
    update();
}
