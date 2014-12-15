#ifndef BATCHJOB_H
#define BATCHJOB_H

#include <QObject>
#include "globalHeader.h"

/*!
 * \brief The batchBox class
 * Subclassing QFrame and overriding the mouseReleaseEvent to allow selection of the batchjob by a click on the Frame
 */
class batchBox : public QFrame
{
    Q_OBJECT
public:
    batchBox( QWidget *parent = 0);

signals:
    void clicked();

protected:
    void mouseReleaseEvent ( QMouseEvent *  ) ;

};


class batchJob : public QObject
{
    Q_OBJECT
public:
    explicit batchJob(QObject *parent = 0);

    // tags
    bool editing;
    bool selected;

    QString name;
    QString description;

    // select / deselect
    void select(QVector < QSharedPointer < batchJob > > * batchjobs);
    void deselect();

    //
    batchBox * getBox(viewBLBatchPanelHandler *);

private:
    batchBox * currBoxPtr;

signals:

public slots:

};

#endif // BATCHJOB_H
