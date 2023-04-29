#include <iostream>

#include <noa/utils/combine/combine.hh>
#include <noa/utils/cfd/cfd.hh>

int main(int argc, char** argv) {
    using DomainType = noa::utils::domain::Domain<noa::TNL::Meshes::Topologies::Triangle>;
    using Method     = noa::utils::cfd::methods::MHFE<DomainType>;
    using CFDProblemType = noa::utils::cfd::tasks::CFDProblem<DomainType>;

    using noa::utils::combine::StaticComputation;

    StaticComputation<noa::utils::cfd::tasks::MHFE<Method>> computation;
    auto& problem = computation.template get<CFDProblemType>();
    const auto& cproblem = computation.template get<CFDProblemType>();

    std::cout << problem.solution->getSize() << std::endl;
    std::cout << problem.edgeSolution->getSize() << std::endl;
    
    DomainType domain;
    noa::utils::domain::generate2DGrid(domain, 10, 20, 1.0, 1.0);

    problem.setMesh(domain.getMesh());

    std::cout << problem.solution->getSize() << std::endl;
    std::cout << problem.edgeSolution->getSize() << std::endl;

    return 0;
}
