// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2025
#include "ayu/ui/boxes/json_viewer_box.h"

#include "lang_auto.h"
#include "styles/style_boxes.h"
#include "styles/style_layers.h"
#include "ui/rp_widget.h"
#include "ayu/ui/settings/ayu_hant_helper.h"
#include <rpl/rpl.h>

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonParseError>
#include <QtCore/QStringList>
#include <QtGui/QClipboard>
#include <QtGui/QGuiApplication>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QVBoxLayout>

namespace Ui {
namespace {

QString JsonTypeName(const QJsonValue &value) {
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

QString JsonScalarToString(const QJsonValue &value) {
	switch (value.type()) {
	case QJsonValue::String: return value.toString();
	case QJsonValue::Double: return QString::number(value.toDouble(), 'g', 15);
	case QJsonValue::Bool: return value.toBool() ? qsl("true") : qsl("false");
	case QJsonValue::Null: return qsl("null");
	default: return QString();
	}
}

void AddJsonNode(QTreeWidgetItem *parent, const QString &key, const QJsonValue &value) {
	auto item = new QTreeWidgetItem(parent);
	item->setText(0, key);
	item->setText(2, JsonTypeName(value));

	if (value.isObject()) {
		const auto object = value.toObject();
		item->setText(1, QString("{%1}").arg(object.size()));
		for (auto i = object.begin(); i != object.end(); ++i) {
			AddJsonNode(item, i.key(), i.value());
		}
	} else if (value.isArray()) {
		const auto array = value.toArray();
		item->setText(1, QString("[%1]").arg(array.size()));
		for (int i = 0; i != array.size(); ++i) {
			AddJsonNode(item, QString("[%1]").arg(i), array.at(i));
		}
	} else {
		item->setText(1, JsonScalarToString(value));
	}
}

void BuildJsonTree(not_null<QTreeWidget*> tree, const QJsonDocument &doc) {
	tree->clear();
	if (doc.isObject()) {
		AddJsonNode(tree->invisibleRootItem(), qsl("$"), QJsonValue(doc.object()));
	} else if (doc.isArray()) {
		AddJsonNode(tree->invisibleRootItem(), qsl("$"), QJsonValue(doc.array()));
	}
	tree->expandToDepth(1);
}

} // namespace

void FillJsonViewerBox(not_null<GenericBox*> box, const QString &json) {
	box->setTitle(rpl::single(AyuHantHelper(qsl("ayu_ViewJsonTitle"), qsl("View JSON Data"))));
	box->setWidth(int(st::boxWideWidth * 1.2));

	const auto content = box->verticalLayout();
	auto container = content->add(object_ptr<Ui::RpWidget>(content));
	auto layout = new QVBoxLayout(container);
	layout->setContentsMargins(st::boxRowPadding);
	layout->setSpacing(st::boxRowPadding.top());

	QJsonParseError parseError;
	const auto parsed = QJsonDocument::fromJson(json.toUtf8(), &parseError);
	auto displayText = json;
	if (parseError.error == QJsonParseError::NoError && !parsed.isNull()) {
		displayText = QString::fromUtf8(parsed.toJson(QJsonDocument::Indented));
		auto tree = new QTreeWidget(container);
		tree->setHeaderLabels(QStringList()
			<< AyuHantHelper(qsl("ayu_MessageDetailsKey"), qsl("Key"))
			<< AyuHantHelper(qsl("ayu_MessageDetailsValue"), qsl("Value"))
			<< AyuHantHelper(qsl("ayu_MessageDetailsType"), qsl("Type")));
		auto treeFont = tree->font();
		treeFont.setPointSize(11);
		tree->setFont(treeFont);
		tree->header()->setStretchLastSection(false);
		tree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
		tree->header()->setSectionResizeMode(1, QHeaderView::Stretch);
		tree->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
		auto headerFont = tree->header()->font();
		headerFont.setPointSize(11);
		headerFont.setBold(true);
		tree->header()->setFont(headerFont);
		tree->setAlternatingRowColors(true);
		tree->setMinimumHeight(430);
		tree->setStyleSheet(
			"QTreeWidget {"
			" border: 1px solid rgba(255, 255, 255, 0.18);"
			" border-radius: 8px;"
			" background: rgba(12, 20, 38, 0.70);"
			" color: #DDE6F2;"
			" alternate-background-color: rgba(255, 255, 255, 0.03);"
			"}"
			"QTreeWidget::item {"
			" padding-top: 3px;"
			" padding-bottom: 3px;"
			"}"
			"QHeaderView::section {"
			" background: rgba(8, 14, 28, 0.96);"
			" color: #DDE6F2;"
			" border: 0px;"
			" padding: 6px;"
			"}"
			"QTableCornerButton::section {"
			" background: rgba(8, 14, 28, 0.96);"
			" border: 0px;"
			"}");
		BuildJsonTree(tree, parsed);
		layout->addWidget(tree);
	} else {
		auto fallback = new QTextEdit(container);
		fallback->setReadOnly(true);
		fallback->setMinimumHeight(430);
		fallback->setPlainText(
			AyuHantHelper(qsl("ayu_ViewJsonParseError"), qsl("JSON parse failed, showing raw text:"))
			+ qsl("\n\n")
			+ json);
		layout->addWidget(fallback);
	}

	container->resizeToWidth(box->width());
	container->resize(container->width(), 500);

	box->addButton(rpl::single(AyuHantHelper(qsl("ayu_MessageDetailsCopy"), qsl("Copy JSON"))), [=] {
		QGuiApplication::clipboard()->setText(displayText);
	});
	box->addLeftButton(tr::lng_close(), [=] {
		box->closeBox();
	});
}

} // namespace Ui
