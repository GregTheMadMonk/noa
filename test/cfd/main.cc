#include <iostream>

#include <noa/utils/combine/combine.hh>
#include <noa/utils/cfd/cfd.hh>

int main(int argc, char** argv) {
    using DomainType     = noa::utils::domain::Domain<noa::TNL::Meshes::Topologies::Triangle>;
    using CFDProblemType = noa::utils::cfd::tasks::CFDProblem<DomainType>;
    using MHFEType       = noa::utils::cfd::tasks::MHFE<DomainType, true>;

    using noa::utils::combine::StaticComputation;

    StaticComputation<MHFEType> computation;
    auto& problem = computation.template get<CFDProblemType>();
    auto& mhfe    = computation.template get<MHFEType>();
    const auto& cproblem = computation.template get<CFDProblemType>();

    std::cout << problem.solution->getSize() << std::endl;
    std::cout << problem.edgeSolution->getSize() << std::endl;
    
    DomainType domain;
    noa::utils::domain::generate2DGrid(domain, 10, 20, 1.0, 1.0);

    problem.setMesh(domain.getMesh());

    std::cout << problem.solution->getSize() << std::endl;
    std::cout << problem.edgeSolution->getSize() << std::endl;

    // Set up the problem
    problem.solution.fill(0);
    problem.a.fill(1);
    problem.c.fill(1);

    problem.dirichlet.fill(0);
    problem.dirichletMask.fill(0);
    problem.neumann.fill(0);
    problem.neumannMask.fill(0);

    problem.edgeSolution.fill(0);

    problem.getDomain().getMesh().forBoundary<DomainType::dimEdge>(
        [&problem] (auto edgeIdx) {
            const auto& mesh = problem.getDomain().getMesh();

            const auto p1 = mesh.getPoint(mesh.template getSubentityIndex<DomainType::dimEdge, 0>(edgeIdx, 0));
            const auto p2 = mesh.getPoint(mesh.template getSubentityIndex<DomainType::dimEdge, 0>(edgeIdx, 1));

            // Edge direction vector
            const auto r = p2 - p1;
            // Edge center
            const auto c = (p1 + p2) / 2;

            // x = const boundary
            if (r[0] == 0) {
                problem.dirichletMask[edgeIdx] = 1;

                bool mask = (p1[0] == 0) && (c[1] > 1) && (c[1] < 9);

                problem.dirichlet[edgeIdx] = mask; // Implicit cast bool -> float
                problem.edgeSolution[edgeIdx] = mask;
            } else if (r[1] == 0) { // y = const boundary
                problem.neumannMask[edgeIdx] = 1;
            }
        }
    );

    mhfe.tau = 0.01;

    std::cout << problem.needsUpdate() << std::endl;
    computation.run();
    std::cout << problem.needsUpdate() << std::endl;
    // computation.run();
    // std::cout << problem.needsUpdate() << std::endl;

    return 0;
}
