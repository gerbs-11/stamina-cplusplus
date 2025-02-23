#ifndef STAMINA_PROPERTY_WIZARD_H
#define STAMINA_PROPERTY_WIZARD_H

#include "ui/ui_PropertyWizard.h"

#include <QDialog>
#include <QString>
#include <cstdint>
#include <QStandardItemModel>
#include <QVector>
#include <QVariant>

namespace stamina {
	namespace gui {
		enum OPERAND_TYPE {
			BINARY_OPERAND
			, UNARY_OPERAND
			, VARIABLE
			, VALUE
			, EMPTY
		};
		typedef uint8_t operandType_t;
		struct OperandAndDescription {
			QString operand;
			QString description;
			OperandAndDescription(QString operand, QString description)
				: operand(operand), description(description) { /* Intentionally left empty */ }
		};
		struct OperandInformation {
			inline const static OperandAndDescription binaryOperands[] = {
				// Logic operands
				OperandAndDescription("&", "AND Operand") // AND Operand
				, OperandAndDescription("|", "OR Operand") // OR Operand
				// Path-formula Operands
				, OperandAndDescription("U", "Until Operand") // Until operand
// 				, OperandAndDescription("U <=", "Bounded Until Operand") // TODO: Until operand
				// Math Operands
				, OperandAndDescription("+", "Addition Operand") // Add
				, OperandAndDescription("-", "Subtraction Operand") // Subtract
				, OperandAndDescription("*", "Multiply Operand") // Multiply
				, OperandAndDescription("/", "Division Operand") // divide
			};
			inline const static OperandAndDescription unaryOperands[] = {
				// Logic operands
				OperandAndDescription("!", "NOT Operand") // NOT Operand
				// Path-formula Operands
				, OperandAndDescription("F", "Eventually Operand") // Eventually Operand
				, OperandAndDescription("G", "Always (globally) Operand") // Always Operand
				, OperandAndDescription("X", "Next Operand") // Next Operand
				// Math Operands
				, OperandAndDescription("-", "Numeric Negation Operand") // Subtract
			};
		};
		class PropertyWizard : public QDialog {
			Q_OBJECT
		public:
			PropertyWizard(QWidget * parent = nullptr);
		protected:
			void setupActions();
			Ui::PropertyWizard ui;
			/**
			 * Inserts into the operand tree. Will insert over "EMPTY" if selected. If no "EMPTY" is
			 * selected, raises messagebox
			 *
			 * @param opString The string of the operand
			 * @param opType The type of the Operand
			 * */
			void insertOperand(QString opString, operandType_t opType);
			std::vector<QString> variables;
		private slots:
			/**
			 * Gets information from the comboboxes in ui and inserts the operand
			 * */
			void getInfoAndInsertOperand();
			/**
			 * Deletes the selected operand in the tree.
			 * */
			void deleteSelectedOperand();
			/**
			 * Updates the values in expressionOptions
			 * */
			void updateValuesInExpressionOptions(int oldIndex);
		};

		/* An operand item stored in the PropertyTree */
		class OperandItem : QStandardItem {
			Q_OBJECT
		public:
			// TODO: https://doc.qt.io/qt-5/qtwidgets-itemviews-simpletreemodel-example.html
			explicit OperandItem(
				int rows
				, int columns = 1
				, operandType_t opType = OPERAND_TYPE::EMPTY
			);

			QString createExpressionFromThisAndChildren();
			operandType_t opType;
		private:
		};

		class PropertyTreeModel : public QStandardItemModel {
			Q_OBJECT
		public:
			PropertyTreeModel(const QString &data, QObject *parent = nullptr);

			QString toPropertyString();
		};
	}
}
#endif // STAMINA_PROPERTY_WIZARD_H
