#pragma once

#include <noa/utils/domain/domain.hh>

#include <gtest/gtest.h>

TEST(DOMAIN, Create2DGridTriangle) {
	using namespace noa;

	using DomainType = utils::domain::Domain<TNL::Meshes::Topologies::Triangle>;

	DomainType domain;

	constexpr auto N = 10;
    domain.generateGrid({ N, N }, { 1.0, 1.0 });

	ASSERT_EQ(domain.getMesh().template getEntitiesCount<DomainType::dCell>(), N * N * 2);
}

TEST(DOMAIN, SaveLoadDomain) {
	using namespace noa;

	using DomainType = utils::domain::Domain<TNL::Meshes::Topologies::Triangle>;

	DomainType domain;

	constexpr auto N = 10;
	domain.generateGrid({ N, N }, { 1.0, 1.0 });

	auto& cellLayers = domain.getLayers(DomainType::dCell);

	auto& l1 = cellLayers.template add<float>(0, 3.1415f);
	l1.alias = "Float Layer";
	l1.exportHint = true;
	auto& l2 = cellLayers.template add<double>(1, 2.7182);
	l2.alias = "Double Layer";
	l2.exportHint = true;
	auto& l3 = cellLayers.template add<int>(2, 42);
	l3.alias = "Integer Layer";
	l3.exportHint = true;
	auto& l4 = cellLayers.template add<int>(3, 142);
	l4.alias = "Unsaved layer";

	constexpr auto test_domain_file = "domain-test-save-load.vtu";
	domain.write(test_domain_file);

	DomainType domain2;
	domain2.loadFrom(test_domain_file, { { "Float Layer", 0 }, { "Double Layer", 1 }, { "Integer Layer", 2 } });

	ASSERT_EQ(
        domain.getMesh().template getEntitiesCount<DomainType::dCell>(),
        domain2.getMesh().template getEntitiesCount<DomainType::dCell>()
    );

	std::size_t i = 0;
	ASSERT_EQ(cellLayers.getLayer(i).alias, domain2.getLayers(DomainType::dCell).getLayer(i).alias);
	ASSERT_EQ(cellLayers.template get<float>(i), domain2.getLayers(DomainType::dCell).template get<float>(i));
	i = 1;
	ASSERT_EQ(cellLayers.getLayer(i).alias, domain2.getLayers(DomainType::dCell).getLayer(i).alias);
	ASSERT_EQ(cellLayers.template get<double>(i), domain2.getLayers(DomainType::dCell).template get<double>(i));
	i = 2;
	ASSERT_EQ(cellLayers.getLayer(i).alias, domain2.getLayers(DomainType::dCell).getLayer(i).alias);
	ASSERT_EQ(cellLayers.template get<int>(i), domain2.getLayers(DomainType::dCell).template get<int>(i));

	constexpr auto test_domain_file_2 = "domain-test-save-load-2.vtu";
	domain2.write(test_domain_file_2);

	ASSERT_EQ(utils::compare_files(test_domain_file, test_domain_file_2), true);
}

TEST(DOMAIN, CopyMoveDomain) {
	using namespace noa;

	using DomainType = utils::domain::Domain<TNL::Meshes::Topologies::Triangle>;
	DomainType domain;

	constexpr auto N = 10;
	domain.generateGrid({ N, N }, { 1.0, 1.0 });

	auto& cellLayers = domain.getLayers(DomainType::dCell);

	cellLayers.template add<float>(0, 3.1415f);

    DomainType domainCopy = domain;

    ASSERT_EQ(
        domain.getMesh().getEntitiesCount<DomainType::dCell>(),
        domainCopy.getMesh().getEntitiesCount<DomainType::dCell>()
    );

    ASSERT_EQ(
        domain.getLayers(DomainType::dCell).template get<float>(0).getView(),
        domainCopy.getLayers(DomainType::dCell).template get<float>(0).getView()
    );

    DomainType domainMoved = std::move(domain);

    ASSERT_EQ(
        domainMoved.getMesh().getEntitiesCount<DomainType::dCell>(),
        domainCopy.getMesh().getEntitiesCount<DomainType::dCell>()
    );

    ASSERT_EQ(
        domainMoved.getLayers(DomainType::dCell).template get<float>(0).getView(),
        domainCopy.getLayers(DomainType::dCell).template get<float>(0).getView()
    );
}
