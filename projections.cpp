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
**          Authors: Alex Cope, Seb James                                 **
**  Website/Contact: http://bimpa.group.shef.ac.uk/                       **
****************************************************************************/

#include "projections.h"
#include "genericinput.h"
#include "connection.h"
#include "experiment.h"
#include "projectobject.h"

synapse::synapse(QSharedPointer <projection> proj, projectObject * data, bool dontAddInputs) {

   this->postsynapseType = QSharedPointer<NineMLComponentData>(new NineMLComponentData(data->catalogPS[0]));
   this->postsynapseType->owner = proj;
   this->weightUpdateType = QSharedPointer<NineMLComponentData>(new NineMLComponentData(data->catalogWU[0]));
   this->weightUpdateType->owner = proj;
   this->connectionType = new alltoAll_connection;

    if (!dontAddInputs) {
        // add the inputs:

        // source -> synapse
        this->weightUpdateType->addInput(proj->source->neuronType, true);

        // synapse -> PSP
        this->postsynapseType->addInput(this->weightUpdateType, true);

        // PSP -> destination
        if (proj->destination != NULL)
            proj->destination->neuronType->addInput(this->postsynapseType, true);
    }

    //attach to the projection (shared pointers mean this must be done elsewhere)
    //proj->synapses.push_back(QSharedPointer<synapse>(this));

    this->proj = proj;

    this->type = synapseObject;

    this->isVisualised = false;

    this->strength = 0;
    this->center[0] = 0;
    this->center[1] = 0;
    this->center[2] = 0;

    this->colorScheme = 0;

}

synapse::synapse(QSharedPointer <projection> proj, rootData * data, bool dontAddInputs) {

   this->postsynapseType = QSharedPointer<NineMLComponentData>(new NineMLComponentData(data->catalogPS[0]));
   this->postsynapseType->owner = proj;
   this->weightUpdateType = QSharedPointer<NineMLComponentData>(new NineMLComponentData(data->catalogWU[0]));
   this->weightUpdateType->owner = proj;
   this->connectionType = new alltoAll_connection;

    if (!dontAddInputs) {
        // add the inputs:

        // source -> synapse
        this->weightUpdateType->addInput(proj->source->neuronType, true);

        // synapse -> PSP
        this->postsynapseType->addInput(this->weightUpdateType, true);

        // PSP -> destination
        if (proj->destination != NULL)
            proj->destination->neuronType->addInput(this->postsynapseType, true);
    }

    //attach to the projection (shared pointers mean this must be done elsewhere)
    //proj->synapses.push_back(QSharedPointer<synapse>(this));

    this->proj = proj;

    this->type = synapseObject;

    this->isVisualised = false;

    this->strength = 0;
    this->center[0] = 0;
    this->center[1] = 0;
    this->center[2] = 0;

    this->colorScheme = 0;

}


synapse::~synapse() {

    // remove components (they will clean up their inputs themselves) NOW QSharedPointers - so no need
    //delete this->postsynapseType;
    //delete this->weightUpdateType;
    this->postsynapseType.clear();
    this->weightUpdateType.clear();
    delete this->connectionType;

}


void synapse::delAll(rootData *) {

    // remove components (they will clean up their inputs themselves)
    this->postsynapseType->removeReferences();
    this->weightUpdateType->removeReferences();

    //delete this->postsynapseType;
    //delete this->weightUpdateType;
    this->postsynapseType.clear();
    this->weightUpdateType.clear();
    delete this->connectionType;

    //delete this;

}

QString synapse::getName() {

    // get index:
    int index = -1;
    for (int i = 0; i < this->proj->synapses.size(); ++i)
        if (this->proj->synapses[i].data() == this)
            index = i;

    if (index == -1) {
        qDebug() << "Can't find synapse! In synapse::getName()";
        return "Err";
    }

    return this->proj->getName() + ": Synapse " + QString::number(index);

}

QSharedPointer < systemObject > synapse::newFromExisting(QMap<systemObject *, QSharedPointer<systemObject> > &objectMap)
{

    // create a new, identical, synapse

    QSharedPointer <synapse> newSyn = QSharedPointer <synapse>(new synapse());

    newSyn->weightUpdateType = QSharedPointer<NineMLComponentData>(new NineMLComponentData(this->weightUpdateType, true/*copy inputs / outputs*/));
    newSyn->postsynapseType = QSharedPointer<NineMLComponentData>(new NineMLComponentData(this->postsynapseType, true/*copy inputs / outputs*/));
    newSyn->connectionType = this->connectionType->newFromExisting();
    newSyn->isVisualised = this->isVisualised;

    objectMap.insert(this, newSyn);

    // now we must create copies of all the projInput GenericInputs:
    // we only do inputs for weightupdates, but inputs AND outputs for
    // postsynapses. This is because the input to the postsynapse
    // is the same as the output from the weightupdate,
    // and they'll be remapped when we sort out the pointers in the
    // second copy step...
    for (int i = 0; i < this->weightUpdateType->inputs.size(); ++i) {
        if (this->weightUpdateType->inputs[i]->projInput) {
            // create a new copy
            QSharedPointer <genericInput> in = qSharedPointerDynamicCast <genericInput> (this->weightUpdateType->inputs[i]->newFromExisting(objectMap));
        }
    }
    for (int i = 0; i < this->postsynapseType->inputs.size(); ++i) {
        if (this->postsynapseType->inputs[i]->projInput) {
            // create a new copy
            QSharedPointer <genericInput> in = qSharedPointerDynamicCast <genericInput> (this->postsynapseType->inputs[i]->newFromExisting(objectMap));
        }
    }
    for (int i = 0; i < this->postsynapseType->outputs.size(); ++i) {
        if (this->postsynapseType->outputs[i]->projInput) {
            // create a new copy
            QSharedPointer <genericInput> in = qSharedPointerDynamicCast <genericInput> (this->postsynapseType->outputs[i]->newFromExisting(objectMap));
        }
    }

    return qSharedPointerCast <systemObject> (newSyn);

}

void synapse::remapSharedPointers(QMap <systemObject *, QSharedPointer <systemObject> > objectMap)
{
    this->weightUpdateType->remapPointers(objectMap);
    this->postsynapseType->remapPointers(objectMap);

    // connection, if it has a generator
    if (this->connectionType->type == CSV) {
        csv_connection * c = dynamic_cast < csv_connection * > (this->connectionType);
        if (c && c->generator != NULL) {
            pythonscript_connection * g = dynamic_cast < pythonscript_connection * > (c->generator);
            if (g) {
                g->src = qSharedPointerDynamicCast <population> (objectMap[g->src.data()]);
                g->dst = qSharedPointerDynamicCast <population> (objectMap[g->dst.data()]);
                if (!g->src || !g->dst) {
                    qDebug() << "Error casting objectMap lookup to population in synapse::remapSharedPointers";
                    exit(-1);
                }
            }
        }
    }

}

projection::projection()
{
    this->type = projectionObject;

    this->destination.clear();
    this->source.clear();

    currTarg = 0;
    this->start = QPointF(0,0);

    this->tempTrans.GLscale = 100;
    this->tempTrans.height = 1;
    this->tempTrans.width = 1;
    this->tempTrans.viewX = 0;
    this->tempTrans.viewY = 0;

    this->selectedControlPoint.ind = -1;
    this->selectedControlPoint.start = false;
    this->selectedControlPoint.type = C1;

    this->projDrawStyle = standardDrawStyleExcitatory;

}

projection::~projection()
{

}


void projection::connect(QSharedPointer<projection> in) {

    // connect might be called multiple times due to the nature of Undo
    for (int i = 0; i < destination->reverseProjections.size(); ++i) {
        if (destination->reverseProjections[i].data() == in.data()) {
            // already there - give up
            return;
        }
    }
    for (int i = 0; i < source->projections.size(); ++i) {
        if (source->projections[i].data() == in.data()) {
            // already there - give up
            return;
        }
    }

    destination->reverseProjections.push_back(in);
    source->projections.push_back(in);

    // connect inputs
    /*for (int i = 0; i < this->disconnectedInputs.size(); ++i) {
        this->disconnectedInputs[i]->connect();
    }

    disconnectedInputs.clear();*/

}

void projection::disconnect() {

    if (destination != NULL) {
        // remove projection
        for (int i = 0; i < destination->reverseProjections.size(); ++i) {
            if (destination->reverseProjections[i].data() == this) {
                destination->reverseProjections.erase(destination->reverseProjections.begin()+i);
                dstPos = i;
            }
        }
    }
    if (source != NULL) {
        for (int i = 0; i < source->projections.size(); ++i) {
            if (source->projections[i].data() == this) {
                source->projections.erase(source->projections.begin()+i);
                srcPos = i;
            }
        }
    }

    // remove inputs & outputs
    /*for (int i = 0; i < synapses.size(); ++i) {
        QSharedPointer <synapse> syn = synapses[i];
        // remove inputs
        for (int j = 0; j < syn->weightUpdateType->inputs.size(); ++j) {
            disconnectedInputs.push_back(syn->weightUpdateType->inputs[j]);
            syn->weightUpdateType->inputs[j]->disconnect();
        }
        for (int j = 0; j < syn->postsynapseType->inputs.size(); ++j) {
            disconnectedInputs.push_back(syn->postsynapseType->inputs[j]);
            syn->postsynapseType->inputs[j]->disconnect();
        }

        // remove outputs
        for (int j = 0; j < syn->weightUpdateType->outputs.size(); ++j) {
            disconnectedInputs.push_back(syn->weightUpdateType->outputs[j]);
            syn->weightUpdateType->outputs[j]->disconnect();
        }
        for (int j = 0; j < syn->postsynapseType->outputs.size(); ++j) {
            disconnectedInputs.push_back(syn->postsynapseType->outputs[j]);
            syn->postsynapseType->outputs[j]->disconnect();
        }
    }*/
}

void projection::remove(rootData * data) {

    // remove from experiment
    for (int j = 0; j < data->experiments.size(); ++j) {
        //data->experiments[j]->purgeBadPointer(this);
    }

    //delete this;

}

void projection::delAll(rootData *) {

    // remove other references so we don't get deleted twice!
    this->disconnect();

    //delete this;

}

void projection::delAll(projectObject *) {

    // remove other references so we don't get deleted twice!
    this->disconnect();

    //delete this;

}

QPointF projection::currentLocation()
{
    if (curves.size() > 0) {
        return this->curves.back().end;
    }
    return start;
}

QPointF projection::selectedControlPointLocation()
{
    QPointF rtn(0,0);
    if (this->selectedControlPoint.start == true || this->selectedControlPoint.ind == -1) {
        rtn = this->start;
    } else {
        if (this->selectedControlPoint.type == C1) {
            rtn = this->curves[this->selectedControlPoint.ind].C1;
        } else if (this->selectedControlPoint.type == C2) {
            rtn = this->curves[this->selectedControlPoint.ind].C2;
        } else if (this->selectedControlPoint.type == p_end) {
            rtn = this->curves[this->selectedControlPoint.ind].end;
        } else {
            // error.
        }
    }
    return rtn;
}

void projection::move(float x, float y) {

    if (curves.size() > 1) {

        // move mid points:
        this->curves[0].C2 = (this->curves[0].C2 - this->start) + QPointF(x,y) + locationOffset;
        this->curves[0].end = (this->curves[0].end - this->start) + QPointF(x,y) + locationOffset;

        for (int i = 1; i < this->curves.size() -1; ++i) {

            this->curves[i].C1 = (this->curves[i].C1 - this->start) + QPointF(x,y) + locationOffset;
            this->curves[i].C2 = (this->curves[i].C2 - this->start) + QPointF(x,y) + locationOffset;
            this->curves[i].end = (this->curves[i].end - this->start) + QPointF(x,y) + locationOffset;

        }

    this->curves.back().C1 = (this->curves.back().C1 - this->start) + QPointF(x,y) + locationOffset;
    }


}

void projection::animate(QSharedPointer<systemObject>movingObj, QPointF delta, QSharedPointer<projection>thisSharedPointer) {

    QSharedPointer <population> movingPop;

    if (movingObj->type == populationObject) {
        movingPop = qSharedPointerDynamicCast <population>(movingObj);
    } else {
        qDebug() << "Incorrect object fed to projection animation";
        return;
    }

    // if we are a self connection we get moved twice, so only move half as much each time
    if (!(this->destination.isNull())) {
        if (this->source->name == this->destination->name) {
            delta = delta / 2;
        }
    }

    // crash avoidance
    if (this->curves.size() == 0) {
        qDebug() << "Projection created with no curves or bad access";
        return;
    }

    // source is moving
    if (movingPop->name == this->source->name) {
        this->start = this->start + delta;
        this->curves.front().C1 = this->curves.front().C1 + delta;
    }
    // if destination is set:
    if (!(this->destination.isNull())) {
        // destination is moving
        if (movingPop->name == this->destination->name) {
            this->curves.back().end = this->curves.back().end + delta;
            this->curves.back().C2 = this->curves.back().C2 + delta;

            // update inputs:
            for (int i = 0; i < this->synapses.size(); ++i) {
                for (int j = 0; j < this->synapses[i]->weightUpdateType->inputs.size(); ++j) {
                    this->synapses[i]->weightUpdateType->inputs[j]->animate(thisSharedPointer, delta);
                }
                for (int j = 0; j < this->synapses[i]->postsynapseType->inputs.size(); ++j) {
                    this->synapses[i]->postsynapseType->inputs[j]->animate(thisSharedPointer, delta);
                }
            }
        }

    }

}

void projection::setStyle(drawStyle style) {
    this->projDrawStyle = style;
}

drawStyle projection::style() {
    return this->projDrawStyle;
}

void projection::draw(QPainter *painter, float GLscale, float viewX, float viewY, int width, int height, QImage, drawStyle style) {

    // setup for drawing curves
    this->setupTrans(GLscale, viewX, viewY, width, height);

    // switch if we have standardDrawStyle
    if (style == standardDrawStyle) {
        style = this->projDrawStyle;
    }

    if (this->curves.size() > 0) {

        QColor colour;

        QPen oldPen = painter->pen();

        QPointF start;
        QPointF end;

        switch (style) {
        case microcircuitDrawStyle:
        case spikeSourceDrawStyle:
        {
            if (this->type == projectionObject)
                colour = QColor(0,0,255,255);
            else
                colour = QColor(0,255,0,255);

            if (source != NULL) {
                QLineF temp = QLineF(QPointF(source->x, source->y), this->curves.front().C1);
                temp.setLength(0.501);
                start = temp.p2();
            }
            else
                start = this->start;

            if (destination != NULL) {
                QLineF temp = QLineF(QPointF(destination->x, destination->y), this->curves.back().C2);
                temp.setLength(0.501);
                end = temp.p2();
            }
            else
                end = this->curves.back().end;

            // set pen width
            QPen pen2 = painter->pen();
            pen2.setWidthF((pen2.widthF()+1.0)*GLscale/100.0);
            pen2.setColor(colour);
            painter->setPen(pen2);

            QPainterPath path;

            path.moveTo(this->transformPoint(start));


            for (int i = 0; i < this->curves.size(); ++i) {
                if (this->curves.size()-1 == i)
                    path.cubicTo(this->transformPoint(this->curves[i].C1), this->transformPoint(this->curves[i].C2), this->transformPoint(end));
                else
                    path.cubicTo(this->transformPoint(this->curves[i].C1), this->transformPoint(this->curves[i].C2), this->transformPoint(this->curves[i].end));
            }

            // draw start and end markers

            QPolygonF arrow_head;
            QPainterPath endPoint;
            //calculate arrow head polygon
            QPointF end_point = path.pointAtPercent(1.0);
            QPointF temp_end_point = path.pointAtPercent(0.995);
            QLineF line = QLineF(end_point, temp_end_point).unitVector();
            QLineF line2 = QLineF(line.p2(), line.p1());
            line2.setLength(line2.length()+0.05*GLscale/2.0);
            end_point = line2.p2();
            if (this->type == projectionObject) {
                line.setLength(0.2*GLscale/2.0);
            } else {
                line.setLength(0.1*GLscale/2.0);
            }
            QPointF t = line.p2() - line.p1();
            QLineF normal = line.normalVector();
            normal.setLength(normal.length()*0.8);
            QPointF a1 = normal.p2() + t;
            normal.setLength(-normal.length());
            QPointF a2 = normal.p2() + t;
            arrow_head.clear();
            arrow_head << end_point << a1 << a2 << end_point;
            endPoint.addPolygon(arrow_head);
            painter->fillPath(endPoint, colour);

            // only draw number of synapses for Projections
            if (this->type == projectionObject) {
                QPen pen = painter->pen();
                QVector<qreal> dash;
                dash.push_back(4);
                for (int syn = 1; syn < this->synapses.size(); ++syn) {
                    dash.push_back(2.0);
                    dash.push_back(1.0);
                }
                if (synapses.size() > 1) {
                    dash.push_back(2.0);
                    dash.push_back(1.0);
                    dash.push_back(2.0);
                    pen.setWidthF((pen.widthF()+1.0) * 1.5);
                } else {
                    dash.push_back(0.0);
                }
                dash.push_back(100000.0);

                pen.setDashPattern(dash);
                painter->setPen(pen);

            }

            // DRAW
            painter->drawPath(path);
            painter->setPen(oldPen);

            break;
        }
        case layersDrawStyle:

            return;
        case standardDrawStyleExcitatory:
        {
            if (this->type == projectionObject)
                colour = QColor(0,0,255,255);
            else
                colour = QColor(0,255,0,255);

            start = this->start;
            end = this->curves.back().end;

            // draw end marker

            QSettings settings;
            float dpi_ratio = settings.value("dpi", 1.0).toFloat();

            // account for hidpi in line width
            QPen linePen = painter->pen();
            linePen.setWidthF(linePen.widthF()*dpi_ratio);
            painter->setPen(linePen);

            QPainterPath path;

            // start curve drawing
            path.moveTo(this->transformPoint(start));

            // draw curves
            for (int i = 0; i < this->curves.size(); ++i) {
                if (this->curves.size()-1 == i)
                    path.cubicTo(this->transformPoint(this->curves[i].C1), this->transformPoint(this->curves[i].C2), this->transformPoint(end));
                else
                    path.cubicTo(this->transformPoint(this->curves[i].C1), this->transformPoint(this->curves[i].C2), this->transformPoint(this->curves[i].end));
            }

            QPolygonF arrow_head;
            QPainterPath endPoint;
            //calculate arrow head polygon
            QPointF end_point = path.pointAtPercent(1.0);
            QPointF temp_end_point = path.pointAtPercent(0.995);
            QLineF line = QLineF(end_point, temp_end_point).unitVector();
            QLineF line2 = QLineF(line.p2(), line.p1());
            line2.setLength(line2.length()+0.05*GLscale/2.0);
            end_point = line2.p2();
            if (this->type == projectionObject) {
                line.setLength(0.2*GLscale/2.0);
            } else {
                line.setLength(0.1*GLscale/2.0);
            }
            QPointF t = line.p2() - line.p1();
            QLineF normal = line.normalVector();
            normal.setLength(normal.length()*0.8);
            QPointF a1 = normal.p2() + t;
            normal.setLength(-normal.length());
            QPointF a2 = normal.p2() + t;
            arrow_head.clear();
            arrow_head << end_point << a1 << a2 << end_point;
            endPoint.addPolygon(arrow_head);
            painter->fillPath(endPoint, colour);

            // only draw number of synapses for Projections
            if (this->type == projectionObject) {
                QPen pen = painter->pen();
                QVector<qreal> dash;
                dash.push_back(4);
                for (int syn = 1; syn < this->synapses.size(); ++syn) {
                    dash.push_back(2.0);
                    dash.push_back(1.0);
                }
                if (synapses.size() > 1) {
                    dash.push_back(2.0);
                    dash.push_back(1.0);
                    dash.push_back(2.0);
                    pen.setWidthF((pen.widthF()+1.0) * 1.5);
                } else {
                    dash.push_back(0.0);
                }
                dash.push_back(100000.0);
                dash.push_back(0.0);

                pen.setDashPattern(dash);
                painter->setPen(pen);

            }

            // DRAW
            painter->drawPath(path);
            painter->setPen(oldPen);
            return;
        }
        case standardDrawStyle:
        {
            start = this->start;
            end = this->curves.back().end;

            // draw end marker
            QPainterPath endPoint;

            QSettings settings;
            float dpi_ratio = settings.value("dpi", 1.0).toFloat();

            // account for hidpi in line width
            QPen linePen = painter->pen();
            linePen.setWidthF(linePen.widthF()*dpi_ratio);
            painter->setPen(linePen);

            if (this->type == projectionObject) {
                endPoint.addEllipse(this->transformPoint(this->curves.back().end),0.025*dpi_ratio*GLscale,0.025*dpi_ratio*GLscale);
                painter->drawPath(endPoint);
                painter->fillPath(endPoint, QColor(0,0,255,255));
            }
            else {
                endPoint.addEllipse(this->transformPoint(this->curves.back().end),0.015*dpi_ratio*GLscale,0.015*dpi_ratio*GLscale);
                painter->drawPath(endPoint);
                painter->fillPath(endPoint, QColor(0,210,0,255));
            }

            QPainterPath path;

            // start curve drawing
            path.moveTo(this->transformPoint(start));

            // draw curves
            for (int i = 0; i < this->curves.size(); ++i) {
                if (this->curves.size()-1 == i)
                    path.cubicTo(this->transformPoint(this->curves[i].C1), this->transformPoint(this->curves[i].C2), this->transformPoint(end));
                else
                    path.cubicTo(this->transformPoint(this->curves[i].C1), this->transformPoint(this->curves[i].C2), this->transformPoint(this->curves[i].end));
            }

            // only draw number of synapses for Projections
            if (this->type == projectionObject) {
                QPen pen = painter->pen();
                QVector<qreal> dash;
                dash.push_back(4);
                for (int syn = 1; syn < this->synapses.size(); ++syn) {
                    dash.push_back(2.0);
                    dash.push_back(1.0);
                }
                if (synapses.size() > 1) {
                    dash.push_back(2.0);
                    dash.push_back(1.0);
                    dash.push_back(2.0);
                    pen.setWidthF((pen.widthF()+1.0) * 1.5);
                } else {
                    dash.push_back(0.0);
                }
                dash.push_back(100000.0);
                dash.push_back(0.0);

                pen.setDashPattern(dash);
                painter->setPen(pen);

            }

            // DRAW
            painter->drawPath(path);
            painter->setPen(oldPen);

            break;
        }
        }

    }

}

void projection::drawInputs(QPainter *painter, float GLscale, float viewX, float viewY, int width, int height, QImage ignored, drawStyle style) {

    if (this->destination != (QSharedPointer <population>)0) {
        for (int i = 0; i < this->synapses.size(); ++i) {
            for (int j = 0; j < this->synapses[i]->weightUpdateType->inputs.size(); ++j) {
                this->synapses[i]->weightUpdateType->inputs[j]->draw(painter, GLscale, viewX, viewY, width, height, ignored, style);
            }
            for (int j = 0; j < this->synapses[i]->postsynapseType->inputs.size(); ++j) {
                this->synapses[i]->postsynapseType->inputs[j]->draw(painter, GLscale, viewX, viewY, width, height, ignored, style);
            }
        }
    }

}

void projection::setupTrans(float GLscale, float viewX, float viewY, int width, int height) {

    this->tempTrans.GLscale = GLscale;
    this->tempTrans.viewX = viewX;
    this->tempTrans.viewY = viewY;
    this->tempTrans.width = float(width);
    this->tempTrans.height = float(height);

}

QPointF projection::transformPoint(QPointF point) {

    point.setX(((point.x()+this->tempTrans.viewX)*this->tempTrans.GLscale+this->tempTrans.width)/2);
    point.setY(((-point.y()+this->tempTrans.viewY)*this->tempTrans.GLscale+this->tempTrans.height)/2);
    return point;
}

void projection::drawHandles(QPainter *painter, float GLscale, float viewX, float viewY, int width, int height) {

    if (curves.size()>0) {
        this->setupTrans(GLscale, viewX, viewY, width, height);

        QPainterPath path;
        QPainterPath lines;

        QSettings settings;
        float dpi_ratio = settings.value("dpi", 1.0).toFloat();

#ifdef Q_OS_MAC
    dpi_ratio *= 0.5;
#endif

        path.addEllipse(this->transformPoint(this->start), 4*dpi_ratio, 4*dpi_ratio);
        lines.moveTo(this->transformPoint(this->start));

        for (int i = 0; i < this->curves.size(); ++i) {
            lines.lineTo(this->transformPoint(this->curves[i].C1));
            lines.moveTo(this->transformPoint(this->curves[i].C2));
            lines.lineTo(this->transformPoint(this->curves[i].end));
            path.addEllipse(this->transformPoint(this->curves[i].C1), 4*dpi_ratio, 4*dpi_ratio);
            path.addEllipse(this->transformPoint(this->curves[i].C2), 4*dpi_ratio, 4*dpi_ratio);
            path.addEllipse(this->transformPoint(this->curves[i].end), 4*dpi_ratio, 4*dpi_ratio);
        }

        painter->drawPath(path);
        painter->drawPath(lines);
        path.addEllipse(this->transformPoint(this->curves.back().end),4*dpi_ratio,4*dpi_ratio);
        painter->drawPath(path);
        painter->fillPath(path,QColor(255,0,0,100));

        // redraw selected handle:
        if (this->selectedControlPoint.start) {
            QPainterPath sel;
            sel.addEllipse(this->transformPoint(this->start), 4*dpi_ratio, 4*dpi_ratio);
            painter->drawPath(sel);
            painter->fillPath(sel,QColor(0,255,0,255));
        } else if (this->selectedControlPoint.ind != -1) {
            QPainterPath sel;
            QPointF Transformed;
            switch (this->selectedControlPoint.type) {

            case C1:
                sel.addEllipse(this->transformPoint(this->curves[this->selectedControlPoint.ind].C1), 4*dpi_ratio, 4*dpi_ratio);
                break;

            case C2:
                sel.addEllipse(this->transformPoint(this->curves[this->selectedControlPoint.ind].C2), 4*dpi_ratio, 4*dpi_ratio);
                break;

            case p_end:
                sel.addEllipse(this->transformPoint(this->curves[this->selectedControlPoint.ind].end), 4*dpi_ratio, 4*dpi_ratio);
                break;

            default:
                break;

            }
            painter->drawPath(sel);
            painter->fillPath(sel,QColor(0,255,0,255));

        }
    }

}

QPainterPath projection::makeIntersectionLine(int first, int last) {

    // draw the path to a QPainterPath
    QPainterPath colPath;
    if (first == 0) {
        colPath.moveTo(this->start);
    } else {
        colPath.moveTo(this->curves[first-1].end);
    }

    for (int i = first; i < last; ++i) {
        colPath.cubicTo(this->curves[i].C1, this->curves[i].C2, this->curves[i].end);
    }

    for (int i = last-1; i > first-1; --i) {
        QLineF line(this->curves[i].end, this->curves[i].C2);
        QLineF norm = line.normalVector();
        QLineF unitNorm = norm.unitVector();
        // reset norm as offset
        unitNorm.translate(-unitNorm.p1());
        unitNorm.setLength(0.001);

        if (i == 0) {
            line.setPoints(this->curves[i].C1, this->start);
        } else {
            line.setPoints(this->curves[i].C1, this->curves[i-1].end);
        }
        norm = line.normalVector();
        QLineF unitNormC1 = norm.unitVector();
        // reset norm as offset
        unitNormC1.translate(-unitNormC1.p1());
        unitNormC1.setLength(0.001);
        if (i == 0) {
            colPath.cubicTo(this->curves[i].C2 + unitNorm.p2(), this->curves[i].C1 + unitNormC1.p2(), this->start + unitNorm.p2());
        } else {
            colPath.cubicTo(this->curves[i].C2 + unitNorm.p2(), this->curves[i].C1 + unitNormC1.p2(), this->curves[i-1].end + unitNormC1.p2());
        }
    }

    return colPath;

}

bool projection::is_clicked(float xGL, float yGL, float GLscale) {

    // do an intersection using a QPainterPath to see if we meet:
    QPainterPath colPath = this->makeIntersectionLine(0, this->curves.size());

    // intersect with the cursor
    if (colPath.intersects(QRectF(xGL-10.0/GLscale, yGL-10.0/GLscale, 20.0/GLscale, 20.0/GLscale))) {
        return true;
    }

    return false;

}

bool projection::selectControlPoint(float xGL, float yGL, float GLscale) {

    QPointF cursor(xGL, yGL);

    QSettings settings;
    float dpi_ratio = settings.value("dpi", 1.0).toFloat();

    // test start:
    QPainterPath test;
    test.addEllipse(this->start, 10.0/GLscale*dpi_ratio, 10.0/GLscale*dpi_ratio);
    if (test.contains(cursor)) {
        this->selectedControlPoint.start = true;
        // NB: What happends to selectedControlPoint.ind here?
        return true;
    }

    // now check all the bezierCurves in turn:
    for (int i = 0; i < this->curves.size(); ++i) {
        QPainterPath testEnd;
        testEnd.addEllipse(this->curves[i].end, 10.0/GLscale*dpi_ratio, 10.0/GLscale*dpi_ratio);
        if (testEnd.contains(cursor)) {
            this->selectedControlPoint.start = false;
            this->selectedControlPoint.type = p_end;
            this->selectedControlPoint.ind = i;
            return true;
        }
        QPainterPath testC1;
        testC1.addEllipse(this->curves[i].C1, 10.0/GLscale*dpi_ratio, 10.0/GLscale*dpi_ratio);
        if (testC1.contains(cursor)) {
            this->selectedControlPoint.start = false;
            this->selectedControlPoint.type = C1;
            this->selectedControlPoint.ind = i;
            return true;
        }
        QPainterPath testC2;
        testC2.addEllipse(this->curves[i].C2, 10.0/GLscale*dpi_ratio, 10.0/GLscale*dpi_ratio);
        if (testC2.contains(cursor)) {
            this->selectedControlPoint.start = false;
            this->selectedControlPoint.type = C2;
            this->selectedControlPoint.ind = i;
            return true;
        }
    }

    return false;
}

bool projection::deleteControlPoint(float xGL, float yGL, float GLscale) {
    // check if we are over a control point
    if (this->selectControlPoint(xGL, yGL, GLscale)) {

        // then remove it if it is not the first or last, or a C1 or C2
        if (!this->selectedControlPoint.start && this->selectedControlPoint.type != C1 \
                && this->selectedControlPoint.type != C2 && this->selectedControlPoint.ind != (int) this->curves.size()-1) {

            // first transfer the old C1 to the next curve:
            this->curves[this->selectedControlPoint.ind+1].C1 = this->curves[this->selectedControlPoint.ind].C1;

            // the remove:
            this->curves.erase(this->curves.begin()+this->selectedControlPoint.ind);

            // deleted!
            return true;
        }

    }

    // nothing deleted!
    return false;

}

void projection::moveSelectedControlPoint(float xGL, float yGL) {

    // convert to QPointF
    QPointF cursor(xGL, yGL);

    // move start
    if (this->selectedControlPoint.start) {

        // if source is a spike source
        if (source->isSpikeSource) {
            QLineF temp = QLineF(QPointF(source->x, source->y), cursor);
            // move source
            temp.setLength(0.501);
            start = temp.p2();
            QLineF C1handle(this->curves.front().C1, QPointF(source->x, source->y));
            float len = C1handle.length();
            temp.setLength(len);
            this->curves.front().C1 = temp.p2();
            return;
        }

        // work out closest point on edge of source population
        QLineF line(QPointF(this->source->x, this->source->y), cursor);
        QLineF nextLine = line.unitVector();
        nextLine.setLength(1000.0);
        QPointF point = nextLine.p2();

        QPointF boxEdge = this->findBoxEdge(this->source, point.x(), point.y());

        // realign the handle
        QLineF handle(QPointF(this->source->x, this->source->y), this->curves.front().C1);
        handle.setAngle(nextLine.angle());
        this->curves.front().C1 = handle.p2();

        // move the point
        this->start = boxEdge;
        return;
    }
    // move other controls
    else if (this->selectedControlPoint.ind != -1) {
        // move end point
        if (this->selectedControlPoint.ind == (int) this->curves.size()-1 && this->selectedControlPoint.type == p_end) {

            // work out closest point on edge of destination population
            QLineF line(QPointF(this->destination->x, this->destination->y), cursor);
            QLineF nextLine = line.unitVector();
            nextLine.setLength(1000.0);
            QPointF point = nextLine.p2();

            QPointF boxEdge = this->findBoxEdge(this->destination, point.x(), point.y());

            // realign the handle
            QLineF handle(QPointF(this->destination->x, this->destination->y), this->curves.back().C2);
            handle.setAngle(nextLine.angle());
            this->curves.back().C2 = handle.p2();

            // move the point
            this->curves.back().end = boxEdge;

            // move the inputs attached to the proj
            for (int i = 0; i < this->synapses.size(); ++i) {
                for (int j = 0; j < this->synapses[i]->weightUpdateType->inputs.size(); ++j) {
                    if (this->synapses[i]->weightUpdateType->inputs[j]->curves.size() > 0)
                        this->synapses[i]->weightUpdateType->inputs[j]->curves.back().end = this->curves.back().end;
                }
                for (int j = 0; j < this->synapses[i]->postsynapseType->inputs.size(); ++j) {
                    if (this->synapses[i]->postsynapseType->inputs[j]->curves.size() > 0)
                        this->synapses[i]->postsynapseType->inputs[j]->curves.back().end = this->curves.back().end;
                }
            }

            return;
        }

        // move other points
        switch (this->selectedControlPoint.type) {

        case C1:
            this->curves[this->selectedControlPoint.ind].C1 = cursor;
            break;

        case C2:
            this->curves[this->selectedControlPoint.ind].C2 = cursor;
            break;

        case p_end:
            // move control points either side as well
            this->curves[this->selectedControlPoint.ind+1].C1 = cursor - (this->curves[this->selectedControlPoint.ind].end - this->curves[this->selectedControlPoint.ind+1].C1);
            this->curves[this->selectedControlPoint.ind].C2 = cursor - (this->curves[this->selectedControlPoint.ind].end - this->curves[this->selectedControlPoint.ind].C2);
            this->curves[this->selectedControlPoint.ind].end = cursor;
            break;

        default:
            break;

        }
    }

}

void projection::insertControlPoint(float xGL, float yGL, float GLscale) {

    // convert to QPointF
    QPointF cursor(xGL, yGL);

    // do an intersection using a QPainterPath to see if, and where, we meet:

    for (int i = 0; i < (int) this->curves.size(); ++i) {
        QPainterPath segPath = this->makeIntersectionLine(i, i+1);

        // intersect with the cursor
        if (segPath.intersects(QRectF(xGL-10.0/GLscale, yGL-10.0/GLscale, 20.0/GLscale, 20.0/GLscale))) {

            // add a segment at cursor
            // move the old C1 to the new bezierCurve:
            bezierCurve newCurve;
            newCurve.C1 = this->curves[i].C1;
            // add a new C2 and end, and a new C1 to the old bezierCurve above
            newCurve.end = cursor;
            QLineF toStart;
            if (i == 0) {toStart.setPoints(cursor, this->start);} else {toStart.setPoints(cursor, this->curves[i-1].end);}
            QLineF toEnd(cursor, this->curves[i].end);
            toStart.setLength(0.3);
            toEnd.setLength(0.3);
            newCurve.C2 = toStart.p2();
            this->curves[i].C1 = toEnd.p2();
            this->curves.insert(this->curves.begin()+i, newCurve);
            return;
        }

    }


}

QPointF projection::findBoxEdge(QSharedPointer <population> pop, float xGL, float yGL) {

    float newX;
    float newY;

    // if spike source do differently
    if (pop->isSpikeSource) {

        QLineF temp = QLineF(QPointF(pop->x, pop->y), QPointF(xGL, yGL));
        // move source
        temp.setLength(0.501);
        newX = temp.p2().x();
        newY = temp.p2().y();

    } else {

        QPainterPath box;
        box = *pop->addToPath(&box);
        QPainterPath line;
        line.moveTo(pop->targx, pop->targy);
        line.lineTo(xGL, yGL);
        line.lineTo(xGL+0.01, yGL+0.01);
        QPainterPath overlap = line & box;
        if (overlap.isEmpty())
        {cerr << "oops! Collision not found";
            newX = 0;
            newY = 0;
        } else {
            // get the last point of the intersection
            newX = overlap.elementAt(1).x;
            newY = overlap.elementAt(1).y;
        }
    }

    return QPointF(newX, newY);

}

void projection::setAutoHandles(QSharedPointer <population> pop1, QSharedPointer <population> pop2, QPointF end) {

    // line for drawing handles
    QPainterPath startToEnd;
    QPointF oldEnd;
    if (this->curves.size() > 1) {
        startToEnd.moveTo(curves[curves.size()-2].end);
        oldEnd = curves[curves.size()-2].end;
    } else {
        startToEnd.moveTo(this->start);
        oldEnd = this->start;
    }
    startToEnd.lineTo(end);

    // setup handles
    if (this->curves.size() > 1) {
        QPointF oldHandle = this->curves[curves.size()-2].C2;
        QLineF line(oldEnd, oldEnd + (oldEnd - oldHandle));
        line.setLength(startToEnd.length()/2);
        this->curves.back().C1 = line.p2();
    } else {
        if (this->start.x() >= pop1->getRight() - 0.001 || this->start.x() <= pop1->getLeft() + 0.001) {
            this->curves.back().C1 = QPointF(startToEnd.pointAtPercent(0.5).x(), this->start.y());
        } else {
            this->curves.back().C1 = QPointF(this->start.x(), startToEnd.pointAtPercent(0.5).y());
        }
    }

    // see if we have a destination draw handles for
    if (pop2 != this->source) {
        if (this->curves.back().end.x() >= pop2->getRight() - 0.001 || this->curves.back().end.x() <= pop2->getLeft() + 0.001) {
            this->curves.back().C2 = QPointF(startToEnd.pointAtPercent(0.5).x(), this->curves.back().end.y());
        } else {
            this->curves.back().C2 = QPointF(this->curves.back().end.x(), startToEnd.pointAtPercent(0.5).y());
        }
    } else {
        this->curves.back().C2 = startToEnd.pointAtPercent(0.5);
    }
}

QString projection::getName() {
    if (source != NULL && destination != NULL)
        return this->source->getName() + " to " + this->destination->getName();
    else
        return "Disconnected projection";
}

void projection::write_model_meta_xml(QDomDocument &meta, QDomElement &root) {

    // write a new element for this projection:
    QDomElement col = meta.createElement( "projection" );
    root.appendChild(col);
    col.setAttribute("source", this->source->name);
    col.setAttribute("destination", this->destination->name);
    col.setAttribute("style", QString::number(this->projDrawStyle));

    // start position
    QDomElement start = meta.createElement( "start" );
    col.appendChild(start);
    start.setAttribute("x", this->start.x());
    start.setAttribute("y", this->start.y());

    // bezierCurves
    QDomElement curves = meta.createElement( "curves" );
    col.appendChild(curves);

    for (int i = 0; i < this->curves.size(); ++i) {

        QDomElement curve = meta.createElement( "curve" );
        QDomElement C1 = meta.createElement( "C1" );
        C1.setAttribute("xpos", this->curves[i].C1.x());
        C1.setAttribute("ypos", this->curves[i].C1.y());
        curve.appendChild(C1);

        QDomElement C2 = meta.createElement( "C2" );
        C2.setAttribute("xpos", this->curves[i].C2.x());
        C2.setAttribute("ypos", this->curves[i].C2.y());
        curve.appendChild(C2);

        QDomElement end = meta.createElement( "end" );
        end.setAttribute("xpos", this->curves[i].end.x());
        end.setAttribute("ypos", this->curves[i].end.y());
        curve.appendChild(end);

        curves.appendChild(curve);

    }

    // write out connection metadata
    for (int i = 0; i < synapses.size(); ++i) {

        // write container (name after the weight update)
        QDomElement c = meta.createElement( "synapseConnection" );
        c.setAttribute( "name", synapses[i]->weightUpdateType->getXMLName() );

        // add the metadata description (if there is one)
        synapses[i]->connectionType->write_metadata_xml(meta, c);

        col.appendChild(c);

    }

    // write inputs out
    for (int i = 0; i < synapses.size(); ++i) {

        for (int j = 0; j < synapses[i]->weightUpdateType->inputs.size(); ++j) {

            synapses[i]->weightUpdateType->inputs[j]->write_model_meta_xml(meta, root);

        }
        for (int j = 0; j < synapses[i]->postsynapseType->inputs.size(); ++j) {

            synapses[i]->postsynapseType->inputs[j]->write_model_meta_xml(meta, root);

        }

    }

}

void projection::readFromXML(QDomElement  &e, QDomDocument *, QDomDocument * meta, projectObject * data, QSharedPointer<projection> thisSharedPointer) {

    this->type = projectionObject;

    this->selectedControlPoint.ind = -1;
    this->selectedControlPoint.start = false;

    // take the given node element and begin extracting the data:

    QString srcName;
    QString destName;
    QDomNodeList nrn = e.parentNode().toElement().elementsByTagName("LL:Neuron");

    // get src name
    if (nrn.size() == 1) {
        srcName = nrn.item(0).toElement().attribute("name");
        // this must exist as the population has been loaded successfully by this point so no error check
    }

    if (nrn.size() == 1) {
        destName = e.attribute("dst_population");
        if (destName == "") {
            QSettings settings;
            int num_errs = settings.beginReadArray("errors");
            settings.endArray();
            settings.beginWriteArray("errors");
                settings.setArrayIndex(num_errs + 1);
                settings.setValue("errorText",  "XML error: missing Projection attribute 'dst_population'");
            settings.endArray();
        }
    }

    this->source.clear();
    this->destination.clear();

    // link up src and dest
    bool linked = false;
    for (int i = 0; i < data->network.size(); ++i) {
        //qDebug() << data->network[i]->name << srcName;
        if (data->network[i]->name == srcName) {
            this->source = data->network[i];
            linked = true;
        }
    }
    if (!linked) {
        QSettings settings;
        int num_errs = settings.beginReadArray("errors");
        settings.endArray();
        settings.beginWriteArray("errors");
            settings.setArrayIndex(num_errs + 1);
            settings.setValue("errorText",  "Error: Projection references missing source '" + srcName + "'");
        settings.endArray();
        return;
    }

    linked = false;
    for (int i = 0; i < data->network.size(); ++i) {
        if (data->network[i]->name == destName) {
            this->destination = data->network[i];
            linked = true;
        }
    }
    if (!linked) {
        QSettings settings;
        int num_errs = settings.beginReadArray("errors");
        settings.endArray();
        settings.beginWriteArray("errors");
            settings.setArrayIndex(num_errs + 1);
            settings.setValue("errorText",  "Error: Projection references missing destination '" + destName + "'");
        settings.endArray();
        return;
    }

    // add reverse projection
    this->destination->reverseProjections.push_back(thisSharedPointer);

    this->currTarg = 0;

    // load the synapses:

    QDomNodeList colList = e.elementsByTagName("LL:Synapse");

    if (colList.count() == 0) {
        QSettings settings;
        int num_errs = settings.beginReadArray("errors");
        settings.endArray();
        settings.beginWriteArray("errors");
            settings.setArrayIndex(num_errs + 1);
            settings.setValue("errorText",  "XML error: Projection contains no Synapse tags");
        settings.endArray();
        return;
    }

    for (int i = 0; i < (int) colList.count(); ++i) {
        // create a new Synapse on the projection
        QSharedPointer <synapse> newSynapse = QSharedPointer<synapse>(new synapse(thisSharedPointer, data, true)); // add bool to avoid adding the projInputs - we need to do that later
        QString pspName;
        QString synName;
        QDomNode n = colList.item(i).toElement().firstChild();
        while (!n.isNull()) {

            // get connectivity

            if (n.toElement().tagName() == "AllToAllConnection") {
                delete newSynapse->connectionType;
                newSynapse->connectionType = new alltoAll_connection;
                newSynapse->connectionType->import_parameters_from_xml(n);
            }
            else if (n.toElement().tagName() == "OneToOneConnection") {
                delete newSynapse->connectionType;
                newSynapse->connectionType = new onetoOne_connection;
                newSynapse->connectionType->import_parameters_from_xml(n);
            }
            else if (n.toElement().tagName() == "FixedProbabilityConnection") {
                delete newSynapse->connectionType;
                newSynapse->connectionType = new fixedProb_connection;
                newSynapse->connectionType->import_parameters_from_xml(n);
            }
            else if (n.toElement().tagName() == "ConnectionList") {
                delete newSynapse->connectionType;
                newSynapse->connectionType = new csv_connection;
                newSynapse->connectionType->import_parameters_from_xml(n);
            }
            else if (n.toElement().tagName() == "KernelConnection") {
                // fixme: fix this
                delete newSynapse->connectionType;
                newSynapse->connectionType = new kernel_connection;
                newSynapse->connectionType->import_parameters_from_xml(n);
                ((kernel_connection *) newSynapse->connectionType)->src = (QSharedPointer <population>) this->source;
                ((kernel_connection *) newSynapse->connectionType)->dst = (QSharedPointer <population>) this->destination;
            }
            else if (n.toElement().tagName() == "PythonScriptConnection") {

            }

            else if (n.toElement().tagName() == "LL:PostSynapse") {

                // get postsynapse component name
                pspName = n.toElement().attribute("url");
                QString real_url = pspName;
                if (pspName == "") {
                    QSettings settings;
                    int num_errs = settings.beginReadArray("errors");
                    settings.endArray();
                    settings.beginWriteArray("errors");
                        settings.setArrayIndex(num_errs + 1);
                        settings.setValue("errorText",  "XML error: Missing PostSynapse 'url' attribute");
                    settings.endArray();
                    return;
                }
                QStringList tempName = pspName.split('.');
                // first section will hold the name
                if (tempName.size() > 0)
                    pspName = tempName[0];
                pspName.replace("_", " ");

                newSynapse->postsynapseType.clear();

                // see if PS is loaded
                for (int u = 0; u < data->catalogPS.size(); ++u) {
                    if (data->catalogPS[u]->name == pspName) {
                        newSynapse->postsynapseType = QSharedPointer<NineMLComponentData> (new NineMLComponentData(data->catalogPS[u]));
                        newSynapse->postsynapseType->owner = thisSharedPointer;
                        newSynapse->postsynapseType->import_parameters_from_xml(n);
                        break;
                    }
                }

                // if still missing then we have an issue
                if (newSynapse->postsynapseType.isNull()) {
                    newSynapse->postsynapseType = QSharedPointer<NineMLComponentData> (new NineMLComponentData(data->catalogPS[0]));
                    newSynapse->postsynapseType->owner = thisSharedPointer;
                    QSettings settings;
                    int num_errs = settings.beginReadArray("warnings");
                    settings.endArray();
                    settings.beginWriteArray("warnings");
                        settings.setArrayIndex(num_errs + 1);
                        settings.setValue("warnText",  "Network references missing Component '" + pspName + "'");
                    settings.endArray();
                }

            }
            else if (n.toElement().tagName() == "LL:WeightUpdate") {

                // get postsynapse component name
                synName = n.toElement().attribute("url");
                QString real_url = synName;
                if (synName == "") {
                    QSettings settings;
                    int num_errs = settings.beginReadArray("errors");
                    settings.endArray();
                    settings.beginWriteArray("errors");
                        settings.setArrayIndex(num_errs + 1);
                        settings.setValue("errorText",  "XML error: Missing WeightUpdate 'url' attribute");
                    settings.endArray();
                    return;
                }
                QStringList tempName = synName.split('.');
                // first section will hold the name
                if (tempName.size() > 0)
                    synName = tempName[0];
                synName.replace("_", " ");

                newSynapse->weightUpdateType.clear();

                // see if WU loaded
                for (int u = 0; u < data->catalogWU.size(); ++u) {
                    if (data->catalogWU[u]->name == synName) {
                        newSynapse->weightUpdateType = QSharedPointer<NineMLComponentData> (new NineMLComponentData(data->catalogWU[u]));
                        newSynapse->weightUpdateType->owner = thisSharedPointer;
                        newSynapse->weightUpdateType->import_parameters_from_xml(n);
                        break;
                    }
                }

                // if still missing then we have a load error
                if (newSynapse->weightUpdateType.isNull()) {
                    newSynapse->weightUpdateType = QSharedPointer<NineMLComponentData> (new NineMLComponentData(data->catalogWU[0]));
                    newSynapse->weightUpdateType->owner = thisSharedPointer;
                    QSettings settings;
                    int num_errs = settings.beginReadArray("warnings");
                    settings.endArray();
                    settings.beginWriteArray("warnings");
                        settings.setArrayIndex(num_errs + 1);
                        settings.setValue("warnText",  "Network references missing Component '" + synName + "'");
                    settings.endArray();
                }

            } else {
                QSettings settings;
                int num_errs = settings.beginReadArray("errors");
                settings.endArray();
                settings.beginWriteArray("errors");
                    settings.setArrayIndex(num_errs + 1);
                    settings.setValue("errorText",  "XML error: misplaced or unknown tag '" + n.toElement().tagName() + "'");
                settings.endArray();
            }
        n = n.nextSibling();
        }
        // add the synapse
        this->synapses.push_back(newSynapse);

    }

    // now load the metadata for the projection:
    QDomNode metaNode = meta->documentElement().firstChild();

    while(!metaNode.isNull()) {

        if (metaNode.toElement().attribute("source", "") == this->source->name && metaNode.toElement().attribute("destination", "") == this->destination->name) {

            this->projDrawStyle = (drawStyle) metaNode.toElement().attribute("style", QString::number(standardDrawStyleExcitatory)).toUInt();

            QDomNode metaData = metaNode.toElement().firstChild();
            while (!metaData.isNull()) {

                if (metaData.toElement().tagName() == "start") {
                    this->start = QPointF(metaData.toElement().attribute("x","").toFloat(), metaData.toElement().attribute("y","").toFloat());
                }

                // find the curves tag
                if (metaData.toElement().tagName() == "curves") {

                    // add each curve
                    QDomNodeList edgeNodeList = metaData.toElement().elementsByTagName("curve");
                    for (int i = 0; i < (int) edgeNodeList.count(); ++i) {
                        QDomNode vals = edgeNodeList.item(i).toElement().firstChild();
                        bezierCurve newCurve;
                        while (!vals.isNull()) {
                            if (vals.toElement().tagName() == "C1") {
                                newCurve.C1 = QPointF(vals.toElement().attribute("xpos").toFloat(), vals.toElement().attribute("ypos").toFloat());
                            }
                            if (vals.toElement().tagName() == "C2") {
                                newCurve.C2 = QPointF(vals.toElement().attribute("xpos").toFloat(), vals.toElement().attribute("ypos").toFloat());
                            }
                            if (vals.toElement().tagName() == "end") {
                                newCurve.end = QPointF(vals.toElement().attribute("xpos").toFloat(), vals.toElement().attribute("ypos").toFloat());
                            }

                            vals = vals.nextSibling();
                        }
                        // add the filled out curve to the list
                        this->curves.push_back(newCurve);
                    }

                }

                // find tags for connection generators
                if (metaData.toElement().tagName() == "synapseConnection") {

                    // extract the name from the tag
                    QString synapseName = metaData.toElement().attribute("name", "");

                    // if we are not an empty node
                    if (!metaData.firstChildElement().isNull()) {

                        for (int i = 0; i < this->synapses.size(); ++i) {
                             // check if we have the current node
                            if (synapseName == this->synapses[i]->weightUpdateType->getXMLName()) {
                                // A synapse was found!
                                // add connection generator if we are a csv
                                if (this->synapses[i]->connectionType->type == CSV) {
                                    csv_connection * conn = (csv_connection *) this->synapses[i]->connectionType;
                                    // add generator
                                    conn->generator = new pythonscript_connection(this->source, this->destination, conn);
                                    // extract data for connection generator
                                    ((pythonscript_connection *) conn->generator)->read_metadata_xml(metaData);
                                    // prevent regeneration
                                    ((pythonscript_connection *) conn->generator)->setUnchanged(true);
                                }
                            }
                        }
                    }
                }

                metaData = metaData.nextSibling();
            }

        }

        metaNode = metaNode.nextSibling();
    }


        this->print();

}

void projection::add_curves() {

    // add sensible curves
    // add curves for drawing:
    bezierCurve newCurve;
    newCurve.end = destination->currentLocation();
    this->start = source->currentLocation();

    newCurve.C1 = 0.5*(destination->currentLocation()+source->currentLocation()) + QPointF(float(rand() % 100)/200.0,float(rand() % 100)/200.0);
    newCurve.C2 = 0.5*(destination->currentLocation()+source->currentLocation()) + QPointF(float(rand() % 100)/200.0,float(rand() % 100)/200.0);

    this->curves.push_back(newCurve);

    // source

    // if we are from a population to a projection and the pop is the Synapse of the proj, handle differently for aesthetics
    QPointF boxEdge = this->findBoxEdge(this->source, destination->currentLocation().x(), destination->currentLocation().y());
    this->start = boxEdge;

    // destination

    boxEdge = this->findBoxEdge(this->destination, source->currentLocation().x(), source->currentLocation().y());
    this->curves.back().end = boxEdge;

    // self connection aesthetics
    if (this->destination == this->source) {

        QPointF boxEdge = this->findBoxEdge(this->destination, this->destination->currentLocation().x(), 1000000.0);
        this->curves.back().end = boxEdge;
        boxEdge = this->findBoxEdge(this->source, 1000000.0, 1000000.0);
        this->start = boxEdge;
        this->curves.back().C1 = QPointF(this->destination->currentLocation().x()+1.0, this->destination->currentLocation().y()+1.0);
        this->curves.back().C2 = QPointF(this->destination->currentLocation().x(), this->destination->currentLocation().y()+1.4);

    }

}


void projection::read_inputs_from_xml(QDomElement  &e, QDomDocument * meta, projectObject * data, QSharedPointer<projection> thisSharedPointer) {

    // load the inputs:
    QDomNodeList colList = e.elementsByTagName("LL:Synapse");

    if (colList.count() != this->synapses.size()) {
        // oh dear, something has gone badly wrong
        qDebug() << "Size mismatch " << colList.count() << " != " << this->synapses.size();
    }

    for (int t = 0; t < (int) colList.count(); ++t) {
        QDomNode n = colList.item(t).toElement().firstChild();
        while (!n.isNull()) {

            // postsynapse inputs
            if (n.toElement().tagName() == "LL:PostSynapse") {

                // generic inputs
                QDomNodeList nList = n.toElement().elementsByTagName("LL:Input");
                QDomElement e2;

                for (int i = 0; i < (int) nList.size(); ++i) {
                    e2 = nList.item(i).toElement();

                    QSharedPointer<genericInput> newInput = QSharedPointer<genericInput> (new genericInput);
                    newInput->src = (QSharedPointer <NineMLComponentData>)0;
                    newInput->dst = (QSharedPointer <NineMLComponentData>)0;
                    newInput->destination = thisSharedPointer;
                    newInput->projInput = false;

                    // read in and locate src:
                    QString srcName = e2.attribute("src");

                    for (int i = 0; i < data->network.size(); ++i) {
                        if (data->network[i]->neuronType->getXMLName() == srcName) {
                            newInput->src = data->network[i]->neuronType;
                            newInput->source = data->network[i];
                        }
                        for (int j = 0; j < data->network[i]->projections.size(); ++j) {
                            for (int k = 0; k < data->network[i]->projections[j]->synapses.size(); ++k) {
                                if (data->network[i]->projections[j]->synapses[k]->weightUpdateType->getXMLName() == srcName) {
                                    newInput->src  = data->network[i]->projections[j]->synapses[k]->weightUpdateType;
                                    newInput->source = data->network[i]->projections[j];
                                }
                                if (data->network[i]->projections[j]->synapses[k]->postsynapseType->getXMLName() == srcName) {
                                    newInput->src  = data->network[i]->projections[j]->synapses[k]->postsynapseType;
                                    newInput->source = data->network[i]->projections[j];
                                }
                            }
                        }
                    }

                    // read in port names
                    newInput->srcPort = e2.attribute("src_port");
                    newInput->dstPort = e2.attribute("dst_port");


                    // get connectivity
                    QDomNodeList type = e2.elementsByTagName("AllToAllConnection");
                    if (type.count() == 1) {
                        delete newInput->connectionType;
                        newInput->connectionType = new alltoAll_connection;
                        QDomNode cNode = type.item(0);
                        newInput->connectionType->import_parameters_from_xml(cNode);
                    }
                    type = e2.elementsByTagName("OneToOneConnection");
                    if (type.count() == 1) {
                        delete newInput->connectionType;
                        newInput->connectionType = new onetoOne_connection;
                        QDomNode cNode = type.item(0);
                        newInput->connectionType->import_parameters_from_xml(cNode);
                    }
                    type = e2.elementsByTagName("FixedProbabilityConnection");
                    if (type.count() == 1) {
                        delete newInput->connectionType;
                        newInput->connectionType = new fixedProb_connection;
                        QDomNode cNode = type.item(0);
                        newInput->connectionType->import_parameters_from_xml(cNode);
                    }
                    type = e2.elementsByTagName("ConnectionList");
                    if (type.count() == 1) {
                        delete newInput->connectionType;
                        newInput->connectionType = new csv_connection;
                        QDomNode cNode = type.item(0);
                        newInput->connectionType->import_parameters_from_xml(cNode);
                    }


                    if (newInput->src != (QSharedPointer <NineMLComponentData>)0)
                    {this->synapses[t]->postsynapseType->inputs.push_back(newInput);
                        newInput->dst = this->synapses[t]->postsynapseType;
                        newInput->src->outputs.push_back(newInput);}
                    else {}
                        // ERRR

                }

                // read in the postsynapse Input
                QSharedPointer<genericInput> newInput = QSharedPointer<genericInput> (new genericInput);
                newInput->src = this->synapses[t]->weightUpdateType;
                newInput->projInput = true;

                // read the ports
                newInput->srcPort = n.toElement().attribute("input_src_port");
                newInput->dstPort = n.toElement().attribute("input_dst_port");

                // setup dst
                newInput->dst = this->synapses[t]->postsynapseType;
                this->synapses[t]->postsynapseType->inputs.push_back(newInput);

                // setup source and destination
                newInput->source = thisSharedPointer;
                newInput->destination = thisSharedPointer;

                // add output in Synapse:
                this->synapses[t]->weightUpdateType->outputs.push_back(newInput);

                // match inputs if not specified:
                newInput->dst->matchPorts();


                // read in the postsynapse output
                newInput = QSharedPointer<genericInput> (new genericInput);
                newInput->src = this->synapses[t]->postsynapseType;
                newInput->projInput = true;

                // read the ports
                newInput->srcPort = n.toElement().attribute("output_src_port");
                newInput->dstPort = n.toElement().attribute("output_dst_port");

                //setup dst
                newInput->dst = this->destination->neuronType;
                this->destination->neuronType->inputs.push_back(newInput);

                // setup source and destination
                newInput->source = thisSharedPointer;
                newInput->destination = this->destination;

                // add to src output list
                this->synapses[t]->postsynapseType->outputs.push_back(newInput);

            }

            // synapse inputs
            if (n.toElement().tagName() == "LL:WeightUpdate") {

                // generic inputs
                QDomNodeList nList = n.toElement().elementsByTagName("LL:Input");
                QDomElement e2;

                for (int i = 0; i < (int) nList.size(); ++i) {
                    e2 = nList.item(0).toElement();

                    QSharedPointer<genericInput> newInput = QSharedPointer<genericInput> (new genericInput);
                    newInput->src = (QSharedPointer <NineMLComponentData>)0;
                    newInput->destination = thisSharedPointer;
                    newInput->projInput = false;

                    // read in and locate src:
                    QString srcName = e2.attribute("src");

                    for (int i = 0; i < data->network.size(); ++i) {
                        if (data->network[i]->neuronType->getXMLName() == srcName) {
                            newInput->src = data->network[i]->neuronType;
                            newInput->source = data->network[i];
                        }
                        for (int j = 0; j < data->network[i]->projections.size(); ++j) {
                            for (int k = 0; k < data->network[i]->projections[j]->synapses.size(); ++k) {
                                if (data->network[i]->projections[j]->synapses[k]->weightUpdateType->getXMLName() == srcName) {
                                    newInput->src  = data->network[i]->projections[j]->synapses[k]->weightUpdateType;
                                    newInput->source = data->network[i]->projections[j];
                                }
                                if (data->network[i]->projections[j]->synapses[k]->postsynapseType->getXMLName() == srcName) {
                                    newInput->src  = data->network[i]->projections[j]->synapses[k]->postsynapseType;
                                    newInput->source = data->network[i]->projections[j];
                                }
                            }
                        }
                    }

                    // read in port names
                    newInput->srcPort = e2.attribute("src_port");
                    newInput->dstPort = e2.attribute("dst_port");

                    // get connectivity
                    QDomNodeList type = e2.elementsByTagName("AllToAllConnection");
                    if (type.count() == 1) {
                        delete newInput->connectionType;
                        newInput->connectionType = new alltoAll_connection;
                        QDomNode cNode = type.item(0);
                        newInput->connectionType->import_parameters_from_xml(cNode);
                    }
                    type = e2.elementsByTagName("OneToOneConnection");
                    if (type.count() == 1) {
                        delete newInput->connectionType;
                        newInput->connectionType = new onetoOne_connection;
                        QDomNode cNode = type.item(0);
                        newInput->connectionType->import_parameters_from_xml(cNode);
                    }
                    type = e2.elementsByTagName("FixedProbabilityConnection");
                    if (type.count() == 1) {
                        delete newInput->connectionType;
                        newInput->connectionType = new fixedProb_connection;
                        QDomNode cNode = type.item(0);
                        newInput->connectionType->import_parameters_from_xml(cNode);
                    }

                    if (newInput->src != (QSharedPointer <NineMLComponentData>)0)
                    {this->synapses[t]->weightUpdateType->inputs.push_back(newInput);
                        newInput->dst = this->synapses[t]->weightUpdateType;
                        newInput->src->outputs.push_back(newInput);}
                    else {}
                        // ERRR

                }

                // read in the synapseInput
                QSharedPointer<genericInput> newInput = QSharedPointer<genericInput> (new genericInput);
                newInput->src = this->source->neuronType;
                newInput->projInput = true;

                // read in ports
                newInput->srcPort = n.toElement().attribute("input_src_port");
                newInput->dstPort = n.toElement().attribute("input_dst_port");

                // read in dst
                newInput->dst = this->synapses[t]->weightUpdateType;
                this->synapses[t]->weightUpdateType->inputs.push_back(newInput);

                // setup source and destination
                newInput->source = this->source;
                newInput->destination = thisSharedPointer;

                newInput->src->outputs.push_back(newInput);

            }
            n = n.nextSibling();
        }

        // do matchPorts()
        this->synapses[t]->weightUpdateType->matchPorts();
        this->synapses[t]->postsynapseType->matchPorts();
    }

    // load metadata (curves etc...):
    for (int i = 0; i < synapses.size(); ++i) {

        for (int j = 0; j < synapses[i]->weightUpdateType->inputs.size(); ++j) {

            synapses[i]->weightUpdateType->inputs[j]->read_meta_data(meta);
            synapses[i]->weightUpdateType->inputs[j]->dst->matchPorts();

        }
        for (int j = 0; j < synapses[i]->postsynapseType->inputs.size(); ++j) {

            synapses[i]->postsynapseType->inputs[j]->read_meta_data(meta);
            synapses[i]->postsynapseType->inputs[j]->dst->matchPorts();

        }

    }

}

QSharedPointer < systemObject > projection::newFromExisting(QMap <systemObject *, QSharedPointer <systemObject> > &objectMap)
{

    // create a new, identical, projection

    QSharedPointer <projection> newProj = QSharedPointer <projection>(new projection());

    newProj->type = projectionObject;

    newProj->destination = this->destination;
    newProj->source = this->source;

    newProj->currTarg =  this->currTarg;
    newProj->start = this->start;
    newProj->curves = this->curves;

    newProj->tempTrans.GLscale = this->tempTrans.GLscale;
    newProj->tempTrans.height = this->tempTrans.height;
    newProj->tempTrans.width = this->tempTrans.width;
    newProj->tempTrans.viewX = this->tempTrans.viewX;
    newProj->tempTrans.viewY = this->tempTrans.viewY;

    newProj->selectedControlPoint.ind = -1;
    newProj->selectedControlPoint.start = false;
    newProj->selectedControlPoint.type = C1;

    newProj->projDrawStyle = this->projDrawStyle;

    newProj->srcPos = this->srcPos;
    newProj->dstPos = this->dstPos;

    // now do the synapses
    for (int i = 0; i < this->synapses.size(); ++i) {
        newProj->synapses.push_back(qSharedPointerCast <synapse> (this->synapses[i]->newFromExisting(objectMap)));
        newProj->synapses.back()->proj = newProj;
    }

    objectMap.insert(this, newProj);

    return qSharedPointerCast <systemObject> (newProj);

}

void projection::remapSharedPointers(QMap <systemObject *, QSharedPointer <systemObject> > objectMap)
{

    // remap src and dst:
    this->source = qSharedPointerDynamicCast <population> (objectMap[this->source.data()]);
    this->destination = qSharedPointerDynamicCast <population> (objectMap[this->destination.data()]);

    if (!this->source || !this->destination) {
        qDebug() << "Error casting objectMap lookup to population in projection::remapSharedPointers";
        exit(-1);
    }

    // now do the synapses
    for (int i = 0; i < this->synapses.size(); ++i) {
        this->synapses[i]->remapSharedPointers(objectMap);
    }


}


void projection::print() {

    std::cerr << "\n";
    cerr << "   " << this->getName().toStdString() << " ####\n";
    std::cerr << "   " <<  float(this->currTarg) << "\n";
    std::cerr << "   " <<  this->destination->name.toStdString() << "\n";
    std::cerr << "   " <<  this->source->name.toStdString() << "\n";
    std::cerr << "   " <<  "Synapses:\n";
    for (int i=0; i < (int) this->synapses.size(); ++i) {
        std::cerr << "       " <<  this->synapses[i]->postsynapseType->component->name.toStdString() << " " << this->synapses[i]->weightUpdateType->component->name.toStdString()<< " " << "\n";
    }
    cerr << "\n";
}


