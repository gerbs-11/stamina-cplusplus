#include "PropertyWizard.h"

#include <iostream>

namespace stamina {
namespace gui {

PropertyWizard::PropertyWizard(QWidget * parent)
: QDialog(parent)
{
	setupActions();
}

void
PropertyWizard::setupActions() {
	ui.setupUi(this);
	ui.expressionType->insertItem(OPERAND_TYPE::BINARY_OPERAND, QString("Binary Operand"));
	ui.expressionType->insertItem(OPERAND_TYPE::UNARY_OPERAND, QString("Unary Operand"));
	ui.expressionType->insertItem(OPERAND_TYPE::VARIABLE, QString("Variable"));
	ui.expressionType->insertItem(OPERAND_TYPE::VALUE, QString("Value"));

	connect(
		ui.expressionType
		, SIGNAL(currentIndexChanged(int))
		, this
		, SLOT(updateValuesInExpressionOptions(int))
	);

	connect(
		ui.addExpression
		, SIGNAL(clicked())
		, this
		, SLOT(getInfoAndInsertOperand())
	);
}

void
PropertyWizard::insertOperand(QString opString, operandType_t opType) {
	auto selectedItems = ui.propertyTree->selectedItems();
	for (auto item : selectedItems) {
		ui.propertyTree->insertTopLevelItem(
			ui.propertyTree->currentColumn() + 1
			, new QTreeWidgetItem(
				ui.propertyTree
				, item
				, QStringList(QString(opString))
			)
		);
	}
}
void
PropertyWizard::deleteSelectedOperand() {
// 	QModelIndexList selectedIndecies = ui.propertyTree->selectedIndexes();
// 	for (auto index : selectedIndecies) {
// 		// TODO: delete items. Can't seem to find the method in QStandardItemModel
// 		int row = index.row();
// 		int column = index.row();
// 	}
}

void
PropertyWizard::getInfoAndInsertOperand() {
	QString info("Some information");
	insertOperand(info, 0);
}

void
PropertyWizard::updateValuesInExpressionOptions(int oldIndex) {
	int index = 0;
	ui.expressionOptions->setEditable(false);
	std::cout << "Resetting values in combobox" << std::endl;
	switch (oldIndex) {
		case OPERAND_TYPE::BINARY_OPERAND:
			ui.expressionOptions->clear();
			for (OperandAndDescription opAndDesc : OperandInformation::binaryOperands) {
				ui.expressionOptions->insertItem(index, opAndDesc.description);
				index++;
			}
			break;
		case OPERAND_TYPE::UNARY_OPERAND:
			ui.expressionOptions->clear();
			for (OperandAndDescription opAndDesc : OperandInformation::unaryOperands) {
				ui.expressionOptions->insertItem(index, opAndDesc.description);
				index++;
			}
			break;
		case OPERAND_TYPE::VARIABLE:
			ui.expressionOptions->clear();
			for (QString var : variables) {
				ui.expressionOptions->insertItem(index, var);
				index++;
			}
			break;
		case OPERAND_TYPE::VALUE:
			ui.expressionOptions->clear();
			ui.expressionOptions->setEditable(true);
			ui.expressionOptions->insertItem(0, QString("True"));
			ui.expressionOptions->insertItem(1, QString("False"));
			break;
		default:
			std::cerr << "INVALID OPERAND TYPE!" << std::endl;
	}
}

// OperandItem implementation

OperandItem::OperandItem(
	int rows
	, int columns
	, operandType_t opType
)
	: QStandardItem(rows, columns)
	, opType(opType)
{
	// Intentionally left empty
}

QString
OperandItem::createExpressionFromThisAndChildren() {
	switch (opType) {
		case OPERAND_TYPE::BINARY_OPERAND:
			if (childCount() != 2) {
				std::cerr << "Cannot create binary operand with child count " << childCount() << "!" << std::endl;
				return QString("OPERROR");
			}
			return QString("(  ")
				+ child(0)->createExpressionFromThisAndChildren()
				+ ' ' + data(1).toString() + ' '
				+ child(1)->createExpressionFromThisAndChildren()
				+ QString(" )");
		case OPERAND_TYPE::UNARY_OPERAND:
			if (childCount() != 1) {
				std::cerr << "Cannot create unary operand with child count " << childCount() << "!" << std::endl;
				return QString("OPERROR");
			}
			return QString("(  ")
				+ child(0)->createExpressionFromThisAndChildren()
				+ ' ' + data(1).toString() + ' '
				+ QString(" )");
		case OPERAND_TYPE::VARIABLE:
		case OPERAND_TYPE::VALUE:
			// fallthrough intentional
			if (childCount() != 0) {
				std::cerr << "Cannot create unary operand with nonzero child count " << childCount() << "!" << std::endl;
				return QString("OPERROR");
			}
			return data(1).toString();
		case OPERAND_TYPE::EMPTY:
			std::cerr << "Operand is empty!" << std::endl;
			return QString("EMPTY");
		default:
			return QString("OPERROR");
	}
}

// Implementation for PropertyTreeModel
PropertyTreeModel::PropertyTreeModel(const QString &data, QObject *parent)
    : QStandardItemModel(parent)
{
	// Intentionally left empty
}

QString
PropertyTreeModel::toPropertyString() {
	QString property = "[";

	return property;
}

} // namespace gui
} // namespace stamina
