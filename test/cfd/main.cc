#include <iostream>

#include <noa/utils/combine/combine.hh>
#include <noa/utils/cfd/cfd.hh>

template <typename VectorType>
void gWrtP(typename VectorType::ConstViewType in, typename VectorType::ViewType out) {
}

int main(int argc, char** argv) {
    using DomainType     = noa::utils::domain::Domain<noa::TNL::Meshes::Topologies::Triangle>;
    using VectorType     = DomainType::LayerManagerType::Vector<typename DomainType::RealType>;
    using CFDProblemType = noa::utils::cfd::tasks::CFDProblem<DomainType>;
    using MHFEType       = noa::utils::cfd::tasks::MHFE<DomainType, false>;
    // using GradEvType     = noa::utils::cfd::tasks::GradEv<gWrtP<VectorType>, DomainType>;

    using noa::utils::combine::StaticComputation;

    StaticComputation<MHFEType> computation;
    // StaticComputation<GradEvType> computation;
    auto& problem = computation.template get<CFDProblemType>();
    auto& mhfe    = computation.template get<MHFEType>();

    std::cout << problem.solution->getSize() << std::endl;
    std::cout << problem.edgeSolution->getSize() << std::endl;
    
    DomainType domain;
    noa::utils::domain::generate2DGrid(domain, 20, 10, 1.0, 1.0);

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

    mhfe.tau = 0.005;

    while (mhfe.getTime() < 1) {
        computation.run();
        std::cerr << "\r" << mhfe.getTime() << "                 ";
        problem.getDomain().write("temp/" + std::to_string(mhfe.getTime()) + ".vtu");
    }
    std::cerr << std::endl;

    return 0;
}
