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

	/**
	 * Draws the card next to a point of the view: on its right, or on its left if there is no room, and within @p limits.
	 * @param anchor the point that the card belongs to (the middle of the node)
	 * @param distance how far from the point the card starts (the radius of the node and some room)
	 * @param dark the colours: for a dark view, or a light one
	 */
	void
	Draw(BView* view, BRect limits, BPoint anchor, float distance, bool dark) const
	{
		BFont plain;
		view->GetFont(&plain);
		plain.SetSize(plain.Size() * 0.9f);
		BFont bold(plain);
		bold.SetFace(B_BOLD_FACE);

		font_height height;
		plain.GetHeight(&height);
		const float line = ceilf(height.ascent + height.descent + height.leading) + 2;
		const float pad = ceilf(plain.Size() * 0.7f);
		const float letter = plain.StringWidth("M");

		// the sizes of the parts
		float labelWidth = 0, valueWidth = 0;
		for (const Row& row : fRows) {
			labelWidth = std::max(labelWidth, plain.StringWidth(row.label.String()));
			valueWidth = std::max(valueWidth, plain.StringWidth(row.value.String()));
		}
		valueWidth = std::min(valueWidth, letter * 24);
		const float titleWidth = std::min(bold.StringWidth(fTitle.String()), letter * 30);
		const float gap = pad;
		const float textWidth = std::max(titleWidth, fRows.empty() ? 0 : labelWidth + gap + valueWidth);
		const float pictureWidth = fPicture != NULL ? fPicture->Bounds().Width() + 1 : 0;
		const float pictureHeight = fPicture != NULL ? fPicture->Bounds().Height() + 1 : 0;
		const float textHeight = line + (fRows.empty() ? 0 : pad / 2 + line * fRows.size());

		BRect box(0, 0, pad + (pictureWidth > 0 ? pictureWidth + pad : 0) + textWidth + pad, pad + std::max(pictureHeight, textHeight) + pad);
		box.OffsetTo(anchor.x + distance, anchor.y - box.Height() / 2);
		if (box.right > limits.right - 4)
			box.OffsetTo(anchor.x - distance - box.Width(), box.top);
		box.OffsetTo(std::max(limits.left + 4, std::min(box.left, limits.right - 4 - box.Width())),
			std::max(limits.top + 4, std::min(box.top, limits.bottom - 4 - box.Height())));

		// colours: the background lets the graph below show through, the text is not translucent
		const rgb_color background = dark ? _Color(0x1c, 0x1f, 0x26, 205) : _Color(0xf6, 0xf6, 0xf8, 205);
		const rgb_color border = dark ? _Color(0x6a, 0x72, 0x80, 170) : _Color(0x30, 0x30, 0x38, 110);
		const rgb_color text = dark ? _Color(0xe8, 0xea, 0xee, 255) : _Color(0x1c, 0x1c, 0x22, 255);
		const rgb_color dimmed = dark ? _Color(0x9c, 0xa3, 0xaf, 255) : _Color(0x5c, 0x60, 0x6a, 255);

		view->PushState();
		view->SetDrawingMode(B_OP_ALPHA);
		view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
		view->SetHighColor(background);
		view->FillRoundRect(box, 6, 6);
		view->SetHighColor(border);
		view->StrokeRoundRect(box, 6, 6);

		float left = box.left + pad;
		if (fPicture != NULL) {
			view->DrawBitmap(fPicture, BPoint(left, box.top + (box.Height() - pictureHeight) / 2));
			left += pictureWidth + pad;
		}

		float baseline = box.top + pad + ceilf(height.ascent) + (box.Height() - pad * 2 - textHeight) / 2;
		BString title(fTitle);
		bold.TruncateString(&title, B_TRUNCATE_END, textWidth);
		view->SetFont(&bold);
		view->SetHighColor(text);
		view->DrawString(title.String(), BPoint(left, baseline));

		view->SetFont(&plain);
		baseline += pad / 2;
		for (const Row& row : fRows) {
			baseline += line;
			view->SetHighColor(dimmed);
			view->DrawString(row.label.String(), BPoint(left + labelWidth - plain.StringWidth(row.label.String()), baseline));
			BString value(row.value);
			plain.TruncateString(&value, row.isList ? B_TRUNCATE_MIDDLE : B_TRUNCATE_END, valueWidth);
			view->SetHighColor(text);
			view->DrawString(value.String(), BPoint(left + labelWidth + gap, baseline));
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
	std::vector<Row>	fRows;
	BBitmap*			fPicture;
};

}	// namespace sen
