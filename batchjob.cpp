#include "batchjob.h"
#include "viewblbatchpanelhandler.h"
#include "rootdata.h"

// ################################## batchBox

batchBox::batchBox( QWidget * parent ):QFrame(parent)
{
    this->setStyleSheet("background-color :transparent");
}

void batchBox::mouseReleaseEvent ( QMouseEvent * )
{
    emit clicked();
}

// ################################## batchJob

batchJob::batchJob(QObject *parent) :
    QObject(parent)
{
    this->selected = false;
    this->editing = true;
    this->name = "New Batch Job";
    this->description = "add a description for the batch job here";
}

void batchJob::select(QVector < QSharedPointer < batchJob > > * batchjobs) {

    // deselect all
    for (int i = 0; i < batchjobs->size(); ++i) {
        (*batchjobs)[i]->deselect();
        (*batchjobs)[i]->editing = false;
    }

    // select this Batch
    this->selected = true;
}

void batchJob::deselect() {

    // select this Batch
    this->selected = false;

}

batchBox * batchJob::getBox(viewBLBatchPanelHandler * panel) {

#ifdef Q_OS_MAC
    QFont titleFont("Helvetica [Cronyx]", 16);
    QFont descFont("Helvetica [Cronyx]", 12);
#else
    QFont titleFont("Helvetica [Cronyx]", 14);
    QFont descFont("Helvetica [Cronyx]", 8);
#endif


    batchBox * box = new batchBox();
    QGridLayout * layout = new QGridLayout;
    box->setLayout(layout);
    layout->setSpacing(0);

    this->currBoxPtr = box;

    int index = -1;

    for (int i = 0; i < panel->data->batchjobs.size(); ++i) {

        if (panel->data->batchjobs[i] == this) index = i;
    }
    if (index == -1) {qDebug() << "batchjob.cpp: big error"; exit(-1);}

    if (editing)
    {
#ifdef Q_OS_MAC
        layout->setSpacing(6);
#endif
        // set up the Batch editing box
        // title
        currBoxPtr->setStyleSheet("background-color :rgba(200,200,200,255)");
        QRegExp regExp("[A-Za-z0-9 ][A-Za-z0-9 ][A-Za-z0-9 ][A-Za-z0-9 ][A-Za-z0-9 ][A-Za-z0-9 ][A-Za-z0-9 ][A-Za-z0-9 ][A-Za-z0-9 ][A-Za-z0-9 ][A-Za-z0-9 ][A-Za-z0-9 ][A-Za-z0-9 ][A-Za-z0-9 ][A-Za-z0-9 ][A-Za-z0-9 ][A-Za-z0-9 ][A-Za-z0-9 ][A-Za-z0-9 ][A-Za-z0-9 ]");
        QRegExpValidator *validator = new QRegExpValidator(regExp, this);
        QLineEdit * editTitle = new QLineEdit;
        editTitle->setValidator(validator);
        editTitle->setText(name);
        editTitle->setMaximumWidth(401);
        layout->addWidget(editTitle,0,0,1,1);

        // description
        QTextEdit * editDesc = new QTextEdit;
        editDesc->setMaximumWidth(400);
        editDesc->setPlainText(description);
        layout->addWidget(editDesc,1,0,2,1);

        // done button
        QPushButton * done = new QPushButton("Done");
        done->setProperty("index", (int) index);
        done->setProperty("title", qVariantFromValue((void *) editTitle));
        done->setProperty("desc", qVariantFromValue((void *) editDesc));
        QObject::connect(done, SIGNAL(clicked()), panel, SLOT(doneEditBatch()));
        layout->addWidget(done,0,2,1,1);

        // cancel button
        QPushButton * cancel = new QPushButton("Cancel");
        cancel->setProperty("index", (int) index);
        QObject::connect(cancel, SIGNAL(clicked()), panel, SLOT(cancelEditBatch()));
        layout->addWidget(cancel,1,2,1,1);

        return box;
    }
    if (selected)
    {
        // set up a selected box
        currBoxPtr->setStyleSheet("background-color :rgba(0,0,0,40)");
        // title
        QLabel * nameLabel = new QLabel(name);
        nameLabel->setFont(titleFont);
        nameLabel->setStyleSheet("background-color :transparent");
        nameLabel->setMinimumWidth(200);
        layout->addWidget(nameLabel,0,0,1,1);

        // dexription
        QLabel * descLabel = new QLabel(description);
        descLabel->setStyleSheet("background-color :transparent");
        descLabel->setMinimumWidth(200);
        descLabel->setWordWrap(true);
        descLabel->setFont(descFont);
        layout->addWidget(descLabel,1,0,2,1);

        // a label to make the layout look nice
        QLabel * spacerLabel = new QLabel();
        spacerLabel->setStyleSheet("background-color :transparent");
        layout->addWidget(spacerLabel,1,1,1,2);

        // up and down buttons
        QPushButton * up = new QPushButton;
        up->setMaximumWidth(32);
        up->setFlat(true);
        up->setIcon(QIcon(":/icons/toolbar/up.png"));
        up->setProperty("index", (int) index);
        up->setProperty("type", 1);
        layout->addWidget(up,0,2,1,1);
        QObject::connect(up, SIGNAL(clicked()), panel, SLOT(moveBatch()));

        QPushButton * down = new QPushButton;
        down->setMaximumWidth(32);
        down->setFlat(true);
        down->setIcon(QIcon(":/icons/toolbar/down.png"));
        down->setProperty("index", (int) index);
        down->setProperty("type", 0);
        layout->addWidget(down,0,3,1,1);

        QObject::connect(down, SIGNAL(clicked()), panel, SLOT(moveBatch()));
        if (index == 0) {
            up->setEnabled(false);
        }
        if (index == (int) panel->data->batchjobs.size()-1) {
            down->setEnabled(false);
        }

        // edit button
        QPushButton * edit = new QPushButton;
        edit->setMaximumWidth(32);
        //edit->setIconSize(QSize(32,32));
        edit->setFlat(true);
        edit->setIcon(QIcon(":/icons/toolbar/edit.png"));
        edit->setProperty("index", (int) index);
        layout->addWidget(edit,1,3,1,1);
        QObject::connect(edit, SIGNAL(clicked()), panel, SLOT(editBatch()));

        // delete button
        QPushButton * del = new QPushButton;
        del->setMaximumWidth(32);
        del->setFlat(true);
        del->setIcon(QIcon(":/icons/toolbar/delShad.png"));
        del->setProperty("index", (int) index);
        layout->addWidget(del,2,3,1,1);
        QObject::connect(del, SIGNAL(clicked()), panel, SLOT(delBatch()));
    }
    else
    {
        // set up an unselected box
        currBoxPtr->setStyleSheet("background-color :transparent");
        QLabel * nameLabel = new QLabel(name);
        layout->addWidget(nameLabel,0,0,1,1);
        nameLabel->setFont(titleFont);

        QPushButton * up = new QPushButton;
        up->setMaximumWidth(32);
        up->setFlat(true);
        up->setIcon(QIcon(":/icons/toolbar/up.png"));
        up->setProperty("index", (int) index);
        up->setProperty("type", 1);
        layout->addWidget(up,0,2,1,1);
        QObject::connect(up, SIGNAL(clicked()), panel, SLOT(moveBatch()));

        QPushButton * down = new QPushButton;
        down->setMaximumWidth(32);
        down->setFlat(true);
        down->setIcon(QIcon(":/icons/toolbar/down.png"));
        down->setProperty("index", (int) index);
        down->setProperty("type", 0);
        layout->addWidget(down,0,3,1,1);
        QObject::connect(down, SIGNAL(clicked()), panel, SLOT(moveBatch()));

        if (index == 0) {
            up->setEnabled(false);
        }
        if (index == (int) panel->data->batchjobs.size()-1) {
            down->setEnabled(false);
        }
    }

    return box;

}
