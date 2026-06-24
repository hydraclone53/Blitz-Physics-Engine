#pragma once
#include <algorithm>

namespace PhysConst
{
    inline constexpr float pi = 3.14159265358979f;

    // Coulomb
    inline constexpr float perm_of_freespace =1.0f/( 4.0f*pi*100.0f);
    inline float GetK() noexcept
    {
        return 1.0f / (4.0f*pi * perm_of_freespace);
    }
    inline constexpr float softening = 1.5f;

    // Van der Waals
    inline constexpr float vdw_r_min = 0.6f;

    // Polarization

    // Speed of light
    inline float c = 10.0f;  // non-constexpr — runtime tunable

    // Larmor damping — derived, not arbitrary
    // b = 2k / 3c³ from Abraham-Lorentz formula
    inline float GetB() noexcept
    {
        return (2.0f * GetK()) / (3.0f * c * c * c);
    }

    // Substepping
    inline constexpr float MAX_DT = 0.005f;
    inline constexpr int   MAX_SUBSTEPS = 50;

    // Contact
    inline constexpr float contact_radius = 1.2f;
}