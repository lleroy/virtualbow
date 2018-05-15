#pragma once
#include "Series.hpp"

#include <vector>
#include <stdexcept>
#include <algorithm>
#include <cassert>

#include <iostream>

class CubicSpline
{
public:
    CubicSpline(Series data)
        : x(data.args()),
          y(data.vals())
    {
        if(!is_strictly_increasing(data.args()))
            throw std::runtime_error("function arguments must be strictly increasing");


        size_t n = data.size();                 // Data size

        m.resize(n);

        // First point
        x[0] = data.args().front();
        y[0] = data.vals().front();
        m[0] = (data.val(1) - data.val(0))/(data.arg(1) - data.arg(0));

        // Last point
        x[n-1] = data.args().back();
        y[n-1] = data.vals().back();
        m[n-1] = (data.val(n-1) - data.val(n-2))/(data.arg(n-1) - data.arg(n-2));

        // Intermediate points (line centers)
        for(size_t i = 1; i < n - 2; ++i)
        {
            double m_fwd = (data.val(i+1) - data.val(i))/(data.arg(i+1) - data.arg(i));
            double m_bwd = (data.val(i) - data.val(i-1))/(data.arg(i) - data.arg(i-1));
            m[i] = 0.5*(m_fwd + m_bwd);
        }

        for(size_t i = 0; i < m.size(); ++i)
        {
            std::cout << "mi = " << m[i] << "\n";
        }

    }

    Series sample(size_t n)
    {
        double x0 = x.front();
        double x1 = x.back();

        Series result;

        std::vector<double> args;
        args.resize(n+1);

        for(size_t i = 0; i <= n; ++i)
        {
            double p = double(i)/double(n);
            args[i] = x0*(1.0 - p) + x1*p;
        }

        return Series(args, interpolate(args));
    }

private:
    bool is_strictly_increasing(const std::vector<double>& args)
    {
        if(args.size() < 2)
            return false;

        for(size_t i = 0; i < args.size()-1; ++i)
        {
            if(args[i] >= args[i+1])
                return false;
        }

        return true;
    }

    std::vector<double> interpolate(const std::vector<double>& args)
    {
        if(!is_strictly_increasing(args))
            throw std::runtime_error("function arguments must be strictly increasing");

        std::vector<double> vals;
        vals.resize(args.size());

        size_t j = 0; // Last segment index
        auto eval_ascending = [&](double arg)
        {
            while(arg > x[j+1])    // Advance segment index such that x[j] < arg < x[j + 1]
                ++j;

            double h = x[j+1] - x[j];
            double t = (arg - x[j])/h;

            double h00 = 2.0*t*t*t - 3.0*t*t + 1.0;
            double h10 = t*t*t - 2.0*t*t + t;
            double h01 = -2.0*t*t*t + 3.0*t*t;
            double h11 = t*t*t - t*t;

            return h00*y[j] + h10*h*m[j] + h01*y[j+1] + h11*h*m[j+1];
        };

        for(size_t i = 0; i < args.size(); ++i)
            vals[i] = eval_ascending(args[i]);

        return vals;
    }

    std::vector<double> x;
    std::vector<double> y;
    std::vector<double> m;
};
