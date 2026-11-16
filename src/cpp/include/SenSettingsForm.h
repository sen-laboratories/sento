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

/** Messages. To the view: kSetData, kGetData, kRevert, kDefaults, kActionDone. From the view (to its target): kChanged, kRefused, kAction. From a detail to its master: kUpdated. */
enum : uint32_t {
	kSetData	= 'SFsd',		///< `settings`; optional `schema`, `defaults`, `target` (BMessenger), `protocol`, `editable`, `root_label` (`layout` is not used yet); replied to if a reply is waited for
	kGetData	= 'SFgd',		///< asks for the working copy; the reply has `settings` (do not wait for it in the thread of the window that has the form: the form is in that thread)
	kRevert		= 'SFrv',		///< the working copy goes back to the data as it was set; answered with kChanged for the whole message
	kDefaults	= 'SFdf',		///< the working copy becomes the defaults; answered with kChanged for the whole message
	kChanged	= 'SFch',		///< `path` and `value` (text) of a change, or no `path` for the whole message; always `settings` (the working copy)
	kRefused	= 'SFrf',		///< `path`, `value` (what was typed), `status` (int32) and `detail` (text): the control went back to its value
	kAction		= 'SFac',		///< `path`: a button (`kind = button`) was pressed; nothing in the working copy changes
	kActionDone	= 'SFad',		///< to the view, with the `path` of the button (without it: all that wait): the owner has answered that button and the answer changes nothing in the data: the form takes the waiting state of the button away (it is greyed, and the line next to it says that a reply is awaited). An answer with other data is a kSetData, which makes the form again.
	kUpdated	= 'SFup'		///< from a detail to its master (the messenger in `reply_to` of the update, see the master/detail contract below): the user changed the control. `command` (int32) of the update that it answers, and its part (the field id) with `value`
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
/** a choice with this value is not a choice but a line between choices in the menu (it has no text in `choice_labels`); radio buttons leave it out */
const char* const kChoiceSeparator	= "-";
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
const char* const kAttrReadOnly		= "readonly";		///< bool: a column of a table (`kind = table`) or a text field is only shown, not edited: the name of a row, a state that the owner reports
const char* const kAttrShows			= "shows";			///< string, of a button (`kind = button`): the name of a `readonly` text field of the same message that is shown right of the button, in the same line, not in a line of its own
const char* const kAttrStateField	= "state_field";	///< string, of a `readonly` text field: the name of a text field of the same message that says what the text means, `info`, `warning` or `error` (kStateInfo...): the line shows the icon of that alert before the text. The field itself is `hidden`.
const char* const kAttrDetailField	= "detail_field";	///< string, of a `readonly` text field: the name of a text field of the same message with the longer text, several lines allowed, that is the tool tip of the line (the short text of the field is the status, this is the detail). The field itself is `hidden`.
const char* const kAttrReorderable	= "reorderable";	///< bool, of a table (`kind = table`): the rows can be dragged to another place (or moved with Ctrl and the arrows up and down); the owner gets the whole message, with the array of the rows in the new order
const char* const kAttrUpdateMsg		= "update_msg";		///< string of four characters (a message command, `what`), of a control that is the source of an update (a table: when another row is selected, or none): it tells its observers with a message of that command, see the master/detail contract below
const char* const kAttrBindToUpdateMsg	= "bind_to_update_msg";	///< string of four characters, of any control: it listens to the updates of that command and updates itself from them (the detail of a master/detail pair); `field_id` names the part of the message that is meant for it (default: the name of the control)
const char* const kAttrFieldId		= "field_id";		///< string, of a control with `bind_to_update_msg`: the key of the update message that holds what is meant for this control (default: its own name)
const char* const kAttrBeside		= "beside";			///< string, of a button (`kind = button`) or of a text field: the name of another field of the same message: it stands in the line of that field instead of in a line of its own. A button: at the right end of the line of a text field, or in the column of the controls in the line of the label of a table; a text field: in the column of the controls, in the line of a check box (the check box, then the field)
const char* const kAttrBusyField		= "busy_field";		///< string, of a column of a table: the name of a bool field of the same row; while it is true the cell shows a spinner before its text (the service that is asked now)
const char* const kAttrColumnLabels	= "column_labels";	///< strings: the titles of the two columns of `kind = properties` (else "Name" and "Value")

const char* const kStateInfo		= "info";			///< the values of the field of `state_field`: it worked (the icon of an information alert)
const char* const kStateWarning		= "warning";		///< ... something is not as it should be (the icon of a warning)
const char* const kStateError		= "error";			///< ... it failed (the icon of an error)
const char* const kStateOk			= "ok";				///< ... it works (a green check mark)

// Master and detail: a control (the master, a table) and controls that depend on it (the details) are tied by a message command, nothing else.
//  - the master has `update_msg` = a command; when its state changes (another row of a table is selected, or none) it tells its observers (BHandler::SendNotices, the public observer calls of
//    Handler.h) with a message of that command: `master->SendNotices(command, &update)`. A form has one handler for this (`StartWatching` of every master) and passes the notice to the details.
//  - the details have `bind_to_update_msg` = that command; each is updated from the part of the message that has its `field_id`.
//  - the update has `kUpdateReplyTo` (a BMessenger: where the answers go: a notice has no return address of its own) and for each detail a message with the field id as its name; in it the
//    well-known keys `kUpdateValue` (what the control shows, of the type that the master holds: a bool for a check box, a number for a spinner, text for a field) and `kUpdateEnabled` (a bool:
//    whether it can be used). A message without the part of a detail (no row is selected) empties and disables that detail. The master does not know which controls listen, a detail does
//    not know what the master holds (a table: the rows have these messages as fields of their own, so the master puts the fields of the selected row in the message and nothing else).
//  - when the user changes a detail it sends a `kUpdated` message to the messenger of the update: `kUpdateCommand` (the command that it belongs to) and its part with `kUpdateValue` (the new
//    value, of the type that it got). Nothing else is handed back: what the value is for (a table: the field of the row that is selected) is the master's own state. The master changes itself
//    from the answer, as for any change of its own, tells its owner, and tells its observers again, so the detail shows what the master holds (also when the master refused the value).
//  - a part may have `kUpdateDefault`, the value that the master would show by default. A button (`kind = button`) that listens is the **revert button** of its field id: it is enabled while `value`
//    and `default` differ, and pressing it answers with `value` = `default` (it is no value of its own and nothing is stored under its name).
const char* const kUpdateReplyTo	= "reply_to";
const char* const kUpdateCommand	= "command";
const char* const kUpdateValue		= "value";
const char* const kUpdateEnabled	= "enabled";
const char* const kUpdateDefault	= "default";

/** The command of a message from the four characters of the schema (`OVrw`); 0 if there are not four. */
inline uint32_t UpdateCommand(const char* text)
{
	if (text == NULL || text[0] == '\0' || text[1] == '\0' || text[2] == '\0' || text[3] == '\0' || text[4] != '\0')
		return 0;
	return ((uint32_t)(uint8_t)text[0] << 24) | ((uint32_t)(uint8_t)text[1] << 16) | ((uint32_t)(uint8_t)text[2] << 8) | (uint32_t)(uint8_t)text[3];
}

const char* const kKindText			= "text";
const char* const kKindPath			= "path";
const char* const kKindChoice		= "choice";
const char* const kKindSlider		= "slider";			///< a number with `min` and `max` (and `step`) on a track
const char* const kKindSpinner		= "spinner";		///< a number with buttons for up and down; the default for a number with `min` and `max`
const char* const kKindRadio		= "radio";			///< the `choices` as radio buttons instead of a menu
const char* const kKindSecret		= "secret";
const char* const kKindColor		= "color";
const char* const kKindButton		= "button";			///< of a field of the type bool: a button with the `label` that tells the owner that it was pressed (the owner sees the field as `true` in a change; it is never stored)
const char* const kKindTable		= "table";			///< of a field with messages: a table with a row for each message and a column for each of their fields (see ShojiTableControl)
const char* const kKindProperties	= "properties";		///< of a message: a table with a row for each of its fields, the name and the value

}	// namespace form
}	// namespace settings
}	// namespace sen
