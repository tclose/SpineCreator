#ifndef VIEWBLBATCHPANELHANDLER_H
#define VIEWBLBATCHPANELHANDLER_H

#include "globalHeader.h"

struct viewBLstruct;

class viewBLBatchPanelHandler : public QObject
{
    Q_OBJECT
public:
    explicit viewBLBatchPanelHandler(QObject *parent = 0);
    explicit viewBLBatchPanelHandler(viewBLstruct * viewBL, rootData * data, QObject *parent = 0);

    /*!
     * \brief data
     * Local reference to the global data structure
     */
    rootData * data;

private:
    /*!
     * \brief viewBL
     * Local reference to the ViewBL structure
     */
    viewBLstruct * viewBL;

    /*!
     * \brief redrawPanel
     * Redraws the panel with the list of Batch Jobs
     */
    void redrawPanel();

    /*!
     * \brief recursiveDeleteLoop
     * \param parentLayout
     * Loop to delete QWidgets
     */
    void recursiveDeleteLoop(QLayout * parentLayout);
    /*!
     * \brief recursiveDelete
     * \param parentLayout
     * Entry point for deleting the QWidgets in the list of Batch Jobs
     */
    void recursiveDelete(QLayout * parentLayout);

signals:
    /*!
     * \brief deleteConnected
     * This is connected to the deleteLater slot of QWidgets we want to delete
     */
    void deleteConnected();

    /*!
     * \brief enableRun
     * Enable the batch run button
     */
    void enableRun(bool);

public slots:
    /*!
     * \brief addBatch
     * Add a batch job
     */
    void addBatch();

    /*!
     * \brief delBatch
     * Delete a batch job
     */
    void delBatch();

    /*!
     * \brief moveBatch
     * Change the order of the batch jobs
     */
    void moveBatch();

    /*!
     * \brief editBatch
     * Toggle editing of the current selected Batch Job details
     */
    void editBatch();

    /*!
     * \brief doneEditBatch
     * Finish editing of the current selected Batch Job details, save changes
     */
    void doneEditBatch();

    /*!
     * \brief cancelEditBatch
     * Finish editing of the current selected Batch Job details, discard changes
     */
    void cancelEditBatch();

    /*!
     * \brief changeSelection
     * Select the calling batch job
     */
    void changeSelection();

};

#endif // VIEWBLBATCHPANELHANDLER_H
