// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2025
#include "ayu/ui/boxes/json_viewer_box.h"

#include "ayu/ui/settings/ayu_hant_helper.h"
#include "lang_auto.h"
#include "styles/style_boxes.h"
#include "styles/style_layers.h"
#include "ui/rp_widget.h"

#include <rpl/rpl.h>

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonParseError>
#include <QtCore/QStringList>
#include <QtGui/QClipboard>
#include <QtGui/QGuiApplication>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QVBoxLayout>

namespace Ui {
namespace {

enum Column : int {
	kColumnKey = 0,
	kColumnValue = 1,
	kColumnType = 2,
};

[[nodiscard]] QString JsonTypeName(const QJsonValue &value) {
	switch (value.type()) {
	case QJsonValue::Object: return qsl("object");
	case QJsonValue::Array: return qsl("array");
	case QJsonValue::String: return qsl("string");
	case QJsonValue::Double: return qsl("number");
	case QJsonValue::Bool: return qsl("boolean");
	case QJsonValue::Null: return qsl("null");
	case QJsonValue::Undefined: return qsl("undefined");
	}
	return qsl("unknown");
}

[[nodiscard]] QString JsonScalarToString(const QJsonValue &value) {
	switch (value.type()) {
	case QJsonValue::String: return value.toString();
	case QJsonValue::Double: return QString::number(value.toDouble(), 'g', 15);
	case QJsonValue::Bool: return value.toBool() ? qsl("true") : qsl("false");
	case QJsonValue::Null: return qsl("null");
	default: return QString();
	}
}

[[nodiscard]] QString CompactScalarPreview(const QJsonValue &value) {
	auto text = JsonScalarToString(value);
	text.replace(qsl("\n"), qsl("\\n"));
	text.replace(qsl("\r"), qsl("\\r"));
	text.replace(qsl("\t"), qsl("\\t"));
	if (text.size() > 160) {
		text = text.mid(0, 160) + qsl("...");
	}
	return text;
}

[[nodiscard]] QString AppendPath(const QString &parentPath, const QString &key) {
	if (parentPath.isEmpty()) {
		return key;
	}
	if (key.startsWith(qsl("["))) {
		return parentPath + key;
	}
	if (parentPath == qsl("$")) {
		return parentPath + qsl(".") + key;
	}
	return parentPath + qsl(".") + key;
}

void AddJsonNode(
		QTreeWidgetItem *parent,
		const QString &key,
		const QJsonValue &value,
		const QString &path) {
	auto *item = new QTreeWidgetItem(parent);
	item->setText(kColumnKey, key);
	item->setText(kColumnType, JsonTypeName(value));
	item->setData(kColumnKey, Qt::UserRole, path);

	if (value.isObject()) {
		const auto object = value.toObject();
		item->setText(kColumnValue, QString("{%1}").arg(object.size()));
		item->setData(kColumnValue, Qt::UserRole, qsl("<object>"));
		for (auto i = object.begin(); i != object.end(); ++i) {
			AddJsonNode(item, i.key(), i.value(), AppendPath(path, i.key()));
		}
	} else if (value.isArray()) {
		const auto array = value.toArray();
		item->setText(kColumnValue, QString("[%1]").arg(array.size()));
		item->setData(kColumnValue, Qt::UserRole, qsl("<array>"));
		for (auto i = 0; i != array.size(); ++i) {
			const auto keyAt = QString("[%1]").arg(i);
			AddJsonNode(item, keyAt, array.at(i), AppendPath(path, keyAt));
		}
	} else {
		const auto full = JsonScalarToString(value);
		item->setText(kColumnValue, CompactScalarPreview(value));
		item->setData(kColumnValue, Qt::UserRole, full);
	}
}

void BuildJsonTree(not_null<QTreeWidget*> tree, const QJsonDocument &doc) {
	tree->clear();
	if (doc.isObject()) {
		AddJsonNode(tree->invisibleRootItem(), qsl("$"), QJsonValue(doc.object()), qsl("$"));
	} else if (doc.isArray()) {
		AddJsonNode(tree->invisibleRootItem(), qsl("$"), QJsonValue(doc.array()), qsl("$"));
	}
	tree->expandToDepth(1);
}

[[nodiscard]] bool FilterItemRecursive(not_null<QTreeWidgetItem*> item, const QString &needle) {
	if (needle.isEmpty()) {
		item->setHidden(false);
		for (auto i = 0; i < item->childCount(); ++i) {
			FilterItemRecursive(item->child(i), needle);
		}
		return true;
	}

	const auto selfMatch = item->text(kColumnKey).contains(needle, Qt::CaseInsensitive)
		|| item->text(kColumnValue).contains(needle, Qt::CaseInsensitive)
		|| item->text(kColumnType).contains(needle, Qt::CaseInsensitive);

	auto childMatch = false;
	for (auto i = 0; i < item->childCount(); ++i) {
		if (FilterItemRecursive(item->child(i), needle)) {
			childMatch = true;
		}
	}

	const auto visible = selfMatch || childMatch;
	item->setHidden(!visible);
	if (visible && !needle.isEmpty()) {
		item->setExpanded(true);
	}
	return visible;
}

void ApplyFilter(not_null<QTreeWidget*> tree, const QString &needle) {
	for (auto i = 0; i < tree->topLevelItemCount(); ++i) {
		FilterItemRecursive(tree->topLevelItem(i), needle.trimmed());
	}
}

} // namespace

void FillJsonViewerBox(not_null<GenericBox*> box, const QString &json) {
	box->setTitle(rpl::single(AyuHantHelper(qsl("ayu_ViewJsonTitle"), qsl("View JSON Data"))));
	box->setWidth(int(st::boxWideWidth * 1.25));

	const auto content = box->verticalLayout();
	auto container = content->add(object_ptr<Ui::RpWidget>(content));
	auto *layout = new QVBoxLayout(container);
	layout->setContentsMargins(st::boxRowPadding);
	layout->setSpacing(st::boxRowPadding.top());

	QJsonParseError parseError;
	const auto parsed = QJsonDocument::fromJson(json.toUtf8(), &parseError);
	auto displayText = json;
	if (parseError.error == QJsonParseError::NoError && !parsed.isNull()) {
		displayText = QString::fromUtf8(parsed.toJson(QJsonDocument::Indented));

		auto *search = new QLineEdit(container);
		search->setPlaceholderText(AyuHantHelper(
			qsl("ayu_ViewJsonSearchPlaceholder"),
			qsl("Search key / value / type...")));
		layout->addWidget(search);

		auto *tree = new QTreeWidget(container);
		tree->setHeaderLabels(QStringList()
			<< AyuHantHelper(qsl("ayu_MessageDetailsKey"), qsl("Key"))
			<< AyuHantHelper(qsl("ayu_MessageDetailsValue"), qsl("Value"))
			<< AyuHantHelper(qsl("ayu_MessageDetailsType"), qsl("Type")));
		tree->setAlternatingRowColors(true);
		tree->setUniformRowHeights(true);
		tree->setIndentation(16);
		tree->setMinimumHeight(420);
		tree->header()->setStretchLastSection(false);
		tree->header()->setSectionResizeMode(kColumnKey, QHeaderView::ResizeToContents);
		tree->header()->setSectionResizeMode(kColumnValue, QHeaderView::Stretch);
		tree->header()->setSectionResizeMode(kColumnType, QHeaderView::ResizeToContents);

		auto font = tree->font();
		font.setPointSize(12);
		tree->setFont(font);

		tree->setStyleSheet(
			"QTreeWidget {"
			" border: 1px solid rgba(255, 255, 255, 0.14);"
			" border-radius: 8px;"
			" background-color: rgba(10, 19, 36, 0.88);"
			" color: #DDE6F2;"
			" alternate-background-color: rgba(255, 255, 255, 0.03);"
			"}"
			"QTreeWidget::item { padding: 4px 2px; }"
			"QTreeWidget::item:selected {"
			" background-color: rgba(80, 130, 220, 0.35);"
			" color: #FFFFFF;"
			"}"
			"QHeaderView::section {"
			" background-color: rgba(7, 14, 28, 0.98);"
			" color: #DDE6F2;"
			" border: none;"
			" padding: 6px;"
			"}"
			"QTableCornerButton::section {"
			" background-color: rgba(7, 14, 28, 0.98);"
			" border: none;"
			"}");

		BuildJsonTree(tree, parsed);
		layout->addWidget(tree);

		auto *detail = new QTextEdit(container);
		detail->setReadOnly(true);
		detail->setMinimumHeight(120);
		detail->setPlaceholderText(AyuHantHelper(
			qsl("ayu_ViewJsonDetailPlaceholder"),
			qsl("Select a node to view full value and path.")));
		detail->setStyleSheet(
			"QTextEdit {"
			" border: 1px solid rgba(255, 255, 255, 0.14);"
			" border-radius: 8px;"
			" background-color: rgba(10, 19, 36, 0.88);"
			" color: #DDE6F2;"
			"}");
		layout->addWidget(detail);

		QObject::connect(search, &QLineEdit::textChanged, tree, [=](const QString &text) {
			ApplyFilter(tree, text);
		});
		QObject::connect(tree, &QTreeWidget::currentItemChanged, tree, [=](QTreeWidgetItem *current, QTreeWidgetItem *) {
			if (!current) {
				detail->clear();
				return;
			}
			const auto path = current->data(kColumnKey, Qt::UserRole).toString();
			const auto type = current->text(kColumnType);
			const auto full = current->data(kColumnValue, Qt::UserRole).toString();
			detail->setPlainText(
				qsl("Path: ") + path
				+ qsl("\nType: ") + type
				+ qsl("\n\nValue:\n") + full);
		});

		box->addButton(rpl::single(AyuHantHelper(qsl("ayu_ViewJsonExpandAll"), qsl("Expand All"))), [=] {
			tree->expandAll();
		});
		box->addButton(rpl::single(AyuHantHelper(qsl("ayu_ViewJsonCollapseAll"), qsl("Collapse All"))), [=] {
			tree->collapseAll();
			tree->expandToDepth(1);
		});
		box->addButton(rpl::single(AyuHantHelper(qsl("ayu_ViewJsonCopySelectedValue"), qsl("Copy Value"))), [=] {
			if (const auto current = tree->currentItem()) {
				QGuiApplication::clipboard()->setText(current->data(kColumnValue, Qt::UserRole).toString());
			}
		});
	} else {
		auto *fallback = new QTextEdit(container);
		fallback->setReadOnly(true);
		fallback->setMinimumHeight(520);
		fallback->setPlainText(
			AyuHantHelper(qsl("ayu_ViewJsonParseError"), qsl("JSON parse failed, showing raw text:"))
			+ qsl("\n\n")
			+ json);
		layout->addWidget(fallback);
	}

	container->resizeToWidth(box->width());
	container->resize(container->width(), 600);

	box->addButton(rpl::single(AyuHantHelper(qsl("ayu_MessageDetailsCopy"), qsl("Copy JSON"))), [=] {
		QGuiApplication::clipboard()->setText(displayText);
	});
	box->addLeftButton(tr::lng_close(), [=] {
		box->closeBox();
	});
}

} // namespace Ui
