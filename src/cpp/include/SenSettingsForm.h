/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2026 SEN Labs e.U.
 */
#pragma once

/**
 * @file SenSettingsForm.h
 * @brief Constants only: the messages between a program that shows a settings form and the form itself, and the names in a schema
 * (see docs/spec-settings-and-message-template.md, sections 4 and 7).
 *
 * The form is the view of Shoji for a BMessage (`ShojiMessageView`). A program does not link it: it makes the stub of an archive, `instantiate_object()` loads the
 * class from the executable of Shoji, and the program talks to the view with these messages only. The form itself is code of Shoji.
 */

#include <stdint.h>

namespace sen {
namespace settings {
namespace form {

/** The application that has the class of the form, and the class (the `add_on` and the `class` of the stub of the archive). */
const char* const kShojiSignature	= "application/x-vnd.sen-labs.shoji";
const char* const kViewClass		= "ShojiMessageView";

/** The number of the protocol of these messages, in the field `protocol` of kSetData (and of every answer). */
const int32_t kProtocol = 1;

/** Messages. To the view: kSetData, kGetData, kRevert, kDefaults. From the view (to its target): kChanged, kRefused. */
enum : uint32_t {
	kSetData	= 'SFsd',		///< `settings`; optional `schema`, `defaults`, `target` (BMessenger), `protocol`, `editable`, `root_label` (`layout` is not used yet); replied to if a reply is waited for
	kGetData	= 'SFgd',		///< asks for the working copy; the reply has `settings` (do not wait for it in the thread of the window that has the form: the form is in that thread)
	kRevert		= 'SFrv',		///< the working copy goes back to the data as it was set; answered with kChanged for the whole message
	kDefaults	= 'SFdf',		///< the working copy becomes the defaults; answered with kChanged for the whole message
	kChanged	= 'SFch',		///< `path` and `value` (text) of a change, or no `path` for the whole message; always `settings` (the working copy)
	kRefused	= 'SFrf'		///< `path`, `value` (what was typed), `status` (int32) and `detail` (text): the control went back to its value
};

/** Fields of the messages. */
const char* const kFieldSettings	= "settings";
const char* const kFieldSchema		= "schema";
const char* const kFieldDefaults	= "defaults";
const char* const kFieldLayout		= "layout";
const char* const kFieldTarget		= "target";
const char* const kFieldProtocol	= "protocol";
const char* const kFieldPath		= "path";
const char* const kFieldValue		= "value";
const char* const kFieldStatus		= "status";
const char* const kFieldDetail		= "detail";
const char* const kFieldResult		= "result";			///< int32: the status in a reply (B_OK, or B_NOT_SUPPORTED for a protocol that is too new, B_BAD_VALUE for a message that lacks `settings`)
const char* const kFieldEditable	= "editable";		///< bool in kSetData: false shows the values and changes nothing (default true)
const char* const kFieldRootLabel	= "root_label";		///< string in kSetData: the text of the first node of the tree

/**
 * A date needs no kind: a `B_TIME_TYPE` field is edited with a date control. The kind names what the value means, the form decides which control of the system
 * shows it (a kind that it does not know is a text field).
 *
 * The schema is a message with one field per path key (the key of the field, without an index: `nested.tags`): a message that holds the attributes below.
 * The field `version` (int32) of the schema itself is its format.
 */
const char* const kSchemaVersion	= "version";
const char* const kAttrLabel		= "label";			///< string: the text before the control
const char* const kAttrHelp			= "help";			///< string: the tool tip
const char* const kAttrKind			= "kind";			///< string: one of the kinds below
const char* const kAttrChoices		= "choices";		///< strings: the values to choose from
const char* const kAttrChoiceLabels	= "choice_labels";	///< strings: the texts of the choices, in the same order (else the values are shown)
const char* const kAttrMin			= "min";			///< double: the smallest value of a number
const char* const kAttrMax			= "max";			///< double: the largest value of a number
const char* const kAttrMaxLabel		= "max_label";		///< string: the text of a slider instead of its value when the thumb is at the right end ("All")
const char* const kAttrMinLabel		= "min_label";		///< string: the text of a slider instead of its value when the thumb is at the left end ("None" for 0)
const char* const kAttrStep			= "step";			///< double
const char* const kAttrOrder		= "order";			///< int32: the place among the fields of its message (the fields without one keep the order of the message)
const char* const kAttrRestart		= "restart";		///< bool: a change takes effect when the program is started again
const char* const kAttrHidden		= "hidden";			///< bool: not shown
const char* const kAttrEnabledWhen	= "enabled_when";	///< string `key=value`: the control is enabled only while the field `key` of the same message has this value; `&` joins conditions (all hold), `|` alternatives (one holds): `mode=custom&flag=true|mode=other`
const char* const kAttrList			= "list";			///< bool: the field is a list (edited in a list) also with one value or none

const char* const kKindText			= "text";
const char* const kKindPath			= "path";
const char* const kKindChoice		= "choice";
const char* const kKindSlider		= "slider";			///< a number with `min` and `max` (and `step`) on a track
const char* const kKindSpinner		= "spinner";		///< a number with buttons for up and down; the default for a number with `min` and `max`
const char* const kKindRadio		= "radio";			///< the `choices` as radio buttons instead of a menu
const char* const kKindSecret		= "secret";
const char* const kKindColor		= "color";

}	// namespace form
}	// namespace settings
}	// namespace sen
