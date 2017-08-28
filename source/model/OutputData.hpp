#pragma once
#include "model/LimbProperties.hpp"
#include <boost/optional.hpp>
#include <vector>
#include <valarray>

struct SetupData
{
    LimbProperties limb;
    double string_length;
};

struct BowStates
{
    std::vector<double> time;
    std::vector<double> draw_force;
    std::vector<double> draw_length;

    std::vector<double> pos_arrow;
    std::vector<double> vel_arrow;
    std::vector<double> acc_arrow;

    std::vector<double> e_pot_limbs;
    std::vector<double> e_kin_limbs;
    std::vector<double> e_pot_string;
    std::vector<double> e_kin_string;
    std::vector<double> e_kin_arrow;

    std::vector<double> y_arrow;
    std::vector<std::valarray<double>> x_limb;
    std::vector<std::valarray<double>> y_limb;
    std::vector<std::valarray<double>> x_string;
    std::vector<std::valarray<double>> y_string;

    std::vector<std::valarray<double>> sigma_upper;
    std::vector<std::valarray<double>> sigma_lower;
};

struct StaticData
{
    BowStates states;
    double final_draw_force = 0.0;
    double drawing_work = 0.0;
    double storage_ratio = 0.0;
};

struct DynamicData
{
    BowStates states;
    double final_arrow_velocity = 0.0;
    double final_arrow_energy = 0.0;
    double efficiency = 0.0;
};

struct OutputData
{
    SetupData setup;
    StaticData statics;
    DynamicData dynamics;
};
