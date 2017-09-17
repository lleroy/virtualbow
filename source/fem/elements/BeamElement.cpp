#include "BeamElement.hpp"

BeamElement::BeamElement(Node nd0, Node nd1, double rhoA, double L)
    : dofs{nd0[0], nd0[1], nd0[2], nd1[0], nd1[1], nd1[2]},
      phi_ref_0(0.0),
      phi_ref_1(0.0),
      rhoA(rhoA),
      L(L),
      C(Matrix<3, 3>::Zero())
{

}

void BeamElement::set_reference_angles(double phi_ref_0, double phi_ref_1)
{
    this->phi_ref_0 = phi_ref_0;
    this->phi_ref_1 = phi_ref_1;
}

void BeamElement::set_stiffness(double Cee, double Ckk, double Cek)
{
    C << Cee,    -Cek,     Cek,
        -Cek, 4.0*Ckk, 2.0*Ckk,
         Cek, 2.0*Ckk, 4.0*Ckk;

    // Todo: Assert that C > 0
}

// p in [0, 1]
double BeamElement::get_epsilon() const
{
    return e(0)/L;
}

// p in [0, 1]
double BeamElement::get_kappa(double p) const
{
    return (6.0*p - 4.0)/L*e(1) + (6.0*p - 2.0)/L*e(2);
}

void BeamElement::update_state()
{
    double dx = dofs[3].u() - dofs[0].u();
    double dy = dofs[4].u() - dofs[1].u();
    double phi = std::atan2(dy, dx);

    // Elastic coordinates
    double sin_e1 = sin(dofs[2].u() + phi_ref_0 - phi);
    double cos_e1 = cos(dofs[2].u() + phi_ref_0 - phi);
    double sin_e2 = sin(dofs[5].u() + phi_ref_1 - phi);
    double cos_e2 = cos(dofs[5].u() + phi_ref_1 - phi);

    // Todo: Replace atan(sin(x)/cos(x)) with a cheaper function
    e = {hypot(dx, dy) - L, std::atan(sin_e1/cos_e1), std::atan(sin_e2/cos_e2)};

    // Jacobian of the elastic coordinates
    double a0 = pow(dx*dx + dy*dy, -0.5);
    double a1 = 1.0/(dx*dx + dy*dy);

    double j0 = a0*dx;
    double j1 = a0*dy;
    double j2 = a1*dx;
    double j3 = a1*dy;

    J << -j0, -j1, 0.0, j0,  j1, 0.0,
         -j3,  j2, 1.0, j3, -j2, 0.0,
         -j3,  j2, 0.0, j3, -j2, 1.0;
}

void BeamElement::get_masses(VectorView M) const
{
    double alpha = 0.02;
    double m = 0.5*rhoA*L;
    double I = alpha*rhoA*pow(L, 3);

    M(dofs[0]) += m;
    M(dofs[1]) += m;
    M(dofs[2]) += I;
    M(dofs[3]) += m;
    M(dofs[4]) += m;
    M(dofs[5]) += I;
}

void BeamElement::get_internal_forces(VectorView q) const
{
    q(dofs) += 1.0/L*J.transpose()*C*e;
}

void BeamElement::get_tangent_stiffness(MatrixView K) const
{
    double dx = dofs[3].u() - dofs[0].u();
    double dy = dofs[4].u() - dofs[1].u();

    double a0 = pow(dx*dx + dy*dy, -0.5);
    double a1 = 1.0/(dx*dx + dy*dy);
    double a2 = pow(dx*dx + dy*dy, -1.5);
    double a3 = pow(dx*dx + dy*dy, -2.0);

    double b0 = a2*dx*dx - a0;
    double b1 = a2*dy*dy - a0;
    double b2 = a2*dx*dy;
    double b3 = 2.0*a3*dx*dx - a1;
    double b4 = 2.0*a3*dy*dy - a1;
    double b5 = 2.0*a3*dx*dy;

    Matrix<3, 6> dJ0;
    dJ0 << -b0, -b2, 0.0, b0,  b2, 0.0,
           -b5,  b3, 0.0, b5, -b3, 0.0,
           -b5,  b3, 0.0, b5, -b3, 0.0;

    Matrix<3, 6> dJ1;
    dJ1 << -b2, -b1, 0.0, b2,  b1, 0.0,
           -b4,  b5, 0.0, b4, -b5, 0.0,
           -b4,  b5, 0.0, b4, -b5, 0.0;

    Matrix<6, 6> Kn = Matrix<6, 6>::Zero();
    Kn.col(0) =  1.0/L*dJ0.transpose()*C*e;
    Kn.col(1) =  1.0/L*dJ1.transpose()*C*e;
    Kn.col(3) = -1.0/L*dJ0.transpose()*C*e;
    Kn.col(4) = -1.0/L*dJ1.transpose()*C*e;

    K(dofs) += Kn + 1.0/L*J.transpose()*C*J;
}

double BeamElement::get_potential_energy() const
{
    return 0.5/L*e.transpose()*C*e;
}
