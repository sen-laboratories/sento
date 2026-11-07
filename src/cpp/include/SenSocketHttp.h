/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2026 SEN Labs e.U.
 */
#pragma once

#include <SupportDefs.h>

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include <NetworkAddress.h>
#include <SecureSocket.h>
#include <Socket.h>
#include <String.h>
#include <Url.h>

/**
 * @file SenSocketHttp.h
 * @brief A small HTTP/1.1 client over a plain socket, for the servers that the HTTP client of Haiku (BHttpSession) cannot talk to because of a bug in it.
 *
 * Wikimedia (Wikidata, Commons, Wikipedia and their pictures) sends `x-analytics:` with every answer: a header with an EMPTY value. That is valid HTTP: RFC 9110 defines
 * `field-value = *field-content`, which may be empty. `BHttpFields` of Haiku (libnetservices2, `HttpFields.cpp`) rejects an empty value and throws `InvalidInput` on a thread of the session,
 * and the program ends; it cannot be caught. This client makes the same GET over a socket (BSecureSocket for https), reads the answer to the end of the connection and reads only the headers
 * that it needs (the status, `Content-Length`, `Transfer-Encoding`, `Location`), so a header that it does not need cannot matter. It was Paperboy's and is here for every plugin;
 * BaseEnricher uses it for the hosts of Wikimedia (see NeedsSocketClient()); Shoji reads the tiles of its map with it. It is a header (as all of sento is) so that every program of SEN can use it. It can go when Haiku accepts an empty value.
 */
namespace sen {
namespace http {


namespace detail {

const bigtime_t kTimeout = 20 * 1000000LL;
const int kMaxRedirections = 5;
const size_t kMaxAnswer = 32 * 1024 * 1024;

inline std::string
Lower(std::string text)
{
	for (char& c : text)
		c = (char) std::tolower((unsigned char) c);
	return text;
}

/** the value of a header of a block of headers ("name: value" lines), "" if it is not there (the empty value is no reason to stop: that is what this is for) */
inline std::string
HeaderValue(const std::string& headers, const std::string& name)
{
	const std::string wanted = Lower(name) + ":";
	size_t from = 0;
	while (from < headers.size()) {
		size_t end = headers.find("\r\n", from);
		if (end == std::string::npos)
			end = headers.size();
		const std::string line = headers.substr(from, end - from);
		if (Lower(line.substr(0, wanted.size())) == wanted) {
			size_t start = wanted.size();
			while (start < line.size() && (line[start] == ' ' || line[start] == '\t'))
				start++;
			return line.substr(start);
		}
		from = end + 2;
	}
	return "";
}

/** the body of a chunked answer (size in hex, line break, data, line break... and a chunk of size 0); false if it is cut off */
inline bool
Dechunk(const std::string& raw, std::string* body)
{
	size_t at = 0;
	body->clear();
	while (at < raw.size()) {
		size_t lineEnd = raw.find("\r\n", at);
		if (lineEnd == std::string::npos)
			return false;
		const size_t size = std::strtoul(raw.substr(at, lineEnd - at).c_str(), nullptr, 16);
		at = lineEnd + 2;
		if (size == 0)
			return true;
		if (at + size > raw.size())
			return false;
		body->append(raw, at, size);
		at += size + 2;
	}
	return false;
}

inline bool
EndsWith(const std::string& text, const std::string& end)
{
	return text.size() >= end.size() && text.compare(text.size() - end.size(), end.size(), end) == 0;
}

inline status_t
Get(const std::string& url, const std::string& userAgent, const char* accept, std::string* body, int* httpStatus, int redirects, bigtime_t timeout)
{
	BUrl address(url.c_str(), true);
	if (!address.IsValid() || !address.HasHost())
		return B_BAD_VALUE;
	const bool secure = address.Protocol() == "https";
	if (!secure && address.Protocol() != "http")
		return B_BAD_VALUE;
	const int port = address.HasPort() ? address.Port() : (secure ? 443 : 80);

	BString target(address.HasPath() ? address.Path() : BString("/"));
	if (address.HasRequest()) {
		BString query(address.Request());
		if (query.ByteAt(0) == '?')
			query.Remove(0, 1);
		target << "?" << query;
	}

	BNetworkAddress peer(address.Host().String(), (uint16) port);
	BSecureSocket secureSocket;
	BSocket plainSocket;
	BSocket* socket = secure ? &secureSocket : &plainSocket;
	status_t status = socket->Connect(peer, timeout);
	if (status != B_OK) {
		printf("could not connect to %s: %s\n", address.Host().String(), strerror(status));
		return status;
	}
	socket->SetTimeout(timeout);

	std::string request = std::string("GET ") + target.String() + " HTTP/1.1\r\nHost: " + address.Host().String()
		+ (address.HasPort() ? ":" + std::to_string(port) : "") + "\r\nUser-Agent: " + userAgent + "\r\nAccept: " + accept
		+ "\r\nConnection: close\r\n\r\n";
	for (size_t sent = 0; sent < request.size();) {
		ssize_t count = socket->Write(request.data() + sent, request.size() - sent);
		if (count <= 0)
			return count < 0 ? (status_t) count : B_IO_ERROR;
		sent += (size_t) count;
	}

	// the whole answer: it ends when the server closes the connection
	std::string raw;
	char buffer[16384];
	for (;;) {
		ssize_t count = socket->Read(buffer, sizeof(buffer));
		if (count == 0)
			break;
		if (count < 0) {
			if (raw.empty())
				return (status_t) count;
			break;		// (a secure connection that is closed without the notice: what was read is what there is)
		}
		raw.append(buffer, (size_t) count);
		if (raw.size() > kMaxAnswer)
			return B_NO_MEMORY;
	}
	socket->Disconnect();

	const size_t headEnd = raw.find("\r\n\r\n");
	if (headEnd == std::string::npos || raw.compare(0, 5, "HTTP/") != 0)
		return B_BAD_DATA;
	const std::string head = raw.substr(0, headEnd);
	const size_t statusLineEnd = head.find("\r\n");
	const std::string statusLine = head.substr(0, statusLineEnd);
	const std::string headers = statusLineEnd == std::string::npos ? "" : head.substr(statusLineEnd + 2);
	const size_t space = statusLine.find(' ');
	const int code = space == std::string::npos ? 0 : std::atoi(statusLine.c_str() + space + 1);
	if (httpStatus != NULL)
		*httpStatus = code;

	if (code >= 300 && code < 400 && !HeaderValue(headers, "Location").empty()) {
		if (redirects >= kMaxRedirections)
			return B_ERROR;
		BUrl next(address, BString(HeaderValue(headers, "Location").c_str()));
		return Get(next.UrlString().String(), userAgent, accept, body, httpStatus, redirects + 1, timeout);
	}
	if (code == 404)
		return B_ENTRY_NOT_FOUND;
	if (code == 401 || code == 403)
		return B_NOT_ALLOWED;
	if (code == 429 || code == 503)
		return B_BUSY;
	if (code < 200 || code >= 300) {
		printf("HTTP %d from %s\n", code, url.c_str());
		return B_ERROR;
	}

	const std::string content = raw.substr(headEnd + 4);
	if (Lower(HeaderValue(headers, "Transfer-Encoding")).find("chunked") != std::string::npos)
		return Dechunk(content, body) ? B_OK : B_BAD_DATA;
	const std::string length = HeaderValue(headers, "Content-Length");
	if (!length.empty() && std::strtoul(length.c_str(), nullptr, 10) < content.size())
		body->assign(content, 0, std::strtoul(length.c_str(), nullptr, 10));
	else
		*body = content;
	return B_OK;
}

}	// namespace detail


/** Whether the host is asked over a socket from the start, because the client of Haiku cannot read its answers: the hosts of Wikimedia (wikimedia.org, wikipedia.org, wikidata.org and what is below them). */
inline bool
NeedsSocketClient(const std::string& host)
{
	const std::string lower = detail::Lower(host);
	for (const char* domain : {"wikimedia.org", "wikipedia.org", "wikidata.org"}) {
		if (lower == domain || detail::EndsWith(lower, std::string(".") + domain))
			return true;
	}
	return false;
}


/**
 * One GET. Redirects are followed (at most five).
 * @param accept the value of the header `Accept`
 * @param timeout how long to wait for the connection and for each part of the answer, in microseconds (20 s); a service that works out its answer first (Overpass: half a minute when it is busy) wants more
 * @param httpStatus gets the HTTP status of the last answer (0 if there was none); may be NULL
 * @return B_OK for a 2xx answer; B_ENTRY_NOT_FOUND for 404; B_NOT_ALLOWED for 401 and 403; B_BUSY for 429 and 503; B_ERROR for any other status; B_BAD_VALUE for an address that is not
 *         http or https; the error of the network (B_TIMED_OUT, ...); B_BAD_DATA for an answer that is no HTTP answer or is cut off; B_NO_MEMORY for an answer of more than 32 MiB
 */
inline status_t
SocketGet(const std::string& url, const std::string& userAgent, const char* accept, std::string* body, int* httpStatus, bigtime_t timeout = detail::kTimeout)
{
	if (httpStatus != NULL)
		*httpStatus = 0;
	return detail::Get(url, userAgent, accept != NULL ? accept : "*/*", body, httpStatus, 0, timeout);
}


}	// namespace http
}	// namespace sen
