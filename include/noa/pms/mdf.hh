/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2021, Roland Grinis, GrinisRIT ltd. (roland.grinis@grinisrit.com)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "noa/pms/physics.hh"
#include "noa/utils/common.hh"

#include <algorithm>
#include <regex>
#include <unordered_map>
#include <vector>

namespace noa::pms::mdf
{

    using MDFFilePath = utils::Path;
    using DEDXFolderPath = utils::Path;
    using DEDXFilePath = utils::Path;

    using ElementName = std::string;
    using MaterialName = std::string;
    using MaterialComponents =
        std::unordered_map<ElementName, ComponentFraction>;
    using Material =
        std::tuple<DEDXFilePath, MaterialDensity, MaterialComponents>;

    using CompositeName = std::string;
    using Composite = std::unordered_map<MaterialName, ComponentFraction>;

    using Elements = std::unordered_map<ElementName, AtomicElement>;
    using Materials = std::unordered_map<MaterialName, Material>;
    using Composites = std::unordered_map<CompositeName, Composite>;

    using Settings = std::tuple<Elements, Materials, Composites>;
    using ParticleName = std::string;
    using GeneratorName = std::string;

    inline const ParticleName Muon = "Muon";
    inline const ParticleName Tau = "Tau";
    inline const GeneratorName pumas = "pumas";

    struct DEDXMaterialCoefficients
    {
        MaterialDensityEffect density_effect;
        Scalar ZoA, I;
    };
    struct DEDXTable
    {
        std::vector<Scalar> T, p, Ionisation, brems, pair, photonuc, Radloss,
            dEdX, CSDArange, delta, beta;
    };
    using DEDXData = std::tuple<ParticleMass, DEDXMaterialCoefficients, DEDXTable>;
    using MaterialsDEDXData = std::unordered_map<MaterialName, DEDXData>;

    inline std::regex mass_pattern(const ParticleName &particle_name)
    {
        return std::regex{"\\s*Incident particle.*" + particle_name +
                          ".*M = [0-9.E\\+-]+ MeV"};
    }
    inline const auto zoa_pattern =
        std::regex{"\\s*Absorber with <Z/A>\\s*=\\s*[0-9.E\\+-]+"};
    inline const auto coef_pattern = std::regex{"\\s*Sternheimer coef:"};
    inline const auto table_pattern =
        std::regex{"\\s*T\\s+p\\s+Ionization\\s+brems\\s+pair\\s+photonuc\\s+"
                   "Radloss\\s+dE/dx\\s+CSDA Range\\s+delta\\s+beta"};
    inline const auto units_pattern = std::regex{
        "\\s*\\[MeV\\].*\\[MeV/c\\].*\\[MeV\\s+cm\\^2/g\\].*\\[g/cm\\^2\\]"};

    inline void print_elements(const Elements &elements)
    {
        std::cout << "Elements:\n";
        for (const auto &[name, element] : elements)
        {
            std::cout << " " << name << " <Z=" << element.Z
                      << ", A=" << element.A << ", I=" << element.I
                      << ">\n";
        }
    }

    inline void print_materials(const Materials &materials)
    {
        std::cout << "Materials:\n";
        for (const auto &[name, material] : materials)
        {
            auto [file, density, comps] = material;
            std::cout << " " << name << "\n"
                      << "  dedx file: " << file << "\n"
                      << "  density: " << density << "\n"
                      << "  components:\n";
            for (const auto &[element, fraction] : comps)
                std::cout << "   " << element << ": " << fraction
                          << "\n";
        }
    }

    inline void print_dedx_header(const DEDXData &dedx_data)
    {
        std::cout << " mass=" << std::get<ParticleMass>(dedx_data) << "\n";
        auto coefs = std::get<DEDXMaterialCoefficients>(dedx_data);
        std::cout << " ZoA=" << coefs.ZoA << ", I=" << coefs.I
                  << ", a=" << coefs.density_effect.a << ", k=" << coefs.density_effect.k
                  << ", x0=" << coefs.density_effect.x0 << ", x1=" << coefs.density_effect.x1
                  << ", Cbar=" << coefs.density_effect.Cbar << ", delta0=" << coefs.density_effect.delta0
                  << "\n";
    }

    std::optional<Settings> parse_settings(const GeneratorName &generated_by, const MDFFilePath &mdf_path);
   
    inline std::optional<ParticleMass> parse_particle_mass(
        std::ifstream &dedx_stream, const ParticleName &particle_name)
    {
        auto line = utils::find_line(dedx_stream, mass_pattern(particle_name));
        if (!line.has_value())
            return std::nullopt;
        auto mass = utils::get_numerics<Scalar>(*line, 1);
        return (mass.has_value()) ? mass->at(0) : std::optional<ParticleMass>{};
    }

    inline std::optional<DEDXMaterialCoefficients> parse_material_coefs(
        std::ifstream &dedx_stream)
    {
        auto coefs = DEDXMaterialCoefficients{};

        auto no_data = std::sregex_iterator();

        auto line = utils::find_line(dedx_stream, zoa_pattern);
        if (!line.has_value())
            return std::nullopt;
        auto nums = utils::get_numerics<Scalar>(*line, 1);
        if (!nums.has_value())
            return std::nullopt;
        coefs.ZoA = nums->at(0);

        line = utils::find_line(dedx_stream, coef_pattern);
        if (!line.has_value())
            return std::nullopt;

        std::getline(dedx_stream, *line);
        nums = utils::get_numerics<Scalar>(*line, 7);
        if (!nums.has_value())
            return std::nullopt;

        coefs.density_effect.a = nums->at(0);
        coefs.density_effect.k = nums->at(1);
        coefs.density_effect.x0 = nums->at(2);
        coefs.density_effect.x1 = nums->at(3);
        coefs.I = nums->at(4);
        coefs.density_effect.Cbar = nums->at(5);
        coefs.density_effect.delta0 = nums->at(6);

        return coefs;
    }

    inline std::optional<DEDXTable> parse_dedx_table(std::ifstream &dedx_stream)
    {
        auto table = DEDXTable{};

        auto no_data = std::sregex_iterator();

        auto line = utils::find_line(dedx_stream, table_pattern);
        if (!line.has_value())
            return std::nullopt;

        line = utils::find_line(dedx_stream, units_pattern);
        if (!line.has_value())
            return std::nullopt;

        while (std::getline(dedx_stream, *line))
        {
            auto nums = utils::get_numerics<Scalar>(*line, 11);
            if (!nums.has_value())
                return std::nullopt;

            table.T.push_back(nums->at(0));
            table.p.push_back(nums->at(1));
            table.Ionisation.push_back(nums->at(2));
            table.brems.push_back(nums->at(3));
            table.pair.push_back(nums->at(4));
            table.photonuc.push_back(nums->at(5));
            table.Radloss.push_back(nums->at(6));
            table.dEdX.push_back(nums->at(7));
            table.CSDArange.push_back(nums->at(8));
            table.delta.push_back(nums->at(9));
            table.beta.push_back(nums->at(10));
        }

        return table;
    }

    inline std::optional<DEDXData> parse_dedx_file(
        const DEDXFilePath &dedx_file_path, const ParticleName &particle_name)
    {
        if (!utils::check_path_exists(dedx_file_path))
            return std::nullopt;

        auto dedx_stream = std::ifstream{dedx_file_path};

        auto mass = parse_particle_mass(dedx_stream, particle_name);
        if (!mass.has_value())
        {
            std::cerr << "Particle mass entry corrupted in "
                      << dedx_file_path << std::endl;
            return std::nullopt;
        }

        auto coefs = parse_material_coefs(dedx_stream);
        if (!coefs.has_value())
        {
            std::cerr << "Material coefficients data corrupted in "
                      << dedx_file_path << std::endl;
            return std::nullopt;
        }

        auto table = parse_dedx_table(dedx_stream);
        if (!table.has_value())
        {
            std::cerr << "DEDX Table corrupted in " << dedx_file_path
                      << std::endl;
            return std::nullopt;
        }

        return std::make_optional<DEDXData>(*mass, *coefs, *table);
    }

    inline std::optional<MaterialsDEDXData> parse_materials(
        const Materials &materials, const DEDXFolderPath &dedx,
        const ParticleName &particle_name)
    {
        auto materials_data = MaterialsDEDXData{};
        for (const auto &[name, material] : materials)
        {
            auto dedx_data = parse_dedx_file(
                dedx / std::get<DEDXFilePath>(material), particle_name);
            if (!dedx_data.has_value())
                return std::nullopt;
            materials_data.emplace(name, *dedx_data);
        }
        return materials_data;
    }

    inline bool check_ZoA(
        const Settings &mdf_settings, const MaterialsDEDXData &dedx_data)
    {
        auto elements = std::get<Elements>(mdf_settings);
        auto materials = std::get<Materials>(mdf_settings);
        for (const auto &[material, data] : dedx_data)
        {
            auto coefs = std::get<DEDXMaterialCoefficients>(data);
            auto ZoA = 0.f;
            for (const auto &[elmt, frac] :
                 std::get<MaterialComponents>(materials.at(material)))
                ZoA += frac * elements.at(elmt).Z / elements.at(elmt).A;
            if ((std::abs(ZoA - coefs.ZoA) > utils::TOLERANCE))
                return false;
        }
        return true;
    }

} // namespace noa::pms::mdf