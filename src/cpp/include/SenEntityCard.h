/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2026 SEN Labs e.U.
 */
#pragma once

/**
 * @file SenEntityCard.h
 * @brief A small read-only card of a file, drawn over a view with a translucent background (header only, Haiku only).
 *
 * For views that show objects (the graph) and want to say more about the one that is selected: the picture of the file (its thumbnail, else its icon), its title and
 * the attributes that are worth a look, each with its label. Which attributes, in which order and under which labels is what Shoji says in the template of the type
 * of the file (the archived view in `shoji/templates/<MIME type>/template.view` of a data folder), so a card and the window of Shoji show the same; a type without a
 * template shows the viewable attributes of the attribute info of its MIME type. Values that a file does not have are left out, so a card is as small as it can be.
 *
 *     sen::EntityCard* card = new sen::EntityCard();
 *     if (card->Load(ref) == B_OK)
 *         view->...keep it...
 *     ...
 *     card->Draw(view, view->Bounds(), nodeCentre, nodeRadius + 8, dark);    // in Draw() of the view, after everything the card should be over
 *
 * The card draws only what is its own: the background and the text are drawn with alpha (B_OP_ALPHA), so what is below shows through, and there are no controls
 * with a background of their own. It is not interactive.
 */

#include <Bitmap.h>
#include <DataIO.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <fs_attr.h>
#include <Font.h>
#include <Message.h>
#include <MimeType.h>
#include <Node.h>
#include <NodeInfo.h>
#include <Path.h>
#include <StringView.h>
#include <TextControl.h>
#include <TranslationUtils.h>
#include <TypeConstants.h>
#include <View.h>

#include <algorithm>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <string>
#include <vector>

#include "SenAttrInfo.h"
#include "SenList.h"
#include "SenStripeDialog.h"

namespace sen {

class EntityCard {
public:
	EntityCard() : fPicture(NULL) {}

	~EntityCard() { delete fPicture; }

	/** The picture, title and values of the file. B_OK also if there is nothing to show but the name. */
	status_t
	Load(const entry_ref& ref, float pictureHeight = 48)
	{
		BNode node(&ref);
		status_t result = node.InitCheck();
		if (result != B_OK)
			return result;

		char type[B_MIME_TYPE_LENGTH];
		type[0] = '\0';
		BNodeInfo(&node).GetType(type);
		BMessage attrInfo;
		if (type[0] != '\0')
			BMimeType(type).GetAttrInfo(&attrInfo);

		std::vector<Field> fields;
		if (type[0] == '\0' || !_FieldsOfTemplate(type, &fields))
			_FieldsOfAttrInfo(attrInfo, &fields);

		fTitle = ref.name;
		fRows.clear();
		for (const Field& field : fields) {
			const ListConfig list = ListConfigOf(attrInfo, field.attribute.String());
			BString text = _ValueText(node, field.attribute.String(), list);
			if (text.IsEmpty())
				continue;
			if (fTitle == ref.name && _IsTitleAttribute(field.attribute.String())) {
				fTitle = text;
				continue;
			}
			Row row;
			row.label = field.label;
			row.value = text;
			row.isList = list.isList;
			fRows.push_back(row);
		}

		delete fPicture;
		fPicture = _LoadPicture(node, ref, pictureHeight);
		return B_OK;
	}

	/** where the card is beside its anchor: in the middle of it, or above it (the anchor at the lower edge) or below it (at the upper edge), for a line that the card must not cover */
	enum Align { kCentred, kAbove, kBelow };

	/** What a card says can also be set by hand (a relation, a node without a file): the picture is taken over. */
	void SetTitle(const char* title) { fTitle = title; }
	void SetPicture(BBitmap* picture) { delete fPicture; fPicture = picture; }
	void AddRow(const char* label, const char* value, bool isList = false)
	{
		Row row;
		row.label = label;
		row.value = value;
		row.isList = isList;
		fRows.push_back(row);
	}
	/** a line at the bottom, set off by a rule ("3 relations") */
	void SetFooter(const char* footer) { fFooter = footer; }

	/**
	 * Where the card is when it is drawn next to a point of the view: on its right, or on its left if there is no room, and within @p limits.
	 * It is the place for the hit test of the mouse, so the view knows when the mouse is on the card.
	 * @param anchor the point that the card belongs to (the middle of the node)
	 * @param distance how far from the point the card starts (the radius of the node and some room)
	 */
	BRect
	Frame(BView* view, BRect limits, BPoint anchor, float distance, Align align = kCentred) const
	{
		return _Measure(view, limits, anchor, distance, align).box;
	}

	/**
	 * Draws the card where Frame() says.
	 * @param dark the colours: for a dark view, or a light one
	 * @param highlight the mouse is on the card: it lights up a little
	 */
	void
	Draw(BView* view, BRect limits, BPoint anchor, float distance, bool dark, bool highlight = false, Align align = kCentred) const
	{
		const Metrics m = _Measure(view, limits, anchor, distance, align);
		const BRect box = m.box;

		// colours: the background lets the graph below show through, the text is not translucent
		rgb_color background = dark ? _Color(0x1c, 0x1f, 0x26, 205) : _Color(0xf6, 0xf6, 0xf8, 205);
		rgb_color border = dark ? _Color(0x6a, 0x72, 0x80, 170) : _Color(0x30, 0x30, 0x38, 110);
		const rgb_color text = dark ? _Color(0xe8, 0xea, 0xee, 255) : _Color(0x1c, 0x1c, 0x22, 255);
		const rgb_color dimmed = dark ? _Color(0x9c, 0xa3, 0xaf, 255) : _Color(0x5c, 0x60, 0x6a, 255);
		if (highlight) {
			// a little lighter (on a dark view) or a little darker, and a stronger border, and not as see-through
			background = dark ? _Color(0x30, 0x36, 0x44, 235) : _Color(0xe4, 0xe8, 0xf2, 235);
			border = dark ? _Color(0x8f, 0xa3, 0xc4, 230) : _Color(0x3a, 0x5a, 0x9a, 190);
		}

		view->PushState();
		view->SetDrawingMode(B_OP_ALPHA);
		view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
		view->SetHighColor(background);
		view->FillRoundRect(box, 6, 6);
		view->SetHighColor(border);
		view->StrokeRoundRect(box, 6, 6);

		float left = box.left + m.pad;
		const float contentHeight = box.Height() - m.pad * 2 - m.footerHeight;
		if (fPicture != NULL) {
			view->DrawBitmap(fPicture, BPoint(left, box.top + m.pad + (contentHeight - m.pictureHeight) / 2));
			left += m.pictureWidth + m.pad;
		}

		float baseline = box.top + m.pad + ceilf(m.ascent) + (contentHeight - m.textHeight) / 2;
		BString title(fTitle);
		m.bold.TruncateString(&title, B_TRUNCATE_END, m.textWidth);
		view->SetFont(&m.bold);
		view->SetHighColor(text);
		view->DrawString(title.String(), BPoint(left, baseline));

		view->SetFont(&m.plain);
		baseline += m.pad / 2;
		for (const Row& row : fRows) {
			baseline += m.line;
			view->SetHighColor(dimmed);
			view->DrawString(row.label.String(), BPoint(left + m.labelWidth - m.plain.StringWidth(row.label.String()), baseline));
			BString value(row.value);
			m.plain.TruncateString(&value, row.isList ? B_TRUNCATE_MIDDLE : B_TRUNCATE_END, m.valueWidth);
			view->SetHighColor(text);
			view->DrawString(value.String(), BPoint(left + m.labelWidth + m.gap, baseline));
		}

		if (m.footerHeight > 0) {
			const float rule = box.bottom - m.pad - m.line - m.pad / 2;
			view->SetHighColor(_Color(border.red, border.green, border.blue, 90));
			view->StrokeLine(BPoint(box.left + m.pad, rule), BPoint(box.right - m.pad, rule));
			BString footer(fFooter);
			m.plain.TruncateString(&footer, B_TRUNCATE_END, box.Width() - m.pad * 2);
			view->SetHighColor(dimmed);
			view->DrawString(footer.String(), BPoint(box.left + m.pad, box.bottom - m.pad - m.descent - 1));
		}
		view->PopState();
	}

	const BString& Title() const { return fTitle; }

	/** the number of lines with a value (without the title) */
	int32 CountRows() const { return (int32) fRows.size(); }

private:
	struct Field {
		BString attribute;
		BString label;
	};

	/** the sizes of the parts of the card, and where it is */
	struct Metrics {
		BFont	plain, bold;
		float	ascent, descent, line, pad, gap;
		float	labelWidth, valueWidth, textWidth, textHeight;
		float	pictureWidth, pictureHeight, footerHeight;
		BRect	box;
	};

	Metrics
	_Measure(BView* view, BRect limits, BPoint anchor, float distance, Align align) const
	{
		Metrics m;
		view->GetFont(&m.plain);
		m.plain.SetSize(m.plain.Size() * 0.9f);
		m.bold = m.plain;
		m.bold.SetFace(B_BOLD_FACE);

		font_height height;
		m.plain.GetHeight(&height);
		m.ascent = height.ascent;
		m.descent = height.descent;
		m.line = ceilf(height.ascent + height.descent + height.leading) + 2;
		m.pad = ceilf(m.plain.Size() * 0.7f);
		m.gap = m.pad;
		const float letter = m.plain.StringWidth("M");

		m.labelWidth = m.valueWidth = 0;
		for (const Row& row : fRows) {
			m.labelWidth = std::max(m.labelWidth, m.plain.StringWidth(row.label.String()));
			m.valueWidth = std::max(m.valueWidth, m.plain.StringWidth(row.value.String()));
		}
		m.valueWidth = std::min(m.valueWidth, letter * 24);
		const float titleWidth = std::min(m.bold.StringWidth(fTitle.String()), letter * 30);
		const float footerWidth = std::min(m.plain.StringWidth(fFooter.String()), letter * 30);
		m.textWidth = std::max(titleWidth, fRows.empty() ? 0 : m.labelWidth + m.gap + m.valueWidth);
		m.pictureWidth = fPicture != NULL ? fPicture->Bounds().Width() + 1 : 0;
		m.pictureHeight = fPicture != NULL ? fPicture->Bounds().Height() + 1 : 0;
		m.textHeight = m.line + (fRows.empty() ? 0 : m.pad / 2 + m.line * fRows.size());
		m.footerHeight = fFooter.IsEmpty() ? 0 : m.pad / 2 + m.line;

		const float contentWidth = std::max(m.textWidth, footerWidth);
		BRect box(0, 0, m.pad + (m.pictureWidth > 0 ? m.pictureWidth + m.pad : 0) + contentWidth + m.pad,
			m.pad + std::max(m.pictureHeight, m.textHeight) + m.footerHeight + m.pad);
		const float top = align == kAbove ? anchor.y - 6 - box.Height() : align == kBelow ? anchor.y + 6 : anchor.y - box.Height() / 2;
		box.OffsetTo(anchor.x + distance, top);
		if (box.right > limits.right - 4)
			box.OffsetTo(anchor.x - distance - box.Width(), box.top);
		box.OffsetTo(std::max(limits.left + 4, std::min(box.left, limits.right - 4 - box.Width())),
			std::max(limits.top + 4, std::min(box.top, limits.bottom - 4 - box.Height())));
		m.box = box;
		return m;
	}

	struct Row {
		Row() : isList(false) {}

		BString label;
		BString value;
		bool	isList;	///< a list, as simple CSV: too long, it is cut in the middle
	};

	static rgb_color
	_Color(uint8 red, uint8 green, uint8 blue, uint8 alpha)
	{
		rgb_color color = {red, green, blue, alpha};
		return color;
	}

	static bool
	_IsTitleAttribute(const char* name)
	{
		return strcmp(name, "dc:title") == 0 || strcmp(name, "META:name") == 0;
	}

	// ---- which attributes

	/** The fields of the template of Shoji, in the order of the view: a text control has the name of the attribute, its label (or the string view before it) says what it is. */
	static bool
	_FieldsOfTemplate(const char* mimeType, std::vector<Field>* fields)
	{
		if (strstr(mimeType, "..") != NULL || mimeType[0] == '/')
			return false;

		static const directory_which folders[] = {B_USER_SETTINGS_DIRECTORY, B_USER_DATA_DIRECTORY, B_USER_NONPACKAGED_DATA_DIRECTORY, B_SYSTEM_DATA_DIRECTORY,
			B_SYSTEM_NONPACKAGED_DATA_DIRECTORY};
		for (directory_which which : folders) {
			BPath path;
			if (find_directory(which, &path) != B_OK || path.Append("shoji/templates") != B_OK || path.Append(mimeType) != B_OK || path.Append("template.view") != B_OK)
				continue;
			BFile file(path.Path(), B_READ_ONLY);
			BMessage archive;
			if (file.InitCheck() != B_OK || archive.Unflatten(&file) != B_OK)
				continue;

			BView* root = new BView(&archive);
			BString pending;
			_Collect(root, &pending, fields);
			delete root;
			return !fields->empty();
		}
		return false;
	}

	static void
	_Collect(BView* view, BString* pendingLabel, std::vector<Field>* fields)
	{
		if (BStringView* label = dynamic_cast<BStringView*>(view)) {
			if (label->Text() != NULL && label->Text()[0] != '\0')
				pendingLabel->SetTo(label->Text());
		} else if (BTextControl* control = dynamic_cast<BTextControl*>(view)) {
			Field field;
			field.attribute = control->Name();
			field.label = control->Label() != NULL && control->Label()[0] != '\0' ? control->Label() : pendingLabel->String();
			pendingLabel->SetTo("");
			if (!field.attribute.IsEmpty())
				fields->push_back(field);
			return;	// (what is inside a control is not the template's)
		}
		for (BView* child = view->ChildAt(0); child != NULL; child = child->NextSibling())
			_Collect(child, pendingLabel, fields);
	}

	/** Without a template: the attributes of the MIME type that are to be viewed. */
	static void
	_FieldsOfAttrInfo(const BMessage& attrInfo, std::vector<Field>* fields)
	{
		const char* name = NULL;
		for (int32 i = 0; attrInfo.FindString("attr:name", i, &name) == B_OK; i++) {
			bool viewable = false;
			attrInfo.FindBool("attr:viewable", i, &viewable);
			if (!viewable || name == NULL)
				continue;
			Field field;
			field.attribute = name;
			const char* publicName = NULL;
			attrInfo.FindString("attr:public_name", i, &publicName);
			field.label = publicName != NULL && publicName[0] != '\0' ? publicName : name;
			fields->push_back(field);
		}
	}

	// ---- the values

	/** The value of an attribute as one line of text (a list: its values with a comma between, simple CSV: values that need quotes in the file are shown without). */
	static BString
	_ValueText(BNode& node, const char* attribute, const ListConfig& list)
	{
		attr_info info;
		if (node.GetAttrInfo(attribute, &info) != B_OK || info.size <= 0 || info.size > 4096)
			return BString();

		BString text;
		switch (info.type) {
			case B_STRING_TYPE:
			case B_MIME_STRING_TYPE:
			{
				std::string value(info.size, '\0');
				ssize_t got = node.ReadAttr(attribute, info.type, 0, &value[0], info.size);
				if (got <= 0)
					return BString();
				value.resize(strnlen(value.c_str(), got));
				if (list.isList) {
					std::string joined;
					for (const std::string& item : list::Split(value, list.separator))
						joined += (joined.empty() ? "" : ", ") + item;
					value = joined;
				}
				text = value.c_str();
				break;
			}
			case B_INT32_TYPE:
			{
				int32 value = 0;
				if (node.ReadAttr(attribute, info.type, 0, &value, sizeof(value)) == sizeof(value))
					text << value;
				break;
			}
			case B_INT64_TYPE:
			{
				int64 value = 0;
				if (node.ReadAttr(attribute, info.type, 0, &value, sizeof(value)) == sizeof(value))
					text << value;
				break;
			}
			case B_UINT32_TYPE:
			{
				uint32 value = 0;
				if (node.ReadAttr(attribute, info.type, 0, &value, sizeof(value)) == sizeof(value))
					text << value;
				break;
			}
			case B_BOOL_TYPE:
			{
				bool value = false;
				if (node.ReadAttr(attribute, info.type, 0, &value, sizeof(value)) == sizeof(value))
					text = value ? "yes" : "no";
				break;
			}
			case B_DOUBLE_TYPE:
			{
				double value = 0;
				if (node.ReadAttr(attribute, info.type, 0, &value, sizeof(value)) == sizeof(value))
					text << value;
				break;
			}
			case B_TIME_TYPE:
			{
				time_t value = 0;
				if (node.ReadAttr(attribute, info.type, 0, &value, sizeof(value)) == sizeof(value)) {
					struct tm parts;
					char formatted[32];
					if (localtime_r(&value, &parts) != NULL && strftime(formatted, sizeof(formatted), "%Y-%m-%d", &parts) > 0)
						text = formatted;
				}
				break;
			}
			default:
				break;	// (raw data, pictures and the like are not text)
		}
		return text;
	}

	/** The thumbnail of the file (Media:Thumbnail, an encoded image), scaled to the height; else the icon of the file. */
	static BBitmap*
	_LoadPicture(BNode& node, const entry_ref& ref, float height)
	{
		attr_info info;
		if (node.GetAttrInfo("Media:Thumbnail", &info) == B_OK && info.size > 0 && info.size <= 4 * 1024 * 1024) {
			BMallocIO data;
			data.SetSize(info.size);
			if (node.ReadAttr("Media:Thumbnail", info.type, 0, (void*) data.Buffer(), info.size) == info.size) {
				BBitmap* thumbnail = BTranslationUtils::GetBitmap(&data);
				BBitmap* scaled = ScaleDown(thumbnail, height);
				delete thumbnail;
				if (scaled != NULL)
					return scaled;
			}
		}

		BBitmap* icon = new BBitmap(BRect(0, 0, height - 1, height - 1), B_RGBA32);
		if (icon->IsValid() && BNodeInfo::GetTrackerIcon(&ref, icon, (icon_size)(int) height) == B_OK)
			return icon;
		delete icon;
		return NULL;
	}

	BString				fTitle;
	BString				fFooter;
	std::vector<Row>	fRows;
	BBitmap*			fPicture;
};

}	// namespace sen
