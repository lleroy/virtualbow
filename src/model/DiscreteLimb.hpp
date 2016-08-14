#pragma once
#include "InputData.hpp"
#include "../numerics/SplineFunction.hpp"
#include "../numerics/StepFunction.hpp"

#include <boost/numeric/odeint.hpp>

// sigma(s) = He(s)*epsilon(s) + Hk(s)*kappa(s)
struct DiscreteStresses
{
    std::vector<double> s;
    std::vector<double> He;
    std::vector<double> Hk;
};

struct DiscreteLimb
{
    // Nodes
    std::vector<double> s;
    std::vector<double> x;
    std::vector<double> y;
    std::vector<double> phi;

    // Properties
    std::vector<double> Cee;
    std::vector<double> Ckk;
    std::vector<double> Cek;
    std::vector<double> rhoA;
    std::vector<double> hc;          // Total cross section height (used for contact)

    DiscreteLimb(const InputData& input)
    {
        calculate_nodes(input);
        calculate_properties(input);
    }

private:
    // Calculate positions and orientations of the nodes by integrating the limb curve from it's curvature.
    void calculate_nodes(const InputData& input)
    {
        using namespace boost::numeric::odeint;
        typedef std::array<double, 3> state_type;
        typedef runge_kutta_cash_karp54<state_type> error_stepper_type;

        StepFunction kappa(input.limb.curvature);
        auto system = [&](const state_type& z, state_type& dzds, double s)
        {
            dzds[0] = std::cos(z[2]);
            dzds[1] = std::sin(z[2]);
            dzds[2] = kappa(s);
        };

        auto observer = [&](const state_type &z, double s)
        {
            this->s.push_back(s);
            this->x.push_back(z[0]);
            this->y.push_back(z[1]);
            this->phi.push_back(z[2]);
        };

        state_type z0 = {input.limb.offset_x, input.limb.offset_y, input.limb.angle + M_PI_2};
        unsigned n = input.settings.n_elements_limb;
        double s0 = kappa.arg_min();
        double ds = (kappa.arg_max() - kappa.arg_min())/n;

        auto stepper = make_controlled<error_stepper_type>(1.0e-10, 1.0e-6);    // Todo: Magic numbers
        integrate_n_steps(stepper, system, z0, s0, ds, n, observer);
    }

    void calculate_properties(const InputData& input)
    {
        // Construct splines for layer geometry
        SplineFunction layer_width(input.limb.width);
        std::vector<SplineFunction> layer_heights;
        for(size_t i = 0; i < input.limb.layers.size(); ++i)
        {
            layer_heights.push_back({input.limb.layers[i].height});
        }

        // Calculate section properties at every node
        for(size_t i = 0; i < s.size(); ++i)    // Iterate over nodes
        {
            Cee.push_back(0.0);
            Ckk.push_back(0.0);
            Cek.push_back(0.0);
            rhoA.push_back(0.0);
            hc.push_back(0.0);

            double yu = 0.0;    // Upper Position layer top
            double yl = 0.0;    // Position layer bottom

            double p = s[i]/s.back();

            for(size_t j = 0; j < input.limb.layers.size(); ++j)    // Iterate over layers
            {
                if(p >= layer_heights[j].arg_min() && p <= layer_heights[j].arg_max()) // Todo: Abstract this?
                {
                    double w = layer_width(p);      // Todo: Ensure that the domain of width is [0, 1].
                    double h = layer_heights[j](p);

                    yu = yl;
                    yl -= h;

                    double y = 0.5*(yu + yl);
                    double A = w*h;
                    double I = A*(h*h/12.0 + y*y);
                    double E = input.limb.layers[j].E;
                    double rho = input.limb.layers[j].rho;

                    Cee.back() += E*A;
                    Ckk.back() += E*I;
                    Cek.back() += E*A*y;
                    rhoA.back() += rho*A;
                    hc.back() += h;
                }
            }
        }
    }
};
