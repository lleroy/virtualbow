#pragma once
#include "RootFinding.hpp"
#include <algorithm>

// Checks if the triangle {a, b, c} is right-handed
static bool is_right_handed(Vector<2> a, Vector<2> b, Vector<2> c)
{
    return (b[0] - a[0])*(c[1] - a[1]) - (b[1] - a[1])*(c[0] - a[0]) > 0;
}

// Traverses a the curve given by a list of input points and returns a subset of points with
// strictly right handed or left handed orientation.
// Implementation inspired by Graham scan for finding the convex hull of a point set
// (https://de.wikipedia.org/wiki/Graham_Scan)
//
// Todo: Find a better name. What's a curve with strictly positive/negative curvature called?
static std::vector<Vector<2>> one_sided_curvature_subset(std::vector<Vector<2>> input, bool right_handed)
{
    assert(input.size() >= 2);

    std::vector<Vector<2>> output;
    output.reserve(input.size());

    output.push_back(input[0]);
    output.push_back(input[1]);

    size_t i = 2;
    while(i < input.size())
    {
        auto p1 = output[output.size()-2];
        auto p2 = output[output.size()-1];
        auto p3 = input[i];

        if(is_right_handed(p1, p2, p3) == right_handed)
        {
            output.push_back(p3);
            ++i;
        }
        else
        {
            output.pop_back();
        }
    }

    return output;
}

// Partitions a curve of linear segments given by a list of points into n_out points such that
// the first and last point coincide with the start and end of the curve and all points
// are evenly spaced out by euclidean distance.
static std::vector<Vector<2>> equipartition(const std::vector<Vector<2>>& input, size_t n_out)
{
    assert(input.size() >= 2);
    assert(n_out >= 2);

    size_t n_in = input.size();              // Number of input points
    std::vector<Vector<2>> output(n_out);    // Output data

    // Calculate lenths of the input curve
    VectorXd s_in(n_in);
    s_in[0] = 0.0;
    for(size_t i = 1; i < n_in; ++i)
        s_in[i] = s_in[i-1] + (input[i] - input[i-1]).norm();

    // Assign first and last points
    output[0] = input[0];
    output[n_out-1] = input[n_in-1];

    // Function that returns two values eta_1/2 such that
    // ||A + eta_i*(B-A), C|| = d.
    auto find_by_distance = [](Vector<2> A, Vector<2> B, Vector<2> C, double d)
    {
        double x_ab = B[0] - A[0];  double x_ac = C[0] - A[0];
        double y_ab = B[1] - A[1];  double y_ac = C[1] - A[1];

        return solve_quadratic(x_ac*x_ac + y_ac*y_ac - d*d,
                              -2.0*(x_ac*x_ab + y_ac*y_ab),
                               x_ab*x_ab+ y_ab*y_ab);
    };

    // Function that calculates the intermediate points for a fixed distance
    // d and returns a measure of error. The real equipartition is done by finding
    // the root of this function.
    auto partition = [&](double d)
    {
        // i: Input point index
        // j: Output point index.
        // Iterate over all intermediate output points...
        for(size_t i = 0, j = 1; j < n_out; ++j)
        {
            while(true)
            {
                // Try to find a point on the current input segment (i, i+1)
                // with distance d to the previous output point (j-1)
                double eta = find_by_distance(input[i], input[i+1], output[j-1], d).maxCoeff();

                if(eta > 1.0 && i < n_in-2)    // If eta lies outside the current segment and it's not the last segment: move to the next segment.
                {
                    ++i;
                }
                else if(j < n_out-1)    // Intermediate point: Calculate position and assign
                {
                    output[j] = input[i] + eta*(input[i+1] - input[i]);
                    break;
                }
                else    // End point: Don't assign, calculate arc lenth to end of input curve
                {
                    double l = (1.0 - eta)*(s_in[i+1] - s_in[i]);
                    for(i = i+1; i < n_in-1; ++i)
                        l += s_in[i+1] - s_in[i];

                    return l/s_in[n_in-1];    // Return arc length relative to total length as error
                }
            }
        }
    };

    // Find root of the partition function, use input curve lenth divided by number of
    // output segments as a reasonable initial value for the distance d.
    double d = s_in[n_in-1]/(n_out-1);
    secant_method(partition, d, 1.1*d, 1e-6, 50);    // Magic numbers
    //bracket_and_bisect<false>(partition, d, 2.0, 1e-6, 1e-6, 50);    // Magic numbers

    return output;
}