/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2026 SEN Labs e.U.
 */
#pragma once

#include <stdint.h>
#include <stdio.h>

#include <algorithm>
#include <cmath>
#include <string>
#include <unordered_map>
#include <vector>

/**
 * @file SenGraph.h
 * @brief A small graph with a force layout and an SVG writer, for the objects of SEN and their relations. Plain C++ (no Haiku API, no libraries).
 *
 * The nodes are the files that have a `SEN:ID`, the edges are the entries of their `SEN:TO` lists (a relation in one direction; relations are stored
 * in both directions, so the two are one edge that is marked as reciprocal, with the label of each direction). The labels and the types are those of
 * the relations in `SEN:REL:*`. The layout is that of Fruchterman and Reingold (the nodes push each other away, the edges pull their ends together,
 * a weak pull to the middle keeps separate parts of the graph together), which is fine for some thousands of nodes. The same positions are drawn by
 * a view and written as SVG, see ToSvg().
 */
namespace sen {
namespace graph {

/** A node: a file with a SEN:ID. */
struct Node {
	std::string	id;			///< the SEN:ID
	std::string	label;		///< what is shown (the name)
	std::string	group;		///< what colours it (the type of the file)
	float		x = 0;
	float		y = 0;
	int			degree = 0;	///< the number of edges at it (see Graph::Layout())
	bool		ontology = false;	///< the file of an ontology (what defines the types: the meta level of SEN), which can be left out
};

/** An edge: a relation from one node to another (and back, if it is reciprocal). */
struct Edge {
	int			from = 0;
	int			to = 0;
	std::string	label;			///< of the relation from -> to
	std::string	inverseLabel;	///< of the opposite direction (reciprocal edges)
	std::string	type;			///< the relation type
	bool		reciprocal = false;
	bool		meta = false;	///< a meta relation (classification, context): drawn lighter
};

struct Rgb {
	uint8_t	r, g, b;
};

/** A colour for a group, always the same one for the same name (a hue out of a hash); a bit lighter on a dark background. */
inline Rgb
GroupColor(const std::string& group, bool dark = false)
{
	uint32_t hash = 2166136261u;
	for (unsigned char c : group) {
		hash ^= c;
		hash *= 16777619u;
	}
	const float hue = (hash % 360) / 60.0f, saturation = dark ? 0.6f : 0.55f, lightness = dark ? 0.64f : 0.56f;
	const float chroma = (1 - std::fabs(2 * lightness - 1)) * saturation;
	const float second = chroma * (1 - std::fabs(std::fmod(hue, 2.0f) - 1));
	float r = 0, g = 0, b = 0;
	switch (static_cast<int>(hue)) {
		case 0: r = chroma; g = second; break;
		case 1: r = second; g = chroma; break;
		case 2: g = chroma; b = second; break;
		case 3: g = second; b = chroma; break;
		case 4: r = second; b = chroma; break;
		default: r = chroma; b = second; break;
	}
	const float match = lightness - chroma / 2;
	return Rgb{static_cast<uint8_t>((r + match) * 255), static_cast<uint8_t>((g + match) * 255), static_cast<uint8_t>((b + match) * 255)};
}

/** The colours of the graph, light or dark: the same for a view and for the SVG. */
struct Palette {
	Rgb	background;
	Rgb	text;		///< names of the nodes
	Rgb	edgeText;	///< labels of the edges
	Rgb	edge;
	Rgb	metaEdge;
	Rgb	emphasis;	///< the edges of the selected node, the ring of a selected node
};

inline Palette
PaletteFor(bool dark)
{
	if (dark)
		return Palette{{0x16, 0x18, 0x1d}, {0xe8, 0xea, 0xee}, {0x9a, 0xa1, 0xad}, {0x6c, 0x73, 0x80}, {0x3b, 0x40, 0x4a}, {0xff, 0xff, 0xff}};
	return Palette{{0xff, 0xff, 0xff}, {0x22, 0x22, 0x22}, {0x66, 0x66, 0x66}, {0x8a, 0x8a, 0x8a}, {0xd0, 0xd0, 0xd0}, {0x00, 0x00, 0x00}};
}

/** "#rrggbb" */
inline std::string
Hex(const Rgb& color)
{
	char buffer[8];
	snprintf(buffer, sizeof(buffer), "#%02x%02x%02x", color.r, color.g, color.b);
	return buffer;
}

/** The width of the lines: the others, and those of the selection. */
inline constexpr float kLineWidth = 1.2f;
inline constexpr float kStrongLineWidth = 2.0f;

/** The size (radius) of a node: more edges, bigger. */
inline float
NodeRadius(int degree)
{
	return 5.0f + 2.0f * std::sqrt(static_cast<float>(std::max(degree, 0)));
}

/** The size of a node when the graph is shown at a zoom: the nodes do not grow as fast as the distances do (a view and the SVG of it use this). */
inline float
NodeRadiusAt(int degree, float scale)
{
	return NodeRadius(degree) * std::min(std::max(scale, 0.7f), 1.6f);
}

/** A name as it is shown: not longer than 28 characters (UTF-8), cut with an ellipsis. */
inline std::string
ShownName(const std::string& name)
{
	const size_t limit = 28;
	size_t characters = 0, end = 0;
	for (size_t i = 0; i < name.size(); i++) {
		if ((static_cast<unsigned char>(name[i]) & 0xC0) != 0x80) {
			if (characters == limit - 1)
				end = i;
			characters++;
		}
	}
	return characters <= limit ? name : name.substr(0, end) + "\xE2\x80\xA6";
}

/**
 * What ToSvg() draws. By default the whole graph, in the size that it has (with a margin). Given the size, the zoom and the shift of a view, the selection and
 * the labels that the view shows, it is the picture of that view: what is seen there is in the SVG.
 */
struct SvgOptions {
	bool	nodeLabels = true;		///< the names of the nodes (if nodeLabelShown is not given: of all)
	bool	edgeLabels = true;
	bool	dark = false;
	float	margin = 50;
	float	fontSize = 10;

	// as a view shows the graph (all optional)
	float	width = 0;				///< the size of the picture: 0 is the size of the graph with the margin
	float	height = 0;
	float	scale = 1;				///< (with the size) the positions of the nodes are x * scale + offsetX
	float	offsetX = 0;
	float	offsetY = 0;
	int		selectedNode = -1;		///< emphasised with its relations, the rest is dimmed
	int		selectedEdge = -1;
	bool	emphasiseLines = true;	///< the lines of the selection are emphasised and the rest is dimmed (not for a picture of the selection alone: only the node stands out)
	const std::vector<bool>*	nodeLabelShown = NULL;	///< which names are there (the view leaves out those that would lie on others)
	const std::vector<bool>*	edgeLabelShown = NULL;
};

class Graph {
public:
	/** Add a node; the node of the id that is there is returned if there is one. @return its index */
	int AddNode(const std::string& id, const std::string& label, const std::string& group, bool ontology = false)
	{
		auto found = fIndex.find(id);
		if (found != fIndex.end())
			return found->second;
		Node node;
		node.id = id;
		node.label = label;
		node.group = group;
		node.ontology = ontology;
		fNodes.push_back(node);
		fIndex[id] = static_cast<int>(fNodes.size()) - 1;
		return fIndex[id];
	}

	/** @return the index of the node of an id, or -1 */
	int IndexOf(const std::string& id) const
	{
		auto found = fIndex.find(id);
		return found == fIndex.end() ? -1 : found->second;
	}

	/**
	 * Add a relation. Both nodes must be there (else false). A relation to itself is none. When the opposite direction is there already, it is the
	 * same edge (reciprocal) that gets the label of this direction; the same direction twice is one edge.
	 * @return true if the graph has the relation now (new, or merged)
	 */
	bool AddEdge(const std::string& fromId, const std::string& toId, const std::string& label, const std::string& type, bool meta = false)
	{
		const int from = IndexOf(fromId), to = IndexOf(toId);
		if (from < 0 || to < 0 || from == to)
			return false;
		if (fEdges.count(Key(from, to)) != 0)
			return true;	// that direction is there

		auto opposite = fEdges.find(Key(to, from));
		if (opposite != fEdges.end()) {
			Edge& edge = fList[opposite->second];
			edge.reciprocal = true;
			edge.inverseLabel = label;
			return true;
		}

		Edge edge;
		edge.from = from;
		edge.to = to;
		edge.label = label;
		edge.type = type;
		edge.meta = meta;
		fList.push_back(edge);
		fEdges[Key(from, to)] = static_cast<int>(fList.size()) - 1;
		return true;
	}

	const std::vector<Node>& Nodes() const { return fNodes; }
	std::vector<Node>& Nodes() { return fNodes; }
	const std::vector<Edge>& Edges() const { return fList; }

	/**
	 * A part of the graph: without the meta relations (classification, context) unless they are wanted, without the nodes that have no edge
	 * (counting the edges that are wanted) unless the unconnected ones are wanted, and without the ontologies (and so their relations) unless they are
	 * wanted. The positions are not taken over: Layout() it.
	 */
	Graph Filtered(bool withMeta, bool withUnconnected, bool withOntologies = true) const
	{
		// an edge is left out if it is a meta relation nobody wants, or if it ends at an ontology that is left out
		auto skipped = [&](const Edge& edge) {
			return (edge.meta && !withMeta) || (!withOntologies && (fNodes[edge.from].ontology || fNodes[edge.to].ontology));
		};
		std::vector<int> degree(fNodes.size(), 0);
		for (const Edge& edge : fList) {
			if (skipped(edge))
				continue;
			degree[edge.from]++;
			degree[edge.to]++;
		}
		Graph result;
		for (size_t i = 0; i < fNodes.size(); i++) {
			if (fNodes[i].ontology && !withOntologies)
				continue;
			if (withUnconnected || degree[i] > 0)
				result.AddNode(fNodes[i].id, fNodes[i].label, fNodes[i].group, fNodes[i].ontology);
		}
		for (const Edge& edge : fList) {
			if (skipped(edge))
				continue;
			Edge copy = edge;
			copy.from = result.IndexOf(fNodes[edge.from].id);
			copy.to = result.IndexOf(fNodes[edge.to].id);
			result.fList.push_back(copy);
			result.fEdges[Key(copy.from, copy.to)] = static_cast<int>(result.fList.size()) - 1;
		}
		return result;
	}

	/** Without the nodes that have no edge (and optionally without the meta relations): Filtered(withMeta, false). */
	Graph Connected(bool withMeta) const { return Filtered(withMeta, false); }

	/**
	 * A part of the graph with the positions that it has now (it is not laid out again, so it looks as it does in the whole): the nodes and the edges (indices
	 * of this graph; the nodes at the ends of the edges are in it too). A node keeps the number of edges that it has in the whole (its size).
	 */
	Graph Subgraph(const std::vector<int>& nodes, const std::vector<int>& edges) const
	{
		Graph result;
		auto take = [&](int index) {
			const Node& node = fNodes[index];
			const int added = result.AddNode(node.id, node.label, node.group, node.ontology);
			result.fNodes[added].x = node.x;
			result.fNodes[added].y = node.y;
			result.fNodes[added].degree = node.degree;
		};
		for (int index : nodes)
			take(index);
		for (int index : edges) {
			take(fList[index].from);
			take(fList[index].to);
			Edge copy = fList[index];
			copy.from = result.IndexOf(fNodes[copy.from].id);
			copy.to = result.IndexOf(fNodes[copy.to].id);
			result.fList.push_back(copy);
			result.fEdges[Key(copy.from, copy.to)] = static_cast<int>(result.fList.size()) - 1;
		}
		return result;
	}

	/** A node with the nodes that it is directly connected to, and the edges that it has (not those between the others). */
	Graph Neighborhood(int node) const
	{
		std::vector<int> nodes(1, node), edges;
		for (size_t i = 0; i < fList.size(); i++) {
			if (fList[i].from == node || fList[i].to == node)
				edges.push_back(static_cast<int>(i));
		}
		return Subgraph(nodes, edges);
	}

	/**
	 * Place the nodes (x and y of each, from 0 on) and count the degrees. Every connected part of the graph is laid out by itself (the nodes push each
	 * other away, the edges pull their ends together), then the parts are put next to each other, the biggest first: the many small parts of a graph
	 * of ontologies and documents do not push each other around, and each is as readable as if it was alone. The same graph always gets the same layout.
	 */
	void Layout(int iterations = 0)
	{
		const int count = static_cast<int>(fNodes.size());
		for (Node& node : fNodes)
			node.degree = 0;
		for (const Edge& edge : fList) {
			fNodes[edge.from].degree++;
			fNodes[edge.to].degree++;
		}
		if (count == 0)
			return;

		// the connected parts
		std::vector<int> parent(count);
		for (int i = 0; i < count; i++)
			parent[i] = i;
		auto root = [&parent](int i) {
			while (parent[i] != i) {
				parent[i] = parent[parent[i]];
				i = parent[i];
			}
			return i;
		};
		for (const Edge& edge : fList)
			parent[root(edge.from)] = root(edge.to);

		std::vector<std::vector<int>> parts;
		std::unordered_map<int, int> partOf;
		for (int i = 0; i < count; i++) {
			const int r = root(i);
			if (partOf.find(r) == partOf.end()) {
				partOf[r] = static_cast<int>(parts.size());
				parts.emplace_back();
			}
			parts[partOf[r]].push_back(i);
		}
		std::stable_sort(parts.begin(), parts.end(), [](const std::vector<int>& a, const std::vector<int>& b) { return a.size() > b.size(); });

		// each part by itself
		struct Placed {
			std::vector<int>	members;
			float				width, height;
		};
		std::vector<Placed> placed;
		std::unordered_map<int, int> localIndex;
		std::vector<std::vector<std::pair<int, int>>> partEdges(parts.size());
		std::vector<int> partIndex(count);
		for (size_t p = 0; p < parts.size(); p++) {
			for (size_t j = 0; j < parts[p].size(); j++) {
				partIndex[parts[p][j]] = static_cast<int>(p);
				localIndex[parts[p][j]] = static_cast<int>(j);
			}
		}
		for (const Edge& edge : fList)
			partEdges[partIndex[edge.from]].push_back(std::make_pair(localIndex[edge.from], localIndex[edge.to]));

		for (size_t p = 0; p < parts.size(); p++) {
			Placed part;
			part.members = parts[p];
			const int m = static_cast<int>(parts[p].size());
			std::vector<float> x(m, 0.0f), y(m, 0.0f);
			ForceLayout(&x, &y, partEdges[p], iterations);
			float left = x[0], right = x[0], top = y[0], bottom = y[0];
			for (int j = 0; j < m; j++) {
				left = std::min(left, x[j]);
				right = std::max(right, x[j]);
				top = std::min(top, y[j]);
				bottom = std::max(bottom, y[j]);
			}
			for (int j = 0; j < m; j++) {
				fNodes[part.members[j]].x = x[j] - left;
				fNodes[part.members[j]].y = y[j] - top;
			}
			part.width = right - left;
			part.height = bottom - top;
			placed.push_back(part);
		}

		// the parts in rows, the biggest first, in a field that is wider than high
		double area = 0;
		for (const Placed& part : placed)
			area += static_cast<double>(part.width + gap) * (part.height + gap);
		float rowWidth = std::max(static_cast<float>(std::sqrt(area) * 1.5), placed[0].width);
		float x = 0, y = 0, rowHeight = 0;
		for (const Placed& part : placed) {
			if (x > 0 && x + part.width > rowWidth) {
				x = 0;
				y += rowHeight + gap;
				rowHeight = 0;
			}
			for (int index : part.members) {
				fNodes[index].x += x;
				fNodes[index].y += y;
			}
			x += part.width + gap;
			rowHeight = std::max(rowHeight, part.height);
		}
	}

	/** The area of the nodes (after Layout()), without the margin. */
	void Bounds(float* left, float* top, float* right, float* bottom) const
	{
		*left = *top = 0;
		*right = *bottom = 1;
		if (fNodes.empty())
			return;
		*left = *right = fNodes[0].x;
		*top = *bottom = fNodes[0].y;
		for (const Node& node : fNodes) {
			*left = std::min(*left, node.x);
			*right = std::max(*right, node.x);
			*top = std::min(*top, node.y);
			*bottom = std::max(*bottom, node.y);
		}
	}

	/**
	 * The graph (after Layout()) as an SVG document: the edges (with arrows for relations that are in one direction only), then the nodes, with a title each,
	 * the names. With the options of a view it is the picture of the view: its selection is emphasised, what is not connected to it is dimmed.
	 */
	std::string ToSvg(const SvgOptions& options = SvgOptions()) const
	{
		const Palette palette = PaletteFor(options.dark);
		const std::string backgroundHex = Hex(palette.background), edgeHex = Hex(palette.edge), metaHex = Hex(palette.metaEdge);
		const std::string emphasisHex = Hex(palette.emphasis);

		float width = options.width, height = options.height, scale = options.scale, offsetX = options.offsetX, offsetY = options.offsetY;
		if (width <= 0) {
			float left, top, right, bottom;
			Bounds(&left, &top, &right, &bottom);
			scale = 1;
			width = right - left + 2 * options.margin;
			height = bottom - top + 2 * options.margin;
			offsetX = options.margin - left;
			offsetY = options.margin - top;
		}
		auto X = [&](const Node& node) { return node.x * scale + offsetX; };
		auto Y = [&](const Node& node) { return node.y * scale + offsetY; };

		// what is emphasised: the selected node and its edges, the selected edge
		const bool selectingNode = options.selectedNode >= 0 && options.selectedNode < static_cast<int>(fNodes.size());
		const bool selectingEdge = options.selectedEdge >= 0 && options.selectedEdge < static_cast<int>(fList.size());
		const bool selecting = (selectingNode || selectingEdge) && options.emphasiseLines;
		std::vector<bool> related(fNodes.size(), false);
		if (selectingNode) {
			related[options.selectedNode] = true;
			for (const Edge& edge : fList) {
				if (edge.from == options.selectedNode)
					related[edge.to] = true;
				else if (edge.to == options.selectedNode)
					related[edge.from] = true;
			}
		}
		if (selectingEdge) {
			related[fList[options.selectedEdge].from] = true;
			related[fList[options.selectedEdge].to] = true;
		}

		std::string svg;
		char buffer[640];
		snprintf(buffer, sizeof(buffer),
			"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"%.0f\" height=\"%.0f\" viewBox=\"0 0 %.0f %.0f\" "
			"font-family=\"sans-serif\" font-size=\"%.1f\" font-weight=\"normal\">\n<title>SEN: objects and relations</title>\n"
			"<defs><marker id=\"arrow\" viewBox=\"0 0 10 10\" refX=\"9\" refY=\"5\" markerWidth=\"7\" markerHeight=\"7\" orient=\"auto\">"
			"<path d=\"M 0 0 L 10 5 L 0 10 z\" fill=\"%s\"/></marker>"
			"<marker id=\"arrow-strong\" viewBox=\"0 0 10 10\" refX=\"9\" refY=\"5\" markerWidth=\"7\" markerHeight=\"7\" orient=\"auto\">"
			"<path d=\"M 0 0 L 10 5 L 0 10 z\" fill=\"%s\"/></marker></defs>\n<rect width=\"100%%\" height=\"100%%\" fill=\"%s\"/>\n",
			width, height, width, height, options.fontSize, edgeHex.c_str(), emphasisHex.c_str(), backgroundHex.c_str());
		svg += buffer;

		auto strong = [&](size_t index) {
			const Edge& edge = fList[index];
			return options.emphasiseLines && ((selectingNode && (edge.from == options.selectedNode || edge.to == options.selectedNode))
				|| static_cast<int>(index) == options.selectedEdge);
		};

		for (size_t index = 0; index < fList.size(); index++) {
			const Edge& edge = fList[index];
			const Node& from = fNodes[edge.from];
			const Node& to = fNodes[edge.to];
			const bool emphasised = strong(index), active = !selecting || emphasised;
			float deltaX = X(to) - X(from), deltaY = Y(to) - Y(from);
			const float length = std::max(std::sqrt(deltaX * deltaX + deltaY * deltaY), 0.01f);
			deltaX /= length;
			deltaY /= length;
			// the line ends at the rim of the nodes, so that the arrow is seen
			const float startR = NodeRadiusAt(from.degree, scale), endR = NodeRadiusAt(to.degree, scale) + (edge.reciprocal ? 0 : 1);
			const std::string color = emphasised ? emphasisHex : (edge.meta ? metaHex : edgeHex);
			snprintf(buffer, sizeof(buffer),
				"<line x1=\"%.1f\" y1=\"%.1f\" x2=\"%.1f\" y2=\"%.1f\" stroke=\"%s\" stroke-width=\"%.1f\"%s%s/>\n",
				X(from) + deltaX * startR, Y(from) + deltaY * startR, X(to) - deltaX * endR, Y(to) - deltaY * endR, color.c_str(), emphasised ? kStrongLineWidth : kLineWidth,
				active ? "" : " opacity=\"0.2\"",
				edge.reciprocal || length <= startR + endR + 8 ? "" : (emphasised ? " marker-end=\"url(#arrow-strong)\"" : " marker-end=\"url(#arrow)\""));
			svg += buffer;
		}

		const float font = options.fontSize;
		const std::string edgeTextHex = Hex(palette.edgeText), textHex = Hex(palette.text);

		// a name on the background: the halo is a text of its own below the one that is read, so that it looks the same in every viewer
		auto drawLabel = [&](float x, float y, const std::string& text, const std::string& color) {
			char line[400];
			snprintf(line, sizeof(line), "<text x=\"%.1f\" y=\"%.1f\" text-anchor=\"middle\" fill=\"%s\" stroke=\"%s\" stroke-width=\"3\" stroke-linejoin=\"round\">",
				x, y, backgroundHex.c_str(), backgroundHex.c_str());
			svg += line;
			svg += XmlEscape(text) + "</text>\n";
			snprintf(line, sizeof(line), "<text x=\"%.1f\" y=\"%.1f\" text-anchor=\"middle\" fill=\"%s\">", x, y, color.c_str());
			svg += line;
			svg += XmlEscape(text) + "</text>\n";
		};

		for (size_t index = 0; index < fList.size(); index++) {
			const Edge& edge = fList[index];
			const bool wanted = options.edgeLabelShown != NULL ? (index < options.edgeLabelShown->size() && (*options.edgeLabelShown)[index]) : options.edgeLabels;
			if (!wanted)
				continue;
			std::string label = edge.label;
			if (edge.reciprocal && !edge.inverseLabel.empty() && edge.inverseLabel != edge.label)
				label += label.empty() ? edge.inverseLabel : " / " + edge.inverseLabel;
			if (label.empty())
				continue;
			const Node& from = fNodes[edge.from];
			const Node& to = fNodes[edge.to];
			drawLabel((X(from) + X(to)) / 2, (Y(from) + Y(to)) / 2 + font * 0.35f, label, strong(index) ? textHex : edgeTextHex);
		}

		for (size_t index = 0; index < fNodes.size(); index++) {
			const Node& node = fNodes[index];
			const bool dimmed = selecting && !related[index];
			const bool chosen = static_cast<int>(index) == options.selectedNode;
			const float radius = NodeRadiusAt(node.degree, scale) + (chosen ? 2 : 0);
			const Rgb color = GroupColor(node.group, options.dark);
			snprintf(buffer, sizeof(buffer),
				"<circle cx=\"%.1f\" cy=\"%.1f\" r=\"%.1f\" fill=\"#%02x%02x%02x\" stroke=\"%s\" stroke-width=\"%.1f\"%s>",
				X(node), Y(node), radius, color.r, color.g, color.b, chosen ? emphasisHex.c_str() : backgroundHex.c_str(), chosen ? 2.0f : 1.2f,
				dimmed ? " opacity=\"0.35\"" : "");
			svg += buffer;
			svg += "<title>" + XmlEscape(node.label) + (node.group.empty() ? "" : " (" + XmlEscape(node.group) + ")") + "</title></circle>\n";
		}

		for (size_t index = 0; index < fNodes.size(); index++) {
			const Node& node = fNodes[index];
			const bool wanted = options.nodeLabelShown != NULL ? (index < options.nodeLabelShown->size() && (*options.nodeLabelShown)[index]) : options.nodeLabels;
			if (!wanted)
				continue;
			drawLabel(X(node), Y(node) + NodeRadiusAt(node.degree, scale) + font + 2, ShownName(node.label), textHex);
		}

		svg += "</svg>\n";
		return svg;
	}

	/** Text for an XML attribute or element. */
	static std::string XmlEscape(const std::string& text)
	{
		std::string result;
		for (char c : text) {
			switch (c) {
				case '&': result += "&amp;"; break;
				case '<': result += "&lt;"; break;
				case '>': result += "&gt;"; break;
				case '"': result += "&quot;"; break;
				default: result += c;
			}
		}
		return result;
	}

private:
	static constexpr float gap = 70;

	/**
	 * The force layout of one connected part (Fruchterman and Reingold): positions of x and y are made for the edges (pairs of indices). There is no wall: a weak
	 * pull to the middle keeps the part together, and the part is as big as its forces make it.
	 */
	static void ForceLayout(std::vector<float>* xs, std::vector<float>* ys, const std::vector<std::pair<int, int>>& edges, int iterations)
	{
		std::vector<float>& x = *xs;
		std::vector<float>& y = *ys;
		const int count = static_cast<int>(x.size());
		if (count == 1)
			return;
		const float side = std::max(160.0f, 75.0f * std::sqrt(static_cast<float>(count)));
		if (iterations <= 0)
			iterations = std::max(60, std::min(300, static_cast<int>(4.0e7 / (static_cast<double>(count) * count + 1))));

		// start on a spiral (it is the same every time, and no two nodes are in one place)
		for (int i = 0; i < count; i++) {
			const float radius = std::sqrt((i + 0.5f) / count) * 0.45f * side;
			const float angle = i * 2.39996323f;
			x[i] = side / 2 + radius * std::cos(angle);
			y[i] = side / 2 + radius * std::sin(angle);
		}

		const float k = 0.75f * std::sqrt(side * side / count);
		const float gravity = 0.8f * k / (side / 2);
		float temperature = side / 6;
		const float cooling = temperature / (iterations + 1);
		std::vector<float> dx(count), dy(count);

		for (int step = 0; step < iterations; step++) {
			std::fill(dx.begin(), dx.end(), 0.0f);
			std::fill(dy.begin(), dy.end(), 0.0f);
			for (int i = 0; i < count; i++) {
				for (int j = i + 1; j < count; j++) {
					const float deltaX = x[i] - x[j], deltaY = y[i] - y[j];
					const float distance2 = std::max(deltaX * deltaX + deltaY * deltaY, 0.01f);
					const float push = k * k / distance2;	// k^2 / d, as a factor of the vector
					dx[i] += deltaX * push;
					dy[i] += deltaY * push;
					dx[j] -= deltaX * push;
					dy[j] -= deltaY * push;
				}
			}
			for (const std::pair<int, int>& edge : edges) {
				const float deltaX = x[edge.first] - x[edge.second], deltaY = y[edge.first] - y[edge.second];
				const float distance = std::max(std::sqrt(deltaX * deltaX + deltaY * deltaY), 0.1f);
				const float pull = distance / k;	// d^2 / k, as a factor of the vector
				dx[edge.first] -= deltaX * pull;
				dy[edge.first] -= deltaY * pull;
				dx[edge.second] += deltaX * pull;
				dy[edge.second] += deltaY * pull;
			}
			for (int i = 0; i < count; i++) {
				dx[i] -= (x[i] - side / 2) * gravity;
				dy[i] -= (y[i] - side / 2) * gravity;
				const float length = std::max(std::sqrt(dx[i] * dx[i] + dy[i] * dy[i]), 0.0001f);
				const float move = std::min(length, temperature) / length;
				x[i] += dx[i] * move;
				y[i] += dy[i] * move;
			}
			temperature -= cooling;
		}
	}

	static uint64_t Key(int from, int to) { return (static_cast<uint64_t>(from) << 32) | static_cast<uint32_t>(to); }

	std::vector<Node>					fNodes;
	std::vector<Edge>					fList;
	std::unordered_map<std::string, int>	fIndex;
	std::unordered_map<uint64_t, int>	fEdges;
};

}	// namespace graph
}	// namespace sen
