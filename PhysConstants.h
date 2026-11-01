#pragma once
#include <algorithm>


namespace PhysConst
{
    inline float a = 100.0f;
    inline constexpr float pi = 3.14159265358979f;

    // Coulomb
    inline float Getpermit_of_freespace() noexcept
    {    
        return 1.0f / (a * 4.0f * pi);
    }
    inline float GetK() noexcept
    {
        return 1.0f / (4.0f*pi * Getpermit_of_freespace());
    }
    inline constexpr float softening = 1.5f;
    //magnetic
    inline float Getpermeb_of_freespace() noexcept
    {
        return (4.0f * pi)/ a;
    }
    // Speed of light
    inline float GetC() noexcept
    {
        return 1 / std::sqrt(Getpermeb_of_freespace() 
            * Getpermit_of_freespace());
    }

    // Van der Waals
    inline constexpr float vdw_r_min = 0.6f;

    // Polarization

  
        inline float gamma = 1.0f;

    // Larmor damping — derived, not arbitrary
    // b = 2k / 3c³ from Abraham-Lorentz formula
    inline float GetB() noexcept
    {
        float c = GetC();
        float b = (2.0f * GetK()*Getpermeb_of_freespace()) / (3.0f * c );
        return b < 0.1f ? b : 0.1f;
    }

    // Substepping
    inline constexpr float MAX_DT = 0.005f;
    inline constexpr int   MAX_SUBSTEPS = 50;

    // Contact
    inline constexpr float contact_radius = 1.2f;
}