/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2026 SEN Labs e.U.
 */

/** Tests of SenGraph.h: the graph of the objects of SEN, its layout and its SVG. Plain C++, no Haiku API. */

#include <chrono>
#include <cstdlib>

#include "TestHarness.h"
#include "../src/cpp/include/SenGraph.h"

using namespace sen::graph;

static float Distance(const Graph& graph, int a, int b)
{
	const Node& first = graph.Nodes()[a];
	const Node& second = graph.Nodes()[b];
	return std::sqrt((first.x - second.x) * (first.x - second.x) + (first.y - second.y) * (first.y - second.y));
}

TEST(NodesAreUniqueById)
{
	Graph graph;
	CHECK_EQ(graph.AddNode("A", "Anna", "person"), 0);
	CHECK_EQ(graph.AddNode("B", "Ben", "person"), 1);
	CHECK_EQ(graph.AddNode("A", "other name", "x"), 0);
	CHECK_EQ(graph.Nodes().size(), 2u);
	CHECK_STR(graph.Nodes()[0].label, "Anna");
	CHECK_EQ(graph.IndexOf("B"), 1);
	CHECK_EQ(graph.IndexOf("Z"), -1);
}

TEST(EdgesNeedNodesAndAreNoLoops)
{
	Graph graph;
	graph.AddNode("A", "Anna", "person");
	graph.AddNode("B", "Ben", "person");
	CHECK(!graph.AddEdge("A", "Z", "knows", "knows"));
	CHECK(!graph.AddEdge("Z", "A", "knows", "knows"));
	CHECK(!graph.AddEdge("A", "A", "knows", "knows"));
	CHECK(graph.AddEdge("A", "B", "knows", "knows"));
	CHECK(graph.AddEdge("A", "B", "knows", "knows"));	// the same direction again: it is there
	CHECK_EQ(graph.Edges().size(), 1u);
}

TEST(TheOppositeDirectionIsTheSameEdge)
{
	Graph graph;
	graph.AddNode("A", "Anna", "person");
	graph.AddNode("B", "Ben", "person");
	graph.AddEdge("A", "B", "parent of", "family");
	graph.AddEdge("B", "A", "child of", "family");
	CHECK_EQ(graph.Edges().size(), 1u);
	CHECK(graph.Edges()[0].reciprocal);
	CHECK_STR(graph.Edges()[0].label, "parent of");
	CHECK_STR(graph.Edges()[0].inverseLabel, "child of");
}

TEST(ConnectedKeepsWhatHasEdges)
{
	Graph graph;
	for (const char* id : {"A", "B", "C", "D", "E"})
		graph.AddNode(id, id, "g");
	graph.AddEdge("A", "B", "x", "t");
	graph.AddEdge("C", "D", "in", "association", true);	// meta
	Graph all = graph.Connected(true);
	CHECK_EQ(all.Nodes().size(), 4u);
	CHECK_EQ(all.Edges().size(), 2u);
	Graph noMeta = graph.Connected(false);
	CHECK_EQ(noMeta.Nodes().size(), 2u);
	CHECK_EQ(noMeta.Edges().size(), 1u);
	CHECK_STR(noMeta.Nodes()[noMeta.Edges()[0].to].id, "B");
}

TEST(FilteredKeepsWhatIsWanted)
{
	Graph graph;
	for (const char* id : {"A", "B", "C", "D", "E"})
		graph.AddNode(id, id, "g");
	graph.AddEdge("A", "B", "x", "t");
	graph.AddEdge("C", "D", "in", "association", true);
	Graph everything = graph.Filtered(true, true);
	CHECK_EQ(everything.Nodes().size(), 5u);
	CHECK_EQ(everything.Edges().size(), 2u);
	Graph withoutMeta = graph.Filtered(false, true);	// C and D stay, as nodes without an edge
	CHECK_EQ(withoutMeta.Nodes().size(), 5u);
	CHECK_EQ(withoutMeta.Edges().size(), 1u);
	CHECK(graph.Filtered(false, false).IndexOf("C") < 0);
	CHECK(graph.Filtered(true, false).IndexOf("C") >= 0);
	CHECK(graph.Filtered(true, false).IndexOf("E") < 0);
}

TEST(LayoutIsTheSameEveryTime)
{
	auto make = [] {
		Graph graph;
		for (int i = 0; i < 30; i++)
			graph.AddNode("n" + std::to_string(i), "n", "g");
		for (int i = 1; i < 30; i++)
			graph.AddEdge("n" + std::to_string(i), "n" + std::to_string(i / 2), "to", "t");
		return graph;
	};
	Graph first = make(), second = make();
	first.Layout();
	second.Layout();
	for (size_t i = 0; i < first.Nodes().size(); i++) {
		CHECK(first.Nodes()[i].x == second.Nodes()[i].x);
		CHECK(first.Nodes()[i].y == second.Nodes()[i].y);
		CHECK(first.Nodes()[i].x >= 0 && first.Nodes()[i].y >= 0);
	}
	CHECK_EQ(first.Nodes()[1].degree, 3);	// n1: its parent n0 and its children n2 and n3
}

TEST(ConnectedNodesAreCloserThanOthers)
{
	// two chains of 6 nodes that have nothing to do with each other
	Graph graph;
	for (int i = 0; i < 12; i++)
		graph.AddNode("n" + std::to_string(i), "n", "g");
	for (int i = 0; i < 5; i++) {
		graph.AddEdge("n" + std::to_string(i), "n" + std::to_string(i + 1), "to", "t");
		graph.AddEdge("n" + std::to_string(6 + i), "n" + std::to_string(7 + i), "to", "t");
	}
	graph.Layout();
	float near = 0, far = 0;
	for (int i = 0; i < 5; i++) {
		near += Distance(graph, i, i + 1);
		near += Distance(graph, 6 + i, 7 + i);
	}
	near /= 10;
	for (int i = 0; i < 6; i++)
		far += Distance(graph, i, 6 + i);
	far /= 6;
	CHECK(near < far);
	// no two nodes in one place
	for (int i = 0; i < 12; i++)
		for (int j = i + 1; j < 12; j++)
			CHECK(Distance(graph, i, j) > 1.0f);
}

TEST(SeparatePartsAreLaidOutByThemselvesAndDoNotLookLikeAFrame)
{
	// twelve stars of different sizes that have nothing to do with each other (the types that the ontologies define)
	Graph graph;
	std::vector<std::vector<int>> stars;
	int total = 0;
	for (int s = 0; s < 12; s++) {
		const int leaves = 3 + (s * 5) % 9;
		const std::string hub = "hub" + std::to_string(s);
		graph.AddNode(hub, hub, "ontology");
		for (int l = 0; l < leaves; l++) {
			graph.AddNode(hub + "." + std::to_string(l), "leaf", "type");
			graph.AddEdge(hub, hub + "." + std::to_string(l), "defines", "dependency");
		}
		total += leaves + 1;
	}
	graph.Layout();

	float left, top, right, bottom;
	graph.Bounds(&left, &top, &right, &bottom);
	const float width = right - left, height = bottom - top;
	CHECK(width / height < 3.0f && height / width < 3.0f);

	// no node is on top of another, and a star holds together: its leaves are near their hub
	for (size_t i = 0; i < graph.Nodes().size(); i++)
		for (size_t j = i + 1; j < graph.Nodes().size(); j++)
			CHECK(Distance(graph, static_cast<int>(i), static_cast<int>(j)) > 3.0f);
	for (const Edge& edge : graph.Edges())
		CHECK(Distance(graph, edge.from, edge.to) < 0.5f * std::max(width, height));

	// the nodes of a star are closer to each other than to those of the next star
	float inside = 0, between = 0;
	int insideCount = 0, betweenCount = 0;
	for (const Edge& edge : graph.Edges()) {
		inside += Distance(graph, edge.from, edge.to);
		insideCount++;
	}
	for (int s = 0; s + 1 < 12; s++) {
		between += Distance(graph, graph.IndexOf("hub" + std::to_string(s)), graph.IndexOf("hub" + std::to_string(s + 1)));
		betweenCount++;
	}
	CHECK(inside / insideCount < between / betweenCount);
	(void)total;
}

TEST(OneNodeAndNoNode)
{
	Graph empty;
	empty.Layout();
	CHECK(empty.ToSvg().find("</svg>") != std::string::npos);
	Graph one;
	one.AddNode("A", "Anna", "person");
	one.Layout();
	CHECK_EQ(one.Nodes()[0].degree, 0);
	CHECK(one.Nodes()[0].x >= 0);
}

TEST(TheSvgHasWhatIsInTheGraph)
{
	Graph graph;
	graph.AddNode("A", "Anna <&> \"Co\"", "person");
	graph.AddNode("B", "Ben", "person");
	graph.AddNode("C", "Carl", "book");
	graph.AddEdge("A", "B", "parent of", "family");
	graph.AddEdge("B", "A", "child of", "family");
	graph.AddEdge("A", "C", "reads", "reading");
	graph.Layout();
	const std::string svg = graph.ToSvg();
	CHECK(svg.find("<svg ") == 0 || svg.find("<svg ") != std::string::npos);
	CHECK(svg.find("Anna &lt;&amp;&gt; &quot;Co&quot;") != std::string::npos);
	CHECK(svg.find("parent of / child of") != std::string::npos);
	CHECK(svg.find("reads") != std::string::npos);
	size_t arrows = 0;
	for (size_t at = svg.find("marker-end"); at != std::string::npos; at = svg.find("marker-end", at + 1))
		arrows++;
	CHECK_EQ(arrows, 1u);	// only the relation in one direction has an arrow
	size_t circles = 0;
	for (size_t at = svg.find("<circle"); at != std::string::npos; at = svg.find("<circle", at + 1))
		circles++;
	CHECK_EQ(circles, 3u);

	SvgOptions plain;
	plain.nodeLabels = false;
	plain.edgeLabels = false;
	SvgOptions dark;
	dark.dark = true;
	const std::string night = graph.ToSvg(dark);
	CHECK(night.find("#16181d") != std::string::npos);	// the dark background
	CHECK(svg.find("#16181d") == std::string::npos);
	CHECK(svg.find("#ffffff") != std::string::npos);
	const std::string small = graph.ToSvg(plain);
	CHECK(small.find("reads") == std::string::npos);
	CHECK(small.find(">Ben</text>") == std::string::npos);
}

TEST(TheSameGroupHasTheSameColour)
{
	Rgb a = GroupColor("entity/x-vnd.sen-labs.entity.person"), b = GroupColor("entity/x-vnd.sen-labs.entity.person");
	CHECK(a.r == b.r && a.g == b.g && a.b == b.b);
	Rgb c = GroupColor("application/pdf");
	CHECK(!(a.r == c.r && a.g == c.g && a.b == c.b));
}

TEST(ManyNodesAreLaidOutInAFewSeconds)
{
	Graph graph;
	const int count = 1500;
	for (int i = 0; i < count; i++)
		graph.AddNode("n" + std::to_string(i), "n" + std::to_string(i), "g" + std::to_string(i % 7));
	srand(7);
	for (int i = 1; i < count; i++)
		graph.AddEdge("n" + std::to_string(i), "n" + std::to_string(rand() % i), "to", "t");
	const auto start = std::chrono::steady_clock::now();
	graph.Layout();
	const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
	printf("  (%d nodes laid out in %.2f s)\n", count, seconds);
	CHECK(seconds < 10);
}

TEST_MAIN()
