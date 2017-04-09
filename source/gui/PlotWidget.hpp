#pragma once
#include "external/qcustomplot/qcustomplot.h"
#include <boost/optional.hpp>

class PlotWidget: public QCustomPlot
{
public:
    PlotWidget()
    {
        // Styling

        this->xAxis2->setTickLabels(false);
        this->xAxis2->setVisible(true);
        this->yAxis2->setTickLabels(false);
        this->yAxis2->setVisible(true);

        // Interaction

        this->setInteraction(QCP::iRangeDrag, true);
        this->setInteraction(QCP::iRangeZoom, true);

        // Context menu

        this->setContextMenuPolicy(Qt::CustomContextMenu);
        QObject::connect(this, &QCustomPlot::customContextMenuRequested, [&](QPoint pos)
        {
            auto menu = new QMenu(this);
            menu->addAction("Export as...", this, [&]()
            {
                const char* PNG_FILE  = "PNG image (*.png)";
                const char* BMP_FILE  = "BMP image (*.bmp)";
                const char* PDF_FILE  = "Portable Document Format (*.pdf)";

                QFileDialog dialog(this);
                dialog.setAcceptMode(QFileDialog::AcceptSave);

                QStringList filters;
                filters << PNG_FILE << BMP_FILE << PDF_FILE;
                dialog.setNameFilters(filters);

                // Todo: Is there a better way to connect default suffix to the selected name filter?
                // Todo: Handle the case of the save[...] methods returning false
                QObject::connect(&dialog, &QFileDialog::filterSelected, [&](const QString &filter)
                {
                    if(filter == PNG_FILE)
                        dialog.setDefaultSuffix(".png");
                    else if(filter == BMP_FILE)
                        dialog.setDefaultSuffix(".bmp");
                    else if(filter == PDF_FILE)
                        dialog.setDefaultSuffix(".pdf");
                });
                dialog.filterSelected(PNG_FILE);

                if(dialog.exec() == QDialog::Accepted)
                {
                    QString filter = dialog.selectedNameFilter();
                    QString path = dialog.selectedFiles().first();

                    if(filter == PNG_FILE)
                        this->savePng(path);
                    else if(filter == BMP_FILE)
                        this->saveBmp(path);
                    else if(filter == PDF_FILE)
                        this->savePdf(path);
                }
            });

            menu->exec(this->mapToGlobal(pos));
        });

        // Limit axis ranges for zooming and panning

        QObject::connect(this->xAxis, static_cast<void (QCPAxis::*)(const QCPRange&)>(&QCPAxis::rangeChanged), [&](const QCPRange& range)
        {
            if(max_x_range)
            {
                QCPRange bounded = range.bounded(max_x_range->lower, max_x_range->upper);
                this->xAxis->setRange(bounded);
            }
        });

        QObject::connect(this->yAxis, static_cast<void (QCPAxis::*)(const QCPRange&)>(&QCPAxis::rangeChanged), [&](const QCPRange& range)
        {
            if(max_y_range)
            {
                QCPRange bounded = range.bounded(max_y_range->lower, max_y_range->upper);
                this->yAxis->setRange(bounded);
            }
        });
    }

    void setupTopLegend()
    {
        this->legend->setVisible(true);
        this->legend->setFillOrder(QCPLayoutGrid::foColumnsFirst);

        // Move legend at to the top outside of the plot
        // http://qcustomplot.com/index.php/support/forum/63
        // Todo: Better solution with QCustomPlot 2.0?
        QCPLayoutGrid *subLayout = new QCPLayoutGrid;
        this->plotLayout()->insertRow(0);
        this->plotLayout()->addElement(0, 0, subLayout);
        subLayout->addElement(0, 0, new QCPLayoutElement);
        subLayout->addElement(0, 1, this->legend);
        subLayout->addElement(0, 2, new QCPLayoutElement);
        this->plotLayout()->setRowStretchFactor(0, 0.001);
    }

    // Limit the axis maximum ranges to current range
    void rescaleAxes(bool include_zero_x = false, bool include_zero_y = false)
    {
        max_x_range = boost::none;
        max_y_range = boost::none;

        QCustomPlot::rescaleAxes();

        QCPRange x_range = xAxis->range();
        QCPRange y_range = yAxis->range();

        if(include_zero_x)
            x_range.expand(0.0);

        if(include_zero_y)
            y_range.expand(0.0);

        setAxesLimits(x_range, y_range);
    }

    void setAxesLimits(QCPRange x_range, QCPRange y_range)
    {
        max_x_range = x_range;
        max_y_range = y_range;

        xAxis->setRange(x_range);
        yAxis->setRange(y_range);
    }

private:
    boost::optional<QCPRange> max_x_range;
    boost::optional<QCPRange> max_y_range;
};
