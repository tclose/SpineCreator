#include "viewblbatchpanelhandler.h"
#include "rootdata.h"
#include "mainwindow.h"
#include "experiment.h"
#include "batchjob.h"

viewBLBatchPanelHandler::viewBLBatchPanelHandler(QObject *parent) :
    QObject(parent)
{
}

viewBLBatchPanelHandler::viewBLBatchPanelHandler(viewBLstruct * viewBL, rootData * data, QObject *parent) :
    QObject(parent)
{
    this->viewBL = viewBL;
    this->data = data;
    this->redrawPanel();
}


void viewBLBatchPanelHandler::recursiveDeleteLoop(QLayout * parentLayout)
{
    QLayoutItem * item;
    while ((item = parentLayout->takeAt(0))) {
        if (item->widget()) {
            item->widget()->disconnect((QObject *)0);
            item->widget()->hide();
            connect(this, SIGNAL(deleteConnected()), item->widget(), SLOT(deleteLater()));
        }
        if (item->layout()) {
            connect(this, SIGNAL(deleteConnected()), item->layout(), SLOT(deleteLater()));
        }
        delete item;
    }
    connect(this, SIGNAL(deleteConnected()), parentLayout, SLOT(deleteLater()));
}

void viewBLBatchPanelHandler::recursiveDelete(QLayout * parentLayout)
{
    QLayoutItem * item;
    while ((item = parentLayout->takeAt(2))) {
        if (item->widget()) {
            item->widget()->disconnect((QObject *)0);
            item->widget()->hide();
            connect(this, SIGNAL(deleteConnected()), item->widget(), SLOT(deleteLater()));
        }
        if (item->layout()) {
            connect(this, SIGNAL(deleteConnected()), item->layout(), SLOT(deleteLater()));
        }
        delete item;
    }
}


void viewBLBatchPanelHandler::redrawPanel()
{
    QVBoxLayout * panel = ((QVBoxLayout *) viewBL->panel->layout());

    // clear panel except toolbar
    recursiveDelete(panel);
    emit deleteConnected();

    // add strech to avoid layout spacing out items
    panel->addStretch();

    // add button
    QPushButton * add = new QPushButton("Add batch job");
    add->setIcon(QIcon(":/icons/toolbar/addShad.png"));
    add->setFlat(true);
    add->setFocusPolicy(Qt::NoFocus);
    add->setToolTip("Add a new batch job to the current model");
    add->setFont(addFont);
    // grey out if editing:
    for (int i=0; i < data->batchjobs.size(); ++i)
        if (data->batchjobs[i]->editing)
            add->setDisabled(true);

    connect(add, SIGNAL(clicked()), this, SLOT(addBatch()));

    panel->insertWidget(2, add);

    // add batchjobs
    for (int i = data->batchjobs.size()-1; i >= 0; --i) {

        batchBox * box = data->batchjobs[i]->getBox(this);

        box->setProperty("index", i);
        connect(box, SIGNAL(clicked()), this, SLOT(changeSelection()));

        panel->insertWidget(2, box);

    }
}


void viewBLBatchPanelHandler::addBatch()
{
    QVBoxLayout * panel = ((QVBoxLayout *) viewBL->panel->layout());

    // add batchjob to list
    data->batchjobs.push_back(QSharedPointer<batchJob>(new batchJob));

    // select the new batchjob
    batchBox * layout = data->batchjobs.back()->getBox(this);
    panel->insertWidget(3, layout);
    data->batchjobs.back()->select(&(data->batchjobs));

    data->batchjobs.back()->editing = true;

    // disable run button
    emit enableRun(false);

    // redraw to show the new experiment
    redrawPanel();
}

void viewBLBatchPanelHandler::delBatch()
{
    int index = sender()->property("index").toUInt();

    data->batchjobs.erase(data->batchjobs.begin() + index);

    // if we delete the last experiment then disable the run button
    if (data->batchjobs.size() == 0) {
        emit enableRun(false);
    }

    // redraw to update the selection
    redrawPanel();
}

void viewBLBatchPanelHandler::moveBatch()
{
    int index = sender()->property("index").toUInt();
    int type = sender()->property("type").toUInt();

    QSharedPointer <batchJob> tempBatch;

    switch (type)
    {
    case 0: // up

        tempBatch = data->batchjobs[index];
        data->batchjobs.erase(data->batchjobs.begin() + index);
        data->batchjobs.insert(data->batchjobs.begin() + index+1, tempBatch);
        break;

    case 1: // down

        tempBatch = data->batchjobs[index];
        data->batchjobs.erase(data->batchjobs.begin() + index);
        data->batchjobs.insert(data->batchjobs.begin() + index-1, tempBatch);
        break;
    }

    // redraw to updata the selection
    redrawPanel();
}

void viewBLBatchPanelHandler::editBatch()
{
    int index = sender()->property("index").toUInt();

    data->batchjobs[index]->editing = true;

    // enable to run button
    emit enableRun(false);

    // redraw to update the editBox
    redrawPanel();
}

void viewBLBatchPanelHandler::doneEditBatch()
{
    int index = sender()->property("index").toUInt();

    // update the experiment
    QLineEdit * titleEdit = (QLineEdit *) sender()->property("title").value<void *>();
    data->batchjobs[index]->name = titleEdit->text();

    QTextEdit * descEdit = (QTextEdit *) sender()->property("desc").value<void *>();
    data->batchjobs[index]->description = descEdit->toPlainText();

    data->batchjobs[index]->editing = false;

    // enable to run button
    emit enableRun(true);

    // redraw to update the editBox
    redrawPanel();
}

void viewBLBatchPanelHandler::cancelEditBatch()
{
    int index = sender()->property("index").toUInt();

    data->batchjobs[index]->editing = false;

    // enable to run button
    emit enableRun(true);

    // redraw to update the editBox
    redrawPanel();
}

void viewBLBatchPanelHandler::changeSelection()
{
    int index = sender()->property("index").toUInt();

    // also make sure that the run button is enabled
    emit enableRun(true);

    // check if this is the selected box - if it is then do nothing
    if (data->batchjobs[index]->selected)
        return;

    // otherwise select it
    data->batchjobs[index]->select(&(data->batchjobs));

    // redraw to update the selection
    redrawPanel();
}

