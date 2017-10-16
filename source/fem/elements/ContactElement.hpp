#pragma once
#include "fem/Element.hpp"
#include "fem/Node.hpp"
#include "numerics/Eigen.hpp"
#include <array>

class ContactElement: public Element
{
public:
    ContactElement(System& system, Node node0, Node node1, Node node2, double h0, double h1, double k);

    virtual void add_masses() const override;
    virtual void add_internal_forces() const override;
    virtual void add_tangent_stiffness() const override;

    virtual double get_potential_energy() const override;
    virtual double get_kinetic_energy() const override;

private:
    std::array<Dof, 8> dofs;

    double h0;
    double h1;
    double k;

    struct State
    {
        double e;
        Vector<8> De;
        Matrix<8> DDe;
    };

    State get_state() const;
};
