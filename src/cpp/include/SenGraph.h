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

/** A colour for a group, always the same one for the same name (a hue out of a hash). */
inline Rgb
GroupColor(const std::string& group)
{
	uint32_t hash = 2166136261u;
	for (unsigned char c : group) {
		hash ^= c;
		hash *= 16777619u;
	}
	const float hue = (hash % 360) / 60.0f, saturation = 0.55f, lightness = 0.58f;
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

/** The size (radius) of a node: more edges, bigger. */
inline float
NodeRadius(int degree)
{
	return 5.0f + 2.0f * std::sqrt(static_cast<float>(std::max(degree, 0)));
}

/** What ToSvg() draws. */
struct SvgOptions {
	bool	nodeLabels = true;
	bool	edgeLabels = true;
	float	margin = 50;
};

class Graph {
public:
	/** Add a node; the node of the id that is there is returned if there is one. @return its index */
	int AddNode(const std::string& id, const std::string& label, const std::string& group)
	{
		auto found = fIndex.find(id);
		if (found != fIndex.end())
			return found->second;
		Node node;
		node.id = id;
		node.label = label;
		node.group = group;
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

	/** A graph without the nodes that have no edge, optionally also without the meta relations (and the nodes that only they connect). */
	Graph Connected(bool withMeta) const
	{
		std::vector<int> degree(fNodes.size(), 0);
		for (const Edge& edge : fList) {
			if (edge.meta && !withMeta)
				continue;
			degree[edge.from]++;
			degree[edge.to]++;
		}
		Graph result;
		for (size_t i = 0; i < fNodes.size(); i++) {
			if (degree[i] > 0)
				result.AddNode(fNodes[i].id, fNodes[i].label, fNodes[i].group);
		}
		for (const Edge& edge : fList) {
			if (edge.meta && !withMeta)
				continue;
			const int from = result.IndexOf(fNodes[edge.from].id), to = result.IndexOf(fNodes[edge.to].id);
			Edge copy = edge;
			copy.from = from;
			copy.to = to;
			result.fList.push_back(copy);
			result.fEdges[Key(from, to)] = static_cast<int>(result.fList.size()) - 1;
		}
		return result;
	}

	/**
	 * Place the nodes (x and y of each, in the area of width times height, which is made for the number of nodes when it is not given) and count the
	 * degrees. The same graph always gets the same layout.
	 */
	void Layout(float width = 0, float height = 0, int iterations = 0)
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

		if (width <= 0)
			width = std::max(420.0f, 75.0f * std::sqrt(static_cast<float>(count)));
		if (height <= 0)
			height = width;
		if (iterations <= 0)
			iterations = std::max(60, std::min(300, static_cast<int>(4.0e7 / (static_cast<double>(count) * count + 1))));

		// start on a spiral (it is the same every time, and no two nodes are in one place)
		for (int i = 0; i < count; i++) {
			const float radius = std::sqrt((i + 0.5f) / count) * 0.45f * std::min(width, height);
			const float angle = i * 2.39996323f;
			fNodes[i].x = width / 2 + radius * std::cos(angle);
			fNodes[i].y = height / 2 + radius * std::sin(angle);
		}
		if (count == 1)
			return;

		const float k = 0.75f * std::sqrt(width * height / count);
		const float gravity = 0.6f * k / (std::min(width, height) / 2);
		float temperature = std::min(width, height) / 8;
		const float cooling = temperature / (iterations + 1);
		std::vector<float> dx(count), dy(count);

		for (int step = 0; step < iterations; step++) {
			std::fill(dx.begin(), dx.end(), 0.0f);
			std::fill(dy.begin(), dy.end(), 0.0f);

			for (int i = 0; i < count; i++) {
				for (int j = i + 1; j < count; j++) {
					const float deltaX = fNodes[i].x - fNodes[j].x, deltaY = fNodes[i].y - fNodes[j].y;
					const float distance2 = std::max(deltaX * deltaX + deltaY * deltaY, 0.01f);
					const float push = k * k / distance2;	// k^2 / d, as a factor of the vector
					dx[i] += deltaX * push;
					dy[i] += deltaY * push;
					dx[j] -= deltaX * push;
					dy[j] -= deltaY * push;
				}
			}
			for (const Edge& edge : fList) {
				const float deltaX = fNodes[edge.from].x - fNodes[edge.to].x, deltaY = fNodes[edge.from].y - fNodes[edge.to].y;
				const float distance = std::max(std::sqrt(deltaX * deltaX + deltaY * deltaY), 0.1f);
				const float pull = distance / k;	// d^2 / k, as a factor of the vector
				dx[edge.from] -= deltaX * pull;
				dy[edge.from] -= deltaY * pull;
				dx[edge.to] += deltaX * pull;
				dy[edge.to] += deltaY * pull;
			}
			for (int i = 0; i < count; i++) {
				dx[i] -= (fNodes[i].x - width / 2) * gravity;
				dy[i] -= (fNodes[i].y - height / 2) * gravity;
				const float length = std::max(std::sqrt(dx[i] * dx[i] + dy[i] * dy[i]), 0.0001f);
				const float move = std::min(length, temperature) / length;
				fNodes[i].x = std::min(std::max(fNodes[i].x + dx[i] * move, 10.0f), width - 10);
				fNodes[i].y = std::min(std::max(fNodes[i].y + dy[i] * move, 10.0f), height - 10);
			}
			temperature -= cooling;
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

	/** The graph (after Layout()) as an SVG document: the edges (with arrows for relations that are in one direction only), then the nodes, with a title each. */
	std::string ToSvg(const SvgOptions& options = SvgOptions()) const
	{
		float left, top, right, bottom;
		Bounds(&left, &top, &right, &bottom);
		const float margin = options.margin;
		const float width = right - left + 2 * margin, height = bottom - top + 2 * margin;
		const float offsetX = margin - left, offsetY = margin - top;

		std::string svg;
		char buffer[512];
		snprintf(buffer, sizeof(buffer),
			"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"%.0f\" height=\"%.0f\" viewBox=\"0 0 %.0f %.0f\" "
			"font-family=\"sans-serif\">\n<title>SEN: objects and relations</title>\n"
			"<defs><marker id=\"arrow\" viewBox=\"0 0 10 10\" refX=\"9\" refY=\"5\" markerWidth=\"7\" markerHeight=\"7\" orient=\"auto\">"
			"<path d=\"M 0 0 L 10 5 L 0 10 z\" fill=\"#8a8a8a\"/></marker></defs>\n<rect width=\"100%%\" height=\"100%%\" fill=\"#ffffff\"/>\n",
			width, height, width, height);
		svg += buffer;

		for (const Edge& edge : fList) {
			const Node& from = fNodes[edge.from];
			const Node& to = fNodes[edge.to];
			float deltaX = to.x - from.x, deltaY = to.y - from.y;
			const float length = std::max(std::sqrt(deltaX * deltaX + deltaY * deltaY), 0.01f);
			deltaX /= length;
			deltaY /= length;
			// the line ends at the rim of the nodes, so that the arrow is seen
			const float startR = NodeRadius(from.degree), endR = NodeRadius(to.degree) + (edge.reciprocal ? 0 : 1);
			snprintf(buffer, sizeof(buffer),
				"<line x1=\"%.1f\" y1=\"%.1f\" x2=\"%.1f\" y2=\"%.1f\" stroke=\"%s\" stroke-width=\"%.1f\"%s%s/>\n",
				from.x + offsetX + deltaX * startR, from.y + offsetY + deltaY * startR, to.x + offsetX - deltaX * endR, to.y + offsetY - deltaY * endR,
				edge.meta ? "#d0d0d0" : "#8a8a8a", edge.meta ? 1.0f : 1.4f, edge.meta ? " stroke-dasharray=\"4 3\"" : "",
				edge.reciprocal ? "" : " marker-end=\"url(#arrow)\"");
			svg += buffer;
		}

		if (options.edgeLabels) {
			for (const Edge& edge : fList) {
				std::string label = edge.label;
				if (edge.reciprocal && !edge.inverseLabel.empty() && edge.inverseLabel != edge.label)
					label += label.empty() ? edge.inverseLabel : " / " + edge.inverseLabel;
				if (label.empty())
					continue;
				const Node& from = fNodes[edge.from];
				const Node& to = fNodes[edge.to];
				snprintf(buffer, sizeof(buffer),
					"<text x=\"%.1f\" y=\"%.1f\" font-size=\"8\" text-anchor=\"middle\" fill=\"#666666\" stroke=\"#ffffff\" stroke-width=\"2.5\" "
					"paint-order=\"stroke\">", (from.x + to.x) / 2 + offsetX, (from.y + to.y) / 2 + offsetY);
				svg += buffer;
				svg += XmlEscape(label) + "</text>\n";
			}
		}

		for (const Node& node : fNodes) {
			const Rgb color = GroupColor(node.group);
			snprintf(buffer, sizeof(buffer), "<circle cx=\"%.1f\" cy=\"%.1f\" r=\"%.1f\" fill=\"#%02x%02x%02x\" stroke=\"#ffffff\" stroke-width=\"1.5\">",
				node.x + offsetX, node.y + offsetY, NodeRadius(node.degree), color.r, color.g, color.b);
			svg += buffer;
			svg += "<title>" + XmlEscape(node.label) + (node.group.empty() ? "" : " (" + XmlEscape(node.group) + ")") + "</title></circle>\n";
		}

		if (options.nodeLabels) {
			for (const Node& node : fNodes) {
				snprintf(buffer, sizeof(buffer),
					"<text x=\"%.1f\" y=\"%.1f\" font-size=\"10\" text-anchor=\"middle\" fill=\"#222222\" stroke=\"#ffffff\" stroke-width=\"3\" "
					"paint-order=\"stroke\">", node.x + offsetX, node.y + offsetY + NodeRadius(node.degree) + 11);
				svg += buffer;
				svg += XmlEscape(node.label) + "</text>\n";
			}
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
	static uint64_t Key(int from, int to) { return (static_cast<uint64_t>(from) << 32) | static_cast<uint32_t>(to); }

	std::vector<Node>					fNodes;
	std::vector<Edge>					fList;
	std::unordered_map<std::string, int>	fIndex;
	std::unordered_map<uint64_t, int>	fEdges;
};

}	// namespace graph
}	// namespace sen
