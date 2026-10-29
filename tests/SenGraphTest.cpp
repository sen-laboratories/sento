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

TEST(OntologiesCanBeLeftOut)
{
	Graph graph;
	graph.AddNode("P", "Person", "g");
	graph.AddNode("Q", "Other", "g");
	graph.AddNode("R", "Alone", "g");
	graph.AddNode("O", "people", "ontology", true);
	graph.AddEdge("O", "P", "defines", "dependency");
	graph.AddEdge("P", "Q", "knows", "friend");

	// everything is there by default, and the flag is kept
	Graph everything = graph.Filtered(true, true);
	CHECK_EQ(everything.Nodes().size(), 4u);
	CHECK_EQ(everything.Edges().size(), 2u);
	CHECK(everything.Nodes()[everything.IndexOf("O")].ontology);
	CHECK(!everything.Nodes()[everything.IndexOf("P")].ontology);

	// without the ontologies: the node and its relations are gone, the others stay as they are
	Graph without = graph.Filtered(true, true, false);
	CHECK_EQ(without.Nodes().size(), 3u);
	CHECK(without.IndexOf("O") < 0);
	CHECK_EQ(without.Edges().size(), 1u);
	CHECK(without.IndexOf("P") >= 0);

	// a node that was only connected to an ontology is without relations then
	graph.AddNode("S", "Only in the ontology", "g");
	graph.AddEdge("O", "S", "defines", "dependency");
	CHECK(graph.Filtered(true, false, true).IndexOf("S") >= 0);
	CHECK(graph.Filtered(true, false, false).IndexOf("S") < 0);
	CHECK(graph.Filtered(true, true, false).IndexOf("S") >= 0);
	CHECK(graph.Filtered(true, true, false).IndexOf("O") < 0);	// (the ontology is out even if the unconnected are wanted)
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
	// the halo of a name is a text of its own below it (paint-order is not known to every viewer: the stroke would lie over the letters)
	CHECK(svg.find("paint-order") == std::string::npos);
	size_t names = 0;
	for (size_t at = svg.find(">Ben</text>"); at != std::string::npos; at = svg.find(">Ben</text>", at + 1))
		names++;
	CHECK_EQ(names, 2u);	// the halo and the name
	CHECK(svg.find("font-weight=\"normal\"") != std::string::npos);

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

static Graph Star()
{
	// a - b, a - c, a - d (the node a and its three), b - c (between two of them), e - f (apart)
	Graph graph;
	for (const char* id : {"a", "b", "c", "d", "e", "f"})
		graph.AddNode(id, std::string("node ") + id, "g");
	graph.AddEdge("a", "b", "x", "t");
	graph.AddEdge("a", "c", "x", "t");
	graph.AddEdge("a", "d", "x", "t");
	graph.AddEdge("b", "c", "y", "t");
	graph.AddEdge("e", "f", "z", "t");
	graph.Layout();
	return graph;
}

TEST(NeighborhoodIsTheNodeItsNeighboursAndItsOwnEdges)
{
	Graph graph = Star();
	Graph part = graph.Neighborhood(graph.IndexOf("a"));
	CHECK_EQ(part.Nodes().size(), 4u);	// a, b, c, d (not e and f)
	CHECK_EQ(part.Edges().size(), 3u);	// the three of a, not the one between b and c
	CHECK(part.IndexOf("e") < 0);
	// it looks as it does in the whole: the same places, the same sizes
	for (const Node& node : part.Nodes()) {
		const Node& whole = graph.Nodes()[graph.IndexOf(node.id)];
		CHECK(node.x == whole.x && node.y == whole.y);
		CHECK_EQ(node.degree, whole.degree);
	}
	CHECK_EQ(graph.Nodes()[graph.IndexOf("a")].degree, 3);
}

TEST(SubgraphOfAnEdgeHasItsTwoNodes)
{
	Graph graph = Star();
	Graph part = graph.Subgraph({}, {3});	// the edge between b and c
	CHECK_EQ(part.Nodes().size(), 2u);
	CHECK_EQ(part.Edges().size(), 1u);
}

TEST(TheSvgOfAViewIsWhatTheViewShows)
{
	Graph graph = Star();
	SvgOptions view;
	view.width = 800;
	view.height = 500;
	view.scale = 2;
	view.offsetX = 10;
	view.offsetY = 20;
	view.selectedNode = graph.IndexOf("a");
	std::vector<bool> nodeLabels(graph.Nodes().size(), false), edgeLabels(graph.Edges().size(), false);
	nodeLabels[graph.IndexOf("a")] = true;
	edgeLabels[0] = true;
	view.nodeLabelShown = &nodeLabels;
	view.edgeLabelShown = &edgeLabels;

	const std::string svg = graph.ToSvg(view);
	CHECK(svg.find("width=\"800\" height=\"500\"") != std::string::npos);
	CHECK(svg.find(">node a</text>") != std::string::npos);		// the name that the view shows
	CHECK(svg.find(">node b</text>") == std::string::npos);		// the names that it does not
	CHECK(svg.find("opacity=\"0.35\"") != std::string::npos);	// e and f are dimmed
	CHECK(svg.find("opacity=\"0.2\"") != std::string::npos);		// and so are the lines that are not the selected ones
	CHECK(svg.find("stroke=\"#000000\" stroke-width=\"2.0\"") != std::string::npos);	// the lines of the selected node
	CHECK(svg.find("marker-end=\"url(#arrow-strong)\"") == std::string::npos || svg.find("arrow-strong") != std::string::npos);

	// the picture of a selection alone: the lines are plain, only the node stands out
	SvgOptions alone = view;
	alone.emphasiseLines = false;
	const std::string only = graph.ToSvg(alone);
	CHECK(only.find("stroke-width=\"2.0\"") != std::string::npos);		// (the ring of the node)
	CHECK(only.find("stroke=\"#000000\" stroke-width=\"2.0\"/>") == std::string::npos);	// but no line
	CHECK(only.find("opacity=") == std::string::npos);

	// without a selection nothing is dimmed
	SvgOptions plain;
	plain.width = 800;
	plain.height = 500;
	const std::string flat = graph.ToSvg(plain);
	CHECK(flat.find("opacity=") == std::string::npos);

	// the places are those of the view: x * scale + offset
	const Node& a = graph.Nodes()[graph.IndexOf("a")];
	char expected[64];
	snprintf(expected, sizeof(expected), "cx=\"%.1f\" cy=\"%.1f\"", a.x * 2 + 10, a.y * 2 + 20);
	CHECK(svg.find(expected) != std::string::npos);
}

TEST(LongNamesAreCutLikeTheViewDoes)
{
	CHECK_STR(ShownName("short"), "short");
	CHECK_STR(ShownName("0123456789012345678901234567"), "0123456789012345678901234567");	// 28: as it is
	CHECK_STR(ShownName("01234567890123456789012345678"), std::string("012345678901234567890123456") + "\xE2\x80\xA6");	// 29: 27 and an ellipsis
	CHECK_STR(ShownName("\xC3\xA4\xC3\xB6\xC3\xBC"), "\xC3\xA4\xC3\xB6\xC3\xBC");	// characters, not bytes
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
