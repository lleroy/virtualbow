#include "BowModel.hpp"
#include "solver/model/LimbProperties.hpp"
#include "solver/fem/EigenvalueSolver.hpp"
#include "solver/fem/StaticSolver.hpp"
#include "solver/fem/DynamicSolver.hpp"
#include "solver/fem/elements/BeamElement.hpp"
#include "solver/fem/elements/BarElement.hpp"
#include "solver/fem/elements/MassElement.hpp"
#include "solver/fem/elements/ConstraintElement.hpp"
#include "solver/fem/elements/ContactHandler.hpp"
#include "solver/numerics/RootFinding.hpp"
#include "solver/numerics/Geometry.hpp"
#include <limits>
#include <numeric>

OutputData BowModel::simulate(const InputData& input, SimulationMode mode, const Callback& callback)
{
    BowModel model(input);

    SetupData setup = model.simulate_setup(callback);
    BowStates static_states = model.simulate_statics(callback);
    BowStates dynamic_states;

    if(mode == SimulationMode::Dynamic) {
        dynamic_states = model.simulate_dynamics(callback);
    }

    return OutputData(setup, static_states, dynamic_states);
}

BowModel::BowModel(const InputData& input)
    : input(input)
{
    // Check Settings
    if(input.settings.n_limb_elements < 1)
        throw std::runtime_error("Settings: Number of limb elements must be positive");

    if(input.settings.n_string_elements < 1)
        throw std::runtime_error("Settings: Number of string elements must be positive");

    if(input.settings.n_draw_steps < 1)
        throw std::runtime_error("Settings: Number of draw steps must be positive");

    if(input.settings.time_span_factor <= 0.0)
        throw std::runtime_error("Settings: Time span factor must be positive");

    if(input.settings.time_step_factor <= 0.0)
        throw std::runtime_error("Settings: Time step factor must be positive");

    if(input.settings.sampling_rate <= 0.0)
        throw std::runtime_error("Settings: Sampling rate must be positive");

    // Check Profile
    // ...

    // Check Width
    if(input.width.size() < 2)
        throw std::runtime_error("Width: At least two data points are needed");

    for(double w: input.width.col(1))
    {
        if(w <= 0.0)
            throw std::runtime_error("Width must be positive");
    }

    // Check Layers
    for(size_t i = 0; i < input.layers.size(); ++i)
    {
        const Layer& layer = input.layers[i];

        if(layer.rho <= 0.0)
            throw std::runtime_error("Layer " + std::to_string(i) + " (" + layer.name + ")" + ": rho must be positive");

        if(layer.E <= 0.0)
            throw std::runtime_error("Layer " + std::to_string(i) + " (" + layer.name + ")" + ": E must be positive");

        if(layer.height.size() < 2)
            throw std::runtime_error("Layer " + std::to_string(i) + " (" + layer.name + ")" + ": At least two data points for height are needed");

        for(double h: layer.height.col(1))
        {
            if(h < 0.0)
                throw std::runtime_error("Layer " + std::to_string(i) + " (" + layer.name + ")" + ": Height must not be negative");
        }
    }

    // Check String
    if(input.string.strand_stiffness <= 0.0)
        throw std::runtime_error("String: Strand stiffness must be positive");

    if(input.string.strand_density <= 0.0)
        throw std::runtime_error("String: Strand density must be positive");

    if(input.string.n_strands < 1)
        throw std::runtime_error("String: Number of strands must be positive");

    // Check Masses
    if(input.masses.arrow <= 0.0)
        throw std::runtime_error("Operation: Arrow mass must be positive");

    if(input.masses.string_center < 0.0)
        throw std::runtime_error("Masses: String center mass must not be negative");

    if(input.masses.string_tip < 0.0)
        throw std::runtime_error("Masses: String tip mass must not be negative");

    if(input.masses.limb_tip < 0.0)
        throw std::runtime_error("Masses: Limb tip mass must not be negative");

    // Check Damping
    if(input.damping.damping_ratio_limbs < 0.0 || input.damping.damping_ratio_limbs > 1.0)
        throw std::runtime_error("Damping: Damping of the limb must be 0% ... 100%");

    if(input.damping.damping_ratio_string < 0.0 || input.damping.damping_ratio_string > 1.0)
        throw std::runtime_error("Damping: Damping of the string must be 0% ... 100%");

    // Check Dimensions
    if(input.dimensions.brace_height >= input.dimensions.draw_length)
        throw std::runtime_error("Dimensions: Draw length must be greater than brace height");

    if(input.dimensions.handle_length < 0.0)
        throw std::runtime_error("Dimensions: Handle length must be positive");
}

void BowModel::init_limb(const Callback& callback, SetupData& output)
{
    LimbProperties limb_properties(input);

    // Create limb nodes
    for(size_t i = 0; i < input.settings.n_limb_elements + 1; ++i)
    {
        bool active = (i != 0);
        Node node = system.create_node({active, active, active}, {limb_properties.x_pos[i], limb_properties.y_pos[i], limb_properties.angle[i]});
        nodes_limb.push_back(node);
    }

    // Create limb elements
    for(size_t i = 0; i < input.settings.n_limb_elements; ++i)
    {
        double rhoA = 0.5*(limb_properties.rhoA[i] + limb_properties.rhoA[i+1]);
        double L = system.get_distance(nodes_limb[i], nodes_limb[i+1]);

        double Cee = 0.5*(limb_properties.Cee[i] + limb_properties.Cee[i+1]);
        double Ckk = 0.5*(limb_properties.Ckk[i] + limb_properties.Ckk[i+1]);
        double Cek = 0.5*(limb_properties.Cek[i] + limb_properties.Cek[i+1]);

        // Todo: Document this
        double phi = system.get_angle(nodes_limb[i], nodes_limb[i+1]);
        double phi0 = phi - system.get_u(nodes_limb[i].phi);
        double phi1 = phi - system.get_u(nodes_limb[i+1].phi);

        BeamElement element(system, nodes_limb[i], nodes_limb[i+1], rhoA, L);
        element.set_reference_angles(phi0, phi1);
        element.set_stiffness(Cee, Ckk, Cek);
        system.mut_elements().add(element, "limb");
    }

    // Tune damping parameter

    EigenvalueSolver solver(system);
    auto try_damping_parameter = [&](double beta)
    {
        for(auto& element: system.mut_elements().group<BeamElement>("limb"))
            element.set_damping(beta);

        return solver.compute_minimum_frequency().zeta - input.damping.damping_ratio_limbs;
    };

    if(input.damping.damping_ratio_limbs > 0.0)
        secant_method(try_damping_parameter, 1.0, 1.5, 1e-5, 15);    // Magic numbers

    // Assign discrete limb properties
    output.limb_properties = limb_properties;
    output.limb_mass = std::accumulate(limb_properties.m.begin(), limb_properties.m.end(), 0.0) + input.masses.limb_tip;
}

void BowModel::init_string(const Callback& callback, SetupData& output)
{
    LimbProperties& limb_properties = output.limb_properties;

    const double k = 0.1*system.get_K().maxCoeff();        // Contact stiffness in terms of maximum stiffness already present // Todo: Magic number
    const double epsilon = 0.01*limb_properties.height[0];    // Transition zone of the contact elements // Magic number

    // Calculate curve tangential to the limb na d calculate string node positions by equipartition
    std::vector<Vector<2>> points;
    points.push_back({0.0, -input.dimensions.brace_height});
    for(size_t i = 0; i < nodes_limb.size(); ++i)
    {
        points.push_back({
            // Add small initial penetration epsilon to prevent slip-through on the first static solver iteration
            system.get_u(nodes_limb[i].x) + (0.5*limb_properties.height[i] - epsilon)*sin(system.get_u(nodes_limb[i].phi)),
            system.get_u(nodes_limb[i].y) - (0.5*limb_properties.height[i] - epsilon)*cos(system.get_u(nodes_limb[i].phi))
        });
    }

    points = constant_orientation_subset<Orientation::RightHanded>(points);
    points = equipartition(points, input.settings.n_string_elements + 1);

    // Create string nodes
    for(size_t i = 0; i < points.size(); ++i)
    {
        Node node = system.create_node({i != 0, true, false}, {points[i][0], points[i][1], 0.0});
        nodes_string.push_back(node);
    }

    // Create string elements
    double EA = input.string.n_strands*input.string.strand_stiffness;
    double rhoA = input.string.n_strands*input.string.strand_density;

    for(size_t i = 0; i < input.settings.n_string_elements; ++i)
    {
        BarElement element(system, nodes_string[i], nodes_string[i+1], 0.0, EA, 0.0, rhoA); // Element lengths are reset later when string length is determined
        system.mut_elements().add(element, "string");
    }

    // Create string to limb contact surface and limb tip constraint

    ContactHandler contact(system, ContactForce(k, epsilon));
    for(size_t i = 1; i < nodes_limb.size(); ++i)
        contact.add_segment(nodes_limb[i-1], nodes_limb[i], 0.5*limb_properties.height[i-1], 0.5*limb_properties.height[i]);

    for(size_t i = 0; i < nodes_string.size()-1; ++i)    // Don't include the last string node (tip)
        contact.add_point(nodes_string[i]);

    system.mut_elements().add(contact, "contact");
    system.mut_elements().add(ConstraintElement(system, nodes_limb.back(), nodes_string.back(), k), "constraint");

    // Takes a string element length, iterates to equilibrium with the constraint of the brace height
    // and returns the angle of the string center
    system.set_p(nodes_string[0].y, 1.0);    // Will be scaled by the static algorithm
    StaticSolverDC solver(system, nodes_string[0].y);   // Todo: Reuse solver across function calls?
    StaticSolverDC::Info info;
    auto try_element_length = [&](double l)
    {
        for(auto& element: system.mut_elements().group<BarElement>("string"))
            element.set_length(l);

        info = solver.solve(-input.dimensions.brace_height);
        return system.get_angle(nodes_string[0], nodes_string[1]);
    };

    // Find a element length at which the brace height difference is zero
    // Todo: Perhaps limit the step size of the root finding algorithm to increase robustness.
    double l = (points[1] - points[0]).norm();
    if(try_element_length(l) <= 0.0)
        throw std::runtime_error("Invalid input: Brace height is too low");

    double dl = 1e-3*l;        // Initial step length, later adjusted by the algorithm    // Magic number
    double dl_min = 1e-5*l;   // Minimum step length, abort if smaller                   // Magic number
    const unsigned iterations = 5;    // Desired number of iterations for the static solver      // Magic number
    while(true)
    {
        // Try length = l - dl
        double alpha = try_element_length(l - dl);

        if(info.outcome == StaticSolverDC::Info::Success)
        {
            // Success: Apply step.
            l -= dl;

            // If sign change of alpha: Almost done, do the rest by root finding.
            if(alpha <= 0.0)
            {
                l = bisect<true>(try_element_length, l, l+dl, 1e-5, 1e-10, 20);    // Magic numbers
                break;
            }

            // Adjust step length
            dl *= double(iterations)/info.iterations;
        }
        else
        {
            // Reduce step length by generic factor
            dl *= 0.5;
        }

        if(dl < dl_min)
            throw std::runtime_error("Bracing failed: Step size too small");
    }

    // Set string material damping to match user defined damping ratio
    double etaA = 4.0*l/M_PI*std::sqrt(rhoA*EA)*input.damping.damping_ratio_string;
    for(auto& element: system.mut_elements().group<BarElement>("string"))
        element.set_damping(etaA);

    // Assign output data
    output.string_length = 2.0*l*input.settings.n_string_elements;    // *2 because of symmetry
    output.string_mass = input.string.strand_density*input.string.n_strands*output.string_length
                       + input.masses.string_center + 2.0*input.masses.string_tip;
}

void BowModel::init_masses(const Callback& callback, SetupData& output)
{
    node_arrow = nodes_string[0];
    MassElement mass_limb_tip(system, nodes_limb.back(), input.masses.limb_tip);
    MassElement mass_string_tip(system, nodes_string.back(), input.masses.string_tip);
    MassElement mass_string_center(system, nodes_string.front(), 0.5*input.masses.string_center);   // 0.5 because of symmetry
    MassElement arrow_mass(system, node_arrow, 0.5*input.masses.arrow);                             // 0.5 because of symmetry

    system.mut_elements().add(mass_limb_tip, "limb tip");
    system.mut_elements().add(mass_string_tip, "string tip");
    system.mut_elements().add(mass_string_center, "string center");
    system.mut_elements().add(arrow_mass, "arrow");
}

SetupData BowModel::simulate_setup(const Callback& callback)
{
    SetupData output;
    init_limb(callback, output);
    init_string(callback, output);
    init_masses(callback, output);

    return output;
}

BowStates BowModel::simulate_statics(const Callback& callback)
{
    BowStates output;

    system.set_p(nodes_string[0].y, 1.0);    // Will be scaled by the static algorithm
    StaticSolverDC solver(system, nodes_string[0].y);
    for(unsigned i = 0; i < input.settings.n_draw_steps; ++i)
    {
        double eta = double(i)/(input.settings.n_draw_steps - 1);
        double draw_length = (1.0 - eta)*input.dimensions.brace_height + eta*input.dimensions.draw_length;

        solver.solve(-draw_length);
        add_state(output);

        callback(std::round(100.0*eta), 0);
    }

    return output;
}

BowStates BowModel::simulate_dynamics(const Callback& callback)
{
    BowStates output;

    // Set draw force to zero
    system.set_p(nodes_string[0].y, 0.0);

    double dt = DynamicSolver::estimate_timestep(system, input.settings.time_step_factor);
    double T = std::numeric_limits<double>::max();
    double alpha = input.settings.time_span_factor;

    DynamicSolver solver1(system, dt, input.settings.sampling_rate, [&]{
        double ut = system.get_u(node_arrow.y);
        if(ut < input.dimensions.brace_height)
        {
            double u0 = -input.dimensions.draw_length;
            double u1 = -input.dimensions.brace_height;

            if(ut != u0)
            {
                T = system.get_t()*std::acos(u1/u0)/std::acos(ut/u0);
            }
        }

        return system.get_a(node_arrow.y) <= 0;
    });

    auto run_solver = [&](DynamicSolver& solver)
    {
        do
        {
            add_state(output);
            callback(100, std::round(100.0*system.get_t()/(alpha*T)));
        } while(solver.step());
    };

    run_solver(solver1);

    // Change model by giving the arrow an independent node with the initial position and velocity of the string center
    // Todo: Would more elegant to remove the mass element from the system and create a new one with a new node.
    // Or initially add arrow mass to string center, then subtract that and give arrow its own node and element.
    node_arrow = system.create_node(nodes_string[0]);
    system.mut_elements().front<MassElement>("arrow").set_node(node_arrow);

    DynamicSolver solver2(system, dt, input.settings.sampling_rate, [&]{
        return system.get_t() >= alpha*T;
    });

    run_solver(solver2);

    return output;
}

void BowModel::add_state(BowStates& states) const
{
    states.time.push_back(system.get_t());
    states.draw_force.push_back(-2.0*system.get_p(nodes_string[0].y));    // *2 because of symmetry
    states.draw_length.push_back(-system.get_u(nodes_string[0].y));

    states.pos_arrow.push_back(system.get_u(node_arrow.y));
    states.vel_arrow.push_back(system.get_v(node_arrow.y));
    states.acc_arrow.push_back(system.get_a(node_arrow.y));

    double e_pot_limb = 2.0*system.get_elements().get_potential_energy("limb");
    double e_kin_limb = 2.0*system.get_elements().get_kinetic_energy("limb");
    double e_pot_limb_tip = 2.0*system.get_elements().get_potential_energy("limb tip");
    double e_kin_limb_tip = 2.0*system.get_elements().get_kinetic_energy("limb tip");
    double e_pot_string = 2.0*system.get_elements().get_potential_energy("string");
    double e_kin_string = 2.0*system.get_elements().get_kinetic_energy("string");
    double e_pot_string_tip = 2.0*system.get_elements().get_potential_energy("string tip");
    double e_kin_string_tip = 2.0*system.get_elements().get_kinetic_energy("string tip");
    double e_pot_string_center = 2.0*system.get_elements().get_potential_energy("string center");
    double e_kin_string_center = 2.0*system.get_elements().get_kinetic_energy("string center");
    double e_kin_arrow = 2.0*system.get_elements().get_kinetic_energy("arrow");

    double string_force = 0.0;
    for(auto& element: system.get_elements().group<BarElement>("string"))
        string_force = std::max(string_force, std::abs(element.get_normal_force()));

    states.string_force.push_back(string_force);
    states.strand_force.push_back(string_force/input.string.n_strands);
    states.grip_force.push_back(-2.0*system.get_q(nodes_limb[0].y));    // *2 because of symmetry

    states.e_pot_limbs.push_back(e_pot_limb + e_pot_limb_tip);
    states.e_kin_limbs.push_back(e_kin_limb + e_kin_limb_tip);
    states.e_pot_string.push_back(e_pot_string + e_pot_string_tip + e_pot_string_center);
    states.e_kin_string.push_back(e_kin_string + e_kin_string_tip + e_kin_string_center);
    states.e_kin_arrow.push_back(e_kin_arrow);

    // Limb and string coordinates
    states.x_pos_limb.push_back(VectorXd(nodes_limb.size()));
    states.y_pos_limb.push_back(VectorXd(nodes_limb.size()));
    states.angle_limb.push_back(VectorXd(nodes_limb.size()));

    for(size_t i = 0; i < nodes_limb.size(); ++i)
    {
        states.x_pos_limb.back()[i] = system.get_u(nodes_limb[i].x);
        states.y_pos_limb.back()[i] = system.get_u(nodes_limb[i].y);
        states.angle_limb.back()[i] = system.get_u(nodes_limb[i].phi);
    }

    states.x_pos_string.push_back(VectorXd(nodes_string.size()));
    states.y_pos_string.push_back(VectorXd(nodes_string.size()));

    for(size_t i = 0; i < nodes_string.size(); ++i)
    {
        states.x_pos_string.back()[i] = system.get_u(nodes_string[i].x);
        states.y_pos_string.back()[i] = system.get_u(nodes_string[i].y);
    }

    // Limb deformation

    VectorXd epsilon(nodes_limb.size());
    VectorXd kappa(nodes_limb.size());

    auto elements = system.get_elements().group<BeamElement>("limb");
    for(size_t i = 0; i < nodes_limb.size(); ++i)
    {
        if(i == 0)
        {
            epsilon[i] = elements[i].get_epsilon();
            kappa[i] = elements[i].get_kappa(0.0);
        }
        else if(i == nodes_limb.size()-1)
        {
            epsilon[i] = elements[i-1].get_epsilon();
            kappa[i] = elements[i-1].get_kappa(1.0);
        }
        else
        {
            epsilon[i] = 0.5*(elements[i-1].get_epsilon() + elements[i].get_epsilon());
            kappa[i] = 0.5*(elements[i-1].get_kappa(1.0) + elements[i].get_kappa(0.0));
        }
    }

    states.epsilon.push_back(epsilon);
    states.kappa.push_back(kappa);
}
