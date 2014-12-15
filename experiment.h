/***************************************************************************
**                                                                        **
**  This file is part of SpineCreator, an easy to use GUI for             **
**  describing spiking neural network models.                             **
**  Copyright (C) 2013-2014 Alex Cope, Paul Richmond, Seb James           **
**                                                                        **
**  This program is free software: you can redistribute it and/or modify  **
**  it under the terms of the GNU General Public License as published by  **
**  the Free Software Foundation, either version 3 of the License, or     **
**  (at your option) any later version.                                   **
**                                                                        **
**  This program is distributed in the hope that it will be useful,       **
**  but WITHOUT ANY WARRANTY; without even the implied warranty of        **
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         **
**  GNU General Public License for more details.                          **
**                                                                        **
**  You should have received a copy of the GNU General Public License     **
**  along with this program.  If not, see http://www.gnu.org/licenses/.   **
**                                                                        **
****************************************************************************
**           Author: Alex Cope                                            **
**  Website/Contact: http://bimpa.group.shef.ac.uk/                       **
****************************************************************************/

#ifndef EXPERIMENT_H
#define EXPERIMENT_H

#include "globalHeader.h"
#include "nineML_classes.h"
#include "viewELexptpanelhandler.h"

class exptBox : public QFrame
{
    Q_OBJECT
public:
    exptBox( QWidget *parent = 0);

signals:
    void clicked();

protected:
    void mouseReleaseEvent ( QMouseEvent *  ) ;

};

enum exptInType {

    constant,
    arrayConstant,
    timevarying,
    arrayTimevarying,
    external,
    spikeList

};

class exptInLookup {
public:
    static QString toString(exptInType val) {
        if (val == constant) return "Constant input";
        if (val == arrayConstant) return "Array of constant inputs";
        if (val == timevarying) return "Time varying input";
        if (val == arrayTimevarying) return "Array of time varying inputs";
        if (val == external) return "External input";
        if (val == spikeList) return "Explicit spike list input";
        return "error - unknown experiment type";
    }

};

enum procedure {

    SingleRun,
    RepeatRuns,
    Tuning

};

enum rateDistributionTypes {

    Regular,
    Poisson

};


enum solverType {

    ForwardEuler,
    RungeKutta

};

struct simulatorSetup {

    QString simType;
    float dt;
    float duration;
    solverType solver;
    int solverOrder;
    procedure exptProcedure;

};

struct externalObject {
    int port;
    QString host;
    QString commandline;
    double timestep;
    int size;
};

/*!
 * \brief The exptInput class
 * A class to represent the SpineML Experiment/ *Input tags.
 * The class represents all Inputs, with the exptIntype
 * variable switching between types. The params QVector
 * holds the configuration data for the input, the
 * structure of which varies between input types.
 */
class exptInput : QObject
{
    Q_OBJECT
public:

    exptInput()
    {
        edit = true;
        set=false;
        eventport.name="spike";
        portIsAnalog = false;
        inType = constant;
        params.push_back(0);
        rate = false;currentIndex = 0;
        name = "New Input";
        rateDistribution = Poisson;                                     \
        externalInput.size=1;
        externalInput.port = 50091;
        externalInput.host = "localhost";
        externalInput.timestep = 0.0;
    }
    exptInput(exptInput *);

    bool rate;
    int currentIndex;

    exptInType inType;
    QVector <float> params;
    externalObject externalInput;
    QSharedPointer <NineMLComponentData> target;
    QString portName;
    bool portIsAnalog;
    bool edit;
    bool set;
    QString name;
    rateDistributionTypes rateDistribution;
    EventPort eventport;

    QVBoxLayout * drawInput(rootData *data, viewELExptPanelHandler * handler);
    void writeXML(QXmlStreamWriter *, projectObject * data);
    void readXML(QXmlStreamReader * , projectObject *);
};

/*!
 * \brief The exptOutput class
 * A class to represent the SpineML Experiment/LogOutput tags.
 * The class represents all Outputs, with network outputs
 * slected using the isExternal bool.
 */
class exptOutput : QObject {
    Q_OBJECT
public:

    exptOutput() {
        edit = true;
        set=false;
        isExternal = false;
        name = "New Output";
        portIsAnalog = true;
        indices="all";
        externalOutput.size=1;
        externalOutput.port = 50091;
        externalOutput.host = "localhost";
        externalOutput.timestep = 0.0;
    }

    exptOutput(exptOutput *);

    //exptOutput outType;
    //QVector < float > params;
    QSharedPointer <NineMLComponentData> source;
    QString portName;
    bool portIsAnalog;
    bool edit;
    bool set;
    bool isExternal;
    QString name;
    externalObject externalOutput;
    QString indices;

    QVBoxLayout * drawOutput(rootData *data, viewELExptPanelHandler * handler);
    void writeXML(QXmlStreamWriter *, projectObject * data);
    void readXML(QXmlStreamReader * , projectObject *);

};

/*!
 * \brief The exptLesion class
 * A class to represent the SpineML Experiment/Lesion tags.
 * Denotes a Projection that should not be simulated in the
 * executed model. Holds a shared pointer reference to a projection.
 */
class exptLesion : QObject {
    Q_OBJECT
public:

    exptLesion() {edit = true; set=false;}

    exptLesion(exptLesion *);

    //exptOutput outType;
    //QVector < float > params;
    QSharedPointer <projection> proj;
    Port * port;
    bool edit;
    bool set;

    QVBoxLayout * drawLesion(rootData *data, viewELExptPanelHandler * handler);
    void writeXML(QXmlStreamWriter *, projectObject *);
    void readXML(QXmlStreamReader * , projectObject *);

};

/*!
 * \brief The exptChangeProp class
 * A class to represent the SpineML Experiment/Config/Property tags.
 * Denotes a change to alter a Property of the model. This means
 * that th property will be simulated with the configuration from
 * the experiment. Holds a shared pointer to a component, and an
 * internal ParameterData for overriding the Property.
 */
class exptChangeProp : QObject {
    Q_OBJECT
public:

    exptChangeProp() {edit = true; set=false; par = NULL;}

    exptChangeProp(exptChangeProp *);

    ParameterData * par;
    QSharedPointer <NineMLComponentData> component;
    bool edit;
    bool set;

    QVBoxLayout * drawChangeProp(rootData *data, viewELExptPanelHandler * handler);
    void writeXML(QXmlStreamWriter *, projectObject * data);
    void readXML(QXmlStreamReader *);

};

/*!
 * \brief The experiment class
 * Holds a set of Inputs / Outputs / Lesions and ChangeProps
 * along with simulation data which describe the protocol for
 * simulating the model. Can be used in Batches to simulate
 * sets of experiments.
 */
class experiment : QObject
{
    Q_OBJECT

public:
    experiment();
    experiment(experiment *);
    ~experiment();

    simulatorSetup setup;
    QVector < exptInput * > ins;
    QVector < exptOutput * > outs;
    QVector < exptChangeProp * > changes;
    QVector < exptLesion * > lesions;

    QString name;
    QString description;

    exptBox * getBox(viewELExptPanelHandler *);
    void writeXML(QXmlStreamWriter *, projectObject *data);
    void readXML(QXmlStreamReader * , projectObject *);

    void purgeBadPointer(QSharedPointer <NineMLComponentData>ptr);
    void purgeBadPointer(QSharedPointer<NineMLComponent>ptr, QSharedPointer<NineMLComponent>newPtr);
    void updateChanges(QSharedPointer <NineMLComponentData> ptr);

    bool selected;
    void select(QVector < experiment * > *);
    void deselect();
    bool editing;
    bool saveWithDateStamp;

private:
    exptBox * currBoxPtr;

};


class NTableDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    NTableDelegate(QObject* parent);
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &) const;
    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &) const;
};

class TVGraph : public QWidget
{
    Q_OBJECT

public:
    TVGraph();
    TVGraph(QVector <float>);
    ~TVGraph();
    QVector <float> vals;

protected:
    void paintEvent(QPaintEvent *);

};

#endif // EXPERIMENT_H
