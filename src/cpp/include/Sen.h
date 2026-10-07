/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2024-2026 SEN Labs e.U.
 */
#pragma once

/**
 * @file Sen.h
 * @brief The SEN API: include this to get everything a SEN client needs.
 *
 *  - SenAttributes.h : names of file attributes (sen::attr)
 *  - SenMimeTypes.h  : MIME types and signatures (sen::mime)
 *  - SenMessages.h   : commands, message fields, relation config, status codes (sen::cmd, sen::key, sen::conf, sen::status)
 *  - SenId.h         : TSID, the identifier of SEN (sen::id)
 *  - SenIdList.h     : the chunked lists of ids in SEN:TO and SEN:META (sen::idlist)
 *  - Sensei.h        : the plugin protocol (namespace sensei)
 */

#include <Entry.h>
#include <String.h>

#include "SenAttributes.h"
#include "SenId.h"
#include "SenIdList.h"
#include "SenMessages.h"
#include "SenMimeTypes.h"
#include "Sensei.h"

namespace sen {

/** Simple data exchange object needed for creating and populating the relation view folder. */
struct RelationInfo {
	entry_ref srcRef;
	entry_ref targetRef;
	entry_ref relationDirRef;

	BString relationType;
	BString relationLabel;
	BString srcId;
	BString targetId;
};

}	// namespace sen
