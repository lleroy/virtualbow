#pragma once
#include "numerics/Math.hpp"
#include "fem/View.hpp"
#include "fem/Node.hpp"

class Element
{
public:
    virtual void update_state() = 0;
    virtual void get_masses(VectorView M) const = 0;
    virtual void get_internal_forces(VectorView q) const = 0;
    virtual void get_tangent_stiffness(MatrixView K) const = 0;

    virtual double get_potential_energy() const = 0;
    virtual double get_kinetic_energy(VectorView v) const
    {
        double e_kin = 0.0;
        get_masses(VectorView(nullptr, nullptr, [&](Dof dof, double val)
        {
            e_kin += 0.5*val*pow(v(dof), 2);
        }));

        return e_kin;
    }
};
