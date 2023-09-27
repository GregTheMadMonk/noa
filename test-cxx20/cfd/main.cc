#include <iostream>

#include <noa-cxx20/combine/combine.hh>
#include <noa-cxx20/cfd/mhfe.hh>

int main(int argc, char** argv) {
    using Domain =
        noa::utils::domain::Domain<noa::TNL::Meshes::Topologies::Triangle>;
    using Problem =
        noa::cfd::CFDProblem<Domain>;
    using MHFE =
        noa::cfd::MHFE<Domain, true>;

    namespace cmb = noa::combine;

    cmb::DynamicComposer<MHFE> comp{};
    comp.setTasks<MHFE>({
//  cmb::StaticComposer<MHFE> comp{
        [] (Problem& problem) {
            // Initialize the problem
            // Create the mesh
            const std::size_t   Nx = 10;
            const Problem::Real dx = 10.0 / Nx;
            Domain dummy{};
            dummy.generateGrid(
                { 2 * Nx, Nx },
                { dx,     dx}
            );
            problem.setMesh(std::move(dummy.getMesh()));

            std::cout << problem.a->getSize() << std::endl;
            std::cout << problem.dirichlet->getSize() << std::endl;

            *problem.a = 1;
            *problem.c = 1;

            *problem.dirichlet = 0;
            *problem.dirichletMask = 0;
            *problem.neumann = 0;
            *problem.neumannMask = 0;

            auto& mesh = problem.getDomain().getMesh();
            mesh.forBoundary<Domain::dEdge>(
                [&mesh, &problem] (auto edge) {
                    const auto p1 = mesh.getPoint(
                        mesh.template getSubentityIndex<Domain::dEdge, 0>(
                            edge, 0
                        )
                    );
                    const auto p2 = mesh.getPoint(
                        mesh.template getSubentityIndex<Domain::dEdge, 0>(
                            edge, 1
                        )
                    );

                    // Edge direction vector
                    const auto r = p2 - p1;
                    // Edge center
                    const auto c = (p2 + p1) / 2;

                    // x = const boundary
                    if (r[0] == 0) {
                        problem.dirichletMask[edge] = 1;

                        bool mask =
                            (p1[0] == 0) && (c[1] > 1) && (c[1] < 9);

                        problem.dirichlet[edge] = mask;
                    } else if (r[1] == 0) { // y = const boundary
                        problem.neumannMask[edge] = 1;
                    }
                }
            );

            // Set the time step
            problem.setTau(0.005);
        }
    }
    );

    const auto& problem = comp.template get<Problem>();
    for (
        auto time = problem.getTime();
        time < 10.0;
        comp.run(), time = problem.getTime()
    ) {
        problem.getDomain().write("out/" + std::to_string(time) + ".vtu");
        std::cerr << "\r" << time << "            ";
    }
    std::cerr << '\n';

    return 0;
}
