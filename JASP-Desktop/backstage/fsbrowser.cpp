//
// Copyright (C) 2018 University of Amsterdam
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public
// License along with this program.  If not, see
// <http://www.gnu.org/licenses/>.
//

#include "fsbrowser.h"

#include <QGridLayout>
#include <QScrollArea>
#include <QMessageBox>
#include <QMovie>
#include <QLabel>

#include "fsentrywidget.h"
#include <iostream>
#include "qutils.h"

FSBrowser::FSBrowser(QWidget *parent, FSBrowser::BrowseMode mode) : QWidget(parent)
{
	QLabel *label = NULL;
	
	_browseMode = mode;
	_viewType = FSBrowser::IconView;
	
	QGridLayout *layout = new QGridLayout(this);
	layout->setContentsMargins(12, 12, 0, 0);  //Position all file and folder elements asn recent file label
	setLayout(layout);
	
	switch(mode)
		
	{		
	case FSBrowser::BrowseRecentFiles:
		label = new QLabel("Recent Files");
		break;
		
	case FSBrowser::BrowseExamples:
		label = new QLabel("Examples");		
		break;
		
	case FSBrowser::BrowseCurrent:
		layout->addWidget(new QLabel(QString("Double-click on the file below to synchronize or use ") + getShortCutKey() + "-Y"));
		break;
		
	default:
		break;		
	}
	
	if (label)
	{
		QFont f= QFont("SansSerif");
		f.setPointSize(18);
		label->setFont(f);
		QSizePolicy sp = label->sizePolicy();
		sp.setHorizontalStretch(1);
		label->setSizePolicy(sp);
		label->setContentsMargins(0, 0, 0, 0);
		layout->addWidget(label);
	}
	
	_scrollArea = new VerticalScrollArea(this);
	_scrollArea->setFrameShape(QScrollArea::NoFrame);
	layout->addWidget(_scrollArea);
	
	_scrollPane = new QWidget;
	_scrollArea->setWidget(_scrollPane);
	
	_scrollPaneLayout = new QVBoxLayout(_scrollPane);
	_scrollPaneLayout->setSpacing(1);
	_scrollPaneLayout->setSizeConstraint(QLayout::SetMinAndMaxSize);
	_scrollPane->setLayout(_scrollPaneLayout);
	
	_buttonGroup = new QButtonGroup(this);
	
	_processLabel = new QLabel(this);
	_processLabel->setAlignment(Qt::AlignCenter);
	_processLabel->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
	_processLabel->setMovie(new QMovie(":/icons/loading.gif", QByteArray(), _processLabel));
	_processLabel->setHidden(true);
	layout->addWidget(_processLabel);
	
}

void FSBrowser::StartProcessing()
{
	_processLabel->movie()->start();
	_processLabel->setHidden(false);
	_scrollArea->setHidden(true);
}

void FSBrowser::StopProcessing()
{
	_processLabel->movie()->stop();
	_processLabel->setHidden(true);
	_scrollArea->setHidden(false);
}

void FSBrowser::setFSModel(FSBModel *model)
{
	_model = model;
	_model->refresh();
	refresh();
	
	connect(_model, SIGNAL(entriesChanged()), this, SLOT(refresh()));
	connect(_model, SIGNAL(processingEntries()), this, SLOT(processingEntries()));
	connect(_model, SIGNAL(authenticationSuccess()), this, SLOT(refresh()));
	connect(_model, SIGNAL(authenticationClear()), this, SLOT(refresh()));
	connect(_model, SIGNAL(authenticationFail(QString)), this, SLOT(authenticationFailed(QString)));
}

void FSBrowser::setBrowseMode(FSBrowser::BrowseMode mode)
{
	_browseMode = mode;
}

void FSBrowser::setViewType(FSBrowser::ViewType viewType)
{
	_viewType = viewType;
}

void FSBrowser::clearItems()
{
	foreach (QAbstractButton *button, _buttonGroup->buttons())
		delete button;
}

void FSBrowser::processingEntries()
{
	StartProcessing();
}

void FSBrowser::refresh()
{
	clearItems();
	
	StopProcessing();
	
	if (!_model->requiresAuthentication() || _model->isAuthenticated())
	{
		
		bool compact = false;
		
		if (_viewType == ListView)
		{
			compact = true;
			_scrollPaneLayout->setContentsMargins(12, 8, 8, 8);  //Position Folders
			_scrollPaneLayout->setSpacing(0); 
		}
		else
		{
			_scrollPaneLayout->setContentsMargins(12, 12, 12, 12); //Position Files in recent
			_scrollPaneLayout->setSpacing(8);
		}
		
		int id = 0;
		
		foreach (const FSEntry &entry, _model->entries())
		{
			FSEntryWidget *button = new FSEntryWidget(entry, _scrollPane);
			button->setCompact(compact);
			
			_buttonGroup->addButton(button, id++);
			_scrollPaneLayout->addWidget(button);
			
			connect(button, SIGNAL(selected()), this, SLOT(entrySelectedHandler()));
			connect(button, SIGNAL(opened()), this, SLOT(entryOpenedHandler()));
		}			
	}	
}

void FSBrowser::loginRequested(QString username, QString password)
{
	_model->authenticate(username, password);
}

void FSBrowser::entrySelectedHandler()
{
	FSEntryWidget *entry = qobject_cast<FSEntryWidget*>(this->sender());
	if (entry->entryType() != FSEntry::Folder)
		emit entrySelected(entry->path());
}

void FSBrowser::entryOpenedHandler()
{
	FSEntryWidget *entry = qobject_cast<FSEntryWidget*>(this->sender());
	
	if (_browseMode == BrowseOpenFolder)
	{
		emit entryOpened(entry->path());
	}
	else
	{
		if (entry->entryType() == FSEntry::Folder)
			_model->setPath(entry->path());
		else
			emit entryOpened(entry->path());
	}
}

void FSBrowser::authenticationFailed(QString message)
{
	QMessageBox::warning(this, "", message);
	
}

