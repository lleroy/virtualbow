#pragma once
#include "model/InputData.hpp"
#include "numerics/ArcCurve.hpp"
#include "numerics/CubicSpline.hpp"

#include <vector>
#include <array>

// sigma_upper(s) = He_upper(s)*epsilon(s) + Hk_upper(s)*kappa(s)
// sigma_lower(s) = He_lower(s)*epsilon(s) + Hk_lower(s)*kappa(s)
struct LayerProperties
{
    double E;
    std::vector<double> s;
    std::vector<double> y_upper;
    std::vector<double> y_lower;

    std::vector<double> sigma_upper(const std::vector<double>& epsilon, const std::vector<double>& kappa) const
    {
        std::vector<double> sigma(s.size());
        for(size_t i = 0; i < s.size(); ++i)
        {
            sigma[i] = E*y_upper[i]*kappa[i] - E*epsilon[i];    // Todo: Sign?
        }

        return sigma;
    }

    // Todo: Code duplication
    std::vector<double> sigma_lower(const std::vector<double>& epsilon, const std::vector<double>& kappa) const
    {
        std::vector<double> sigma(s.size());
        for(size_t i = 0; i < s.size(); ++i)
        {
            sigma[i] = E*y_lower[i]*kappa[i] - E*epsilon[i];    // Todo: Sign?
        }

        return sigma;
    }
};

struct LimbProperties
{
    // Nodes
    std::vector<double> s;
    std::vector<double> x;
    std::vector<double> y;
    std::vector<double> w;
    std::vector<double> h;
    std::vector<double> phi;

    // Section properties
    std::vector<double> hc;          // Total cross section height (used for contact)
    std::vector<double> Cee;
    std::vector<double> Ckk;
    std::vector<double> Cek;
    std::vector<double> rhoA;

    // Layer properties
    std::vector<LayerProperties> layers;

    LimbProperties(const InputData& input)
    {
        LimbProperties(input, input.settings_n_elements_limb);
    }

    LimbProperties(const InputData& input, unsigned int n_elements_limb)
    {
        // 1. Nodes
        Curve2D curve = ArcCurve::sample(input.profile_segments,
                                         input.profile_x0,
                                         input.profile_y0,
                                         input.profile_phi0,
                                         n_elements_limb);

        // Todo: Is there a more elegant way? Maybe have a Curve2D member?
        s = curve.s;
        x = curve.x;
        y = curve.y;
        phi = curve.phi;

        // 2. Sections
        Series width = CubicSpline::sample(input.sections_width, n_elements_limb);
        Series height = CubicSpline::sample(input.sections_height, n_elements_limb);

        for(size_t i = 0; i < s.size(); ++i)
        {
            double w_i = width.val(i);
            double h_i = height.val(i);

            double A = w_i*h_i;
            double I = A*h_i*h_i/3.0;

            w.push_back(w_i);
            h.push_back(h_i);

            hc.push_back(h_i);
            Cee.push_back(input.sections_E*A);
            Ckk.push_back(input.sections_E*I);
            Cek.push_back(input.sections_E*A*h_i/2.0);
            rhoA.push_back(input.sections_rho*A);
        }

        // 3. Layers

        layers.push_back({});
        layers[0].E = input.sections_E;

        for(size_t i = 0; i < s.size(); ++i)
        {
            layers[0].s.push_back(s[i]);
            layers[0].y_upper.push_back(0.0);
            layers[0].y_lower.push_back(-height.val(i));
        }
    }

    LimbProperties()
    {

    }
};