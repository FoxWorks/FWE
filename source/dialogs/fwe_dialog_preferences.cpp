////////////////////////////////////////////////////////////////////////////////
/// @file
////////////////////////////////////////////////////////////////////////////////
/// Copyright (C) 2012-2013, Black Phoenix
/// All rights reserved.
///
/// Redistribution and use in source and binary forms, with or without
/// modification, are permitted provided that the following conditions are met:
///   - Redistributions of source code must retain the above copyright
///     notice, this list of conditions and the following disclaimer.
///   - Redistributions in binary form must reproduce the above copyright
///     notice, this list of conditions and the following disclaimer in the
///     documentation and/or other materials provided with the distribution.
///   - Neither the name of the author nor the names of the contributors may
///     be used to endorse or promote products derived from this software without
///     specific prior written permission.
///
/// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
/// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
/// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
/// DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDERS BE LIABLE FOR ANY
/// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
/// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
/// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
/// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
/// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
/// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
////////////////////////////////////////////////////////////////////////////////
#include <QSettings>
#include <QDialog>
#include <QFormLayout>
#include <QListWidget>
#include <QStackedWidget>
#include <QPushButton>
#include <QListWidgetItem>
#include <QSlider>
#include <QSpinBox>
#include <QLabel>
#include <QCheckBox>
#include <QDoubleSpinBox>

#include "fwe_main.h"
#include "fwe_dialog_preferences.h"


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
PreferencesDialog::PreferencesDialog() {
	contents = new QStackedWidget;
	pages = new QListWidget;
	pages->setViewMode(QListView::IconMode);
	pages->setIconSize(QSize(64, 64));
	pages->setMovement(QListView::Static);
	pages->setMaximumWidth(96);
	pages->setSpacing(10);
	pages->setCurrentRow(0);

	connect(pages,
		SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
		this, SLOT(changePage(QListWidgetItem*,QListWidgetItem*)));


	//Create interface buttons
	QPushButton *closeButton = new QPushButton(tr("Close"));	
	connect(closeButton, SIGNAL(clicked()), this, SLOT(close()));
	//QPushButton *applyButton = new QPushButton(tr("Apply"));	
	//connect(applyButton, SIGNAL(clicked()), this, SLOT(apply()));
	needReload = new QLabel;
	needReload->setTextFormat(Qt::RichText);
	needReload->setText("<img src=\":/icon/warning.png\"> Some settings will only be applied after the application is restarted!");
	needReload->hide();


	//Create layout
	QHBoxLayout *horizontalLayout = new QHBoxLayout;
	horizontalLayout->addWidget(pages);
	horizontalLayout->addWidget(contents, 1);

	QHBoxLayout *buttonsLayout = new QHBoxLayout;
	buttonsLayout->addStretch(1);
	buttonsLayout->addWidget(needReload);
	//buttonsLayout->addWidget(applyButton);
	buttonsLayout->addWidget(closeButton);

	QVBoxLayout *mainLayout = new QVBoxLayout;
	mainLayout->addLayout(horizontalLayout);
	//mainLayout->addStretch(1);
	mainLayout->addSpacing(12);
	mainLayout->addLayout(buttonsLayout);
	setLayout(mainLayout);

	setWindowTitle(tr("FoxWorks Editor Preferences"));
	setMinimumWidth(500);
	setMinimumHeight(400);


	//Create pages
	createPerfomance();
	createEngineering();
	createOther();

}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void PreferencesDialog::createPerfomance() {
	QListWidgetItem* button = new QListWidgetItem(pages);
	button->setIcon(QIcon(":/icon/preferences/perfomance.png"));
	button->setText(tr("Perfomance"));
	button->setTextAlignment(Qt::AlignHCenter);
	button->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

	QWidget* page = new QWidget;
	QFormLayout* layout = new QFormLayout;
	page->setLayout(layout);
	contents->addWidget(page);
	layout->addRow(new QLabel("Perfomance and rendering preferences"));


	/*QSlider* slider = new QSlider(Qt::Horizontal);
	slider->setObjectName("rendering.lod_count");
	slider->setMinimum(1);
	slider->setMaximum(20);
	slider->setTickInterval(1);
	slider->setValue(fw_editor_settings->value("rendering.lod_count").toInt());
	connect(slider, SIGNAL(valueChanged(int)), this, SLOT(setInteger(int)));
	renderingPageLayout->addRow("LOD levels count:",slider);*/

	QCheckBox* checkBox = new QCheckBox();
	checkBox->setObjectName("rendering.no_lods");
	checkBox->setChecked(fw_editor_settings->value("rendering.no_lods").toBool());
	connect(checkBox, SIGNAL(stateChanged(int)), this, SLOT(setBoolWarn(int)));
	layout->addRow("Don't use LODs:<br>(default: <i>false</i>)", checkBox);

	QSpinBox* spinBox = new QSpinBox();
	spinBox->setObjectName("rendering.lod_count");
	spinBox->setRange(1,20);
	spinBox->setValue(fw_editor_settings->value("rendering.lod_count").toInt());
	connect(spinBox, SIGNAL(valueChanged(int)), this, SLOT(setIntegerWarn(int)));
	layout->addRow("Number of LOD levels:<br>(default: <i>6</i>)", spinBox);

	spinBox = new QSpinBox();
	spinBox->setObjectName("rendering.lod_quality");
	spinBox->setRange(4,128);
	spinBox->setValue(fw_editor_settings->value("rendering.lod_quality").toInt());
	connect(spinBox, SIGNAL(valueChanged(int)), this, SLOT(setIntegerWarn(int)));
	layout->addRow("Tessellation quality (higher is better/slower):<br>(default: <i>32</i>)", spinBox);

	checkBox = new QCheckBox();
	checkBox->setObjectName("rendering.use_fxaa");
	checkBox->setChecked(fw_editor_settings->value("rendering.use_fxaa").toBool());
	connect(checkBox, SIGNAL(stateChanged(int)), this, SLOT(setBoolWarn(int)));
	layout->addRow("Use FXAA (antialiasing):<br>(default: <i>true</i>)", checkBox);

	spinBox = new QSpinBox();
	spinBox->setObjectName("ui.autosave");
	spinBox->setRange(5,60*60*12);
	spinBox->setSuffix(" seconds");
	spinBox->setValue(fw_editor_settings->value("ui.autosave").toInt()/1000);
	connect(spinBox, SIGNAL(valueChanged(int)), this, SLOT(setIntegerx1000(int)));
	layout->addRow("Interval between autosaves:<br>(default: <i>30</i> seconds)", spinBox);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void PreferencesDialog::createEngineering() {
	QListWidgetItem* button = new QListWidgetItem(pages);
	button->setIcon(QIcon(":/icon/preferences/engineering.png"));
	button->setText(tr("Engineering"));
	button->setTextAlignment(Qt::AlignHCenter);
	button->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

	QWidget* page = new QWidget;
	QFormLayout* layout = new QFormLayout;
	page->setLayout(layout);
	contents->addWidget(page);
	layout->addRow(new QLabel("Engineering input, data output, processing preferences"));


	QDoubleSpinBox* doubleSpinBox = new QDoubleSpinBox();
	doubleSpinBox->setObjectName("rendering.outline_thickness");
	doubleSpinBox->setMinimum(0.5);
	doubleSpinBox->setMaximum(10.0);
	doubleSpinBox->setSingleStep(0.1);
	doubleSpinBox->setValue(fw_editor_settings->value("rendering.outline_thickness").toDouble());
	connect(doubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(setDouble(double)));
	layout->addRow("Outline thickness:<br>(default: <i>1.0</i>)", doubleSpinBox);

	QSpinBox* spinBox = new QSpinBox();
	spinBox->setObjectName("screenshot.width");
	spinBox->setRange(1,2048);
	spinBox->setValue(fw_editor_settings->value("screenshot.width").toInt());
	connect(spinBox, SIGNAL(valueChanged(int)), this, SLOT(setInteger(int)));
	layout->addRow("Screenshot width:<br>(default: <i>2048</i>)", spinBox);

	spinBox = new QSpinBox();
	spinBox->setObjectName("screenshot.height");
	spinBox->setRange(0,2048);
	spinBox->setValue(fw_editor_settings->value("screenshot.height").toInt());
	connect(spinBox, SIGNAL(valueChanged(int)), this, SLOT(setInteger(int)));
	layout->addRow("Screenshot height:<br>(default: <i>0 - automatic</i>)", spinBox);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void PreferencesDialog::createOther() {
	QListWidgetItem* button = new QListWidgetItem(pages);
	button->setIcon(QIcon(":/icon/preferences/other.png"));
	button->setText(tr("Other"));
	button->setTextAlignment(Qt::AlignHCenter);
	button->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

	QWidget* page = new QWidget;
	QFormLayout* layout = new QFormLayout;
	page->setLayout(layout);
	contents->addWidget(page);
	layout->addRow(new QLabel("Other and miscellaneous preferences"));
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void PreferencesDialog::changePage(QListWidgetItem *current, QListWidgetItem *previous) {
	if (!current) current = previous;
	contents->setCurrentIndex(pages->row(current));
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void PreferencesDialog::apply() {
	//close();
}


////////////////////////////////////////////////////////////////////////////////
/// @brief
////////////////////////////////////////////////////////////////////////////////
void PreferencesDialog::setInteger(int value) {
	fw_editor_settings->setValue(sender()->objectName(),value);
}

void PreferencesDialog::setIntegerWarn(int value) {
	needReload->show();
	fw_editor_settings->setValue(sender()->objectName(),value);
}

void PreferencesDialog::setIntegerx1000(int value) {
	fw_editor_settings->setValue(sender()->objectName(),value*1000);
}

void PreferencesDialog::setBool(int value) {
	fw_editor_settings->setValue(sender()->objectName(),value >= 1);
}

void PreferencesDialog::setBoolWarn(int value) {
	needReload->show();
	fw_editor_settings->setValue(sender()->objectName(),value >= 1);
}

void PreferencesDialog::setDouble(double value) {
	fw_editor_settings->setValue(sender()->objectName(),value);
}

void PreferencesDialog::setDoubleWarn(double value) {
	needReload->show();
	fw_editor_settings->setValue(sender()->objectName(),value);
}