#include <iostream>

#include <noa/utils/combine/combine.hh>
#include <noa/cfd/mhfe.hh>

int main(int argc, char** argv) {
    using Domain =
        noa::utils::domain::Domain<noa::TNL::Meshes::Topologies::Triangle>;
    using Problem =
        noa::cfd::CFDProblem<Domain>;
    using MHFE =
        noa::cfd::MHFE<Domain, true>;

    namespace cmb = noa::utils::combine;

    cmb::DynamicComposer<MHFE> comp{};
    comp.setTasks<MHFE>({
//  cmb::StaticComposer<MHFE> comp{
        [] (Problem& problem) {
            // Initialize the problem
            // Create the mesh
            const std::size_t   Nx = 5;
            const Problem::Real dx = 10.0 / Nx;
            Domain dummy{};
            dummy.generateGrid(
                { 2 * Nx, Nx },
                { dx,     dx}
            );
            problem.setMesh(std::move(dummy.getMesh()));

            std::cout << problem.solution->getSize() << std::endl;
            std::cout << problem.edgeSolution->getSize() << std::endl;

            *problem.solution = 0;
            *problem.a = 1;
            *problem.c = 1;

            *problem.dirichlet = 0;
            *problem.dirichletMask = 0;
            *problem.neumann = 0;
            *problem.neumannMask = 0;

            *problem.edgeSolution = 0;

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
                        problem.edgeSolution[edge] = mask;
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

#if 0
    std::ofstream f("dump.dat");
    while (mhfe.getTime() < 25.0) {
        computation.run();
        std::cerr << "\r" << mhfe.getTime()
            << " : " << sum(computation.template get<GradEvType>().getResult())
            << " : " << sum(computation.template get<FinDiffType>().getResult())
            << "                 ";
        f << mhfe.getTime()
            << ", " << sum(computation.template get<GradEvType>().getResult())
            << ", " << sum(computation.template get<FinDiffType>().getResult())
            << std::endl;
        // std::cerr << g<VectorType>(*problem.solution) << std::endl;
        problem.getDomain().write("temp/" + std::to_string(mhfe.getTime()) + ".vtu");
    }
    std::cerr << std::endl;

#endif
    return 0;
}
