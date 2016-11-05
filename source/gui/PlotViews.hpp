#pragma once
#include "Plot.hpp"
#include "../model/InputData.hpp"
#include "../model/InputData.hpp"
#include "../numerics/Series.hpp"
#include "../numerics/CubicSpline.hpp"
#include "../numerics/ArcCurve.hpp"

class SplineView: public Plot
{
public:
    SplineView(const QString& lbx, const QString& lby, DocItem<Series>& doc_item)
        : Plot(lbx, lby)
    {
        this->includeOrigin(true, true);
        this->addSeries();
        this->addSeries();
        this->setLineStyle(0, QCPCurve::lsNone);
        this->setScatterStyle(0, QCPScatterStyle(QCPScatterStyle::ssSquare, Qt::red, 6));

        connection = doc_item.connect([this](const Series& input)
        {
            try
            {
                CubicSpline spline(input);
                Series output = spline.sample(100);

                this->setData(0, input);
                this->setData(1, output);
            }
            catch(const std::runtime_error&)
            {
                this->setData(0, Series());
                this->setData(1, Series());
            }

            this->replot();
        });
    }

private:
    Connection connection;
};

class ProfileView: public Plot
{
public:
    ProfileView(InputData& data)
        : Plot("y", "x", Align::TopLeft),
          data(data)
    {
        this->includeOrigin(true, false);
        this->fixAspectRatio(true);

        this->addSeries();
        this->addSeries();
        this->setLineStyle(1, QCPCurve::lsNone);
        this->setScatterStyle(1, QCPScatterStyle(QCPScatterStyle::ssSquare, Qt::red, 6));

        // Todo: Use std::bind?
        // Todo: Inefficient and ugly
        connections.push_back(data.profile_segments.connect([this](const Series&){ update(); }));
        connections.push_back(data.profile_offset_x.connect([this](const double&){ update(); }));
        connections.push_back(data.profile_offset_y.connect([this](const double&){ update(); }));
        connections.push_back(data.profile_angle.connect([this](const double&){ update(); }));
        update();
    }

private:
    InputData& data;
    std::vector<Connection> connections;

    void update()
    {
        try
        {
            ArcCurve profile(data.profile_segments,
                             data.profile_offset_x,
                             data.profile_offset_y,
                             data.profile_angle,
                             150);  // Todo: Magic number

            ArcCurve segments(data.profile_segments,
                              data.profile_offset_x,
                              data.profile_offset_y,
                              data.profile_angle,
                              0);

            this->setData(0, Series(profile.y, profile.x));
            this->setData(1, Series(segments.y, segments.x));
        }
        catch(const std::runtime_error&)
        {
            this->setData(0, Series());
            this->setData(1, Series());
        }

        this->replot();
    }
};
